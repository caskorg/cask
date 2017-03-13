import com.maxeler.maxcompiler.v2.managers.custom.CustomManager;
import com.maxeler.maxcompiler.v2.managers.custom.DFELink;
import com.maxeler.maxcompiler.v2.managers.custom.blocks.*;
import com.maxeler.maxcompiler.v2.managers.custom.stdlib.*;
import com.maxeler.maxcompiler.v2.managers.engine_interfaces.*;
import com.maxeler.maxcompiler.v2.managers.engine_interfaces.EngineInterface.*;
import com.maxeler.maxcompiler.v2.managers.BuildConfig;

import com.custom_computing_ic.dfe_snippets.manager.*;

public class SpmvManager extends CustomManager{

    private final int cacheSize;
    private final int inputWidth;
    private final int maxRows;
    public final int numPipes;
    public final int numControllers;

    // parameters of CSR format used: float64 values, int32 index.
    private static final int mantissaWidth = 53;
    private static final int indexWidth = 32;

    private static final int FLOATING_POINT_LATENCY = 16;

    private static final boolean DBG_PAR_CSR_CTL = false;
    private static final boolean DBG_SPMV_KERNEL = false;
    private static final boolean DBG_REDUCTION_KERNEL = false;
    private static final boolean dramReductionEnabled = false;

    // private final LMemInterface defaultInterface;

    SpmvManager(SpmvEngineParams ep) {
        super(ep);

        inputWidth = ep.getInputWidth();
        cacheSize = ep.getVectorCacheSize();
        maxRows = ep.getMaxRows();
        // numPipes = ep.getNumPipes();

        // TODO we assume one pipe per controller for now; the number of pipes is ignored
        numPipes = ep.getNumControllers();
        numControllers = ep.getNumControllers();

        if (384 % inputWidth != 0) {
          throw new RuntimeException("Error! 384 is not a multiple of INPUT WIDTH: " +
              "This may lead to stalls due to padding / unpadding ");
        }

        addMaxFileConstant("inputWidth", inputWidth);
        addMaxFileConstant("cacheSize", cacheSize);
        addMaxFileConstant("maxRows", maxRows);
        addMaxFileConstant("numPipes", numPipes);
        addMaxFileConstant("numControllers", numControllers);
        addMaxFileConstant("dramReductionEnabled", dramReductionEnabled ? 1 : 0);

        ManagerUtils.setDRAMFreq(this, ep, 400);
        config.setAllowNonMultipleTransitions(true);
        config.setDefaultStreamClockFrequency(200);

        // CPU --> Demux
        DFELink fromCpu = addStreamFromCPU("fromcpu");
        Demux split = demux("split");
        split.getInput() <== fromCpu;

        // LMEM --> CPU IO
        DFELink toCpu = addStreamToCPU("tocpu");
        Mux join = mux("join");
        toCpu <== join.getOutput();

        // TODO support multiple pipes per controller
        for (int i = 0; i < numPipes; i++) {
            LMemInterface iface = addLMemInterface("ctrl" + i, 1);
            addComputePipe(i, inputWidth, iface);

            // Demux --> LMem
            DFELink cpu2lmem = iface.addStreamToLMem("cpu2lmem" + i,
                LMemCommandGroup.MemoryAccessPattern.LINEAR_1D);
            cpu2lmem <== split.addOutput("tomem" + i);

            // LMem --> Mux
            DFELink lmem2cpu = iface.addStreamFromLMem("lmem2cpu" + i,
                LMemCommandGroup.MemoryAccessPattern.LINEAR_1D);
            join.addInput("frommem" + i) <== lmem2cpu;
        }
    }

