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

//	Implementation of I/O Channel Class

//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <dir.h>

#include "ubcd.h"
#include "UI1410CPUT.h"
#include "UIHOPPER.h"
#include "UI1410CHANNEL.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)

#include <assert.h>
#include <stdio.h>

#include "UI1410DEBUG.h"
#include "UI1410INST.h"
#include "UI1415IO.h"
#include "UI1415CE.h"

//	Constructor.  Initializes state

T1410Channel::T1410Channel(
        TAddressRegister *Addr,
    	TLabel *LampInterlock,
        TLabel *LampRBCInterlock,
        TLabel *LampRead,
        TLabel *LampWrite,
        TLabel *LampOverlap,
        TLabel *LampNotOverlap,
        TLabel *LampNotReady,
        TLabel *LampBusy,
        TLabel *LampDataCheck,
        TLabel *LampCondition,
        TLabel *LampWLRecord,
        TLabel *LampNoTransfer ) {

    int i;

    ChStatus = 0;
    TapeDensity = DENSITY_200_556;
    R1Status = R2Status = false;

    ChStatusDisplay[IOLAMPNOTREADY] = LampNotReady;
    ChStatusDisplay[IOLAMPBUSY] = LampBusy;
    ChStatusDisplay[IOLAMPDATACHECK] = LampDataCheck;
    ChStatusDisplay[IOLAMPCONDITION] = LampCondition;
    ChStatusDisplay[IOLAMPNOTRANSFER] = LampNoTransfer;
    ChStatusDisplay[IOLAMPWLRECORD] = LampWLRecord;

    //	Generally, the channel latches are *not* reset by Program Reset

    ChInterlock = new TDisplayLatch(LampInterlock,false);
    ChRBCInterlock = new TDisplayLatch(LampRBCInterlock,false);
    ChRead = new TDisplayLatch(LampRead,false);
    ChWrite = new TDisplayLatch(LampWrite,false);
    ChOverlap = new TDisplayLatch(LampOverlap,false);
    ChNotOverlap = new TDisplayLatch(LampNotOverlap,false);

    //	Generally, the channel registers are *not* reset by Program Reset

    ChOp = new TRegister(false);
    ChUnitType = new TRegister(false);
    ChUnitNumber = new TRegister(false);
    ChR1 = new TRegister(false);
    ChR2 = new TRegister(false);

    //  Set up a pointer to the associated address register

    ChAddr = Addr;

    //  Clear the device table

    for(i=0; i < 64; ++i) {
        Devices[i] = NULL;
    }

    for(i=0; i<5; ++i) {
        Hopper[i] = NULL;
    }

    CurrentDevice = NULL;
    MoveMode = LoadMode = false;
    ExtEndofTransfer = IntEndofTransfer = false;
    CycleRequired = false;
    InputRequest = false;
    OutputRequest = false;
    LastInputCycle = false;
    EndofRecord = false;
    TapeIndicate = false;
    UnitControlOverlapBusy = NULL;
    PriorityRequest = 0;
    ChNOP = false;
}

//  Channel Register methods

BCD T1410Channel::SetR1(BCD b) {
    ChR1 -> Set(b);
    R1Status = true;
    return(b);
};

BCD T1410Channel::SetR2(BCD b) {
    ChR2 -> Set(b);
    R2Status = true;
    return(b);
};

//  Move data from register 1 to register 2
//  Clearing register 1 in the process.

BCD T1410Channel::MoveR1R2() {
    ChR2 -> Set(ChR1 -> Get());
    R1Status = false;
    R2Status = true;
    return(ChR2 -> Get());
}

//  Method to add a new device to the channel's device table.
//  Typically this is called from the T1410IODevice base class constructor

void T1410Channel::AddIODevice(T1410IODevice *iodevice, int devicenumber) {
    assert(Devices[devicenumber] == NULL);
    Devices[devicenumber] = iodevice;
}

//	Channel is reset during ComputerReset

void T1410Channel::OnComputerReset()
{
    Reset();
    PriorityRequest = 0;
}

