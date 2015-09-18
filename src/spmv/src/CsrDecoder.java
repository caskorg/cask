import com.maxeler.maxcompiler.v2.managers.DFEManager;
import com.maxeler.maxcompiler.v2.statemachine.*;
import com.maxeler.maxcompiler.v2.statemachine.manager.*;

// XXX must reset the CSRDecoder (at least set prevData, counter etc. to zero)
// after a matrix has been fully processed
public class CsrDecoder extends ManagerStateMachine {

    private final DFEsmPullInput iColptr; // containing the indices
    private final DFEsmInput iRowCount;

    private final DFEsmPushOutput oRowLength;

    private final DFEsmStateValue prevData, rowLength;
    private final DFEsmStateValue colptrReady;
    private final DFEsmStateValue rowLengthValid;
    private final DFEsmStateValue rowsProcessed;

    private final int id;

    //    private final DFEsmStateValue sCycleCounter = state.value(dfeUInt(64), 0);
    private boolean dbg = false;

    public CsrDecoder(DFEManager owner, boolean debug) {
        super(owner);
        this.dbg = debug;
        this.id = 1;

        // rowEnd = state.value(dfeUInt(32));
        rowLength = state.value(dfeUInt(32));
        prevData = state.value(dfeUInt(32), 0);
        rowsProcessed = state.value(dfeUInt(32), 0);

        colptrReady = state.value(dfeBool(), false);

        rowLengthValid = state.value(dfeBool(), false);

        // --- inputs
        iColptr = io.pullInput("colptr", dfeUInt(32));
        iRowCount = io.scalarInput("nrows", dfeUInt(32));
        // TODO must discard first value of column pointer

        // -- outputs
        //oRowEnd = io.pushOutput("rowEnd_out", dfeUInt(32), 1);
        oRowLength = io.pushOutput("rowLength_out", dfeUInt(32), 1);
    }

    private DFEsmValue colptrReady() {
        return ~iColptr.empty & ~oRowLength.stall;
    }

    @Override
    protected void nextState() {
      colptrReady.next <== colptrReady();
      rowLengthValid.next <== false;
      IF (colptrReady === true) {
        rowLength.next <== iColptr - prevData;
        rowLengthValid.next <== true;
        prevData.next <== iColptr;
        rowsProcessed.next <== rowsProcessed + 1;
        IF (rowsProcessed  + 1 === iRowCount) {
          prevData.next <== 0;
          rowsProcessed.next <== 0;
        }
      }
    }

    @Override
    protected void outputFunction() {
        iColptr.read <== colptrReady();
        oRowLength.valid <== rowLengthValid;
        oRowLength <== rowLength;
        if (dbg)
          IF (rowLengthValid)
            debug.simPrintf(
                "CsrDecoder SM %d - rowLength: %d, prevData: %d\n",
                id, rowLength, prevData);
    }
}
