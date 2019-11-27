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

#include <assert.h>
#include <dir.h>
#include <stdio.h>

#include "ubcd.h"
#include "UI1410CPUT.h"
#include "UIHOPPER.h"
#include "UI1410CHANNEL.h"
#include "UI1410DEBUG.h"
#include "UI1410INST.h"

//---------------------------------------------------------------------------

#include "UI1415IO.h"
#include "UI1415L.h"

//	This module handles Instruction Decode and Execution in the CPU

/* 	The following table is given in the order of the 1410 BCD codes, and
	contains the opcode common lines - 3 16 bit words.
*/

struct OpCodeCommonLines OpCodeTable[64] = {
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 00 spc */
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 01   1 */
	{ 8+512, 256, 32+128 },											/* 02	2 */
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 03   3 */
	{ 8+512, 256, 32+128 },											/* 04	4 */
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 05   5 */
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 06   6 */
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 07   7 */
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 08   8 */
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 09   9 */
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 10   0 */
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 11   = */
	{ 2+8+32+128+256+1024, 4+16+256, 1+2+4+16+64+256+1024 },		/* 12	@ */
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 13   : */
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 14   > */
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 15  rad*/
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 15 alt */
	{ 2+4+32+128+256+2048+4096, 128, 1+8+16+256+2048 },				/* 17	/ */
    { 2+4+32+128+256+4096, 2+8+16+256, 1+2+4+16+64+4096 },			/* 18	S */
    { 2+8+64+128+256+1024+4096, 64+256, 1+2+4+16+128+2048 },		/* 19	T */
    { 1+8+16+256+8192, 256, 32 },									/* 20	U */
	{ 2+8+64+128+256+2048+4096, 128, 1+8+32+128+512+2048 },			/* 21	V */
	{ 2+8+64+128+256+2048+4096, 128, 1+8+32+128+512+2048 },			/* 22	W */
    { 2+4+16+256, 128, 32+128+256+2048 },							/* 23	X */
	{ 2+4+16+256+2048, 128, 32+128+256+2048 },  					/* 24   Y */
    { 2+8+32+128+256+2048+4096, 32+256, 1+2+4+16+64+1024+2048+4096 }, /* 25 Z */
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 26   RM*/
    { 2+4+32+128+256+2048+4096, 256+512, 1+2+16+2048+4096 },		/* 27	, */
	{ 2+8+32+128+256+1024, 4+16+256, 1+2+4+16+64+256+1024 },		/* 28	% */
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 29   WS*/
    { OP_INVALID, OP_INVALID, OP_INVALID },							/* 30   \ */
    { OP_INVALID, OP_INVALID, OP_INVALID },							/* 31   SM*/
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 32   - */
    { 2+4+16+256+2048, 128, 32+128+256+2048 }, 						/* 33	J */
	{ 8+512, 256, 32+128 },											/* 34	K */
    { 1+8+64+128+256+8192, 256+1024, 0 },							/* 35	L */
    { 1+8+64+128+256+8192, 256+1024, 0 },							/* 36	M */
    { 0, 0, 0 },													/* 37	N */
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 38	O */
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 39   P */
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 40   Q */
    { 2+4+16+256, 128, 32+128+256+2048 },							/* 41	R */
	{ 2+4+32+128+256+4096, 1+8+16+256, 1+2+4+16+64+256+1024+4096 },	/* 42 	! */
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 43   $ */
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 44   * */
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 45   ] */
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 46   ; */
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 47 Delt*/
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 48   + */
    { 2+4+32+128+256+4096, 2+8+16+256, 1+2+4+16+64+4096 },			/* 49 	A */
    { 2+8+64+128+256+2048+4096, 64+128, 1+8+32+128+512+2048 },		/* 52	B */
    { 2+8+32+128+256+2048+4096, 64+256, 1+2+4+16+128+2048+4096 }, 	/* 51	C */
    { 2+8+64+128+256+2048+4096, 256, 2+4+16+64+256+1024+2048+4096 }, /* 52	D */
    { 2+8+32+128+256+2048+4096, 32+256, 1+2+4+16+64+1024+2048+4096 }, /* 53 E */
	{ 8+512, 256, 32+128 },											/* 54	F */
    { 8+16+256+8192, 256, 1 },										/* 55	G */
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 56   H */
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 57   I */
	{ 2+4+32+128+256+4096, 1+8+16+256, 1+2+4+16+64+256+1024+4096 },	/* 58	? */
	{ 2+8+256+2048+4096, 128, 16+128+256+2048 },					/* 59	. */
    { 2+4+32+128+256+2048+4096, 256+512, 1+2+16+2048+4096 },		/* 60 loz.*/
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 61    [*/
	{ OP_INVALID, OP_INVALID, OP_INVALID },							/* 62    <*/
	{ OP_INVALID, OP_INVALID, OP_INVALID }							/* 63   GM*/
};

