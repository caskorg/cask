import com.maxeler.maxcompiler.v2.statemachine.*;
import com.maxeler.maxcompiler.v2.statemachine.manager.*;
import com.maxeler.maxcompiler.v2.managers.DFEManager;

import com.maxeler.maxcompiler.v2.statemachine.*;
import com.maxeler.maxcompiler.v2.statemachine.types.*;
import com.maxeler.maxcompiler.v2.statemachine.manager.*;
import com.maxeler.maxcompiler.v2.managers.DFEManager;

public class ParallelCsrReadControl extends ManagerStateMachine {

  private enum Mode {
    //Flush,
    VectorLoad,
    ReadingLength,
    OutputtingCommands,
    Done,
  }

  private final DFEsmStateEnum<Mode> mode;

    private final DFEsmPullInput iLength;
    private final DFEsmStateValue iLengthReady;
    //private final DFEsmPushOutput oFlush;
    private final DFEsmInput vectorLoadCycles, nRows, nPartitions;
    private final DFEsmPushOutput oControl;
    private final DFEsmStateValue oControlData;

    private final DFEsmStateValue outValid, readLength;
    private final DFEsmStateValue readMaskData;
    private final DFEsmStateValue cycleCounter, rowLengthData;
    private final DFEsmStateValue firstReadPosition;
    private final DFEsmStateValue rowsProcessed, vectorLoadCommands, partitionsProcessed, totalOutputs, paddedOutputs;
    //private final DFEsmStateValue flushData;

    private final DFEsmStateValue crtPos, toread, iLengthRead;
    private final int inputWidth;
    private final boolean dbg;

    public ParallelCsrReadControl(DFEManager owner, int inputWidth, boolean dbg) {
      super(owner);
      this.inputWidth = inputWidth;
      this.dbg = dbg;

      mode = state.enumerated(Mode.class, Mode.VectorLoad);
      iLength = io.pullInput("length", dfeUInt(32));
      vectorLoadCycles = io.scalarInput("vectorLoadCycles", dfeUInt(32));
      nRows = io.scalarInput("nrows", dfeUInt(32));
      nPartitions = io.scalarInput("nPartitions", dfeUInt(32));

      oControl = io.pushOutput("control", dfeUInt(3 + 3 * 32 + inputWidth), 1);
      //oFlush  = io.pushOutput("flush", dfeBool(), 1);

      cycleCounter = state.value(dfeInt(32), 0);
      crtPos = state.value(dfeUInt(32), 0);
      firstReadPosition = state.value(dfeUInt(32), 0);
      toread = state.value(dfeUInt(32), 0);
      iLengthRead = state.value(dfeBool(), false);
      oControlData = state.value(dfeUInt(3 + 3 * 32 + inputWidth), 0);
      //flushData = state.value(dfeBool(), false);
      rowLengthData = state.value(dfeUInt(32), 0);
      outValid = state.value(dfeBool(), false);
      readMaskData = state.value(dfeUInt(inputWidth));
      iLengthReady = state.value(dfeBool(), false);
      readLength = state.value(dfeBool(), false);

      // internal state data, not for output
      rowsProcessed = state.value(dfeUInt(32), 0);
      vectorLoadCommands = state.value(dfeUInt(32), 0);
      partitionsProcessed = state.value(dfeUInt(32), 0);
      totalOutputs = state.value(dfeUInt(32), 0);
      paddedOutputs = state.value(dfeUInt(32), 0);
    }

    DFEsmValue outputNotStall() {
      return ~oControl.stall;
    }

    DFEsmValue canRead() {
      return ~iLength.empty & outputNotStall();
    }

    DFEsmValue needToRead() {
      return readLength;
    }

    void processRow() {
      rowsProcessed.next <== rowsProcessed + 1;
      IF (rowsProcessed === nRows - 1) {
        rowsProcessed.next <== 0;
        vectorLoadCommands.next <== 0;
        crtPos.next <== 0;
        readLength.next <== false;
        mode.next <== Mode.VectorLoad;
        partitionsProcessed.next <== partitionsProcessed + 1;
        IF (partitionsProcessed === nPartitions - 1) {
          //mode.next <== Mode.Flush;
          mode.next <== Mode.Done;
        }
      } ELSE {
        mode.next <== Mode.ReadingLength;
        readLength.next <== true;
      }
    }