    void addComputePipe(int id, int inputWidth, LMemInterface iface) {
        StateMachineBlock readControlBlock = addStateMachine(
            getReadControl(id),
            new ParallelCsrReadControl(this, inputWidth, DBG_PAR_CSR_CTL));

        readControlBlock.getInput("length") <== addUnpaddingKernel("colptr" + id, 32, id * 10 + 1, iface);

        KernelBlock k = addKernel(new SpmvKernel(
              makeKernelParameters(getComputeKernel(id)),
              inputWidth,
              cacheSize,
              indexWidth,
              mantissaWidth,
              DBG_SPMV_KERNEL
              ));

        k.getInput("indptr_values") <== addUnpaddingKernel("indptr_values" + id, inputWidth * 96, id * 10 + 2, iface);
        k.getInput("vromLoad") <== addUnpaddingKernel("vromLoad" + id, 64, id * 10 + 3, iface);
        k.getInput("control") <== readControlBlock.getOutput("control");

        KernelBlock r = null;
        if (dramReductionEnabled) {
          r = addKernel(new DramSpmvReductionKernel(makeKernelParameters(getReductionKernel(id))));

          r.getInput("prevb") <== addUnpaddingKernel("prevb" + id, 64, id * 10 + 5, iface);

        }
        else {
          r = addKernel(new BramSpmvReductionKernel(
                makeKernelParameters(getReductionKernel(id)),
                FLOATING_POINT_LATENCY,
                maxRows,
                DBG_REDUCTION_KERNEL));
        }

        addPaddingKernel("reductionOut" + id, iface) <== r.getOutput("reductionOut");
        r.getInput("reductionIn") <== k.getOutput("output");
        r.getInput("skipCount") <== k.getOutput("skipCount");
    }

    DFELink addPaddingKernel(String stream, LMemInterface iface) {
      System.out.println("Creating kernel  " + getPaddingKernel(stream));
      KernelBlock p = addKernel(new PaddingKernel(
            makeKernelParameters(getPaddingKernel(stream))));
      iface.addStreamToLMem(stream, LMemCommandGroup.MemoryAccessPattern.LINEAR_1D) <== p.getOutput("paddingOut");
      return p.getInput("paddingIn");
    }

    DFELink addUnpaddingKernel(
        String stream,
        int bitwidth,
        int id,
        LMemInterface iface)
   {
     KernelBlock k = addKernel(new
         UnpaddingKernel(makeKernelParameters(getUnpaddingKernel(stream)), bitwidth, id));
     k.getInput("paddingIn") <== iface.addStreamFromLMem(stream, LMemCommandGroup.MemoryAccessPattern.LINEAR_1D);
     return k.getOutput("pout");
   }

    String getComputeKernel(int id) {
      return "computeKernel" + id;
    }

    String getReductionKernel(int id) {
      return "reductionKernel" + id;
    }

    String getPaddingKernel(String id) {
      return "padding_" + id;
    }

    String getReadControl(int id) {
      return "readControl" + id;
    }

    String getFanoutName(int id) {
      return "flush" + id;
    }

    String getUnpaddingKernel(String stream) {
      return "unpadding_" + stream;
    }

    void setUpComputePipe(
        EngineInterface ei, int id,
        InterfaceParam vectorLoadCycles,
        InterfaceParam nPartitions,
        InterfaceParam n,
        InterfaceParam outResultStartAddress,
        InterfaceParam outResultSize,
        InterfaceParam vStartAddress,
        InterfaceParam colPtrStartAddress,
        InterfaceParam colptrSize,
        InterfaceParam indptrValuesStartAddress,
        InterfaceParam indptrValuesSize,
        InterfaceParam totalCycles,
        InterfaceParam reductionCycles,
        InterfaceParam nIterations) {

      String computeKernel = getComputeKernel(id);
      String reductionKernel = getReductionKernel(id);
      String readControl = getReadControl(id);

      ei.setTicks(computeKernel, totalCycles * nIterations);

      ei.setScalar(computeKernel, "vectorLoadCycles", vectorLoadCycles);

      ei.setTicks(reductionKernel, reductionCycles * nIterations);
      ei.setScalar(reductionKernel, "nRows", n);
      ei.setScalar(reductionKernel, "totalCycles", reductionCycles);

      String memoryController = "ctrl" + id;

      String stream = "vromLoad" + id;
      setupUnpaddingKernel(
          ei,
          getUnpaddingKernel(stream),
          stream,
          vectorLoadCycles * nPartitions,
          ei.addConstant(8), // XXX magic constant...
          nIterations,
          vStartAddress,
          memoryController
          );

      InterfaceParam colptrEntries = colptrSize / CPUTypes.INT32.sizeInBytes();
      stream = "colptr" + id;
      setupUnpaddingKernel(
          ei,
          getUnpaddingKernel(stream),
          stream,
          colptrEntries,
          ei.addConstant(4),
          nIterations,
          colPtrStartAddress,
          memoryController);

      InterfaceParam valueEntries = indptrValuesSize / (8 + 4) / inputWidth; // 12 bytes per entry
      stream = "indptr_values" + id;
      setupUnpaddingKernel(
          ei,
          getUnpaddingKernel(stream),
          stream,
          valueEntries,
          inputWidth * ei.addConstant(8 + 4),
          nIterations,
          indptrValuesStartAddress,
          memoryController);

      ei.setScalar(readControl, "nrows", n);
      ei.setScalar(readControl, "vectorLoadCycles", vectorLoadCycles);
      ei.setScalar(readControl, "nPartitions", nPartitions);
      ei.setScalar(readControl, "nIterations", nIterations);

      InterfaceParam size = nIterations;
      if (dramReductionEnabled) {
        setupUnpaddingKernel(ei,
            getUnpaddingKernel("prevb" + id),
            "prevb" + id,
            n,
            ei.addConstant(8),
            nIterations * nPartitions,
                             outResultStartAddress,
                             memoryController
            );
        size *= nPartitions;
      }

      stream = "reductionOut" + id;
      setupUnpaddingKernel(ei,
                           getPaddingKernel(stream),
                           stream,
                           n,
                           ei.addConstant(8),
                           size,
                           outResultStartAddress,
                           memoryController);
    }