//	Table indicating when zones are valid for ops with addresses

static bool IRingZoneTable [] = {
	false, false, false, true, true, false, false, false, true, true,
    false, true, true
};


//	Table of Index Register locations

int IndexRegisterLookup[16] = {
	0,29,34,39,44,49,54,59,64,69,74,79,84,89,94,99
};

//	START BUTTON pressed - moved from UI1410PWR to here

void T1410CPU::DoStartClick()
{

    int refresh = 0;
    int busybee = 0;
    int overlapcycle = 0;
    int opcode;
    int ch;

	switch(Mode) {

    case MODE_ADDR:
    	ProcessRoutineLatch = false;
    	FI1415IO -> DoAddressEntry();
        BranchTo1Latch = false;
        BranchLatch = false;
        break;

    case MODE_DISPLAY:
    	ProcessRoutineLatch = false;
    	FI1415IO -> DoDisplay(1);
        break;

    case MODE_ALTER:
    	ProcessRoutineLatch = false;
    	FI1415IO -> DoAlter();
        break;

    case MODE_IE:
    case MODE_RUN:

    	// We loop here until something makes us stop and return,
        // such as a special cycle control setting, I/E mode when
        // finished fetching, the STOP key at the end of an instruction,
        // etc.

        StopLatch = StopKeyLatch = false;
        F1415L -> Light_Stop -> Enabled = false;

        //	These are debugging statements.  I've had a lot of problems
        //	using ptr = ptr instead of *ptr = *ptr

        assert(STAR != A_AR);
        assert(STAR != B_AR);
        assert(STAR != C_AR);
        assert(STAR != D_AR);
        assert(A_AR != B_AR);
        assert(A_AR != C_AR);
        assert(A_AR != D_AR);
        assert(B_AR != C_AR);
        assert(B_AR != D_AR);
        assert(C_AR != D_AR);

    	while(true) {

        	//	I Cycle start

	    	ProcessRoutineLatch = true;

            //  Check for storage wrap, and handle appropriately

            if(StorageWrapLatch) {
                opcode = Op_Reg -> Get().To6Bit();
                if(opcode != OP_IO_MOVE && opcode != OP_IO_LOAD &&
                   opcode != OP_CLEAR_STORAGE) {
                    AddressCheck -> SetStop("Address Check: Wrap Condition");
                }
            }

            //  See if any of the channels need attention every 12 cycles.
            //  If so, give them a chance to do their thing.

            if(++overlapcycle > 2) {               //  Was 12
                overlapcycle = 0;
                for(ch = 0; ch < MAXCHANNEL; ++ch) {
                    if(Channel[ch] -> ChOverlap -> State()) {
                        Channel[ch] -> DoOverlap();
                    }
                }
            }

    	    if(IRingControl) {
                if(StopKeyLatch || StopLatch) {
                    StopLatch = true;
                    StopKeyLatch = false;
                    F1415L -> Light_Stop -> Enabled = true;
                	FI1415IO -> StopPrintOut('S');
                    return;
                }
        		InstructionDecodeStart();
            	if(CycleControl != CYCLE_OFF ||
                   (LastInstructionReadout && Mode == MODE_IE)) {
                    F1415L -> Light_Stop -> Enabled = true;
                	FI1415IO -> StopPrintOut('C');
            		break;
	            }
    	    }

            //	I Cycle

        	else if(!LastInstructionReadout &&
            	     CycleRing -> State() == CYCLE_I) {
            	InstructionDecode();
                if(StopLatch) {
                    F1415L -> Light_Stop -> Enabled = true;
                	FI1415IO -> StopPrintOut('S');
                	return;
                }
                if(CycleControl != CYCLE_OFF ||
                	(LastInstructionReadout && Mode == MODE_IE)) {
                    F1415L -> Light_Stop -> Enabled = true;
                	FI1415IO -> StopPrintOut('C');
                    break;
                }
            }

            //	X (index) Cycle

            else if(!LastInstructionReadout &&
            		 CycleRing -> State() == CYCLE_X) {
            	InstructionIndex();
                if(StopLatch) {
                    F1415L -> Light_Stop -> Enabled = true;
                	FI1415IO -> StopPrintOut('S');
                    return;
                }
                if(CycleControl != CYCLE_OFF) {
                    F1415L -> Light_Stop -> Enabled = true;
                	FI1415IO -> StopPrintOut('C');
                    break;
                }
            }

            //	Execute Cycle would go here....

            if(LastInstructionReadout ||
            	(!IRingControl && CycleRing -> State() != CYCLE_I &&
                CycleRing -> State() != CYCLE_X) ) {

/*            	//	Temporarily use LastIntructionReadout to set IRingControl
                //	In otherwords, all instructions do nothing.

            	CPU -> IRingControl = true;
                CPU -> LastInstructionReadout = false;
*/

                //	Try and execute the instruction.  Note that we typically
                //	do this in the *same* cycle as the one where instruction
                //	readout completes.

                //	LastInstructionReadout is still set the first time thru.
                //	The execute routine must set the CPU to some cycle other
                //	than I!

                InstructionExecuteRoutine[Op_Reg -> Get().ToInt() & 0x3f]();

                LastInstructionReadout = false;

                if(StopLatch) {
                    F1415L -> Light_Stop -> Enabled = true;
                	FI1415IO -> StopPrintOut('S');
                    return;
                }

                //	If we are in storage/logic cycle, OR if we have finished
                //  an instruction (IRingControl is true) AND we are in IE
                //  cycle mode, do a 'C' print out and break out.

            	if(CycleControl != CYCLE_OFF ||
                    (IRingControl && Mode == MODE_IE)) {
                    F1415L -> Light_Stop -> Enabled = true;
                	FI1415IO -> StopPrintOut('C');
            		break;
            	}
            }

            //	If we aren't in Storage Cycle or I/E Cycle Mode...

            //	Give Windoze a chance to breathe

		    Application -> ProcessMessages();

            //  See if it is time to reduce the busy count (for debugging)
            //  The timing here is approximately Milleseconds in machine
            //  time.  (1 / (.0045ms == 4.5us)) == 222

            if(++busybee > 20) {                       //  Was 222
                busybee = 0;
                TBusyDevice::BusyPass();
            }

            //  See if a refresh of the display is in order

            if(++refresh > 10000) {
                refresh = 0;
                TBusyDevice::BusyPass();
                CPU -> Display();
            }
        }

        break;

    default:
    	break;
    }

}



