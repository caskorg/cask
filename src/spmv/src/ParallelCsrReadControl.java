import com.maxeler.maxcompiler.v2.statemachine.*;
import com.maxeler.maxcompiler.v2.statemachine.manager.*;
import com.maxeler.maxcompiler.v2.managers.DFEManager;

import com.maxeler.maxcompiler.v2.statemachine.*;
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
    private final DFEsmPushOutput oReadMask, oReadEnable, oRowFinished, oRowLength, oNnzCounter, oFirstReadPosition;
    //private final DFEsmPushOutput oFlush;
    private final DFEsmInput vectorLoadCycles, nRows, nPartitions;

    private final DFEsmPushOutput oVectorLoad;
    private final DFEsmStateValue outValid;
    private final DFEsmStateValue readEnableData, readMaskData, rowFinishedData, rowLengthData, vectorLoadData;
    private final DFEsmStateValue cycleCounter;
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

      oReadMask    = io.pushOutput("readmask", dfeUInt(inputWidth), 1);
      oReadEnable  = io.pushOutput("readenable", dfeBool(), 1);
      oRowFinished = io.pushOutput("rowFinished", dfeBool(), 1);
      oRowLength   = io.pushOutput("rowLength", dfeUInt(32), 1);
      oNnzCounter  = io.pushOutput("cycleCounter", dfeUInt(32), 1);
      oFirstReadPosition  = io.pushOutput("firstReadPosition", dfeUInt(32), 1);
      oVectorLoad  = io.pushOutput("vectorLoad", dfeBool(), 1);
      //oFlush  = io.pushOutput("flush", dfeBool(), 1);

      cycleCounter = state.value(dfeInt(32), 0);
      crtPos = state.value(dfeUInt(32), 0);
      firstReadPosition = state.value(dfeUInt(32), 0);
      toread = state.value(dfeUInt(32), 0);
      iLengthRead = state.value(dfeBool(), false);
      readEnableData = state.value(dfeBool(), false);
      rowFinishedData = state.value(dfeBool(), false);
      vectorLoadData = state.value(dfeBool(), false);
      //flushData = state.value(dfeBool(), false);
      rowLengthData = state.value(dfeUInt(32), 0);
      outValid = state.value(dfeBool(), false);
      readMaskData = state.value(dfeUInt(inputWidth));
      iLengthReady = state.value(dfeBool(), false);

      // internal state data, not for output
      rowsProcessed = state.value(dfeUInt(32), 0);
      vectorLoadCommands = state.value(dfeUInt(32), 0);
      partitionsProcessed = state.value(dfeUInt(32), 0);
      totalOutputs = state.value(dfeUInt(32), 0);
      paddedOutputs = state.value(dfeUInt(32), 0);
    }

    DFEsmValue outputNotStall() {
      return ~oReadEnable.stall & ~oReadMask.stall
        & ~oRowFinished.stall & ~oRowLength.stall
        & ~oNnzCounter.stall & ~oFirstReadPosition.stall
        & ~oVectorLoad.stall;
        //& ~oFlush.stall;
    }

    DFEsmValue iLengthReady() {
      return ~iLength.empty & outputNotStall() & mode === Mode.ReadingLength;
    }

    void processRow() {
      rowsProcessed.next <== rowsProcessed + 1;
      IF (rowsProcessed === nRows - 1) {
        rowsProcessed.next <== 0;
        vectorLoadCommands.next <== 0;
        crtPos.next <== 0;
        iLengthReady.next <== false;
        mode.next <== Mode.VectorLoad;
        partitionsProcessed.next <== partitionsProcessed + 1;
        IF (partitionsProcessed === nPartitions - 1) {
          //mode.next <== Mode.Flush;
          mode.next <== Mode.Done;
        }
      } ELSE {
        mode.next <== Mode.ReadingLength;
        iLengthRead.next <== true;
      }
    }

    @Override
    protected void nextState() {
      outValid.next <== false;

      iLengthReady.next <== iLengthReady();

      SWITCH (mode) {
        //CASE (Mode.Flush) {
           //send flush signal to kernels
          //makeOutput(fls(), fls(), zero(inputWidth), zero(), zeroI(), crtPos, fls(), tru());
          //mode.next <== Mode.Done;
        //}
        CASE (Mode.Done) {
          // do nothing
        }
        CASE (Mode.VectorLoad) {
          IF (outputNotStall() & ~iLength.empty) {
            vectorLoadCommands.next <== vectorLoadCommands + 1;
            IF (vectorLoadCommands === vectorLoadCycles - 1) {
              mode.next <== Mode.ReadingLength;
              iLengthRead.next <== true;
            }
            makeOutput(
                fls(), fls(), zero(inputWidth), zero(), zeroI(), crtPos, tru()
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
              makeOutput(fls(), tru(), zero(inputWidth), zero(), oneI(), crtPos, fls());
              processRow();
            } ELSE {
              mode.next <== Mode.OutputtingCommands;
              iLengthReady.next <== false;
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
        iLength.read <== iLengthReady();

        oReadEnable.valid <== outValid;
        oReadMask.valid <== outValid;
        oRowLength.valid <== outValid;
        oRowFinished.valid <== outValid;
        oNnzCounter.valid <== outValid;
        oFirstReadPosition.valid <== outValid;
        oVectorLoad.valid <== outValid;
        //oFlush.valid <== outValid;

        oReadEnable <==readEnableData;
        oReadMask <== readMaskData;
        oRowLength <== rowLengthData;
        oRowFinished <== rowFinishedData;
        oNnzCounter <== cycleCounter.cast(dfeUInt(32));
        oFirstReadPosition <== firstReadPosition;
        oVectorLoad <== vectorLoadData;

        //oFlush <== flushData;

        if (dbg)
          IF (outValid) {
            debug.simPrintf(
                "ReadControl SM -- nPartitions %d, partitionsProcessed %d, vectorLoadCycles %d, vectorLoadCommands %d, vectorLoad %d rowsProcessed %d, iLength %d, readmask: %d, readeenable: %d toread: %d, crtPos: %d, rowLength %d, rowFinished %d cycleCounter %d",
                nPartitions, partitionsProcessed, vectorLoadCycles, vectorLoadCommands, vectorLoadData, rowsProcessed, iLength, readMaskData, readEnableData, toread, crtPos, rowLengthData, rowFinishedData, cycleCounter);
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

      readEnableData.next <== readEnable;
      rowFinishedData.next <== rowFinished;
      readMaskData.next <== readMask;
      rowLengthData.next <== rowLength;
      cycleCounter.next <== cycleCounterP;
      firstReadPosition.next <== firstReadPositionP;
      vectorLoadData.next <== vectorLoad;
      //flushData.next <== flush;

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


}