    /**
     * An unpadding kernel reads a stream from memory and discards the bytes
     * which were added to pad the data to a multiple of BURST_SIZE_BYTES.
     *
     * @param size size in number of elements of the stream
     * @param memoryStream the stream from memory to unpad
     * @param inputWidthBytes the number of bytes the kernel should read (and
     *                        write) every cycle
     */
    private void setupUnpaddingKernel(
        EngineInterface ei,
        String kernelId,
        String memoryStream,
        InterfaceParam size,
        InterfaceParam inputWidthBytes,
        InterfaceParam iterations,
        InterfaceParam startAddress,
        String memoryController
        ) {

      InterfaceParam unpaddingCycles = getPaddingCycles(size * inputWidthBytes, inputWidthBytes);
      InterfaceParam totalSize = unpaddingCycles + size;
      System.out.println("Setting ticks for kernel  " + kernelId);
      System.out.println("Setting up queue for controller" + memoryController);

      ei.setTicks(kernelId, totalSize * iterations);
      ei.setScalar(kernelId, "nInputs", size);
      ei.setScalar(kernelId, "totalCycles", totalSize);

      ei.setLMemLinearWrapped(memoryController,
                              memoryStream,
                              startAddress,
                              totalSize * inputWidthBytes,
                              totalSize * inputWidthBytes * iterations,
                              ei.addConstant(0));
    }

    private InterfaceParam smallestMultipleLargerThan(InterfaceParam x, long y) {
      return
        y * (x / y +
          InterfaceMath.max(
            0l, InterfaceMath.min(x % y, 1l)));
    }

    private InterfaceParam getPaddingCycles(
        InterfaceParam outSizeBytes,
        InterfaceParam outWidthBytes)
    {
      final int burstSizeBytes = 384;
      InterfaceParam writeSize = smallestMultipleLargerThan(outSizeBytes, burstSizeBytes);
      return (writeSize - outSizeBytes) / outWidthBytes;
    }

    private EngineInterface interfaceDefault() {
      EngineInterface ei = new EngineInterface();

      InterfaceParam vectorLoadCycles = ei.addParam("vectorLoadCycles", CPUTypes.INT);
      InterfaceParam nPartitions = ei.addParam("nPartitions", CPUTypes.INT);
      InterfaceParam nIterations = ei.addParam("nIterations", CPUTypes.INT);
      InterfaceParamArray nrows = ei.addParamArray("nrows", CPUTypes.INT32);

      InterfaceParamArray outStartAddresses = ei.addParamArray("outStartAddresses", CPUTypes.INT64);
      InterfaceParamArray outResultSizes = ei.addParamArray("outResultSizes", CPUTypes.INT32);

      InterfaceParamArray totalCycles = ei.addParamArray("totalCycles", CPUTypes.INT32);
      InterfaceParamArray vStartAddresses = ei.addParamArray("vStartAddresses", CPUTypes.INT64);

      InterfaceParamArray indptrValuesAddresses = ei.addParamArray("indptrValuesAddresses", CPUTypes.INT64);
     InterfaceParamArray indptrValuesSizes = ei.addParamArray("indptrValuesSizes", CPUTypes.INT32);

      InterfaceParamArray colptrStartAddresses = ei.addParamArray("colPtrStartAddresses", CPUTypes.INT64);
      InterfaceParamArray colptrSizes = ei.addParamArray("colptrSizes", CPUTypes.INT32);

      InterfaceParamArray reductionCycles = ei.addParamArray("reductionCycles", CPUTypes.INT32);

      for (int i = 0; i < numPipes; i++)
        setUpComputePipe(ei, i,
            vectorLoadCycles,
            nPartitions,
            nrows.get(i),
            outStartAddresses.get(i),
            outResultSizes.get(i),
            vStartAddresses.get(i),
            colptrStartAddresses.get(i),
            colptrSizes.get(i),
            indptrValuesAddresses.get(i),
            indptrValuesSizes.get(i),
            totalCycles.get(i),
            reductionCycles.get(i),
            nIterations);

      ei.ignoreAll(Direction.IN_OUT);
      return ei;
    }