void T1410Channel::Reset() {
	ChStatus = 0;
    R1Status = R2Status = false;
    MoveMode = LoadMode = false;
    ExtEndofTransfer = IntEndofTransfer = false;
    CycleRequired = false;
    InputRequest = false;
    LastInputCycle = false;
    EndofRecord = false;
    ChInterlock -> Reset();
    ChRBCInterlock -> Reset();
    ChRead -> Reset();
    ChWrite -> Reset();
    ChOverlap -> Reset();
    ChNotOverlap -> Reset();
    UnitControlOverlapBusy = NULL;
    ChNOP = false;
    //  Do *not* reset PriorityRequest!
}

//	Channel is not reset during Program Reset

void T1410Channel::OnProgramReset()
{
	//	Channel not affected by Program Reset
}

//	Display Routine.

void T1410Channel::Display() {

	int i;

	ChStatusDisplay[IOLAMPNOTREADY] -> Enabled =
    	((ChStatus & IOCHNOTREADY) != 0);
	ChStatusDisplay[IOLAMPBUSY] -> Enabled =
    	((ChStatus & IOCHBUSY) != 0);
    ChStatusDisplay[IOLAMPDATACHECK] -> Enabled =
    	((ChStatus & IOCHDATACHECK) != 0);
    ChStatusDisplay[IOLAMPCONDITION] -> Enabled =
    	((ChStatus & IOCHCONDITION) != 0);
    ChStatusDisplay[IOLAMPWLRECORD] -> Enabled =
    	((ChStatus & IOCHWLRECORD) != 0);
    ChStatusDisplay[IOLAMPNOTRANSFER] -> Enabled =
    	((ChStatus & IOCHNOTRANSFER) != 0);

    for(i=0; i <= 5; ++i) {
    	ChStatusDisplay[i] -> Repaint();
    }

	//	Although in most instances the following would be redundant,
    //	because these objects are also on the CPU display list, we include
    //	them here in case we want to display a channel separately.

    ChInterlock -> Display();
    ChRBCInterlock -> Display();
    ChRead -> Display();
    ChWrite -> Display();
    ChOverlap -> Display();
    ChNotOverlap -> Display();
}

//	Channel Lamp Test

void T1410Channel::LampTest(bool b)
{
	int i;

    //	Note, we don't have to do anything to the TDisplayLatch objects in
    //	the channel for lamp test.  They will take care of themselves on a
    //	lamp test.

    if(!b) {
    	for(i=0; i <= 5; ++i) {
        	ChStatusDisplay[i] -> Enabled = true;
            ChStatusDisplay[i] -> Repaint();
        }
    }
    else {
    	Display();
    }
}

//  Routine to return pointer to a given device.  Usually used when
//  CPU needs a specific device for some other operation (e.g., branch
//  on carriage channel needs a pointer to the printer device on
//  channel 1.

T1410IODevice *T1410Channel::GetIODevice(int device) {

    if(device < 0 || device > 64) {
        return(NULL);
    }
    return(Devices[device]);
}

//  Channel Ouptut to Device
//  Conditions on Entry: B data register (B_REG) contains character to output
//  Channel Register 1 (E1 or F1) should be emptry.

