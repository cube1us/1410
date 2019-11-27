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

//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "UI1410BRANCH.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)

#include <assert.h>
#include <dir.h>
#include <stdio.h>

#include "ubcd.h"
#include "UI1410CPUT.h"
#include "UIHOPPER.h"
#include "UI1410CHANNEL.h"
#include "UIPRINTER.h"
#include "UI1410DEBUG.h"
#include "UI1410INST.h"

void T1410CPU::InstructionBranchCond()
{
    int op_mod;
    int ch;

    op_mod = Op_Mod_Reg -> Get().ToInt() & 0x3f;
    assert(!BranchLatch);

    switch(op_mod) {

    case OP_MOD_SYMBOL_BLANK:
        BranchLatch = true;
        break;

    case OP_MOD_SYMBOL_SLASH:
        BranchLatch = (CompareBLTA -> State() || CompareBGTA -> State());
        break;

    case OP_MOD_SYMBOL_K:
        for(ch = 0; ch < MAXCHANNEL; ++ch) {
            if(Channel[ch] -> IsTapeIndicate()) {
                BranchLatch = true;
            }
            Channel[ch] -> ResetTapeIndicate();
        }
        break;

    case OP_MOD_SYMBOL_Q:
        BranchLatch = InqReqLatch;
        break;

    case OP_MOD_SYMBOL_S:
        BranchLatch = CompareBEQA -> State();
        break;

    case OP_MOD_SYMBOL_T:
        BranchLatch = CompareBLTA -> State();
        break;

    case OP_MOD_SYMBOL_U:
        BranchLatch = CompareBGTA -> State();
        break;

    case OP_MOD_SYMBOL_V:
        BranchLatch = ZeroBalance -> State();
        break;

    case OP_MOD_SYMBOL_W:
        if(DivideOverflow -> State()) {
            BranchLatch = true;
            DivideOverflow -> Reset();
        }
        break;

    case OP_MOD_SYMBOL_Z:
        if(Overflow -> State()) {
            BranchLatch = true;
            Overflow -> Reset();
        }
        break;

    case OP_MOD_SYMBOL_1:
        BranchLatch = Channel[CHANNEL1] -> ChOverlap -> State();
        break;

    case OP_MOD_SYMBOL_2:
        if(MAXCHANNEL > 1) {
            BranchLatch = Channel[CHANNEL2] -> ChOverlap -> State();
        }
        break;

    case OP_MOD_SYMBOL_9:
        BranchLatch =
           Channel[CHANNEL1] -> GetIODevice(PRINTER_IO_DEVICE) != NULL &&
		   ((T1403Printer*)(Channel[CHANNEL1] -> GetIODevice(PRINTER_IO_DEVICE))) ->
				CarriageChannelTest(9);
		break;

	case OP_MOD_SYMBOL_AT:
		BranchLatch =
		   Channel[CHANNEL1] -> GetIODevice(PRINTER_IO_DEVICE) != NULL &&
		   ((T1403Printer *)(Channel[CHANNEL1] -> GetIODevice(PRINTER_IO_DEVICE))) ->
				CarriageChannelTest(12);
		break;

	default:
		; //  Bad 'd' character - do nothing.
	}

    if(BranchLatch) {                               //  Branching?
        ScanRing -> Set(SCAN_N);                    //  Set to No Scan
        //  BranchLatch is the "Branch to AAR Latch"
        CycleRing -> Set(CYCLE_B);                  //  Take a B Cycle
        *B_AR = *I_AR;                              //  B set to N.S.I. location
        *STAR = *I_AR;
    }

    LastInstructionReadout = false;
    IRingControl = true;
    return;
}

void T1410CPU::InstructionBranchChannel1() {
    InstructionBranchChannel(CHANNEL1);
};

void T1410CPU::InstructionBranchChannel2() {
    if(MAXCHANNEL < 2) {
        LastInstructionReadout = false;
        IRingControl = true;
        return;
    }
    InstructionBranchChannel(CHANNEL2);
}