//	Instruction Decode - initial phase

void T1410CPU::InstructionDecodeStart()
{

	int op_bin;

    SetScan(SCAN_N);			//	During I phase, no storage scan mode set
    CycleRing -> Set(CYCLE_I);	//	Doing I cycles
	IRing -> Reset();			//	Reset I Ring to Op state
    LastInstructionReadout = false;	//	Set true at end of fetch
    IRingControl = false;		//	Reset I Ring control state

    //	Figure out where to go.  If we are branching, then branch to AAR
    //	unless the BranchTo1Latch (e.g. Program Reset) is set.   If not
    //	branching, just use the IAR.  STAR is set to the location to begin
    //	instruction readout.

    if(BranchLatch) {
    	if(BranchTo1Latch) {
        	STAR -> Set(1);
        }
        else {
        	*STAR = *A_AR;
        }
    }
    else {
    	*STAR = *I_AR;
    }

    //	Fetch the instruction code.  Check to make sure it has it's wordmark
    //	If not, stop with an instruction check.

	//	Take an I Cycle

    Readout();
    Cycle();
    I_AR -> Set(STARMod(+1));		//	This one *always* advances I_AR

    if(!B_Reg -> Get().TestWM()) {
    	InstructionCheck -> SetStop("Instruction Check: No WordMark present");
        return;
    }

    //	Copy the opcode into the Op register, and decode it.

    *Op_Reg = *B_Reg;

	op_bin = Op_Reg -> Get().ToInt() & 0x3f;
    OpReadOutLines = OpCodeTable[op_bin].ReadOut;
    OpOperationalLines = OpCodeTable[op_bin].Operational;
    OpControlLines = OpCodeTable[op_bin].Control;
}

//	Instruction Decode