void T1410Channel::DoOutput(TAddressRegister *addr) {

    BCD tempbcd;

    CPU -> CycleRing -> Set(this == CPU -> Channel[CHANNEL1] ? CYCLE_E : CYCLE_F);

    //  Read out the next character.  Also, check for a storage wrap, which will
    //  end the transfer after this character.

    *(CPU -> STAR) = *addr;
    CPU -> Readout();
    CPU -> StorageWrapCheck(+1);                //  Maybe set StorageWrapLatch
    tempbcd = CPU -> B_Reg -> Get();

    //  If we have a GMWM, and we are not doing a Write to End of Core,
    //  then we are done transferring data from memory.  (End of core
    //  operations attempted in overlap mode behave as though they were
    //  normal operations on the 1410).

    //  Note: TEST CHANGE: Do NOT clear WM in move mode.  Let device
    //  see it.  Reason:  M%21 needs to see it to print WM as 1.
    //  Otherwise, Clear the WM from the input data if in Move mode

    //  Then (regardless of mode) transfer the data to Channel register R1
    //  Also, if we actually have read a character out, check for storage
    //  wrap, which also ends the transfer.

    if(tempbcd.TestGMWM() &&
        (ChOverlap -> State() ||
         CPU -> Op_Mod_Reg -> Get().To6Bit() != OP_MOD_SYMBOL_X )) {
        IntEndofTransfer = true;
        EndofRecord = true;
    }
    else {
        /*
        if(MoveMode) {
            tempbcd.ClearWM();
            tempbcd.SetOddParity();
        }
        */
        SetR1(tempbcd);
        if(CPU -> StorageWrapLatch) {
            IntEndofTransfer = true;
            EndofRecord = true;
        }
    }

    //  Now, if we have anything for the device, send it.  We continue
    //  doing this for up to two characters, in case R1 and R2 are both
    //  full.  We have to do it this way so that if, at the end, we set
    //  ExtEndofTransfer, all of the data will have been sent.

    while(GetR1Status() || GetR2Status()) {
        if(GetR1Status() & !GetR2Status()) {
            MoveR1R2();
        }
        CurrentDevice -> DoOutput();
        ResetR2();
    }

    //  Advance the appropriate address register to the next location.
    //  Storage wrap was detected earlier. If there is a wrap, we
    //  end the operation with the register at End of Memory +1 - according
    //  to the diagnostics, at least.

    if(!CPU -> StorageWrapLatch) {
        addr -> Set(CPU -> STARMod(+1));
    }
    else {
        addr -> Set(STORAGE);
    }

    //  Now that the data is all sent, if we are at IntEndofTransfer (either
    //  From a GMWM or from storage wrap), also set ExtEndofTransfer so that
    //  we quit.  Note that the device may have already set ExtEndofTransfer!

    if(IntEndofTransfer) {
        ExtEndofTransfer = true;
    }

}

//  Channel Input Processing (shared overlap/not overlap code)