void T1410CPU::InstructionBranchChannel(int ch) {

    int op_mod;

    op_mod = Op_Mod_Reg -> Get().ToInt() & 0x3f;
    assert(!BranchLatch);
    assert(ch < MAXCHANNEL);                        //  ch is origin 0!

    //  If channel was in overlap state, wait here until overlap is done.

    while(Channel[ch] -> ChOverlap -> State()) {
        Channel[ch] -> DoOverlap();
        TBusyDevice::BusyPass();                    //  Might be waiting (WTM)
        Application -> ProcessMessages();
    }

    //  See if we have a match -- if so, we will branch.

    BranchLatch = (op_mod & Channel[ch] -> GetStatus());

    //  If we branch, or if all bits tested, turn off Interlock,
    //  reset channel read/write status, and turn off the priority
    //  interrupt request.

    if(BranchLatch || (op_mod & 0x3f) == 0x3f) {
        Channel[ch] -> ChInterlock -> Reset();
        Channel[ch] -> ChRead -> Reset();
        Channel[ch] -> ChWrite -> Reset();
        Channel[ch] -> PriorityRequest &= (~PROVERLAP);
    }

    //  Handle branch...

    if(BranchLatch) {                               //  Branching?
        ScanRing -> Set(SCAN_N);                    //  Set to No Scan
        //  BranchLatch is the "Branch to AAR Latch"
        CycleRing -> Set(CYCLE_B);                  //  Take a B Cycle
        *B_AR = *I_AR;                              //  B set to N.S.I. location
        *STAR = *I_AR;
    }

    LastInstructionReadout = false;
    IRingControl = true;
    return;
}


void T1410CPU::InstructionBranchCharEqual() {

    assert(!BranchLatch);

    ScanRing -> Set(SCAN_1);                        //  Set 1st scan
    SubScanRing -> Set(SUB_SCAN_U);                 //  Units
    CycleRing -> Set(CYCLE_B);                      //  B Cycle
    *STAR = *B_AR;
    Readout();                                      //  Get B Char
    Cycle();

    AChannel -> Select(TAChannel::A_Channel_Mod);   //  Op mod to A Ch.
    Comparator();
    AChannel -> Select(TAChannel::A_Channel_A);     //  For safety sake...

    BranchLatch = CompareBEQA -> State();           //  Test results...

    //  Handle branch...

    if(BranchLatch) {                               //  Branching?
        ScanRing -> Set(SCAN_N);                    //  Set to No Scan
        //  BranchLatch is the "Branch to AAR Latch"
        CycleRing -> Set(CYCLE_B);                  //  Take a B Cycle
        *B_AR = *I_AR;                              //  B set to N.S.I. location
        *STAR = *I_AR;
    }

    LastInstructionReadout = false;
    IRingControl = true;
    return;
}


void T1410CPU::InstructionBranchBitEqual() {

    int op_mod;

    op_mod = Op_Mod_Reg -> Get().ToInt() & 0x3f;
    assert(!BranchLatch);

    ScanRing -> Set(SCAN_1);                        //  Set 1st scan
    SubScanRing -> Set(SUB_SCAN_U);                 //  Units
    CycleRing -> Set(CYCLE_B);                      //  B Cycle
    *STAR = *B_AR;
    Readout();                                      //  Get B Char
    Cycle();

    AChannel -> Select(TAChannel::A_Channel_Mod);   //  Op mod to A Ch.
    BranchLatch = ((op_mod & (B_Reg -> Get().ToInt())) != 0);//  Any bits match?
    AChannel -> Select(TAChannel::A_Channel_A);     //  For safety sake...

    //  Handle branch...

    if(BranchLatch) {                               //  Branching?
        ScanRing -> Set(SCAN_N);                    //  Set to No Scan
        //  BranchLatch is the "Branch to AAR Latch"
        CycleRing -> Set(CYCLE_B);                  //  Take a B Cycle
        *B_AR = *I_AR;                              //  B set to N.S.I. location
        *STAR = *I_AR;
    }

    LastInstructionReadout = false;
    IRingControl = true;
    return;
}


void T1410CPU::InstructionBranchZoneWMEqual() {

    int op_mod;

    op_mod = Op_Mod_Reg -> Get().ToInt() & 0x3f;
    assert(!BranchLatch);

    ScanRing -> Set(SCAN_1);                        //  Set 1st scan
    SubScanRing -> Set(SUB_SCAN_U);                 //  Units
    CycleRing -> Set(CYCLE_B);                      //  B Cycle
    *STAR = *B_AR;
    Readout();                                      //  Get B Char
    Cycle();

    AChannel -> Select(TAChannel::A_Channel_Mod);   //  Op mod to A Ch.

    if(((op_mod & 1) && B_Reg -> Get().TestWM()) ||
       ((op_mod & 2) && (B_Reg -> Get() & BIT_ZONE) == (op_mod & BIT_ZONE))) {
       BranchLatch = true;
    }

    AChannel -> Select(TAChannel::A_Channel_A);     //  For safety sake...

    //  Handle branch...

    if(BranchLatch) {                               //  Branching?
        ScanRing -> Set(SCAN_N);                    //  Set to No Scan
        //  BranchLatch is the "Branch to AAR Latch"
        CycleRing -> Set(CYCLE_B);                  //  Take a B Cycle
        *B_AR = *I_AR;                              //  B set to N.S.I. location
        *STAR = *I_AR;
    }

    LastInstructionReadout = false;
    IRingControl = true;
    return;
}