void T1410CPU::InstructionDecode()
{
	int op_bin;
    int Ch;
    BCD b;

	//	The initial state of the ring gets special handling for NOP

    if(IRing -> State() == I_RING_OP) {

        BranchTo1Latch = BranchLatch = false;

        //	Take an I Cycle

        *STAR = *I_AR;
        Readout();					//	Next inst. character now in B Register
        Cycle();

        //	In the real machine, what happens is that if a character past the
        //	op code is read out contained a WM, I_AR is not set, but is then
        //	later set from STAR.  We handle that this way here.

        if(!B_Reg -> Get().TestWM()) {
	        I_AR -> Set(STARMod(+1));
        }

        //	If this is a NOP, it gets special handling right here.
        //	If there is no wordmark, we do nothing, and come right back
        //	here with the next cycle (to check for a WM again).
        //	If there is a WordMark, we decode the op.

        if((Op_Reg -> Get() & 0x3f) == OP_NOP) {

             if(B_Reg -> Get().TestWM()) {

                I_AR -> Set(STARMod(+1));		//	Advance past char with WM
                                                //  (We didn't earlier on)

	             //	Copy the opcode into the Op register, and decode it.

			    *Op_Reg = *B_Reg;

				op_bin = Op_Reg -> Get().ToInt() & 0x3f;
			    OpReadOutLines = OpCodeTable[op_bin].ReadOut;
			    OpOperationalLines = OpCodeTable[op_bin].Operational;
		    	OpControlLines = OpCodeTable[op_bin].Control;
             }
             return;
        }

        //	If there are no control lines set, it is an invalid op code

        if(OpReadOutLines == OP_INVALID || OpOperationalLines == OP_INVALID ||
        	OpControlLines == OP_INVALID) {
            InstructionCheck -> SetStop("Instruction Check: Invalid OP code");
            return;
        }

        //	else Not a NOP.  Advance I Ring

        IRing -> Next();

        //	For % and lozenge ops (that use channel ID), set I Ring to 3

		if(OpReadOutLines & OP_PERCENTTYPE) {
            IRing -> Set(I_RING_3);
        }

        //	Proceed to "D" on the next cycle

        return;
    }

    //	At IRing 1 and IRing 6, the index latches are reset.

    if(IRing -> State() == 1 || IRing -> State() == 6) {
    	IndexLatches = 0;
    }

    //  At IRing 8, the op-modifier register is set to blank
    //  (Principles of Operation page 12, ILD Figure 26)
    if(IRing -> State() == I_RING_8) {
        Op_Mod_Reg -> Set(BCD(BITC));
    }

	//	Entry point "D"

    //	If we have a wordmark, handle Instruction length checking...

    if(B_Reg -> Get().TestWM()) {

        //	Now, since the B_Reg has a WM, we need to restore I_AR from
        //	STAR, just like in the real machine, so that IAR points to the
        //	following opcode.  (Otherwise, it would point one past the opcode)

        *I_AR = *STAR;

		switch(IRing -> State()) {

        case I_RING_1:

			//	Handle opcode with no address  or which don't need
            //	CAR or DAR to chain.

	    	if(OpReadOutLines & OP_NOCORDCY) {
 				LastInstructionReadout = true;
        	   	return;
	        }

            //	Handle chaining of arithmetic type op codes

    	    if(OpOperationalLines & OP_ARITHTYPE) {
                CycleRing -> Set(CYCLE_D);
            	*STAR = *B_AR;
	 			*D_AR = *STAR;		//	Mod by 0

	    	    //	If Multiply or Divide, also set CAR

          	    if(OpOperationalLines & OP_MPYORDIV) {
                	CycleRing -> Set(CYCLE_D);
                  	*STAR = *A_AR;
        	        *C_AR = *STAR;	//	Mod by 0
            	}

              	LastInstructionReadout = true;
	            return;
    	    }

            //	Handle chaining of Table Lookup

	        if((Op_Reg -> Get() & 0x3f) == OP_TABLESEARCH) {
            	CycleRing -> Set(CYCLE_C);
    	     	*STAR = *A_AR;
        	    *C_AR = *STAR;		//	Mod by 0
            	LastInstructionReadout = true;
	            return;
    	    }

            //	Otherwise, an invalid 1 character opcode

	        InstructionCheck -> SetStop("Instruction Check: Invalid length at I1");
    	    return;

        case I_RING_2:

        	//	Handle simple 2 character op codes

        	if(OpReadOutLines & OP_2CHARONLY) {
            	LastInstructionReadout = true;
                return;
            }

            //	Otherwise, we have something invalid

            InstructionCheck -> SetStop("Instruction Check: Invalid length at I2");
            return;

        case I_RING_6:

        	//	First, handle ops that end or chain normally with 1 address

        	if(OpReadOutLines & OP_NODCYIRING6) {
                if((Op_Reg -> Get() & 0x3f) == OP_HALT) {
                    BranchLatch = true;
                }
            	LastInstructionReadout = true;
                return;
            }

            //	Handle Multiply/Divide chaining

       	    if(OpOperationalLines & OP_MPYORDIV) {
            	CycleRing -> Set(CYCLE_D);
               	*STAR = *B_AR;
      	        *D_AR = *STAR;	//	Mod by 0   (fixed 3/23/99)
                LastInstructionReadout = true;
                return;
           	}

            //	Otherwise, something is wrong.

            InstructionCheck -> SetStop("Instruction Check: Invalid length at I6");
            return;

        case I_RING_7:

        	//	Handle opcodes that are 1 character plus Op Modifier

            if(OpReadOutLines & OP_1ADDRPLUSMOD) {
            	LastInstructionReadout = true;
                return;
            }

            //	Otherwise, something is wrong

            InstructionCheck -> SetStop("Instruction Check: Invalid Length at I7");
            return;

        case I_RING_11:

        	//	Handle op codes that have 2 addresses with no Op Modifier
            //  Also handle branch indication for Clear Storage and Branch

            if(OpReadOutLines & OP_2ADDRNOMOD) {
            	LastInstructionReadout = true;
                if((Op_Reg -> Get() & 0x3f) == OP_CLEAR_STORAGE) {
                    BranchLatch = true;
                }
                return;
            }

            //	Otherwise, something is wrong.

            InstructionCheck -> SetStop("Instruction Check: Invalid Length at I11");
        	return;

        case I_RING_12:

        	//	Handle op codes that have 2 addresses plus an Op Modifier

            if(OpReadOutLines & OP_2ADDRPLUSMOD) {
            	LastInstructionReadout = true;
                return;
            }

            InstructionCheck -> SetStop("Instruction Check: Invalid Length at I12");
            return;

        default:

        	InstructionCheck -> SetStop("Instruction Check: Invalid Length");
            return;
        }

    }

    //	Make sure we don't fall thru here by accident.

    assert(!LastInstructionReadout && !(B_Reg -> Get().TestWM()));

    //	End of handling of B Channel WordMark

	//	If this opcode should have addresses, check to make sure that
    //	the instruction isn't too long...

    if(OpReadOutLines & OP_ADDRTYPE) {
    	if(OpReadOutLines & OP_2ADDRESS) {
        	if(OpReadOutLines & OP_2ADDRNOMOD) {
            	if(IRing -> State() == I_RING_11) {
                	InstructionCheck ->
                    	SetStop("Instruction Check: 2 Addr too long at I11");
                    return;
                }
                //	OK, proceed to "E"
            }
            else {
            	assert(OpReadOutLines & OP_2ADDRPLUSMOD);
                if(IRing -> State() == I_RING_12) {
                	InstructionCheck ->
                    	SetStop("Instruction Check: 2 Addr + Mod too long at I12");
                    return;
                }
                //	OK, Proceed to "E"
            }
        }
        else if(OpReadOutLines & OP_1ADDRPLUSMOD) {
        	if(IRing -> State() == 7) {
               	InstructionCheck ->
                	SetStop("Instruction Check: 1 Addr + Mod too long at I7");
                return;
            }
            //	OK, Proceed to "E"
        }
        else if(IRing -> State() == 6) {
        	InstructionCheck ->
            	SetStop("Instruction Check: 1 Addr too long at I6");
            return;
        }
        //	OK, Proceed to "E"
    }

    else if(OpReadOutLines & OP_2CHARONLY) {

    	if(IRing -> State() == 1) {
        	*Op_Mod_Reg = *B_Reg;
            InstructionDecodeIARAdvance();
            return;
        }

        else {
        	InstructionCheck ->
            	SetStop("Instruction Check: 2 Char Only too long at I2");
            return;
        }
    }

    //	ALL ops are either address type or 2 character!

    else {
    	InstructionCheck -> SetStop("Instruction Check: Not Addr or 2 Char ???");
        return;
    }

    //	Entry point "E" - no wordmark

    //	First, handle the I/O stuff (Percent type ops)

    if(OpReadOutLines & OP_PERCENTTYPE) {

        //	The sections of code doing address validity checking show up
        //	under entry point "C" on the flowchart, but since this
        //	section doesn't end up there, we need to do it here.


    	switch(IRing -> State()) {

        case I_RING_3:

            //	IO channel/overlap indicator must have 84 bit configuration

            if((B_Reg -> Get() & BIT_NUM) != 0x0c) {
            	AddressCheck ->
                	SetStop("Address Check: I/O Ch/Ovlp must have 84 config");
                return;
            }

            IOChannelSelect = ((B_Reg -> Get() & BITB) != 0);
            IOOverlapSelect = ((B_Reg -> Get() & BITA) == 0);
            if(IOChannelSelect >= MAXCHANNEL) {
                IOInterlockCheck ->
                    SetStop("I/O Interlock Check: Channel not implemented");
                return;
            }
            if(Channel[IOChannelSelect] -> ChInterlock -> State()) {
            	IOInterlockCheck ->
                	SetStop("I/O Interlock Check: I/O in progress at I3");
                return;
            }
            InstructionDecodeIARAdvance();
            return;

        case I_RING_4:

			Channel[IOChannelSelect] -> ChUnitType -> Set(B_Reg -> Get());
            Channel[IOChannelSelect] -> SetCurrentDevice();
            InstructionDecodeIARAdvance();
            return;

        case I_RING_5:

			//	Unit number is not allowed to have zones

            if(B_Reg -> Get().ToInt() & BIT_ZONE) {
            	AddressCheck ->
                	SetStop("Address Check: Zones over unit number");
                return;
            }

        	Channel[IOChannelSelect] -> ChUnitNumber -> Set(B_Reg -> Get());
            InstructionDecodeIARAdvance();
            return;

        default:

        	assert(IRing -> State() > 5);

            break;	//	Continue on, knowing that for an I/O op we are
            		//	in I6 and above, so we will go to step "1" soon.

        }	//	End IRing switch for Percent Type ops

    }	//	End Percent Type ops

    //  Handle Interrupt: Not percent type, at I6, and not "Y" opcode

    if((OpReadOutLines & OP_NOTPERCENTTYPE) && IRing -> State() == 6 &&
        PriorityAlert -> State() &&
        Op_Reg -> Get().To6Bit() != OP_BRANCH_PR &&
        Op_Reg -> Get().To6Bit() != OP_BRANCH_CH_1 &&
        Op_Reg -> Get().To6Bit() != OP_BRANCH_CH_2) {
        for(Ch = 0; Ch < MAXCHANNEL; ++Ch) {
            if(Channel[Ch] -> PriorityRequest != 0) {
                PriorityAlert -> Reset();
                A_AR -> Set(101);
                BranchLatch = true;
                LastInstructionReadout = false;         //  DO NOT EXECUTE INSTRUCTION!
                IRingControl = true;
                // InstructionDecodeIARAdvance();
                B_AR -> Set(I_AR -> Gate() - 1);
                return;
            }
        }
    }


    //	Check to see if we are handling the 2nd address for 2 address
    //	ops (or the only address for I/O ops) (Step 1 on page 45)

    assert(IRing -> State() > 0);

    if(IRing -> State() > 5) {

		if(OpReadOutLines & OP_1ADDRPLUSMOD) {
           	*Op_Mod_Reg = *B_Reg;
            if(OpOperationalLines & OP_BRANCHTYPE) {
                //  Branch handling done in the "normal" way.
                LastInstructionReadout = true;
                return;
            }
            else {
            	InstructionDecodeIARAdvance();
           		return;
            }
        }

        assert(OpReadOutLines & OP_2ADDRESS);

        //	For 2 address ops, and for I/O operations,
        //	snag op modifier at I11.
        //	(Invalid lengths are checked before we get here).

        if(IRing -> State() > I_RING_10) {
           	*Op_Mod_Reg = *B_Reg;
            if(OpOperationalLines & OP_BRANCHTYPE) {
            	//	Branch Handling will go here!
                InstructionDecodeIARAdvance();
                return;
            }
            else {
            	InstructionDecodeIARAdvance();
   	    	    return;
            }
        }

        //	If we are at I6, we are starting address: reset BAR, DAR

        if(IRing -> State() == I_RING_6) {
         	B_AR -> Reset();
            D_AR -> Reset();
        }

        //	Set the appropriate address character into B & DAR

		b = B_Reg -> Get();
        b = b & BIT_NUM;
        b.SetOddParity();

        B_AR -> Set(TWOOF5(b),IRing -> State() - 5);
        D_AR -> Set(TWOOF5(b),IRing -> State() - 5);

        if(IRing -> State() == I_RING_10 &&
        	IndexLatches > 0) {
        	InstructionIndexStart();		//	Start up indexing
            return;
        }

        //	Fall thru to "C"

    }	//	Ending IRing > 5

    //	Handle IRings 1 thru 5.

    else {

    	if(OpReadOutLines & OP_ADDRDBL) {

        	if(IRing -> State() == 1) {
            	A_AR -> Reset();
                B_AR -> Reset();
                C_AR -> Reset();
                D_AR -> Reset();
            }

            b = B_Reg -> Get();
            b = b & BIT_NUM;
            b.SetOddParity();

	        A_AR -> Set(TWOOF5(b),IRing -> State());
    	    B_AR -> Set(TWOOF5(b),IRing -> State());
        	C_AR -> Set(TWOOF5(b),IRing -> State());
        	D_AR -> Set(TWOOF5(b),IRing -> State());

            if(IRing -> State() == I_RING_5 &&
            	IndexLatches > 0) {
            	InstructionIndexStart();		//	Start up indexing
                return;
            }

        	//	Fall thru to step "C"
        }

		//	This coding varies slightly from the flowchart on page 45.
        //	The logic is the same, but we test for SAR (opcode G) first
        //	to avoid redundant code.

        //	It works alot easier this way becaue SAR has two special features:
        //	It doesn't reset AAR (so you can store AAR), and it cannot be
        //	indexed.

		else if((Op_Reg -> Get() & 0x3f) == OP_SAR_G) {		//	SAR

        	if(IRing -> State() == I_RING_1) {
            	C_AR -> Reset();
            }

            b = B_Reg -> Get();
            b = b & BIT_NUM;
            b.SetOddParity();

			C_AR -> Set(TWOOF5(b),IRing -> State());
        }

        else {		//	Not SAR

        	if(IRing -> State() == I_RING_1) {
            	A_AR -> Reset();
                C_AR -> Reset();
            }

            b = B_Reg -> Get();
            b = b & BIT_NUM;
            b.SetOddParity();

			A_AR -> Set(TWOOF5(b),IRing -> State());
            C_AR -> Set(TWOOF5(b),IRing -> State());

            if(IRing -> State() == I_RING_5 &&
            	IndexLatches != 0) {
                	InstructionIndexStart();	//	Start up indexing
                    return;
            }

        }

    }		//	End of I Ring 1 - 5


	//	Entry point "C" - address validity checking.
   	//	Odd that this happens in the flow chart *after* setting the character
   	//	into the address register.  Oh well - the 2-of-5 translater is robust!
   	//	It sets invalid entries to 0.

    //	First, handle special characters.  They are only allowed on I/O
    //	(Percent Type) opcodes.  Incidentally, the CPU instructional flow
    //  chart is vague on this point -- "Special Characters" as used there is
    //  not the same as when the term is used in the comparator.  What I do here
    //  is to test for a valid two-out-of-5 code value after stripping the zones
    //  and "fixing" the parity.

    b = B_Reg -> Get() & BIT_NUM;                       //  Just the numeric bits
    b.SetOddParity();                                   //  Force odd parity
	if(TWOOF5(b).ToInt() == -1) {                      //  Invalid 2 out of 5?
		if((OpReadOutLines & OP_PERCENTTYPE) == 0) {
            AddressCheck ->
            	SetStop("Address Check: Special Chars, not % type op");
            return;
        }
    }

    //	Next, check for zones, which are only valid at certain times

    if(B_Reg -> Get().ToInt() & BIT_ZONE) {
		if(!IRingZoneTable[IRing -> State()]) {
        	AddressCheck ->
            	SetStop("Address Check: Zones at invalid IRing time");
        	return;
        }

        //	Set the index latches

        IndexLatches |= (B_Reg -> Get().ToInt() & BIT_ZONE) >>
        	((IRing -> State() == I_RING_3  || IRing -> State() == I_RING_8)
            	? 2 : 4);

    }

    //	Step "B": Read out next character, advance I-Ring
    //	Will pick up again at step "D"

    InstructionDecodeIARAdvance();
    return;
}

