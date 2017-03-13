import java.util.*;

import com.maxeler.maxcompiler.v2.kernelcompiler.*;
import com.maxeler.maxcompiler.v2.kernelcompiler.stdlib.KernelMath;
import com.maxeler.maxcompiler.v2.kernelcompiler.stdlib.core.*;
import com.maxeler.maxcompiler.v2.kernelcompiler.stdlib.core.Count.*;
import com.maxeler.maxcompiler.v2.kernelcompiler.stdlib.memory.*;
import com.maxeler.maxcompiler.v2.kernelcompiler.types.base.*;
import com.maxeler.maxcompiler.v2.kernelcompiler.types.composite.*;
import com.maxeler.maxcompiler.v2.utils.MathUtils;

import com.custom_computing_ic.dfe_snippets.utils.Reductions;
import com.custom_computing_ic.dfe_snippets.reductions.LogAddReduce;

class SpmvKernel extends Kernel {

  // N x N matrix, nnzs nonzeros
  //
  // load data for N cycles into cache
  // compute for N cycles
  // -- in the mean time load data for N cycles in double buffer
  // repeat
  //
  // Sources of inefficiency:
  // -- empty rows
  // -- port sharing cannot be used

  private static final int fpL = 16;

  protected SpmvKernel(
      KernelParameters parameters,
      int inputWidth,
      int cacheSize,
      int indexWidth,
      int mantissaWidth,
      boolean dbg) {
    super(parameters);

    // optimization.pushPipeliningFactor(0.5);
    optimization.pushDSPFactor(1);

    DFEVar controlS = io.input("control", dfeUInt(3 + 3 * 32 + inputWidth));
    DFEVar readMask = controlS.slice(99, inputWidth).cast(dfeUInt(inputWidth));
    DFEVar vRomLoadEnable = controlS[98];
    DFEVar rowFinished = controlS[97];
    DFEVar cacheReadEnable = controlS[96];
    DFEVar rowLength = controlS.slice(64, 32).cast(dfeUInt(32));
    DFEVar cycleCounter = controlS.slice(32, 32).cast(dfeUInt(32));
    DFEVar firstReadPosition = controlS.slice(0, 32).cast(dfeUInt(32));

    SpmvCacheKernel cache = new SpmvCacheKernel(this,
        vRomLoadEnable, cacheReadEnable, readMask,
        inputWidth, cacheSize, indexWidth, mantissaWidth);
    DFEVector<DFEVar> matrixValues = cache.getMatrixValues();
    DFEVector<DFEVar> vectorValues = cache.getVectorValues();
    //DFEVector<DFEVar> vectorIndices = cache.getVectorIndices();

    DFEVar result = Reductions.reduce(matrixValues * vectorValues);

    // --- Accumulation

    // must disable input registering to use as flush trigger
    //io.pushInputRegistering(false);
    //DFEVar flushTrigger = io.input("flush", dfeBool());
    //io.popInputRegistering();
    //flush.afterTrigger(flushTrigger);

    DFEVar runLength = firstReadPosition + rowLength;
    DFEVar modulo = KernelMath.modulo(runLength, inputWidth);
    DFEVar quot = (runLength - modulo.cast(dfeUInt(32))) / inputWidth;

    DFEVar totalCycles = quot + (modulo === 0 ? constant.var(dfeUInt(32), 0) : 1);

    DFEVar carriedSum = dfeFloat(11, mantissaWidth).newInstance(this);
    DFEVar newSum = result + (cycleCounter < fpL ? 0 : carriedSum);
    carriedSum <== stream.offset(newSum, -fpL);

    DFEVar firstValidPartialSum = (totalCycles > fpL)? (totalCycles - fpL) : 0;
    DFEVar validPartialSums = (cycleCounter >= firstValidPartialSum);
    LogAddReduce r = new LogAddReduce(this,
        validPartialSums,
        rowFinished,
        newSum,
        dfeFloat(11, mantissaWidth),
        fpL);


    // TODO: remove prefetch
    DFEVar emptyRow = (rowLength === 0);
    DFEVar outputEnable = (~vRomLoadEnable & (rowFinished | emptyRow)); // | flushTrigger;
    io.output("output", r.getOutput(), dfeFloat(11, mantissaWidth), outputEnable);
    //io.output("flushTriggerOut", flushTrigger, dfeBool(), outputEnable);
    // if we find a sequence of empty rows, tell downstream reduction kernrel
    DFEVar skipCount = emptyRow ? cycleCounter : 0;
    io.output("skipCount", skipCount, dfeUInt(32), outputEnable);


    // --- Debug
    if (dbg) {
      DFEVar printEnable = rowLength !== 0;
        Params rowCounterParams = control.count.makeParams(32)
          .withEnable(rowFinished);
        DFEVar rowCounter = control.count.makeCounter(rowCounterParams).getCount();
      debug.dfePrintf(
          printEnable,
          "Kernel " + getName() + " -- row %d totalCycles %d, readmask %d rowFinished %d rowLength %d output: %f cycleCounter %d validPartialSums %d newSum %f firstRead %d skipCount %d",
          rowCounter, totalCycles, readMask, rowFinished, rowLength, r.getOutput(), cycleCounter, validPartialSums, newSum, firstReadPosition, skipCount);

      debug.dfePrintf(printEnable, "\n");
      debug.dfePrintf(printEnable, "Values: ");
      for (int i = 0; i < inputWidth; i++)
        debug.dfePrintf(printEnable, "%f ", matrixValues[i]);
      debug.dfePrintf(printEnable, "Vector Values: ");
      for (int i = 0; i < inputWidth; i++)
        debug.dfePrintf(printEnable, "%f ", vectorValues[i]);
      debug.dfePrintf(printEnable, "\n");
    }
  }

}