void T1410Channel::DoInput(TAddressRegister *addr) {

    CycleRequired = false;                          //  Reset cycle required
    InputRequest = false;                           //  Reset co-routine flag

    if(!IntEndofTransfer) {                         //  Skip this if wrapped!
        CPU -> CycleRing ->
            Set(this == CPU -> Channel[CHANNEL1] ? CYCLE_E : CYCLE_F);
        *(CPU -> STAR) = *addr;                     //  Copy memory address
        CPU -> Readout();                           //  Get existing memory
        CPU -> StorageWrapCheck(+1);                //  Check for storage wrap
        CPU -> AChannel -> Select(                  //  Gate A channel approp.
            this == CPU -> Channel[CHANNEL1] ?
                TAChannel::A_Channel_E : TAChannel::A_Channel_F );
    }

    //  If no data from device, the end is near...

    if(!(GetR1Status() || GetR2Status())) {
        LastInputCycle = true;
    }

    //  If B GMWM, we are typically done storing data (unless this
    //  is read to end of core, which ignores GMWM).  Unless, of course,
    //  we have already hit end of record.

    //  Note that we can only check the op mod if we are NOT overlapped
    //  (Note that the "to end of core" mods are all not overlapped)

    if(!EndofRecord && CPU -> B_Reg -> Get().TestGMWM()) {
        if(ChOverlap -> State() ||
           (CPU -> Op_Mod_Reg -> Get().To6Bit()) != OP_MOD_SYMBOL_DOLLAR) {
            EndofRecord = true;
        }
    }

    //  If no more input, or we hit GMWM, set internal end of transfer,
    //  but keep on accepting characters until External end of transfer.
    //  Also, since we are about to set IntEndofTransfer, we will will
    //  have to advance the address register, to match the earlier fetch.

    if(LastInputCycle || EndofRecord) {
        if(!IntEndofTransfer) {
            if(!CPU -> StorageWrapLatch) {
                addr -> Set(CPU -> STARMod(1));     //  Bump address register
            }
            else {
                addr -> Set(STORAGE);             //  Wrap: Set reg. to EOM+1
            }
        }
        IntEndofTransfer = true;
        if(ExtEndofTransfer) {
            return;
        }
    }

    //  If we get here, we are either ending, or we are still storing data.
    //  (If we are still storing, the address register gets bumped here).

    if(!LastInputCycle && !IntEndofTransfer) {      //  Still storing input?

        //  If parity is no good, set data check...

        if(!CPU -> AChannel -> Select().CheckParity()) {
            SetStatus(GetStatus() | IOCHDATACHECK);

            //  If asterisk insert, store an asterisk!

            if(FI1415CE -> AsteriskInsert -> Checked) {
                ChR2 -> Set(BCD_ASTERISK);
                CPU -> AChannel -> Select(
                    this == CPU -> Channel[CHANNEL1] ?
                        TAChannel::A_Channel_E : TAChannel::A_Channel_F);
            }
        }   //  End initial parity check

        //  Store the data (perhaps with a parity error!

        CPU -> Store(CPU -> AssemblyChannel -> Select(
            (MoveMode ?
                TAssemblyChannel::AsmChannelWMB :
                TAssemblyChannel::AsmChannelWMA),
            TAssemblyChannel::AsmChannelZonesA,
            false,
            TAssemblyChannel::AsmChannelSignNone,
            TAssemblyChannel::AsmChannelNumA) );

        //  If we have bad data, but not asterisk insert, STOP
        //  One good way: Run FORTRAN w/o Asterisk Insert!  ;-)

        if(!CPU -> AChannel -> Select().CheckParity()) {
            CPU -> AChannelCheck -> SetStop("Data Check, No Asterisk Insert!");
            return;
        }

        ResetR2();                                  //  Reset Channel Data
        if(CPU -> StorageWrapLatch) {               //  If storage wrap -- done
            IntEndofTransfer = true;
            EndofRecord = true;
            addr -> Set(STORAGE);
        }
        else {
            addr -> Set(CPU -> STARMod(1));         //  Bump address register
        }

    }   //  End, !LastInputCycle

    //  In the real world, the device keeps reading data, and will
    //  then set CycleRequired.  (In fact, the console matrix will
    //  call ChannelStrobe to actually do this when a key is pressed).
    //  But most of our input devices in the emulator are co-routines,
    //  so we have to call them back to give them a chance to strobe
    //  the channel again.  Also, in the real world, a device would
    //  keep reading input data after the channel set Internal End of
    //  Transfer -- we have to give the input co-routine a way to
    //  finish reading it's record, even if we are no longer storing
    //  it because we hit a GMWM or wrapped storage.

    CurrentDevice -> DoInput();

}

bool T1410Channel::ChannelStrobe(BCD ch) {

    //  If R1 has data already, move it to R2.
    //  (In the emulator, that should probably never happen!)

    if(GetR1Status()) {
        MoveR1R2();
    }

    //  Load R1, and copy to R2 if there is room in R2.

    SetR1(ch);
    if(!GetR2Status()) {
        MoveR1R2();
    }

    //  If the channel has not already terminated the transfer,
    //  ask to send the data to memory.

    if(!IntEndofTransfer) {
        CycleRequired = true;
    }

    InputRequest = true;                           //   Force co-routine call
    return true;
}

//  Channel Unit Control just passes it off to the device.

void T1410Channel::DoUnitControl(BCD opmod) {
    GetCurrentDevice() -> DoUnitControl(opmod);
    if(ExtEndofTransfer) {
        IntEndofTransfer = true;
    }
    return;
}

//  Channel Overlap Processing

