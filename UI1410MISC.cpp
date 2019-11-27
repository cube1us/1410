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

#include "UI1410MISC.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)

#include <assert.h>
#include <dir.h>
#include <stdio.h>
#include <sys/timeb.h>

#include "ubcd.h"
#include "UI1410CPUT.h"
#include "UIHOPPER.h"
#include "UI1410CHANNEL.h"
#include "UI1410DEBUG.h"
#include "UI1410INST.h"

void T1410CPU::InstructionStoreAddressRegister() {

    TAddressRegister *storing;

    static struct {
        char cycle;
        char scan;
        char subscan;
    } next;

    struct timeb tod;
    int hours, hundredths;

    static int hundredths_table[] = {
        00,02,03,05,07, 8,10,12,13,15,17,18,20,22,23,25,27,28,
        30,32,33,35,37,38,40,42,43,45,47,48,50,52,53,55,57,58,
        60,62,23,65,67,68,70,72,73,75,77,78,80,82,83,85,87,88,
        90,92,93,95,97,98 };
    if(LastInstructionReadout) {
        ScanRing -> Set(SCAN_1);
        next.scan = SCAN_1;
        SubScanRing -> Set(SUB_SCAN_U);
        next.subscan = SUB_SCAN_U;
        CycleRing -> Set(CYCLE_C);
        next.cycle = CYCLE_C;
        ARing -> Reset();                          //  A Ring to A1
        LastInstructionReadout = false;
    }

    *STAR = *C_AR;                                  //  Read out using CAR
    Readout();
    ARing -> Next();                                //  Advance A Ring

    storing = NULL;

    switch(Op_Mod_Reg -> Get().ToInt() & 0x3f) {

    case OP_MOD_SYMBOL_A:
        storing = A_AR;
        break;

    case OP_MOD_SYMBOL_B:
        storing = B_AR;
        break;

    case OP_MOD_SYMBOL_E:
        storing = E_AR;
        break;

    case OP_MOD_SYMBOL_F:
        storing = F_AR;
        break;

    case OP_MOD_SYMBOL_T:

        //  At A Ring 2 sample the clock.

        if(ARing -> State() == A_RING_2) {
            ftime(&tod);
            tod.time -= (60*tod.timezone) - 3600*tod.dstflag;   //  Local time
            tod.time %= 3600*24;                                //  No days
            hours = tod.time / 3600;
            hundredths = hundredths_table[(tod.time % 3600)/60];
            if(tod.time % 60 == 0 && tod.millitm < 400) {
                TOD -> Set(99999);
            }
            else {
                TOD -> Set(hours*100 + hundredths);
            }
        }

        //  TOD will remember the stored value for us.

        storing = TOD;
        break;


    default:
        storing = NULL;
        InstructionCheck -> SetStop("Instruction Check: Invalid SAR d-char");
        AddressExitCheck -> SetStop("Address Exit Check: Invalid SAR d-char");
        return;
    }

    assert(storing != NULL);

    A_Reg -> Set(storing -> GateBCD(6 - (ARing -> State())));
    AChannel -> Select(TAChannel::A_Channel_A);
    Store( AssemblyChannel -> Select(
        TAssemblyChannel::AsmChannelWMB,
        TAssemblyChannel::AsmChannelZonesB,
        false,
        TAssemblyChannel::AsmChannelSignNone,
        TAssemblyChannel::AsmChannelNumA) );

    //  Do NOT use CYCLE here, as it will mess up the A Address Register
    //  Instead, modify C by -1 (SCAN_1) right here...

    C_AR -> Set(STARScan());

    assert(ARing -> State() >= A_RING_2 && ARing -> State() <= A_RING_6);
    if(ARing -> State() == A_RING_6) {
        IRingControl = true;
        ARing -> Reset();
    }

    return;
}   //  End Store Address Register

//  The Set and Clear Word Mark instructions are handled by one method

void T1410CPU::InstructionDoWordMark() {

    static struct {
        char cycle;
        char scan;
        char subscan;
    } next;

    if(LastInstructionReadout) {
        next.scan = SCAN_1;
        next.subscan = SUB_SCAN_U;
        next.cycle = CYCLE_A;
        LastInstructionReadout = false;
    }

    ScanRing -> Set(next.scan);
    SubScanRing -> Set(next.subscan);
    CycleRing -> Set(next.cycle);

    if(next.cycle == CYCLE_A) {
        *STAR = *A_AR;
    }
    else {
        assert(next.cycle == CYCLE_B);
        *STAR = *B_AR;
    }

    Readout();
    Store( AssemblyChannel -> Select(
        ((Op_Reg -> Get().ToInt() & 0x3f) == OP_SETWM) ?
            TAssemblyChannel::AsmChannelWMSet :
            TAssemblyChannel::AsmChannelWMNone,
        TAssemblyChannel::AsmChannelZonesB,
        false,
        TAssemblyChannel::AsmChannelSignNone,
        TAssemblyChannel::AsmChannelNumB) );
    Cycle();
    if(CycleRing -> State() == CYCLE_A) {
        next.cycle = CYCLE_B;
    }
    else {
        IRingControl = true;
    }
    return;
}

//  Clear Storage & Clear Storage and Branch are the same instruction, really

void T1410CPU::InstructionClearStorage() {

    static struct {
        char cycle;
        char scan;
        char subscan;
    } next;

    if(LastInstructionReadout) {
        next.scan = SCAN_1;
        next.subscan = SUB_SCAN_U;
        next.cycle = CYCLE_B;
        LastInstructionReadout = false;
    }

    ScanRing -> Set(next.scan);
    SubScanRing -> Set(next.subscan);
    CycleRing -> Set(next.cycle);

    *STAR = *B_AR;
    Readout();

    Store( AssemblyChannel -> Select(
        TAssemblyChannel::AsmChannelWMNone,
        TAssemblyChannel::AsmChannelZonesNone,
        false,
        TAssemblyChannel::AsmChannelSignNone,
        TAssemblyChannel::AsmChannelNumNone) );
    Cycle();

    //  In the real 1410, the end is detected because there is a borrow from
    //  the hundreds digit.  We accomplish the same thing by noting when the
    //  *updated* address ends in 99.

    if(B_AR -> Gate() % 100 == 99) {
        IRingControl = true;
        if(BranchLatch) {
            ScanRing -> Set(SCAN_N);
            CycleRing -> Set(CYCLE_B);
            *B_AR = *I_AR;
            *STAR = *I_AR;
        }
    }
    return;
}


//  Halt & Halt and Branch are the same instruction, really

void T1410CPU::InstructionHalt() {

    LastInstructionReadout = false;

    if(BranchLatch) {
        ScanRing -> Set(SCAN_N);
        CycleRing -> Set(CYCLE_B);
        *B_AR = *I_AR;
        *STAR = *I_AR;
    }

    StopLatch = true;
    IRingControl = true;
    return;
}