// This implements cache
//
class SpmvCacheKernel extends KernelLib {

  private final DFEVectorType<DFEVar> vtype, ivtype;
  private final List<Memory<DFEVar>> vroms;
  private final DFEType addressT;
  private final DFEVector<DFEVar> matrixValues;
  private final DFEVector<DFEVar> vectorValues;
  private final DFEVector<DFEVar> indptrVector;

  protected SpmvCacheKernel(KernelLib owner,
      DFEVar vRomLoadEnable,
      DFEVar cacheReadEnable,
      DFEVar readMask,
      int inputWidth,
      int cacheSize,
      int indexWidth,
      int mantissaWidth) {
    super(owner);

    this.addressT = dfeUInt(MathUtils.bitsToAddress(cacheSize));

    // load entire vector or until cache is full
    int sizeBits = 32; // XXX may need to run for more cycles

    DFEVar vectorLoadCycles = owner.io.scalarInput("vectorLoadCycles", dfeUInt(32));
    Params loadAddressParams = control.count.makeParams(sizeBits )
      .withMax(vectorLoadCycles)
      .withEnable(vRomLoadEnable);
    DFEVar loadAddress = control.count.makeCounter(loadAddressParams).getCount();

    DFEVar vectorValue = owner.io.input("vromLoad", dfeFloat(11, mantissaWidth), vRomLoadEnable);
    vtype = new DFEVectorType<DFEVar> (dfeFloat(11, mantissaWidth), inputWidth);
    ivtype = new DFEVectorType<DFEVar> (dfeUInt(indexWidth), inputWidth);
    DFEVectorType<DFEVar> inputType = new DFEVectorType<DFEVar> (dfeUInt(96), inputWidth);

     //--- Cache allocation and control
    vroms = new ArrayList<Memory<DFEVar>>();
    for (int i = 0; i < inputWidth; i++) {
      Memory<DFEVar> vrom = mem.alloc(dfeFloat(11, mantissaWidth), cacheSize);
      vroms.add(vrom);
      vrom.write(
          loadAddress.cast(addressT),
          vectorValue,
          vRomLoadEnable);
    }

    // --- I/O
    DFEVar readEnable = cacheReadEnable & ~vRomLoadEnable;

    DFEVector<DFEVar> indptrValues = owner.io.input("indptr_values", inputType, readEnable);
    DFEVector<DFEVar> valuesVector = vtype.newInstance(this);
    indptrVector = ivtype.newInstance(this);
    for (int i = 0; i < inputWidth; i++) {
      indptrVector[i] <== indptrValues[i].slice(64, 32).cast(dfeUInt(32));
      valuesVector[i] <== indptrValues[i].slice(0, 64).cast(dfeFloat(11, 53));
    }

    DFEVector<DFEVar> colptr = selectValues(indptrVector, readMask);
    matrixValues = selectValues(valuesVector, readMask);
    vectorValues = resolveVectorReads(colptr);
  }