void T1410Channel::DoOverlap() {

    BCD SaveBReg,SaveAReg;
    long SaveSTAR;
    bool SaveStorageWrapLatch;
    char SaveCycle;
    enum TAChannel::AChannelSelect AChannelSave;

    //  Save the state of things.  Probably this is overkill, but it
    //  makes things a lot easier, because my storage cycles are not
    //  emulated perfectly.

    //  In order for things to work, this module MUST have ONLY ONE
    //  exit point, so that things can be restored properly!

    SaveCycle = CPU -> CycleRing -> State();
    SaveAReg = CPU -> A_Reg -> Get();
    SaveBReg = CPU -> B_Reg -> Get();
    SaveSTAR = CPU -> STAR -> Gate();
    SaveStorageWrapLatch = CPU -> StorageWrapLatch;
    AChannelSave = CPU -> AChannel -> Selected();

    if((ChOp -> Get().ToInt() & OP_MOD_SYMBOL_W) == OP_MOD_SYMBOL_W) {
        if(ExtEndofTransfer) {
            PriorityRequest |= PROVERLAP;
            ChOverlap -> Reset();
            SetStatus(GetCurrentDevice() -> StatusSample());
        }
        else if(OutputRequest) {
            CycleRequired = OutputRequest = false;
            DoOutput(ChAddr);
        }
    }

    else if((ChOp -> Get().ToInt() & OP_MOD_SYMBOL_R) == OP_MOD_SYMBOL_R) {
        if(ExtEndofTransfer) {

            //  We need the same kludge here we have for not overlapped mode.
            //  (No exception for console here).  If there is no
            //  remaining input character, we need to do one more channel
            //  input call, so it can detect the end of transfer.

            if(!(GetR1Status() || GetR2Status())) {
                DoInput(ChAddr);
            }
            SetStatus(GetCurrentDevice() -> StatusSample());
            if(CycleRequired || GetR2Status() || !EndofRecord) {
                SetStatus(GetStatus() | IOCHWLRECORD);
            }
            PriorityRequest |= PROVERLAP;
            ChOverlap -> Reset();
        }
        else if(CycleRequired || InputRequest) {
            CycleRequired = InputRequest = false;
            DoInput(ChAddr);
        }
    }

    else {                                  //  Must be Unit Control
        if(ExtEndofTransfer) {
            SetStatus(GetCurrentDevice() -> StatusSample());
            IntEndofTransfer = true;
            PriorityRequest |= PROVERLAP;
            ChOverlap -> Reset();
        }
        else if(UnitControlOverlapBusy != NULL &&
                !UnitControlOverlapBusy -> TestBusy() ) {
            ExtEndofTransfer = true;
            UnitControlOverlapBusy = NULL;
        }
    }

    //  Need to mark the assembly channel invalid...


    CPU -> AChannel -> Select(AChannelSave);
    CPU -> StorageWrapLatch = SaveStorageWrapLatch;
    CPU -> STAR -> Set(SaveSTAR);
    CPU -> B_Reg -> Set(SaveBReg);
    CPU -> A_Reg -> Set(SaveAReg);
    CPU -> CycleRing -> Set(SaveCycle);
}



//  Class T1410IODevice implementation.  This is an *abstract* base class,
//  intended to be used to derive actual I/O devices

T1410IODevice::T1410IODevice(int devicenumber, T1410Channel *Ch) {
    Channel = Ch;
    Ch -> AddIODevice(this,devicenumber);
}

//  Method to handle Unit Control for those devices which don't have unit
//  control (most of them)

void T1410IODevice::DoUnitControl(BCD opmod) {
    Channel -> SetStatus(Channel -> GetStatus() | IOCHNOTREADY);
    DEBUG("Unit control not implemented for device",0);
    return;
}



//  And, finally, the 1410 IO Instruction Routines.  We implement them
//  here to keep the I/O stuff together!

//  Move and Load mode (M and L) Instructions.

//  Note: The channel selected by the CPU is known to be available, as the
//  interlock test was passed during instruction readout at I3.
//  IOChannelSelect indicates the selected channel.
//  Channel -> ChUnitType has Device Type (e.g. 'T' for console)
//  Channel -> ChUnitNumber has Unit Number