    public static EngineInterface dramWrite(SpmvManager m) {
        EngineInterface ei = new EngineInterface("dramWrite");
        CPUTypes TYPE = CPUTypes.INT;
        InterfaceParamArray size = ei.addParamArray("size_bytes_memory_ctl", TYPE);
        InterfaceParamArray start = ei.addParamArray("start_bytes_memory_ctl", TYPE);
        InterfaceParam sizeCPU = ei.addParam("size_bytes_cpu", TYPE);
        ei.setStream("fromcpu", CPUTypes.UINT8, sizeCPU);
        for (int i = 0; i < m.numPipes; i++ ) {
            ei.setLMemLinear("ctrl" + i, "cpu2lmem" + i, start.get(i), size.get(i));
        }
        ignoreKernels(m, ei);
        for (int i = 0; i < m.numPipes; i++) {
          ei.ignoreLMem("lmem2cpu" + i);
        }
        ei.ignoreStream("tocpu");
        ei.ignoreRoute("join");
        return ei;
    }

    /** Ignore all compute kernels and associated streams. Necessary because
     * ignoreAll() also ignores routes and it is not possible to unignore a
     * route afterwards. Use in CPU read / write interfaces, where compute
     * kernels don't need to be active.
     */
    private static void ignoreKernels(SpmvManager m, EngineInterface ei) {
        for (int i = 0; i < m.numPipes; i++) {
          ei.ignoreKernel(m.getReductionKernel(i));
          ei.ignoreKernel(m.getComputeKernel(i));
          ei.ignoreKernel(m.getPaddingKernel("reductionOut" + i));
          ei.ignoreKernel(m.getUnpaddingKernel("colptr" + i));
          ei.ignoreKernel(m.getUnpaddingKernel("vromLoad" + i));
          ei.ignoreKernel(m.getUnpaddingKernel("indptr_values" + i));
          ei.ignoreKernel(m.getReadControl(i));
          ei.ignoreLMem("colptr" + i);
          ei.ignoreLMem("indptr_values" + i);
          ei.ignoreLMem("reductionOut" + i);
          ei.ignoreLMem("vromLoad" + i);
        }
    }

    private static EngineInterface dramRead(SpmvManager m) {
        EngineInterface ei = new EngineInterface("dramRead");
        CPUTypes TYPE = CPUTypes.INT;
        InterfaceParamArray size = ei.addParamArray("size_bytes_memory_ctl", TYPE);
        InterfaceParamArray start = ei.addParamArray("start_bytes_memory_ctl", TYPE);
        InterfaceParam sizeCPU = ei.addParam("size_bytes_cpu", TYPE);
        ei.setStream("tocpu", CPUTypes.UINT8, sizeCPU);
        for (int i = 0; i < m.numPipes; i++ ) {
            ei.setLMemLinear("ctrl" + i, "lmem2cpu" + i, start.get(i), size.get(i));
        }
        ignoreKernels(m, ei);
        for (int i = 0; i < m.numPipes; i++) {
          ei.ignoreLMem("cpu2lmem" + i);
        }
        ei.ignoreStream("fromcpu");
        ei.ignoreRoute("split");
        return ei;
    }

    public static void main(String[] args) {

      SpmvManager manager = new SpmvManager(new SpmvEngineParams(args));
      // ManagerUtils.debug(manager);
      manager.createSLiCinterface(dramWrite(manager));
      manager.createSLiCinterface(dramRead(manager));
      manager.createSLiCinterface(manager.interfaceDefault());
      ManagerUtils.setFullBuild(manager, BuildConfig.Effort.HIGH, 6, 6);
      manager.build();
    }
}