    @Override
    protected void nextState() {
      outValid.next <== false;

      iLengthReady.next <== canRead() & needToRead();
      // disable read request on next cycle, if we request data this cycle
      IF (canRead() & needToRead())
        readLength.next <== false;

      SWITCH (mode) {
        CASE (Mode.Done) {
          // do nothing
        }
        CASE (Mode.VectorLoad) {
          IF (outputNotStall() & ~iLength.empty) {
            vectorLoadCommands.next <== vectorLoadCommands + 1;
            IF (vectorLoadCommands === vectorLoadCycles - 1) {
              mode.next <== Mode.ReadingLength;
              readLength.next <== true;
            }
            makeOutput(
                fls(), fls(), zero(inputWidth), zero(), zero(), crtPos, tru()
                );
          }
        }
        CASE (Mode.ReadingLength) {
          IF (iLengthReady === true) {
            toread.next <== iLength;
            rowLengthData.next <== iLength;
            firstReadPosition.next <== crtPos;
            IF (iLength === 0) {
              // empty row
              makeOutput(fls(), tru(), zero(inputWidth), zero(), one(), crtPos, fls());
              processRow();
            } ELSE {
              mode.next <== Mode.OutputtingCommands;
              readLength.next <== false;
              cycleCounter.next <== -1;
            }
          }
        }
        CASE (Mode.OutputtingCommands) {
          IF (outputNotStall()) {
            DFEsmValue canread = min(inputWidth - crtPos, toread);
            IF (crtPos + canread >= inputWidth) {
              crtPos.next <== 0;
            } ELSE {
              crtPos.next <== crtPos + canread;
            }
            toread.next <== toread - canread;
            makeOutput(
                crtPos === 0,
                toread - canread === 0,
                buildReadMask(canread),
                rowLengthData,
                cycleCounter + 1,
                firstReadPosition,
                fls());
            IF (toread - canread === 0) {
              processRow();
            }
          }
          }
        }
      }

      @Override
      protected void outputFunction() {
        iLength.read <== canRead() & needToRead();

        oControl.valid <== outValid;

        oControl <== oControlData;

        //oFlush <== flushData;

        if (dbg)
          IF (outValid) {
            debug.simPrintf(
                "ReadControl SM -- nPartitions %d, partitionsProcessed %d, vectorLoadCycles %d, vectorLoadCommands %d, rowsProcessed %d, iLength %d, readmask: %d, toread: %d, crtPos: %d, rowLength %d, cycleCounter %d",
                nPartitions, partitionsProcessed, vectorLoadCycles, vectorLoadCommands, rowsProcessed, iLength, readMaskData, toread, crtPos, rowLengthData, cycleCounter);
            debug.simPrintf(
                "totalOutputs %d\n", totalOutputs);
          }
      }

    void makeOutput(
        DFEsmValue readEnable,
        DFEsmValue rowFinished,
        DFEsmValue readMask,
        DFEsmValue rowLength,
        DFEsmValue cycleCounterP,
        DFEsmValue firstReadPositionP,
        DFEsmValue vectorLoad
        //DFEsmValue flush
        )
    {

      outValid.next <== true;
      //flushData.next <== flush;
      rowLengthData.next <== rowLength;
      cycleCounter.next <== cycleCounterP.cast(dfeInt(32));
      firstReadPosition.next <== firstReadPositionP;
      oControlData.next <== cat(
          readMask,
          vectorLoad, rowFinished, readEnable,
          rowLength, cycleCounterP, firstReadPositionP);
      totalOutputs.next <== totalOutputs + 1;
    }

    DFEsmValue min(DFEsmValue a, DFEsmValue b){
      DFEsmAssignableValue min = assignable.value(dfeUInt(32));
      IF ( a < b) {
        min <== a;
      } ELSE {
        min <== b;
      }
      return min;
    }

    DFEsmValue buildReadMask(DFEsmValue canread) {
      DFEsmAssignableValue pattern = assignable.value(dfeUInt(64));
      pattern <== 0;
      for (long i = 0; i <= inputWidth; i++)
        IF (canread === i)
          pattern <== (1l << i) - 1l;

      DFEsmAssignableValue newReadMask = assignable.value(dfeUInt(inputWidth));
      newReadMask <== 0;
      for (int i = 0; i <= inputWidth; i++)
        IF (crtPos === i)
          newReadMask <== pattern.shiftLeft(i).cast(dfeUInt(inputWidth));

      return newReadMask;
    }

    DFEsmValue fls() {
      return constant.value(false);
    }

    DFEsmValue tru(){
      return constant.value(true);
    }

    DFEsmValue one() {
      return constant.value(dfeUInt(32), 1);
    }

    DFEsmValue zero() {
      return constant.value(dfeUInt(32), 0);
    }

    DFEsmValue zero(int bitWidth) {
      return constant.value(dfeUInt(bitWidth), 0);
    }

    DFEsmValue zeroI() {
      return constant.value(dfeInt(32), 0);
    }

    DFEsmValue oneI() {
      return constant.value(dfeInt(32), 1);
    }

    DFEsmValue cat(DFEsmValue... args) {
      DFEsmValue first = args[0];

      for (int i = 1; i < args.length; i++) {
        first = cat(first, args[i]);
      }

      return first;
    }

    DFEsmValue cat(DFEsmValue left, DFEsmValue right) {
      int bitsLeft = left.getType().getTotalBits();
      int bitsRight = right.getType().getTotalBits();
      int totalBits = bitsRight + bitsLeft;
      DFEsmValueType t = dfeUInt(totalBits);
      DFEsmAssignableValue pattern = assignable.value(t);
      pattern <== left.cast(t).shiftLeft(bitsRight) + right.cast(t);
      return pattern;
    }

}