void T1410CPU::InstructionIO() {

    BCD opmod;
    T1410Channel *Ch = Channel[IOChannelSelect];

    opmod = (Op_Mod_Reg -> Get().To6Bit());
    assert(!(Ch -> ChInterlock -> State()));

    //  Reset the channel, then set Channel Interlock

    Ch -> Reset();
    Ch -> ChInterlock -> Set();

    //  Set Move mode or Load mode, appropriately

    if((Op_Reg -> Get().To6Bit()) == OP_IO_MOVE) {
        Ch -> MoveMode = true;
    }
    else {
        Ch -> LoadMode = true;
    }

    //  Start things out, depending on op modifier.

    switch(opmod.ToInt()) {

    case OP_MOD_SYMBOL_Q:
        Ch -> ChNOP = true;
        //  Fall thru...

    case OP_MOD_SYMBOL_R:
    case OP_MOD_SYMBOL_DOLLAR:
        Ch -> ChRead -> Set();
        break;

    case OP_MOD_SYMBOL_V:
        Ch -> ChNOP = true;
        //  Fall thru...

    case OP_MOD_SYMBOL_W:
    case OP_MOD_SYMBOL_X:
        Ch -> ChWrite -> Set();
        break;

    default:
        InstructionCheck ->
            SetStop("Instruction Check: Invalid I/O d-character");
        return;
    }   //  End switch on op modifier

    //  See if there is a device for this device number.  If not,
    //  return not ready.

    if(Ch -> GetCurrentDevice() == NULL) {
        Ch -> SetStatus(IOCHNOTREADY);
        IRingControl = true;
        return;
    }

    //  Start up the I/O, do initial status check (Status Sample A)
    //  Just return if the status is not 0.

    Ch -> SetStatus(Ch -> GetCurrentDevice() -> Select());
    if(Ch -> GetStatus() != 0 ||
       (opmod.ToInt() == OP_MOD_SYMBOL_Q || opmod.ToInt() == OP_MOD_SYMBOL_V)) {
        IRingControl = true;
        return;
    }

    //  If OK so far, set Overlap/NotOverlap

    //  If overlapped, copy the BAR to the associated channel register,
    //  and start the I/O operation before returning.

    if(CPU -> IOOverlapSelect) {
        Ch -> ChOverlap -> Set();
        *(Ch -> ChAddr) = *B_AR;
        Ch -> ChOp -> Set(opmod);
        if((opmod.ToInt() & OP_MOD_SYMBOL_W) == OP_MOD_SYMBOL_W) {
            Ch -> DoOutput(Ch -> ChAddr);
        }
        else if((opmod.ToInt() & OP_MOD_SYMBOL_R) == OP_MOD_SYMBOL_R) {
            Ch -> GetCurrentDevice() -> DoInput();
        }
        IRingControl = true;
        return;
    }

    //  If we get here, we are not overlapped.

    Ch -> ChNotOverlap -> Set();

    if((opmod.ToInt() & OP_MOD_SYMBOL_W) == OP_MOD_SYMBOL_W) {
        CPU -> Display();
        while(!Ch -> ExtEndofTransfer) {
            Ch -> DoOutput(B_AR);
        }
        Ch -> ChNotOverlap -> Reset();
        Ch -> SetStatus(Ch -> GetCurrentDevice() -> StatusSample());
        IRingControl = true;
        return;
    }
    else if((opmod.ToInt() & OP_MOD_SYMBOL_R) == OP_MOD_SYMBOL_R) {

        //  Input processing continues so long as not External End from device

        CPU -> Display();
        Ch -> GetCurrentDevice() -> DoInput();
        while(!Ch -> ExtEndofTransfer) {

            //  If no input, just wait here (not overlapped -- stuck here)

            while(!(Ch -> CycleRequired || Ch -> InputRequest ||
                    Ch -> ExtEndofTransfer)) {
                Application -> ProcessMessages();
                //  sleep(10);
                continue;
            }

            Ch -> DoInput(B_AR);

        }   //  End, not External End of Transfer

        assert(Ch -> ExtEndofTransfer);

        //  Here comes a bit of a kludge.  Except for the console, whose
        //  inquiry request key in the emulator actually works in real time,
        //  the device will set External End of Transfer before the channel
        //  has a chance to actually read the next character.  This causes
        //  a WLR indication.  So, here, if the device set External End of
        //  Transfer, and there is no data from it, we call the Channel for
        //  input one more time.  Because the device has no data, the
        //  Channel DoInput method will set LastInputCycle, therefore
        //  setting IntEndofTransfer, and return here without calling the
        //  device again.

        if(Ch -> ChUnitType -> Get().To6Bit() != CONSOLE_IO_DEVICE &&
           !(Ch -> GetR1Status() || Ch -> GetR2Status()) ) {
            Ch -> DoInput(B_AR);
        }

        //  If, at the end, things do not match up ==> Wrong Length Record
        //  (Note that we do *not* test InputRequest here!)

        Ch -> SetStatus(Ch -> GetCurrentDevice() -> StatusSample());
        if(Ch -> CycleRequired || Ch -> GetR2Status() || !Ch -> EndofRecord) {
            Ch -> SetStatus(Ch -> GetStatus() | IOCHWLRECORD);
        }

        //  And all done - continue with Instructions

        Ch -> ChNotOverlap -> Reset();
        IRingControl = true;
        return;
    }
}


