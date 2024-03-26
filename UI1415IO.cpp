/* 
 *  COPYRIGHT 1998, 1999, 2000, 2019 Jay R. Jaeger
 *  
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  (file COPYING.txt) along with this program.  
 *  If not, see <https://www.gnu.org/licenses/>.
*/

//	This Unit defines the behavior of the 1415 console typewriter and
//	associated things

//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <dir.h>
#include <stdio.h>

#include "ubcd.h"
#include "UI1410CPUT.h"
#include "UIHOPPER.h"
#include "UI1410CHANNEL.h"
#include "UI1410DEBUG.h"
#include "UI1415IO.h"

#include <assert.h>

//---------------------------------------------------------------------------
#pragma resource "*.dfm"

TFI1415IO *FI1415IO;
//---------------------------------------------------------------------------
__fastcall TFI1415IO::TFI1415IO(TComponent* Owner)
	: TForm(Owner)
{
	Height = 250;
    Width = 599;
    Top = 0;
    Left = 0;

	state = CONSOLE_IDLE;
    Console_AR = 0;
    WMCtrlLatch = false;
    DisplayWMCtrlLatch = false;
    AlterFullLineLatch = false;
    AlterWMConditionLatch = false;
}
//---------------------------------------------------------------------------
void __fastcall TFI1415IO::I1415IOKeyPress(TObject *Sender, char &Key)
{
    BCD bcd_key;
    static bool last_key_was_wm;

	switch(Key) {
    case KBD_RADICAL:
        bcd_key = BCD::BCDConvert(ASCII_RADICAL);
        break;
    case KBD_RECORD_MARK:
     	bcd_key = BCD::BCDConvert(ASCII_RECORD_MARK);
        break;
    case KBD_ALT_BLANK:
     	bcd_key = BCD::BCDConvert(ASCII_ALT_BLANK);
		break;
    case KBD_WORD_SEPARATOR:
     	bcd_key = BCD::BCDConvert(ASCII_WORD_SEPARATOR);
        break;
    case KBD_SEGMENT_MARK:
     	bcd_key = BCD::BCDConvert(ASCII_SEGMENT_MARK);
		break;
    case KBD_DELTA:
     	bcd_key = BCD::BCDConvert(ASCII_DELTA);
        break;
    case KBD_GROUP_MARK:
     	bcd_key = BCD::BCDConvert(ASCII_GROUP_MARK);
        break;
    case KBD_WORD_MARK:		// Escape == Wordmark Key
		last_key_was_wm = DoWordMark();
		return;
    case 'b':
    	bcd_key = BCD::BCDConvert('B');
        break;
    default:
     	if(BCD::BCDCheck(Key) < 0) {		// Return if unmapped key.
        	LockKeyboard();
        	return;
        }
		bcd_key = BCD::BCDConvert(Key);
    }

    if(last_key_was_wm) {
    	last_key_was_wm = false;
        bcd_key.SetWM();
    }

    //	Decide what to do with key, depending on console state.
    //	Note that the really special keys (Wordmark, above, and the
    //	console inquiry buttons), are checked elswehere.  This test
    //	is for "normal" BCD characters only.

	switch(state) {

    case CONSOLE_IDLE:
    case CONSOLE_OUTPUT:
        LockKeyboard();						// Only INQ Request allowed
        return;

    case CONSOLE_NORMAL:                    // All keys allowed
    case CONSOLE_LOAD:
        assert(GetMatrix() == 32);
        bcd_key.SetOddParity();
        if(CPU -> Channel[CHANNEL1] -> ChannelStrobe(bcd_key)) {
            SendBCDTo1415(bcd_key);
        }
        DoMatrix();
     	break;

    case CONSOLE_ALTER:
    	UnlockKeyboard();
	    SendBCDTo1415(bcd_key);

        //	Ensure that the incoming character is odd parity

        bcd_key.SetOddParity();
        DEBUG("CONSOLE ALTER: Key is %d",bcd_key.ToInt())

        //	During an ALTER operation, the E Channel (channel 1) gets
        //	the character on its way to the address register (via the
        //	A channel and Assembly channel)

        CPU -> Channel[CHANNEL1] -> ChR1 -> Set(bcd_key);
        *(CPU -> Channel[CHANNEL1] -> ChR2) =
        	*(CPU -> Channel[CHANNEL1] -> ChR1);

		DoMatrix();

        break;

    case CONSOLE_ADDR:						// Only digits allowed
    	if(isdigit(Key)) {
        	UnlockKeyboard();
            SendBCDTo1415(bcd_key);

            //	Ensure that the incoming character has the correct (odd) parity

            bcd_key.SetOddParity();

            DEBUG("BCD Key set to %d",bcd_key.ToInt())

            //	During an ADDR SET operation, the E Channel (channel 1) gets
            //	the character on its way to the address register (via the
            //	A channel and Assembly channel)

            CPU -> Channel[CHANNEL1] -> ChR1 -> Set(bcd_key);
            *(CPU -> Channel[CHANNEL1] -> ChR2) =
            	*(CPU -> Channel[CHANNEL1] -> ChR1);
            DoMatrix();
        }
        else {
        	LockKeyboard();
        }
        break;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFI1415IO::I1415IOKeyDown(TObject *Sender, WORD &Key,
	TShiftState Shift)
{

	//	Special key handling: Treate Page Down as a Margin Release.

	if(Key == VK_NEXT) {
        NextLine();
    }
}
//---------------------------------------------------------------------------

//	Method to set the state of the console

bool TFI1415IO::SetState(int s)
{
	switch(s) {
    case CONSOLE_IDLE:
    	InqReq -> Enabled = true;
    	InqRlse -> Enabled = false;
		InqCancel -> Enabled = false;
        WordMark -> Enabled = false;
        CPU -> DisplayModeLatch = false;
        break;
    case CONSOLE_NORMAL:
    	InqReq -> Enabled = true;
    	InqRlse -> Enabled = true;
        InqCancel -> Enabled = true;
        WordMark -> Enabled = false;
        break;
    case CONSOLE_LOAD:
    	InqReq -> Enabled = true;
    	InqRlse -> Enabled = true;
        InqCancel -> Enabled = true;
        WordMark -> Enabled = true;
        break;
    case CONSOLE_ALTER:
    	InqReq -> Enabled = false;
    	InqRlse -> Enabled = false;
        InqCancel -> Enabled = false;
        WordMark -> Enabled = true;
        break;
    case CONSOLE_ADDR:
    	InqReq -> Enabled = false;
		InqRlse -> Enabled = false;
        InqCancel -> Enabled = false;
        WordMark -> Enabled = false;
        switch(CPU -> AddressEntry) {
		case T1410CPU::ADDR_ENTRY_I:
        	Console_AR = CPU -> I_AR;
            break;
		case T1410CPU::ADDR_ENTRY_A:
			Console_AR = CPU -> A_AR;
			break;
		case T1410CPU::ADDR_ENTRY_B:
			Console_AR = CPU -> B_AR;
			break;
		case T1410CPU::ADDR_ENTRY_C:
			Console_AR = CPU -> C_AR;
			break;
		case T1410CPU::ADDR_ENTRY_D:
			Console_AR = CPU -> D_AR;
			break;
		case T1410CPU::ADDR_ENTRY_E:
			Console_AR = CPU -> E_AR;
			break;
		case T1410CPU::ADDR_ENTRY_F:
			Console_AR = CPU -> F_AR;
			break;
		default:
			DEBUG("1415IO: Invalid CPU Address Entry case: %d",CPU->AddressEntry);
			Console_AR = NULL;
		}
		if(CPU -> DisplayModeLatch == true) {
			Console_AR = CPU -> C_AR;
		}
		break;
	case CONSOLE_DISPLAY:
    	InqReq -> Enabled = false;
    	InqRlse -> Enabled = false;
        InqCancel -> Enabled = false;
        WordMark -> Enabled = false;
        break;
    default:
    	return(false);
    }
    state = s;
    return(true);
}


//
//	Utility routine to go to next line on console
//

void TFI1415IO::NextLine()
{
	I1415IO -> Lines -> Add("");	// Placeholder for wordmarks
    I1415IO -> Lines -> Add("");

    CPU -> Display();
}

//
//	Utility routines to lock/unlock keyboard - display/clear light.
//

void TFI1415IO::LockKeyboard()
{
   	KeyboardLock -> Enabled = true;
    KeyboardLockReset -> Enabled = true;
}

void TFI1415IO::UnlockKeyboard()
{
	KeyboardLock -> Enabled = false;
	KeyboardLockReset -> Enabled = false;
}

//
//	Utility routine to handle wordmark key
//	Returns true if wordmark was valid
//

bool TFI1415IO::DoWordMark()
{
	int last;

	if(state != CONSOLE_LOAD && state != CONSOLE_ALTER) {
    	LockKeyboard();
        return(false);
    }

	last = I1415IO -> Lines -> Count - 1;
    if(last <= 0) {
    	NextLine();
    	last = I1415IO -> Lines -> Count -1;
    }

    if(I1415IO -> Lines -> Strings[last-1].Length() <=
       I1415IO -> Lines -> Strings[last].Length() ) {
	   	I1415IO -> Lines -> Strings[last-1] =
    	   	I1415IO -> Lines -> Strings[last-1] + "v";
        UnlockKeyboard();
        return(true);
	}
    else {
		LockKeyboard();
        return(false);
    }
}

//
//	Utility routine to send data to console from
//

void TFI1415IO::SendBCDTo1415(BCD bcd)
{
	int last;
	unsigned char c;

    //	Advance to next line if this is the very first line, in
    //	order to make room for first line of wordmarks

	last = I1415IO -> Lines -> Count - 1;
    if(last <= 0) {
    	NextLine();
		last = I1415IO -> Lines -> Count -1;
    }

    //	Advance wordmark line to current position if necessary

	if(I1415IO -> Lines -> Strings[last-1].Length() <=
       I1415IO -> Lines -> Strings[last].Length()) {
    	I1415IO -> Lines -> Strings[last-1] =
        	I1415IO -> Lines -> Strings[last-1] +
			(bcd.TestWM() ? "v" : " ");
    }

    //	If in display mode (registers or memory), print space as 'b'.
    //  (We are using Windows Character Code 'c' for this !!!!!)
    //	Otherwise, just put the character into the line.  (Using ToAscii
    //	helps us to be invariant about WordMarks and Parity in this
    //	situation)

	c = (state == CONSOLE_DISPLAY && bcd.ToAscii() == ' ') ?
			'c' : bcd.ToAscii();

	I1415IO -> Lines -> Strings[last] =
		I1415IO -> Lines -> Strings[last] + static_cast<wchar_t>(c);

    //	Advance to next line if this line is full
    //	If we are doing a DISPLAY, set up the alter latches for later ALTER
    //	If we are doing an ALTER, this will terminate it in matrix 33.

    if(I1415IO -> Lines -> Strings[last].Length() > 79) {
	    if(state == CONSOLE_DISPLAY) {
	    	AlterFullLineLatch = true;
	        AlterWMConditionLatch = false;
	    }
    	if(state == CONSOLE_ALTER) {
        	AlterFullLineLatch = false;
            AlterWMConditionLatch = false;
        }
    }

    //	Tell the form object it has been modified.

    I1415IO -> Modified = true;

    //	Do a full display

	//  Then again, .....  CPU -> Display();

   	//	Give Windoze a chance to breath

    Application -> ProcessMessages();

}

void TFI1415IO::DoMatrix()
{
	int savestate;
    BCD bcd_char;

	if(matrix < 0 || matrix > 42) {
    	DEBUG("Invalid Console Matrix Position: %d",matrix);
        return;
    }

//    DEBUG("Doing Console Matrix Position %d",matrix);

    //	Console Matrix Processing

    //	Positions 0 (Home) thru 36 are used for display/output purposes

	switch(matrix) {

    case 0:
    	//	Home position: do nothing
        SetState(CONSOLE_IDLE);
        break;

    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        SendBCDTo1415(CPU -> I_AR -> GateBCD(matrix));
        break;

    case 6:
    case 12:
    case 18:
    case 21:
    case 25:
    case 31:
    case 36:
    	savestate = state;
        state = CONSOLE_NORMAL;			// So space prints as one !!
		SendBCDTo1415(BCD::BCDConvert(' '));
        state = savestate;
		break;

    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    	SendBCDTo1415(CPU -> A_AR -> GateBCD(matrix-6));
        break;

    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    	SendBCDTo1415(CPU -> B_AR -> GateBCD(matrix-12));
        break;

    case 19:
    	SendBCDTo1415(CPU -> Op_Reg -> Get());
        break;

    case 20:
    	SendBCDTo1415(CPU -> Op_Mod_Reg -> Get());
        break;

    case 22:
    	SendBCDTo1415(CPU -> A_Reg -> Get());
        break;

    case 23:
    	SendBCDTo1415(CPU -> B_Reg -> Get());
        break;

    case 24:
    	SendBCDTo1415(CPU -> AssemblyChannel-> Select());
        break;

    case 26:
    	SendBCDTo1415(CPU -> Channel[CHANNEL1] -> ChUnitType -> Get());
        break;

    case 27:
    	SendBCDTo1415(CPU -> Channel[CHANNEL1] -> ChUnitNumber -> Get());
        break;

    case 28:
#if	MAXCHANNEL > 1
       	SendBCDTo1415(CPU -> Channel[CHANNEL2] -> ChUnitType -> Get());
#endif
        break;

    case 29:
#if MAXCHANNEL > 1
       	SendBCDTo1415(CPU -> Channel[CHANNEL2] -> ChUnitNumber -> Get());
#endif
        break;

	case 30:
    	if(CPU -> DisplayModeLatch) {
        	SendBCDTo1415(BCD::BCDConvert('D'));
        }
        else if(state == CONSOLE_ALTER) {
        	SendBCDTo1415(BCD::BCDConvert('A'));
        }
        else if(CPU -> Channel[CHANNEL1] -> ChWrite -> State() &&
                CPU -> Channel[CHANNEL1] -> GetDeviceNumber() == CONSOLE_IO_DEVICE) {
            SendBCDTo1415(BCD::BCDConvert('R'));
            SetState(CONSOLE_OUTPUT);
        }
        else if(CPU -> Channel[CHANNEL1] -> ChRead -> State() &&
                CPU -> Channel[CHANNEL1] -> GetDeviceNumber() == CONSOLE_IO_DEVICE) {
            SendBCDTo1415(BCD::BCDConvert('I'));
        }
        break;

    case 32:
    	if(CPU -> DisplayModeLatch) {
        	SendBCDTo1415(CPU -> B_Reg -> Get());
	        WMCtrlLatch = CPU -> B_Reg -> Get().TestWM();
    	    StepMatrix();
        }
        else if(state == CONSOLE_ALTER) {
        	CPU -> SetScan(SCAN_2);
            CPU -> CycleRing -> Set(CYCLE_D);
            *(CPU -> STAR) = *(CPU -> C_AR);
			CPU -> Store(CPU -> AssemblyChannel -> GateAChannelToAssembly(
            	CPU -> AChannel -> Select(CPU -> AChannel -> A_Channel_E)));
            CPU -> D_AR -> Set(CPU -> STARScan());
            CPU -> CycleRing -> Set(CYCLE_D);
            CPU -> SetScan(SCAN_N);
            *(CPU -> STAR) = *(CPU -> D_AR);
            CPU -> Readout();
            if(CPU -> StorageWrapCheck(+1) || CPU -> B_Reg -> Get().TestWM()) {
            	AlterWMConditionLatch = false;
            }
        	StepMatrix();
            if(!(AlterWMConditionLatch || AlterFullLineLatch)) {
            	StepMatrix();		// Right to 34
                DoMatrix();
				ResetMatrix();
                SetState(CONSOLE_IDLE);
            }
        }
        else if(CPU -> Channel[CHANNEL1] -> ChWrite -> State() &&
                CPU -> Channel[CHANNEL1] -> GetDeviceNumber() == CONSOLE_IO_DEVICE) {
            bcd_char = CPU -> Channel[CHANNEL1] -> ChR2 -> Get();
            savestate = state;
            if(CPU -> Channel[CHANNEL1] -> MoveMode) {
                bcd_char.ClearWM();                 //  No wordmark;
                state = CONSOLE_NORMAL;             //  So space is a space.
            }
            else {
                state = CONSOLE_DISPLAY;            //  So space is 'b'
            }
            SendBCDTo1415(bcd_char);                //  Send out the char
            state = savestate;                      //  Restore state.
        }
        else if(state == CONSOLE_LOAD || state == CONSOLE_NORMAL) {
            //  This situation is handled up in keypress handler method
        }
        break;

    case 33:
    	if(CPU -> DisplayModeLatch) {
        	SendBCDTo1415(CPU -> B_Reg -> Get());
			WMCtrlLatch = CPU -> B_Reg -> Get().TestWM();
            if(WMCtrlLatch && !AlterFullLineLatch) {
            	AlterWMConditionLatch = true;
    	    }
        	DisplayWMCtrlLatch = WMCtrlLatch;
        }
        else if(state == CONSOLE_ALTER) {
        	CPU -> SetScan(SCAN_2);
            CPU -> CycleRing -> Set(CYCLE_D);
            *(CPU -> STAR) = *(CPU -> D_AR);
            CPU -> Store(CPU -> AssemblyChannel -> GateAChannelToAssembly(
            	CPU -> AChannel -> Select(CPU -> AChannel -> A_Channel_E)));
            CPU -> D_AR -> Set(CPU -> STARScan());
            CPU -> CycleRing -> Set(CYCLE_D);
            CPU -> SetScan(SCAN_N);
            *(CPU -> STAR) = *(CPU -> D_AR);
            CPU -> Readout();
            if(CPU -> StorageWrapCheck(+1) || CPU -> B_Reg -> Get().TestWM()) {
            	AlterWMConditionLatch = false;
            }
            if(!(AlterWMConditionLatch || AlterFullLineLatch)) {
            	StepMatrix();		// Right to 34
                DoMatrix();
				ResetMatrix();
                SetState(CONSOLE_IDLE);
            }
        }
        break;

	case 34:
    	NextLine();
        break;

	//	Positions 37 thru 42 are to accept an address for address set.
    //	For 41, do the last digit, then fall thru to processing for 42.

    case 37:
    case 38:
    case 39:
    case 40:
    case 41:
    	bcd_char = CPU -> AssemblyChannel -> GateAChannelToAssembly(
        	CPU -> AChannel -> Select(CPU -> AChannel -> A_Channel_E));
    	DEBUG("Value to set into register from console is %d",bcd_char.ToInt())
    	Console_AR -> Set(TWOOF5(bcd_char),matrix-36);
        StepMatrix();
    	if(matrix != 42) {
        	break;
        }

        //	Fall thru to next entry for last digit!!

	//	At the end of an address set, go to next line, and reset console

    case 42:
    	NextLine();
       	if(CPU -> DisplayModeLatch) {
        	SetState(CONSOLE_DISPLAY);
            DoDisplay(2);
        }
        else {
	        ResetMatrix();
    	    SetState(CONSOLE_IDLE);
        }
        break;

    default:
    	break;
    }

	//    CPU -> Display();
}

//	Handle a console stop print-out operation

void TFI1415IO::StopPrintOut(char c)
{
	SetState(CONSOLE_DISPLAY);
	NextLine();
    SetMatrix(35);
    SendBCDTo1415(BCD::BCDConvert(c));
    StepMatrix();
    DoMatrix();
    SetMatrix(1);
    while(GetMatrixX() < 5) {
    	DoMatrix();
        StepMatrix();
    }


    while(GetMatrixY() < 6) {
    	DoMatrix();
        StepMatrix();
    }
    SetMatrix(34);
    DoMatrix();
    SetMatrix(CONSOLE_MATRIX_HOME);
    DoMatrix();
    CPU -> Display();
}

//	Initiate a Console Address Set operation

void TFI1415IO::DoAddressEntry()
{
    SetState(CONSOLE_ADDR);
    Console_AR -> Reset();
    SetMatrix(35);
    if(CPU -> DisplayModeLatch) {
    	SendBCDTo1415(BCD::BCDConvert('D'));
    }
    else {
	    SendBCDTo1415(BCD::BCDConvert((Console_AR == CPU -> I_AR) ? 'B' : '#'));
    }
    WindowState = wsNormal;
    SetFocus();
    BringToFront();
    StepMatrix();
    DoMatrix();
	SetMatrix(37);

    //	The console matrix code will automatically handle the rest, so
    //	just return!

}

//	Initiate a Memory Display operation

void TFI1415IO::DoDisplay(int phase)
{
	//	Phase 1 takes us thru the address entry.  When the address entry
    //	is done, the code in the matrix notices we are doing a display,
    //	and fires up Phase 2 (like co-routines).

    //	We also come here if START is pressed to continue a display, in
    //	which case the DisplayWMCtrlLatch will be set.

	if(phase == 1) {
        WindowState = wsNormal;
        SetFocus();
        BringToFront();
    	if(DisplayWMCtrlLatch) {
        	DisplayWMCtrlLatch = false;
            phase = 3;
        }
        else {
			CPU -> StopKeyLatch = false;
		    CPU -> IRing -> Reset();
        	WMCtrlLatch = DisplayWMCtrlLatch =
            	AlterWMConditionLatch = AlterFullLineLatch = false;

	    	// The DisplayModeLatch causes No A Ch, B Ch, Address Wrap Checks
		    // It also forces the Address Entry routine to print 'D', and put
	    	// the resultant address into CAR

	    	CPU -> DisplayModeLatch = true;

		    DoAddressEntry();
        	return;
        }
    }

    //	Phase 2 handles the beginning of the actual display part thru matrix
    //	position 31.

	if(phase == 2) {
    	matrix = 30;
		DoMatrix();							//	Prints "D"
    	StepMatrix();
        DoMatrix();							//	Prints a space
        CPU -> SetScan(SCAN_2);				//	Read out first character
        CPU -> CycleRing -> Set(CYCLE_D);
        *(CPU -> STAR) = *(CPU -> C_AR);
        CPU -> Readout();
        CPU -> StorageWrapCheck(+1);
        StepMatrix();						//	Step to matrix position 32
        phase = 3;
    }

    //	Phase 3 handles normal circumstances for matrix position 33.  The
    //	display is stopped by the STOP key (or moving the MODE switch)
    //	(which goes to phase 4), or by hitting a wordmark, or by wrapping
    //	storage.  (The POO and the CE Instructional materials disagree on
    //	this last point - the POO says it keeps going, the Instructional
    //	materials say the display stops)

    if(phase == 3) {
    	while(true) {
            if(DisplayWMCtrlLatch) {
                break;
            }
           	DoMatrix();
            CPU -> D_AR -> Set(CPU -> STARScan());
            CPU -> SetScan(SCAN_2);
            CPU -> CycleRing -> Set(CYCLE_D);
            *(CPU -> STAR) = *(CPU -> D_AR);

            //	Check for any of: STOP Key/MODE Change, Address Wrap.

            if(CPU -> StopKeyLatch || CPU -> StorageWrapLatch) {
                CPU -> StopKeyLatch = true;
                AlterWMConditionLatch = true;
            	phase = 4;
                break;
            }
            else {
            	CPU -> Readout();
                if(CPU -> StorageWrapCheck(+1)) {
                	phase = 4;
                    CPU -> StopKeyLatch = true;
                    AlterWMConditionLatch = true;
                    break;
                }
            }
        }
    }

    if(phase == 4 && (state == CONSOLE_DISPLAY || state == CONSOLE_ALTER)) {
	    DisplayWMCtrlLatch = false;
        if(state == CONSOLE_ALTER) {
	        AlterWMConditionLatch = false;
        }
        StepMatrix();					// To Position 34.
        DoMatrix();
        ResetMatrix();
        SetState(CONSOLE_IDLE);
	}
}

//	Initiate a memory alter operation
//	A DISPLAY must have happened first (we check the latches), so that
//	CAR has the starting address, and either AlterFullLineLatch or
//	AlterWMConditionLatch is set

void TFI1415IO::DoAlter()
{
	if(!(AlterWMConditionLatch || AlterFullLineLatch)) {
    	DEBUG("Cannot start ALTER without doing DISPLAY FIRST.")
        return;
    }

    SetState(CONSOLE_ALTER);
    WindowState = wsNormal;
    SetFocus();
    BringToFront();

    matrix = 30;				//	Print "A"
    DoMatrix();

    StepMatrix();
    DoMatrix();					//	Print a space

    StepMatrix();				//	Advance matrix to 32

    //	Now we wait for characters...

    //	NOTE: STOP KEY processing for Alter is handled in DoDisplay(4) !!
}

void __fastcall TFI1415IO::WordMarkClick(TObject *Sender)
{
	DoWordMark();
	FocusControl(I1415IO);
}
//---------------------------------------------------------------------------
void __fastcall TFI1415IO::MarginReleaseClick(TObject *Sender)
{
	NextLine();
    FocusControl(I1415IO);
    if(state == CONSOLE_DISPLAY) {
    	AlterFullLineLatch = true;
        AlterWMConditionLatch = false;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFI1415IO::KeyboardLockResetTimer(TObject *Sender)
{
	UnlockKeyboard();
}
//---------------------------------------------------------------------------

//  1415 Console Input/Ouptut (Channel) Device class implementation

T1415Console::T1415Console(int devicenum,T1410Channel *Channel) :
    T1410IODevice(devicenum,Channel) {

    IOInProgress = false;
}

//  Initial I/O Startup (also known as Status Sample A time)

int T1415Console::Select() {

    //  If this is not channel 0, or not unit 0, return NOT READY

    if(Channel -> GetUnitNumber() != 0 || Channel != CPU -> Channel[CHANNEL1]) {
        return(IOCHNOTREADY);
    }

    //  If the console matrix is not at home, or if the busy timer is still
    //  running, return BUSY

    if(FI1415IO -> GetMatrix() != CONSOLE_MATRIX_HOME ||
        FI1415IO -> BusyTimer -> Enabled) {
        return(IOCHBUSY);
    }

    //  I/O NOP.  For output, if we get here, everything is OK.
    //  For input, if the Inquiry Request latch isn't set, return NO TRANSFER

    if(Channel -> ChNOP) {
        if(Channel -> ChWrite -> State()) {
            return(0);
        }
        if(!CPU -> InqReqLatch) {
            return(IOCHNOTRANSFER);
        }
        return(0);
    }

    //  Set up the console appropriately, depending on whether we are
    //  reading or writing.

    if(Channel -> ChWrite -> State()) {
        FI1415IO -> SetMatrix(30);
    }
    else {
        if(!CPU -> InqReqLatch) {
            return(IOCHNOTRANSFER);
        }
        CPU -> Channel[CHANNEL1] -> PriorityRequest &= ~PRINQUIRY;
        FI1415IO -> SetMatrix(5,6);
        FI1415IO -> DoMatrix();
        FI1415IO -> StepMatrix();
        FI1415IO -> DoMatrix();
        FI1415IO -> StepMatrix();
        FI1415IO -> SetState(Channel -> LoadMode ? CONSOLE_LOAD : CONSOLE_NORMAL);
        FI1415IO -> WindowState = wsNormal;
        FI1415IO -> BringToFront();
        FI1415IO -> I1415IO -> SetFocus();
    }
    return(0);
}

//  Handle an output request from the I/O instruction or Channel

void T1415Console::DoOutput() {

    //  We might still be sending the "R ".  If so, finish that first.

    while(FI1415IO -> GetMatrix() != 32) {
        FI1415IO -> DoMatrix();
        FI1415IO -> StepMatrix();
    }

    //  Send out a character.

    FI1415IO -> DoMatrix();

    Channel -> OutputRequest = true;
    Channel -> CycleRequired = true;
}

void T1415Console::DoInput() {

    //  Input is handled in the Keypress method.
}

//  Finally, at the end of an operation, Finish things up, and check
//  the final outcome.  It also returns the current channel status as
//  a final device status.

int T1415Console::StatusSample() {

    if(Channel -> ChWrite -> State()) {
        FI1415IO -> SetMatrix(34);
        FI1415IO -> DoMatrix();
        FI1415IO -> ResetMatrix();
        FI1415IO -> BusyTimer -> Enabled = true;
        return(Channel -> GetStatus());
    }
    else {
        return(Channel -> GetStatus());
    }
}

void __fastcall TFI1415IO::InqReqClick(TObject *Sender)
{
    CPU -> InqReqLatch = true;
    CPU -> Channel[CHANNEL1] -> PriorityRequest |= PRINQUIRY;
    InqRlse -> Enabled = true;
    InqCancel -> Enabled = true;
}
//---------------------------------------------------------------------------

void __fastcall TFI1415IO::InqCancelClick(TObject *Sender)
{
    CPU -> InqReqLatch = false;
    CPU -> Channel[CHANNEL1] -> PriorityRequest &= ~PRINQUIRY;
    InqRlse -> Enabled = false;
    InqCancel -> Enabled = false;
    if(GetMatrix() != CONSOLE_MATRIX_HOME) {
        FI1415IO -> SetMatrix(34);
        FI1415IO -> DoMatrix();
        FI1415IO -> ResetMatrix();
    }
    CPU -> Channel[CHANNEL1] -> ExtEndofTransfer = true;
    CPU -> Channel[CHANNEL1] -> SetStatus(
        CPU -> Channel[CHANNEL1] -> GetStatus() | IOCHCONDITION );
}
//---------------------------------------------------------------------------

void __fastcall TFI1415IO::InqRlseClick(TObject *Sender)
{
    CPU -> InqReqLatch = false;
    CPU -> Channel[CHANNEL1] -> PriorityRequest &= ~PRINQUIRY;
    InqRlse -> Enabled = false;
    InqCancel -> Enabled = false;
    FI1415IO -> SetMatrix(34);
    FI1415IO -> DoMatrix();
    FI1415IO -> ResetMatrix();
    CPU -> Channel[CHANNEL1] -> ExtEndofTransfer = true;
}
//---------------------------------------------------------------------------

void __fastcall TFI1415IO::BusyTimerTimer(TObject *Sender)
{
    BusyTimer -> Enabled = false;
}
//---------------------------------------------------------------------------