void T1410CPU::InstructionPriorityBranch() {

    int op_mod;

    op_mod = Op_Mod_Reg -> Get().ToInt() & 0x3f;
    assert(!BranchLatch);

    switch(op_mod) {

    case OP_MOD_SYMBOL_E:
        BranchLatch = true;
        PriorityAlert -> Set();
        break;

    case OP_MOD_SYMBOL_X:
        BranchLatch = true;
        PriorityAlert -> Reset();
        break;

    case OP_MOD_SYMBOL_1:
        BranchLatch = (Channel[CHANNEL1] -> PriorityRequest & PROVERLAP) != 0;
        //  Does not reset (R bbbbb d will reset)
        break;

    case OP_MOD_SYMBOL_2:
        BranchLatch = (MAXCHANNEL > 1) &&
            (Channel[CHANNEL2] -> PriorityRequest & PROVERLAP) != 0;
        //  Does not reset (X bbbbb d will reset)
        break;

    case OP_MOD_SYMBOL_U:
        BranchLatch = (Channel[CHANNEL1] -> PriorityRequest & PRIOUNIT) != 0;
        Channel[CHANNEL1] -> PriorityRequest &= (~PRIOUNIT);
        break;

    case OP_MOD_SYMBOL_F:
        BranchLatch = (MAXCHANNEL > 1) &&
            (Channel[CHANNEL2] -> PriorityRequest & PRIOUNIT) != 0;
        Channel[CHANNEL2] -> PriorityRequest &= (~PRIOUNIT);
        break;

    case OP_MOD_SYMBOL_Q:
        BranchLatch = (Channel[CHANNEL1] -> PriorityRequest & PRINQUIRY) != 0;
        break;

    case OP_MOD_SYMBOL_ASTERISK:
        BranchLatch = (MAXCHANNEL > 1) &&
            (Channel[CHANNEL2] -> PriorityRequest & PRINQUIRY) != 0;
        break;

    case OP_MOD_SYMBOL_N:
        BranchLatch = (Channel[CHANNEL1] -> PriorityRequest & PROUTQUIRY) != 0;
        Channel[CHANNEL1] -> PriorityRequest &= (~PROUTQUIRY);
        break;

    case OP_MOD_SYMBOL_RM:
        BranchLatch = (MAXCHANNEL > 1) &&
            (Channel[CHANNEL2] -> PriorityRequest & PROUTQUIRY) != 0;
        Channel[CHANNEL2] -> PriorityRequest &= (~PROUTQUIRY);
        break;

    case OP_MOD_SYMBOL_S:
        BranchLatch = (Channel[CHANNEL1] -> PriorityRequest & PRSEEK) != 0;
        break;

    case OP_MOD_SYMBOL_T:
        BranchLatch = (MAXCHANNEL > 1) &&
            (Channel[CHANNEL2] -> PriorityRequest & PRSEEK) != 0;
        break;

    case OP_MOD_SYMBOL_A:
        BranchLatch = (Channel[CHANNEL1] -> PriorityRequest & PRATTENTION) != 0;
        Channel[CHANNEL1] -> PriorityRequest &= (~PRATTENTION);
        break;

    case OP_MOD_SYMBOL_B:
        BranchLatch = (MAXCHANNEL > 1) &&
            (Channel[CHANNEL2] -> PriorityRequest & PRATTENTION) != 0;
        Channel[CHANNEL2] -> PriorityRequest &= (~PRATTENTION);
        break;

    default:
        ; //  Bad 'd' character - do nothing.
    }

    if(BranchLatch) {                               //  Branching?
        ScanRing -> Set(SCAN_N);                    //  Set to No Scan
        //  BranchLatch is the "Branch to AAR Latch"
        CycleRing -> Set(CYCLE_B);                  //  Take a B Cycle
        *B_AR = *I_AR;                              //  B set to N.S.I. location
        *STAR = *I_AR;
    }

    LastInstructionReadout = false;
    IRingControl = true;
    return;

}