//  Unit control (U) Instruction.  Basically, it just passes it on to
//  the appropriate unit.  The rest of the code is similar to the normal
//  I/O instructions.

void T1410CPU::InstructionUnitControl() {

    BCD opmod;
    T1410Channel *Ch = Channel[IOChannelSelect];

    //  We assume that the channel is valid, and not interlocked.

    assert(Ch != NULL);
    assert(!Ch -> ChInterlock -> State());

    //  The unit control function is defined by the d-character.

    opmod = Op_Mod_Reg -> Get().To6Bit();

    //  Start of an I/O operation.  Reset the Channel, and set Interlocked.
    //  Check that the device requested exists, and select it.  If anything
    //  goes wrong, return.

    Ch -> Reset();
    Ch -> ChInterlock -> Set();
    if(Ch -> GetCurrentDevice() == NULL) {
        Ch -> SetStatus(IOCHNOTREADY);
        IRingControl = true;
        return;
    }
    Ch -> SetStatus(Ch -> GetCurrentDevice() -> Select());
    if(Ch -> GetStatus() != 0) {
        IRingControl = true;
        return;
    }

    //  Set Overlap/Not Overlap in progress, appropriately.  If Overlapped,
    //  fire things up, then return to the insturction stream.
    //  (Only a Tape Mark (M) unit control causes overlap).

    if((opmod.To6Bit() & 0x3f) ==  OP_MOD_SYMBOL_M &&
       CPU -> IOOverlapSelect) {
        Ch -> ChOverlap -> Set();
        Ch -> ChOp -> Set(opmod);
        Ch -> DoUnitControl(opmod);
        IRingControl = true;
        return;
    }
    else {
        Ch -> ChNotOverlap -> Set();
    }

    //  If we get here, we are not overlapped...

    Ch -> DoUnitControl(opmod);

    //  All we can do now is wait...

    while(!Ch -> ExtEndofTransfer) {
        Application -> ProcessMessages();
        //  sleep(10);
    }

    //  Finish up the operation...

    Ch -> ChNotOverlap -> Reset();
    Ch -> SetStatus(Ch -> GetCurrentDevice() -> StatusSample());
    IRingControl = true;
    return;
}

//  Carriage Control (F/2) Instruction.  Basically, it just passes it on to
//  the appropriate printer.  The rest of the code is similar to the normal
//  I/O instructions, except that this is always not overlapped.