  DFEVector<DFEVar> getMatrixValues()
  {
    return matrixValues;
  }

  DFEVector<DFEVar> getVectorValues()
  {
    return vectorValues;
  }

  DFEVector<DFEVar> selectValues(
      DFEVector<DFEVar> in,
      DFEVar readMask) {
    DFEVector<DFEVar> out = in.getType().newInstance(this);
    for (int i = 0; i < in.getSize(); i++)
      out[i] <== readMask.slice(i) === 0 ?  0 : in[i];
    return out;
  }


  DFEVector<DFEVar> resolveVectorReads(DFEVector<DFEVar> reads) {
    DFEVector<DFEVar> out = vtype.newInstance(this);
    for (int i = 0; i < vroms.size(); i++)
      out[i] <== vroms.get(i).read(reads[i].cast(addressT));
    return out;
  }

}




// XXX for now we assume the matrix is smaller then max rows
// will have to implement an lmem design above this threshold
// using lmem wrapped above this thresholed may be feasilbe,
// see the DramAccumulator snippet in dfe-snippets
class SpmvReductionKernel extends Kernel {

  protected SpmvReductionKernel(KernelParameters parameters, int fpl, int maxRows) {
    super(parameters);

    DFEVar in = io.input("reductionIn", dfeFloat(11, 53));
    DFEVar n = io.scalarInput("nRows", dfeUInt(32));
    DFEVar totalCycles = io.scalarInput("totalCycles", dfeUInt(32));

    DFEVar cycles = control.count.simpleCounter(32);

    DFEVar sumCarried = dfeFloat(11, 53).newInstance(this);
    DFEVar sum = in + (cycles < n ? 0 : sumCarried);
    sumCarried <== stream.offset(sum, -(n.cast(dfeInt(32))), -maxRows, -(fpl + 2));

    // output on the last n cycles
    DFEVar outputEnable = totalCycles - cycles <= n;
    io.output("reductionOut", sum, dfeFloat(11, 53), outputEnable);
  }
}


// use for large matrices, does not support skiping of empty rows
class DramSpmvReductionKernel extends Kernel {
  protected DramSpmvReductionKernel(KernelParameters parameters) {
    super(parameters);
    DFEVar previous = io.input("prevb", dfeFloat(11, 53));
    DFEVar current = io.input("reductionIn", dfeFloat(11, 53));

    // Not actually used, but ncecessary to maintain interface compatibility
    io.input("skipCount", dfeInt(32));
    DFEVar nRows = io.scalarInput("nRows", dfeUInt(32));
    io.scalarInput("totalCycles", dfeUInt(32));

    DFEVar cycles = control.count.simpleCounter(32);
    DFEVar prev = cycles < nRows ? 0 : previous;
    //DFEVar outputEnable = totalCycles - cycles <= n;
    io.output("reductionOut", prev + current, dfeFloat(11, 53));
  }
}


// uses BRAM storage instead of stream offsets, allows us to skip
// empty rows to achieve better throughput on matrices with long
// sequences of empty rows (e.g. banded matrices)
class BramSpmvReductionKernel extends Kernel {