//	Routine to implement Step "B" in the chart.  It takes an I cycle
//	(reads out character pointed to by I_AR), advances IRing, and, if
//	the newly read character doesn't have a word mark, advances I_AR.

//	In the real 1410, what happens with the I_AR is that the WM inhibits
//	setting the I_AR from STAR+1, and then later causes I_AR to be copied
//	from the STAR (which has the old address).

void T1410CPU::InstructionDecodeIARAdvance()
{
    *STAR = *I_AR;
    IRing -> Next();
	Readout();
    Cycle();
    if(!B_Reg -> Get().TestWM()) {
	    I_AR -> Set(STARMod(+1));
    }
    return;
}

//	Routine to start up indexing.
//	When an index is present at IRing 5 or IRing 11 times, we do this instead
//	of advancing the IRing.  This then causes X Cycles (see below)

void T1410CPU::InstructionIndexStart()
{
    CycleRing -> Set(CYCLE_X);	//	Doing X cycles
	ARing -> Reset();			//	Reset A Ring to initial state

    assert(IndexLatches > 0 && IndexLatches < 16);

    //	Use the "address generator" to address the proper index register

    STAR -> Set(IndexRegisterLookup[IndexLatches]);

    //	Advance to A2, and read out first index register character
    //	Address modification by -1 for this sycle

    ARing -> Next();
    Readout();
    Cycle();					//	Does nothing
    STAR -> Set(STARMod(-1));
}