void T1410CPU::InstructionCarriageControl() {

    BCD opmod;
    T1410Channel *Ch;

    IOChannelSelect = Op_Reg -> Get().To6Bit() == OP_IO_CARRIAGE_1 ?
        CHANNEL1 : CHANNEL2;

    if(IOChannelSelect > MAXCHANNEL) {
       IOInterlockCheck ->
           SetStop("I/O Interlock Check: F/2 Op Channel not implemented");
           return;
    }

    Ch = Channel[IOChannelSelect];
    assert(Ch != NULL);

    if(Ch -> ChInterlock -> State()) {
       IOInterlockCheck ->
           SetStop("I/O Interlock Check: F/2 Op Channel Interlock Check");
           return;
    }

    //  The carriage control function is defined by the d-character.

    opmod = Op_Mod_Reg -> Get().To6Bit();

    //  Start of an I/O operation.  Reset the Channel, and set Interlocked.
    //  Check that the device requested exists, and select it.  If anything
    //  goes wrong, return.

    Ch -> Reset();
    Ch -> ChInterlock -> Set();
    Channel[IOChannelSelect] -> ChUnitType -> Set(2);       //  Printer is %20
    Channel[IOChannelSelect] -> ChUnitNumber -> Set(BCD_0); //  Printer is %20
    Channel[IOChannelSelect] -> SetCurrentDevice();
    if(Ch -> GetCurrentDevice() == NULL) {
        Ch -> SetStatus(IOCHNOTREADY);
        IRingControl = true;
        return;
    }
    Ch -> SetStatus(Ch -> GetCurrentDevice() -> Select());
    if(Ch -> GetStatus() != 0) {
        IRingControl = true;
        return;
    }

    Ch -> ChNotOverlap -> Set();
    Ch -> DoUnitControl(opmod);

    //  All we can do now is wait...

    while(!Ch -> ExtEndofTransfer) {
        Application -> ProcessMessages();
        //  sleep(10);
    }

    //  Finish up the operation...

    Ch -> ChNotOverlap -> Reset();
    Ch -> SetStatus(Ch -> GetCurrentDevice() -> StatusSample());
    IRingControl = true;
    return;
}

//  Select Stacker (K/4) Instruction.  Basically, it just passes it on to
//  the appropriate reader.  The rest of the code is similar to the normal
//  I/O instructions, except that this is always not overlapped.

void T1410CPU::InstructionSelectStacker() {

    BCD opmod;
    T1410Channel *Ch;

    IOChannelSelect = Op_Reg -> Get().To6Bit() == OP_IO_SSF_1 ?
        CHANNEL1 : CHANNEL2;

    if(IOChannelSelect > MAXCHANNEL) {
       IOInterlockCheck ->
           SetStop("I/O Interlock Check: K/4 Op Channel not implemented");
           return;
    }

    Ch = Channel[IOChannelSelect];
    assert(Ch != NULL);

    if(Ch -> ChInterlock -> State()) {
       IOInterlockCheck ->
           SetStop("I/O Interlock Check: K/4 Op Channel Interlock Check");
           return;
    }

    //  The stacker select function is defined by the d-character.

    opmod = Op_Mod_Reg -> Get().To6Bit();

    //  Start of an I/O operation.  Reset the Channel, and set Interlocked.
    //  Check that the device requested exists, and select it.  If anything
    //  goes wrong, return.

    Ch -> Reset();
    Ch -> ChInterlock -> Set();
    Channel[IOChannelSelect] -> ChUnitType -> Set(1);       //  Reader is %10
    Channel[IOChannelSelect] -> ChUnitNumber -> Set(BCD_0); //  Reader is %10
    Channel[IOChannelSelect] -> SetCurrentDevice();
    if(Ch -> GetCurrentDevice() == NULL) {
        Ch -> SetStatus(IOCHNOTREADY);
        IRingControl = true;
        return;
    }
    Ch -> SetStatus(Ch -> GetCurrentDevice() -> Select());
    if(Ch -> GetStatus() != 0) {
        IRingControl = true;
        return;
    }

    Ch -> ChNotOverlap -> Set();
    Ch -> DoUnitControl(opmod);

    //  All we can do now is wait...

    while(!Ch -> ExtEndofTransfer) {
        Application -> ProcessMessages();
        //  sleep(10);
    }

    //  Finish up the operation...

    Ch -> ChNotOverlap -> Reset();
    Ch -> SetStatus(Ch -> GetCurrentDevice() -> StatusSample());
    IRingControl = true;
    return;
}