  protected BramSpmvReductionKernel(KernelParameters parameters, int fpl, int maxRows, boolean dbg) {
    super(parameters);

    // when skipCount != 0, we are to skip skipCount rows (since they are empty)
    DFEVar skipCount = io.input("skipCount", dfeUInt(32)); //io.input("skipCount", dfeUInt(32));
    DFEVar skipEnable = skipCount !== 0;
    DFEVar addressInc = skipEnable ? skipCount : 1;

    DFEVar in = io.input("reductionIn", dfeFloat(11, 53));
    DFEVar n = io.scalarInput("nRows", dfeUInt(32));
    DFEVar totalCycles = io.scalarInput("totalCycles", dfeUInt(32));
    //DFEVar cycles = control.count.simpleCounter(32);

    CounterChain chain = control.count.makeCounterChain();
    DFEVar cycles = chain.addCounter(totalCycles, 1);
    //io.pushInputRegistering(false);
    //DFEVar flushTrigger = io.input("flush", dfeBool());
    //io.popInputRegistering();
    //flush.afterTrigger(flushTrigger);

    // internal buffer for accumulating partial sums
    Memory<DFEVar> memory = mem.alloc(dfeFloat(11, 53), maxRows);


    // the element we are going to accumulate with
    // initially, all data in the BRAM should be 0, it doesn't matter what we read
    int offset = 1; // offset for integer add
    optimization.pushPipeliningFactor(0);
    DFEVar carriedReadAddress = dfeUInt(32).newInstance(this);
    DFEVar nextReadAddress = (cycles < offset ? cycles : carriedReadAddress) + addressInc;
    DFEVar wrappedNextReadAddress = nextReadAddress === n ? 0 : nextReadAddress;
    carriedReadAddress <== stream.offset(wrappedNextReadAddress, -offset);
    optimization.popPipeliningFactor();

    DFEVar readAddress = cycles < offset ? cycles : carriedReadAddress;
    DFEVar castReadAddress = readAddress.cast(dfeUInt(MathUtils.bitsToAddress(maxRows)));
    // we know that the first partition will not have been compressed on the CPU
    DFEVar prevSum = cycles < n ? 0 : memory.read(castReadAddress);
    DFEVar sum = prevSum + in;


    Stream.OffsetExpr off = stream.measureDistance("BramAccOffset", readAddress, sum);
    memory.write(
        stream.offset(castReadAddress, -off),
        stream.offset(sum, -off),
        stream.offset(~skipEnable, -off));

    // output on the last n cycles
    DFEVar outputEnable = totalCycles - cycles <= n;
    io.output("reductionOut", sum, dfeFloat(11, 53), outputEnable);

    if (dbg) {
      debug.simPrintf(
          "ReductionKernel --> cycle %d readAddress %d sum %f prevSum %f in %f skipEnable %d skipCount %d\n",
          cycles, readAddress, sum, prevSum, in, skipEnable, skipCount);
    }
  }
}


// pads the inputs with 0 after nInputs have been processed
class PaddingKernel extends Kernel {
  protected PaddingKernel(KernelParameters parameters) {
    super(parameters);
    DFEVar nInputs = io.scalarInput("nInputs", dfeUInt(32));
    DFEVar totalCycles = io.scalarInput("totalCycles", dfeUInt(32));

    CounterChain chain = control.count.makeCounterChain();
    DFEVar cycles = chain.addCounter(totalCycles, 1);
    DFEVar paddingCycles = cycles >= nInputs;

    DFEVar input = io.input("paddingIn", dfeFloat(11, 53), ~paddingCycles);
    DFEVar out = paddingCycles ? 0 : input;
    io.output("paddingOut", out, dfeFloat(11, 53));
  }
}

// extracts trailing zeros from input stream, after nInputs have been processed
class UnpaddingKernel extends Kernel {
  protected UnpaddingKernel(KernelParameters parameters, int bitWidth, int id) {
    super(parameters);
    DFEVar nInputs = io.scalarInput("nInputs", dfeUInt(32));
    DFEVar totalCycles = io.scalarInput("totalCycles", dfeUInt(32));
    CounterChain chain = control.count.makeCounterChain();
    DFEVar cycles = chain.addCounter(totalCycles, 1);
    DFEVar unpaddingCycles = cycles >= nInputs;

    DFEVar input = io.input("paddingIn", dfeRawBits(bitWidth));
    io.output("pout", input, dfeRawBits(bitWidth), ~unpaddingCycles);
    //if (id == 2) {
      //debug.simPrintf("id: %d cycles %d nInputs %d totalCycles %d unpadding %d\n",
          //id, cycles, nInputs, totalCycles, unpaddingCycles);
      //debug.simPrintf("input %d\n", input);
    //}
  }
}