//	Indexing routine

void T1410CPU::InstructionIndex()
{
	BCD sum;

	assert(IRing -> State() == I_RING_5 || IRing -> State() == I_RING_10);

    if(IRing -> State() == I_RING_5) {
    	if(ARing -> State() == A_RING_2) {
        	if(OpReadOutLines & OP_ADDRDBL) {
                B_AR -> Reset();
                D_AR -> Reset();
            }
           	A_AR -> Reset();
        }

        A_Reg -> Set(C_AR -> GateBCD(6 - (ARing -> State()))); // Gate C Address reg.
    }
    else {
    	if(ARing -> State() == A_RING_2) {
        	B_AR -> Reset();
        }

        A_Reg -> Set(D_AR -> GateBCD(6 - (ARing -> State()))); // Gate D Address reg.
    }

	//	Determine sign of indexing.  If minus, set up a complement add
    //	(Complement add also requires carry to be set).

    if(ARing -> State() == A_RING_2) {
    	if(B_Reg -> Get().IsMinus()) {
	    	BComplement -> Set();
            CarryIn -> Set();
        }
        else {
	        CarryIn -> Reset();
            BComplement -> Reset();
        }
    }
    else {
    	CarryIn -> Set(CarryOut -> State());
    }

	//	Run it thru the adder, with the A channel coming from A Register

    Adder(AChannel -> Select(AChannel -> A_Channel_A),false,
    	B_Reg -> Get(),BComplement -> State());

    // DEBUG("Indexing.  Added %x",AChannel -> Select().ToInt());
    // DEBUG("           And   %x",B_Reg -> Get().ToInt());
    // DEBUG("           COMP  %x",BComplement -> State());
    // DEBUG("           SUM   %x",AdderResult.ToInt());

    //	Gate adder numerics to assembly channel

    sum = AssemblyChannel -> Select(
    	AssemblyChannel -> AsmChannelWMNone,
        AssemblyChannel -> AsmChannelZonesNone,
        false,
        AssemblyChannel -> AsmChannelSignNone,
        AssemblyChannel -> AsmChannelNumAdder );

    if(IRing -> State() == 5) {
    	if(OpReadOutLines & OP_ADDRDBL) {
        	B_AR -> Set(TWOOF5(sum),6 - (ARing -> State()));
        	D_AR -> Set(TWOOF5(sum),6 - (ARing -> State()));
        }
        A_AR -> Set(TWOOF5(sum),6 - (ARing -> State()));
    }
    else {
        B_AR -> Set(TWOOF5(sum),6 - (ARing -> State()));
        D_AR -> Set(TWOOF5(sum),6 - (ARing -> State()));
    }

    //	If indexing operation is done, set registers right, and restart
    //	the instruction readout process

    if(ARing -> State() == A_RING_6) {
    	if(IRing -> State() == I_RING_5) {
        	*C_AR = *A_AR;
        }
        else {
        	*D_AR = *B_AR;
        }
	    CycleRing -> Set(CYCLE_I);	//	Doing I cycles again
        InstructionDecodeIARAdvance();
        return;
    }

    //	Otherwise, advance to the next X Cycle

    ARing -> Next();
    Readout();
    Cycle();
    STAR -> Set(STARMod(-1));
}

//	Dummy instruction execution routine: used for invalid/unimplemented ops

void T1410CPU::InstructionExecuteInvalid()
{
    InstructionCheck -> SetStop("Attempt to execute Invalid/Unimplemented Op");
	return;
}
