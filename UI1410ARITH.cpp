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

//	Arithmetic Instruction execution routines

//---------------------------------------------------------------------------
#include <vcl\vcl.h>
#pragma hdrstop

#include "UI1410ARITH.h"
//---------------------------------------------------------------------------

#include <assert.h>
#include <dir.h>
#include <stdio.h>

#include "UBCD.H"
#include "UI1410CPUT.H"
#include "UIHOPPER.H"
#include "UI1410CHANNEL.H"
#include "UI1410DEBUG.H"
#include "UI1410INST.H"

void T1410CPU::InstructionArith()
{

	//	Since arithmetic involves reading thru two fields, and since
    //	we have to support STORAGE CYCLE, we need to remember which kind
    //	we are expecting to do.

	static int cycle_type;
    static int op_bin;
    int	i;

    BCD	adder_a;                                //  A side of adder
    BCD adder_b;                   				//	B side of adder
    BCD hold_b;                                 //  Temp holding

	//	If the first time in, set things up.

	if(LastInstructionReadout) {
    	SubScanRing -> Set(SUB_SCAN_U);     	//	Set the Units latch
        ScanRing -> Set(SCAN_1);				//	Set Scan to 1 (-1 modif.)

        //	Overflow is *not* reset!  It stays on once set

        CarryIn -> Reset();						//	Reset so it looks right in
        CarryOut -> Reset();					//		cycle testing
        AComplement -> Reset();					//	Here too - so that single
        BComplement -> Reset();					//		cycle looks nice.

        cycle_type = CYCLE_A;					//	Set for initial A cycle
        op_bin = Op_Reg -> Get().ToInt() & 0x3f;
        LastInstructionReadout = false;
        ZeroBalance -> Set();					//	Assume zero balance to start
    }

    if(cycle_type == CYCLE_A) {
        CycleRing -> Set(CYCLE_A);				//	Set up for an A cycle
        *STAR = *A_AR;							//	Initiate cycle
        Readout();								//	Address storage from AAR
        Cycle();								//	Set AAR using proper mod
        cycle_type = CYCLE_B;					//	Next time, do B Cycle
        return;									//	Next time, LIRO is false
        										//	But units latch is set!
	}

    CycleRing -> Set(CYCLE_B);					//	Normally, do a B cycle
    if(SubScanRing -> State() == SUB_SCAN_U) {
    	*STAR = *D_AR;							//	Read B units via DAR
    }
    else {
    	*STAR = *B_AR;							//	others use BAR
    }
    Readout();									//	Address storage frrom BAR

    //	Note that we don't do the address modification cycle yet, as we
    //	have to store the result back where BAR points!

    //	The following  looks complicated on the flow chart in the
    //	IBM CE Instructional materials 1411 Processing Unit Instructions
    //	on page 11.  But, the end result is that if you have an EVEN number
    //	of: -A, -B and SUBTRACT you do a TRUE ADD cycle, and ODD number and
    //	you do a COMPLEMENT ADD cycle

    adder_b = B_Reg -> Get();                   //  What goes into B of adder
    if(ScanRing -> State() == SCAN_1) {			//	Is 1st scan ring on?
    	if(SubScanRing -> State() == SUB_SCAN_U) {	//	Units latch?
			BComplement -> Reset();                 //	Set True Add B latch
	        i  =
    	    	(A_Reg -> Get().IsMinus()) +
        	    (B_Reg -> Get().IsMinus()) +
	            (op_bin == OP_SUBTRACT);
    	    if(i & 1) {
        		AComplement -> Set();				//	Odd #: complement add
	            CarryIn -> Set();
    	    }
        	else {
	        	AComplement -> Reset();				//	Even #: true add
    	        CarryIn -> Reset();
        	}

            adder_a = A_Reg -> Get();

	    }	//	End, units
        else {
			if(SubScanRing -> State() == SUB_SCAN_E) {	//	Extension latch?
            	adder_a = BCD_0;						//	Yes, gate '0'
                										//	(if a compl then 9)
            }
            else {
            	adder_a = A_Reg -> Get();				//	No, use A data
            }
        }
    }	//	End, SCAN 1

    else {

        //  In the Real 1410, the following code would invert the sign
        //  Of the B CHANNEL (not the B Register).  But, I didn't really
        //  implement the B CHANNEL, so we put it into a special variable
        //  which will be fed to the assembly channel

    	assert(ScanRing -> State() == SCAN_3);			//	Else must be 3
        if(SubScanRing -> State() == SUB_SCAN_U) {		//	Units?
            adder_b = B_Reg -> Get();					//	Yes.  Invert sign
            adder_b = (adder_b & ~BIT_ZONE) |
            	(adder_b.IsMinus() ? (BITA | BITB) : BITB);
            adder_b.SetOddParity();
            AComplement -> Reset();						//	Reset compl add
        }
        adder_a = BCD_0;
    }

    //	Run the right stuff thru the adder, and then thru the Assembly Channel

    Adder(adder_a,AComplement -> State(),
    	adder_b,BComplement -> State());

    if(ZeroBalance -> State() &&
    	(AdderResult & BIT_NUM) != (BCD_0 & ~BITC)) {
        ZeroBalance -> Reset();
    }

    //  Again, since I didn't really implement the B Channel,
    //  temporarily use the B Register as the B Channel (whose sign
    //  might have been inverted earlier, and the restore it later

    hold_b = B_Reg -> Get();                            //  Preserve B
    B_Reg -> Set(adder_b);                              //  Capture B Channel

    AssemblyChannel -> Select(
    	AssemblyChannel -> AsmChannelWMB,
        AssemblyChannel -> AsmChannelZonesB,
        false,
        AssemblyChannel -> AsmChannelSignNone,
        AssemblyChannel -> AsmChannelNumAdder
    );

    B_Reg -> Set(hold_b);                               //  Restore B Reg

	if(B_Reg -> Get().TestWM()) {                   	//	B Channel WM?

    	if(ScanRing -> State() == SCAN_3) {				//	3rd scan latch?
        	Store(AssemblyChannel -> Select());			//	Store the last char
            Cycle();									//	Modify B Addr -1
        	IRingControl = true;						//	Done!
        	return;
        }

        if(AComplement -> State()) {					//	Complement add?

			if(CarryOut -> State()) {					//	Carry too?
            	Store(AssemblyChannel -> Select());		//	Store the last char
                Cycle();								//	Modify B addr -1
            	IRingControl = true;					//	Done! (no sign sw)
                return;
            }

            //	What I *think* the following does is to reverse the B sign
            //	where D_AR points, becuase SUB_SCAN_U causes the CPU to use
            //	the DAR for the storage cycle, which starts a recomplement
            //	cycle wherein the entire B field is recomplemented.  This
            //	can only happen if we are doing a complement add,
            //	with no carry out.

            SubScanRing -> Set(SUB_SCAN_U);				//	Set units latch
            ScanRing -> Set(SCAN_3);					//	Set 3rd scan latch
            BComplement -> Set();						//	Set B compl add
            CarryIn -> Set();							//	Set carry
            Store(AssemblyChannel -> Select());			//	Store the last char
            Cycle();									//	Finish the B cycle
            cycle_type = CYCLE_B;						//	Re-enter at B cycle
            return;
        }

        else {											//	True add
        	if(CarryOut -> State()) {					//	Set overflow on carry
            	Overflow -> Set();
            }
            Store(AssemblyChannel -> Select());			//	Store the last char
            Cycle();									//	Modify B addr -1
            IRingControl = true;						//	Done!
            return;
        }
    }	//	End B Ch WM

    else {												//	No B Ch WM
    	CarryIn -> Set(CarryOut -> State());			//	Set carry
        Store(AssemblyChannel -> Select());				//	Store result char
        Cycle();										//	Modify B address -1
        if(ScanRing -> State() == SCAN_1) {				//	Still 1st scan?
        	if(A_Reg -> Get().TestWM()) {				//	A Ch WM?
            	SubScanRing -> Set(SUB_SCAN_E);			//	Yes -- set extension
                cycle_type = CYCLE_B;					//	No more A cycles!
                return;									//	continue on
            }
            else {										//	No A CH WM
            	SubScanRing -> Set(SUB_SCAN_B);			//	Set to body
                cycle_type = CYCLE_A;					//	A cycle next
                return;
            }
        }
        else {
        	assert(ScanRing -> State() == SCAN_3);		//	Must be!
            SubScanRing -> Set(SUB_SCAN_E);				//	Still in extension
            cycle_type = CYCLE_B;						//	Which means no A cyc
            return;
        }
    }
}

void T1410CPU::InstructionZeroArith()
{

	//	Variable to maintain state between cycles

    static int cycle_type;
    static int op_bin;

    BCD a_temp,a_hold;

    //	If this is the first time in, set things up

    if(LastInstructionReadout) {
		SubScanRing -> Set(SUB_SCAN_U);             	//	Units
		ScanRing -> Set(SCAN_1);						//	Scan 1 (-1 mod)

        AComplement -> Reset();
        BComplement -> Reset();
        CarryIn -> Reset();
        CarryOut -> Reset();
        ZeroBalance -> Set();							//	Assume 0 at start

        cycle_type = CYCLE_A;							//	Set for 1st A cycle
        op_bin = Op_Reg -> Get().ToInt() & 0x3f;
        LastInstructionReadout = false;
    }

    //	Initial A cycle

    if(cycle_type == CYCLE_A) {
    	CycleRing -> Set(CYCLE_A);						//	Take an A Cycle
        *STAR = *A_AR;
        Readout();										//	RO A field char.
        Cycle();										//	Set AAR with mod

        //	Units or body latch Regen happens by itself in simulator

        cycle_type = CYCLE_B;							//	Set for B cycle
        return;
    }

    assert(cycle_type == CYCLE_B);						//	Has to be B cycle

    CycleRing -> Set(CYCLE_B);							//	Take B Cycle
    if(SubScanRing -> State() == SUB_SCAN_U) {			//	Read B units via DAR
    	*STAR = *D_AR;
    }
    else {
    	*STAR = *B_AR;
    }
	Readout();											//	Ro B Field Char

    //	If we are in the body/extension, we just insert 0's on the
    //	A channel.  Otherwise, we normalize the A field sign

    a_temp = A_Reg -> Get();
    a_hold = a_temp;									//	Save to restore ...

    if(SubScanRing -> State() == SUB_SCAN_U) {
    	if(op_bin == OP_ZERO_ADD) {						//	ZA - normalize sign
        	a_temp = (a_temp & ~BIT_ZONE) |
            	(a_temp.IsMinus() ? BITB : (BITA | BITB));
        }
        else {											//	ZS - invert sign
        	a_temp = (a_temp & ~BIT_ZONE) |
            	(a_temp.IsMinus() ? (BITA | BITB) : BITB);
        }
        a_temp.SetOddParity();

        //  In the real machine, the sign fixing happens on the A Channel,
        //  and not to the A Register itself.  We'll cheat and use the A
        //  register itself, so that the assembly channel can use the zones.
        //  Then we will reset it later.

        A_Reg -> Set(a_temp);
    }

	if(SubScanRing -> State() == SUB_SCAN_E) {
		a_temp = BCD_0;
    }

    //	Run it thru the adder (kind of pointless in the simulator.  Oh well.

    Adder(a_temp,false,BCD_0,false);

    if(ZeroBalance -> State() &&
    	(AdderResult & BIT_NUM) != (BCD_0 & ~BITC)) {
        	ZeroBalance -> Reset();
    }

    AssemblyChannel -> Select(
    	AssemblyChannel -> AsmChannelWMB,
        (SubScanRing -> State() == SUB_SCAN_U ?
        	AssemblyChannel -> AsmChannelZonesA :
            AssemblyChannel -> AsmChannelZonesNone),
        false,
        AssemblyChannel -> AsmChannelSignNone,		//	Maybe signlatch someday
        AssemblyChannel -> AsmChannelNumAdder
    );

	Store(AssemblyChannel -> Select());					//	Store the results
    Cycle();											//	Finish the cycle

    A_Reg -> Set(a_hold);								//	Reset a register

    if(B_Reg -> Get().TestWM()) {                       //	B Channel WM?
		IRingControl = true;							//	Done!
        return;
    }

    //	Again, 1st Scan Latch and True Add latch regen all by themselves

	CarryIn -> Set(CarryOut -> State());				//	Do carry as needed

	if(a_hold.TestWM()) {								//	A Channel WM?
        SubScanRing -> Set(SUB_SCAN_E);					//	Yes -- extension
        cycle_type = CYCLE_B;							//	B Cycle Next
        return;
    }
    else {
    	SubScanRing -> Set(SUB_SCAN_B);					//	No -- still body
        cycle_type = CYCLE_A;							//	A Cycle next
        return;
    }
}

//  Multiply instruction execution routeine

void T1410CPU::InstructionMultiply()
{
    static int cycle_type;
    static int cycle_subtype;
    static bool MultiplyDivideLastLatch;

    BCD adder_a,b_temp;

    if(LastInstructionReadout) {
        SubScanRing -> Set(SUB_SCAN_U);                 //  Set the Units latch
        ScanRing -> Set(SCAN_1);                        //  Set Scan to 1
        BComplement -> Reset();                         //  True Add B
        AComplement -> Reset();                         //  True Add A
        ZeroBalance -> Set();                           //  Start as 0
        CarryIn -> Reset();                             //  So single cycle
        CarryOut -> Reset();                            //      looks good
        MultiplyDivideLastLatch = false;                //  MDL latch reset
        SignLatch = false;                              //  Start out as +
        cycle_type = CYCLE_A;                           //  Start with A cycle
        cycle_subtype = 0;                              //  No subtype
    }

    if(cycle_type == CYCLE_A && cycle_subtype == 0) {
        CycleRing -> Set(CYCLE_A);                      //  Set for an A cycle
        if(SubScanRing -> State() == SUB_SCAN_U ||
           SubScanRing -> State() == SUB_SCAN_E) {
            *STAR = *C_AR;                      //  A units, extension via CAR
        }
        else {
            assert(SubScanRing -> State() == SUB_SCAN_B);
            *STAR = *A_AR;                              //  A body via AAR
        }
        Readout();
        Cycle();
        assert(ScanRing -> State() == SCAN_1);          //  Regen 1st Scan
        cycle_type = CYCLE_B;                           //  Next cycle: B
        cycle_subtype = 0;
        return;
    }   //  End Cycle A

    if(cycle_type == CYCLE_B && cycle_subtype == 0) {

        CycleRing -> Set(CYCLE_B);                       //  Set for a B cycle
        if(SubScanRing -> State() == SUB_SCAN_U) {
            *STAR = *D_AR;                               //  Read B units - DAR
        }
        else {
            *STAR = *B_AR;                               //  Otherwise, use BAR
        }
        Readout();

        if(SubScanRing -> State() == SUB_SCAN_U ||
           SubScanRing -> State() == SUB_SCAN_B ||
           SubScanRing -> State() == SUB_SCAN_E) {

            Adder(BCD_0,AComplement->State(),            //  Set up 0 in adder
                BCD_0,BComplement->State());
            AssemblyChannel -> Select(                   //  No zones
                AssemblyChannel -> AsmChannelWMB,
                AssemblyChannel -> AsmChannelZonesNone,
                false,
                AssemblyChannel -> AsmChannelSignNone,
                AssemblyChannel -> AsmChannelNumAdder);
            Store(AssemblyChannel -> Select());  //  Store the 0
            Cycle();                             //  Modify B address by -1
            assert(!(AComplement -> State()));    //  regen True add

            if(SubScanRing -> State() == SUB_SCAN_E) {  //  Extension
                CarryIn -> Reset();                     //  No carry
                SubScanRing -> Set(SUB_SCAN_MQ);        //  Set MQ latch
                assert(ScanRing -> State() == SCAN_1);  //  Regen 1st scan
                cycle_type = CYCLE_B;                   //  B Cycle next
                cycle_subtype = 0;
                return;
            }   //  Extension

            assert(SubScanRing -> State() == SUB_SCAN_U ||
               SubScanRing -> State() == SUB_SCAN_B);   //  Units or Body

            if(A_Reg -> Get().TestWM()) {           //  A Channel WM?
                SubScanRing -> Set(SUB_SCAN_E);     //  Set Extension latch
                ScanRing -> Set(SCAN_N);            //  Set No Scan
                cycle_type = CYCLE_C;               //  C Cycle next
                cycle_subtype = 0;
                return;
            }
            else {
                SubScanRing -> Set(SUB_SCAN_B);         //  Set Body latch
                assert(ScanRing -> State() == SCAN_1);  //  Regen 1st scan
                cycle_type = CYCLE_A;                   //  A Cycle next
                cycle_subtype = 0;
                return;
            }  //  End: Units or body

        }   //  End: Units, body or Extension

        else {
            assert(SubScanRing -> State() == SUB_SCAN_MQ);  //  MQ

            //  As with add and subtract, the following is complicated on
            //  the flow chart, but all it is really doing is checking to
            //  see if the number of "-" values is even or odd

            if(((A_Reg -> Get().IsMinus()) + (B_Reg -> Get().IsMinus())) & 1) {
                CPU -> SignLatch = true;                //  Odd: Set - sign
            }
            else {
                CPU -> SignLatch = false;
            }

            //  DEBUG("Multiply, B Cycle, Sign is %d .",CPU -> SignLatch);

            adder_a = BCD_9;                            //  9 on adder A ch.
            b_temp = B_Reg -> Get();                    //  Analyze B ch char
            b_temp = b_temp & BIT_NUM;                  //  Look at just numerics
            if(numeric_value[b_temp.ToInt()] == 0) {    //  Store 0 in B Field
                Store( AssemblyChannel -> Select (
                    AssemblyChannel -> AsmChannelWMB,
                    AssemblyChannel -> AsmChannelZonesNone,
                    false,
                    AssemblyChannel -> AsmChannelSignNone,
                    AssemblyChannel -> AsmChannelNumZero) );
                Cycle();
                AComplement -> Reset();                 //  Set True add latch
                CarryIn -> Reset();                     //  Set No Carry
                if(B_Reg -> Get().TestWM()) {
                    ScanRing -> Set(SCAN_2);            //  Set 2nd Scan
                }
                else {
                    assert(ScanRing -> State() == SCAN_1);  //  Regen 1st scan
                }
            }
            else if(numeric_value[b_temp.ToInt()] > 4) {//  5 - 9
                Store( AssemblyChannel -> Select (      //  Store B numerics
                    AssemblyChannel -> AsmChannelWMB,
                    AssemblyChannel -> AsmChannelZonesNone,
                    false,
                    AssemblyChannel -> AsmChannelSignNone,
                    AssemblyChannel -> AsmChannelNumB) );
                Cycle();
                AComplement -> Set();                   //  Set Complement
                CarryIn -> Set();                       //  Set Carry
                ScanRing -> Set(SCAN_N);                //  Set No Scan
            }

            else {
                Adder(adder_a,AComplement->State(),b_temp,BComplement->State());
                Store( AssemblyChannel -> Select (      //  Store adder output
                    AssemblyChannel -> AsmChannelWMB,
                    AssemblyChannel -> AsmChannelZonesNone,
                    false,
                    AssemblyChannel -> AsmChannelSignNone,
                    AssemblyChannel -> AsmChannelNumAdder) );
                Cycle();
                AComplement -> Reset();                 //  Set True Latch
                CarryIn -> Reset();                     //  No Carry
                ScanRing -> Set(SCAN_N);                //  Set No Scan
            }

            cycle_type = CYCLE_D;
            cycle_subtype = 0;
            return;

        }   // MQ

    }   // End B Cycle, subtype 0

    if(cycle_type == CYCLE_C) {
        CycleRing -> Set(CYCLE_C);                      //  C Cycle
        *STAR = *C_AR;                                  //  Read out units pos
        Readout();                                      //  Read storage
        Cycle();                                        //  C is not modified
        ScanRing -> Set(SCAN_1);
        cycle_type = CYCLE_B;                           //  B Cycle next time
        cycle_subtype = 0;
        return;
    }   //  End C Cycle

    if(cycle_type == CYCLE_D) {
        CycleRing -> Set(CYCLE_D);                      //  D Cycle
        *STAR = *D_AR;
        Readout();
        if(ScanRing -> State() != SCAN_3) {             //  N, 1st or 2nd scan
            Store( AssemblyChannel -> Select(           //  Store 0 & sign
                AssemblyChannel -> AsmChannelWMB,
                AssemblyChannel -> AsmChannelZonesNone,
                false,
                AssemblyChannel -> AsmChannelSignLatch,
                AssemblyChannel -> AsmChannelNumZero) );
        }
        //  If 3rd Scan memory is regened (no store in emulator)

        Cycle();

        CarryIn -> Set(AComplement -> State());         //  Set carry state

        if(ScanRing -> State() == SCAN_2) {             //  Done if 2nd Scan
            IRingControl = true;
            return;
        }

        if(ScanRing -> State() == SCAN_N || MultiplyDivideLastLatch) {
            SubScanRing -> Set(SUB_SCAN_U);             //  Set Units latch
            ScanRing -> Set(SCAN_3);                    //  Set 3rd scan
            BComplement -> Reset();                     //  Set True Add B
                                                        //  AComplement regen'd
            cycle_type = CYCLE_A;                       //  A Cycle next
            cycle_subtype = 1;                          //  2nd kind of A cycle
            return;
        }

        //  1st or 3rd scan, MDL latch reset...

        SubScanRing -> Set(SUB_SCAN_MQ);                //  Set MQ latch
        ScanRing -> Set(SCAN_3);                        //  Set 3rd scan
        BComplement -> Reset();                         //  True add B latch
                                                        //  Acomplement regen'd
        cycle_type = CYCLE_B;                           //  B Cycle next
        cycle_subtype = 1;                              //  2nd kind of B cycle
        return;
    }   //  End D Cycle

    if(cycle_type == CYCLE_A && cycle_subtype == 1) {
        CycleRing -> Set(CYCLE_A);                      //  A Cycle
        if(SubScanRing -> State() == SUB_SCAN_U ||
           SubScanRing -> State() == SUB_SCAN_E) {      //  Units or Extension?
            *STAR = *C_AR;                              //  Yes, use CAR for address
        }
        else {
            *STAR = *A_AR;
        }
        Readout();                                      //  Read out A field
        Cycle();
        assert(ScanRing -> State() == SCAN_3);          //  Regen 3rd scan
        BComplement -> Reset();                         //  True add B latch
                                                        //  A Complement regen'd
        cycle_type = CYCLE_B;                           //  B Cycle next
        cycle_subtype = 1;                              //  2nd kind of B cycle
        return;
    }

    if(cycle_type == CYCLE_B && cycle_subtype == 1) {

        CycleRing -> Set(CYCLE_B);                      //  B Cycle
        if(SubScanRing -> State() == SUB_SCAN_U) {      //  Read Units using DAR
            *STAR = *D_AR;
        }
        else {
            *STAR = *B_AR;
        }
        Readout();

        if(SubScanRing -> State() == SUB_SCAN_MQ) {
            if(AComplement -> State()) {
                adder_a = BCD_0;                            //  Insert 0 on A
                                                            //  With carry, will add 1
            }
            else {
                adder_a = BCD_9;                            //  Else insert a 9
            }                                               //  Which will decr.

            b_temp = B_Reg -> Get();                    //  Get B data
            b_temp = b_temp & BIT_NUM;                  //  Numeric bits

            if(numeric_value[b_temp.ToInt()] == 0 && !(AComplement -> State()) ) {
                Store(  AssemblyChannel -> Select (     //  Store 0, no zones
                    AssemblyChannel -> AsmChannelWMB,
                    AssemblyChannel -> AsmChannelZonesNone,
                    false,
                    AssemblyChannel -> AsmChannelSignNone,
                    AssemblyChannel -> AsmChannelNumZero) );
                Cycle();
                assert(!(AComplement -> State()));      //  True Add (was already)
                CarryIn -> Reset();                     //  Reset Carry
                if(B_Reg -> Get().TestWM()) {           //  B WM?
                    IRingControl = true;                //  Yes.  All done.
                    return;
                }
                assert(ScanRing -> State() == SCAN_3);  //  Regen 3rd Scan latch
                assert(!(BComplement -> State()));       //  Regen B True add
                cycle_type = CYCLE_D;                   //  Next cycle D
                cycle_subtype = 0;
                return;
            }   //  End B is Zero, True Add

            if(AComplement -> State() &&
               (numeric_value[b_temp.ToInt()] < 5)) {
                Store( AssemblyChannel -> Select (      //  Store B (no zones)
                    AssemblyChannel -> AsmChannelWMB,
                    AssemblyChannel -> AsmChannelZonesNone,
                    false,
                    AssemblyChannel -> AsmChannelSignNone,
                    AssemblyChannel -> AsmChannelNumB) );
                Cycle();
                AComplement -> Reset();                  //  Set True Add latch
                CarryIn -> Reset();
                SubScanRing -> Set(SUB_SCAN_U);          //  Set Units latch
                assert(ScanRing -> State() == SCAN_3);   //  Regen 3rd scan
                BComplement -> Reset();                  //  Set True Add B latch
                cycle_type = CYCLE_A;                    //  Next Cycle: A
                cycle_subtype = 1;                       //  second kind of A
                return;
            }   //  End B is 0-4, Complement Add

            if(!(AComplement -> State()) && numeric_value[b_temp.ToInt()] < 5) {
                Adder(adder_a,AComplement -> State(),
                    B_Reg -> Get(),BComplement -> State());
                Store( AssemblyChannel -> Select (     //  Store adder, no zones
                    AssemblyChannel -> AsmChannelWMB,
                    AssemblyChannel -> AsmChannelZonesNone,
                    false,
                    AssemblyChannel -> AsmChannelSignNone,
                    AssemblyChannel -> AsmChannelNumAdder) );
                Cycle();
                AComplement -> Reset();                 //  Set True Add latch
                CarryIn -> Reset();                     //  Set no carry
                SubScanRing -> Set(SUB_SCAN_U);         //  Set units latch
                assert(ScanRing -> State() == SCAN_3);  //  Regen 3rd scan
                BComplement -> Reset();                 //  True add B
                cycle_type = CYCLE_A;                   //  Next cycle A
                cycle_subtype = 1;                      //  second kind of A
                return;
            }   //  End B is 1-4, True Add

            assert(numeric_value[b_temp.ToInt()] >= 5);

            if(numeric_value[b_temp.ToInt()] < 9 && AComplement -> State()) {
                Adder(adder_a,false,                    //  DON'T USE AComplement HERE!
                                                        //  Because it was dealt with
                                                        //  earlier
                    B_Reg -> Get(),BComplement -> State());
                Store( AssemblyChannel -> Select (      //  Store adder, no zones
                    AssemblyChannel -> AsmChannelWMB,
                    AssemblyChannel -> AsmChannelZonesNone,
                    false,
                    AssemblyChannel -> AsmChannelSignNone,
                    AssemblyChannel -> AsmChannelNumAdder) );
                Cycle();
                AComplement -> Set();                   //  Set Complement latch
                CarryIn -> Set();                       //  Set carry latch
                SubScanRing -> Set(SUB_SCAN_U);         //  Set Units latch
                assert(ScanRing -> State() == SCAN_3);  //  Regen 3rd scan
                BComplement -> Reset();                 //  True add B
                cycle_type = CYCLE_A;                   //  Next Cycle A
                cycle_subtype = 1;                      //  second kind of A
                return;
            }

            if(!(AComplement -> State())) {              //  True latch on?
                Store( AssemblyChannel -> Select (      //  Store B, no zones
                    AssemblyChannel -> AsmChannelWMB,
                    AssemblyChannel -> AsmChannelZonesNone,
                    false,
                    AssemblyChannel -> AsmChannelSignNone,
                    AssemblyChannel -> AsmChannelNumB) );
                Cycle();
                AComplement -> Set();                   //  Set Complement latch
                CarryIn -> Set();                       //  Set carry latch
                SubScanRing -> Set(SUB_SCAN_U);         //  Set Units latch
                assert(ScanRing -> State() == SCAN_3);  //  Regen 3rd scan
                BComplement -> Reset();                 //  True add B
                cycle_type = CYCLE_A;                   //  Next Cycle A
                cycle_subtype = 1;                      //  second kind of A
                return;
            }

            assert(numeric_value[b_temp.ToInt()] == 9 && AComplement -> State()) ;

            Store(AssemblyChannel -> Select (           //  Store 0 or B, no zones
                AssemblyChannel -> AsmChannelWMB,
                AssemblyChannel -> AsmChannelZonesNone,
                false,
                AssemblyChannel -> AsmChannelSignNone,
                (ZeroBalance -> State() ?
                    AssemblyChannel -> AsmChannelNumZero :
                    AssemblyChannel -> AsmChannelNumB) ) );
            Cycle();
            if(B_Reg -> Get().TestWM()) {               //  B WM?
                AComplement -> Reset();                 //  Set True Add latch
                MultiplyDivideLastLatch = true;         //  Set MDL latch
            }
            else {
                AComplement -> Set();                   //  No.  Set Complement
            }
            assert(ScanRing -> State() == SCAN_3);      //  Regen 3rd scan
            BComplement -> Reset();                     //  True Add B
            cycle_type = CYCLE_D;                       //  D Cycle Next
            cycle_subtype = 0;
            return;
        }   // End: MQ

        if(SubScanRing -> State() == SUB_SCAN_E) {
            adder_a = BCD_0;                            //  Insert 0
                                                        //  Complement may later
                                                        //  make it a 9.
        }
        else {
            adder_a = A_Reg -> Get();                   //  Gate A Ch to adder
        }

        Adder(adder_a,AComplement -> State(),
            B_Reg -> Get(), BComplement -> State());

        Store ( AssemblyChannel -> Select (             //  Adder, B Zones to Asm
            AssemblyChannel -> AsmChannelWMB,
            AssemblyChannel -> AsmChannelZonesB,
            false,
            AssemblyChannel -> AsmChannelSignNone,
            AssemblyChannel -> AsmChannelNumAdder) );
        Cycle();

        if((AdderResult & BIT_NUM) != (BCD_0 & ~BITC)) { //  Check Zero Balance
            ZeroBalance -> Reset();
        }

        if(SubScanRing -> State() == SUB_SCAN_E) {      //  Extension?
            CarryIn -> Set(AComplement -> State());     //  Yes. Possibly set carry
            if(MultiplyDivideLastLatch) {               //  MDL ?
                IRingControl = true;                    //  Yes - Done
                return;
            }
            SubScanRing -> Set(SUB_SCAN_MQ);            //  No - set MQ latch
            assert(ScanRing -> State() == SCAN_3);      //  Regen 3rd scan
            BComplement -> Reset();                     //  True Add B
            cycle_type = CYCLE_B;                       //  Take another B cycle
            cycle_subtype = 1;                          //  also subtype 1
            return;
        }

        assert(SubScanRing -> State() == SUB_SCAN_U ||
            SubScanRing -> State() == SUB_SCAN_B);      //  Must be Units or Body

        CarryIn -> Set(CarryOut -> State());            //  Forward adder carry

        if(A_Reg -> Get().TestWM()) {                   //  A Channel WM ?
            SubScanRing -> Set(SUB_SCAN_E);             //  Yes.  Set Extension
            assert(ScanRing -> State() == SCAN_3);      //  Regen 3rd scan
            cycle_type = CYCLE_B;                       //  Take another B Cycle
            cycle_subtype = 1;                          //  Also subtype 1
            return;
        }
        else {
            SubScanRing -> Set(SUB_SCAN_B);             //  No WM.  Body.
            assert(ScanRing -> State() == SCAN_3);      //  Regen 3rd scan
            BComplement -> Reset();                     //  True Add B
            cycle_type = CYCLE_A;                       //  Take an A cycle next
            cycle_subtype = 1;                          //  Subtype 1
            return;
        }
    }   // End: B Cycle, subtype 1

    assert(false);                                      //  OOPS.

}


//  Divide Instruction execute routine

void T1410CPU::InstructionDivide()
{

    //  Divide is using a new approach to state, so that when doing
    //  storage cycles, the storage print-out and indicators are the
    //  state of the machine at the end of the instruction.  (The other
    //  instruction routines currently show scan and sub_scan for the
    //  *next* cycle, which confuses debugging.

    static MultiplyDivideLastLatch;

    static struct {
        char cycle;
        char subcycle;
        char scan;
        char subscan;
    } next;

    if(LastInstructionReadout) {
        SubScanRing -> Set(SUB_SCAN_U);
        next.subscan = SUB_SCAN_U;
        ScanRing -> Set(SCAN_1);
        next.scan = SCAN_1;
        BComplement -> Reset();                         //  True Add B
        AComplement -> Set();                           //  Complement add A
        CarryIn -> Set();                               //  NOT reset!
        CarryOut -> Set();                              //  NOT reset!
        MultiplyDivideLastLatch = false;
        SignLatch = false;
        next.cycle = CYCLE_A;                           //  Entering A cycle
        next.subcycle = 0;                              //  First kind
    }

    //  Do common items for all cycle types

    CycleRing -> Set(next.cycle);
    ScanRing -> Set(next.scan);
    SubScanRing -> Set(next.subscan);

    //  The "if" statements use the next.variables, for easy of coding..

    if(next.cycle == CYCLE_A && next.subcycle == 0) {
        assert(SubScanRing -> State() == SUB_SCAN_U);
        *STAR = *C_AR;                                  //  Use CAR to read units
        Readout();                                      //  Read into B register
        Cycle();                                        //  A Cycle copies to A Reg
                                                        //  And selects A Reg in
                                                        //  A Channel.
        next.cycle = CYCLE_B;
        next.subcycle = 0;
        return;
    }

    if(next.cycle == CYCLE_B && next.subcycle == 0) {

        if(SubScanRing -> State() == SUB_SCAN_U) {      //  Units ?
            *STAR = *D_AR;                              //  Use DAR to read units
            Readout();
            if(!(AComplement -> State()) &&             //  True add cycle?
               ((B_Reg -> Get()) & BITB) != 0 ) {       //  Yes.  B Bit set?
                MultiplyDivideLastLatch = true;         //  Yes.  Set MDL.
                if((A_Reg -> Get().IsMinus()) !=        //  Different signs?
                   (B_Reg -> Get().IsMinus()) ) {
                    CPU -> SignLatch = true;            //  Yes.  Set minus latch
                }
                else {
                    CPU -> SignLatch = false;
                }
            }
        }
        else {                                          //  Not units...
            *STAR = *B_AR;                              //  Use BAR for readout
            Readout();
        }   //  End: Units

        if(SubScanRing -> State() == SUB_SCAN_U ||      //  Units or Body?
           SubScanRing -> State() == SUB_SCAN_B) {
            Adder(A_Reg -> Get(),AComplement -> State(),    //  Add A, B
                B_Reg -> Get(),BComplement -> State());
            Store( AssemblyChannel -> Select (         //  And store in *BAR
                AssemblyChannel -> AsmChannelWMB,
                AssemblyChannel -> AsmChannelZonesB,    //  Don't disturb zones
                false,
                AssemblyChannel -> AsmChannelSignNone,
                AssemblyChannel -> AsmChannelNumAdder) );
            Cycle();
            CarryIn -> Set(CarryOut -> State());        //  Use adder to set carry
            if(A_Reg -> Get().TestWM()) {               //  A Channel WM?
                next.subscan = SUB_SCAN_E;              //  Yes --> Extension
                return;
            }
            else {
                next.subscan = SUB_SCAN_B;              //  No.  Set Body
                next.cycle = CYCLE_A;                   //  Next cycle A
                next.subcycle = 1;                      //  Second kind
                return;
            }
        }   //  End: Units, Body

        if(SubScanRing -> State() == SUB_SCAN_E) {      //  Extension?

            //  Add a 0 (or 9 if complement is on) to B, and store
            Adder(BCD_0,AComplement -> State(),
                  B_Reg -> Get(),BComplement -> State());
            Store( AssemblyChannel -> Select (
                AssemblyChannel -> AsmChannelWMB,
                AssemblyChannel -> AsmChannelZonesB,    //  Should be no zones
                false,
                AssemblyChannel -> AsmChannelSignNone,
                AssemblyChannel -> AsmChannelNumAdder) );
            Cycle();
            CarryIn -> Set(CarryOut -> State());        //  Set carry from adder

            if(AComplement -> State()) {                //  Complement cycle?
                if(CarryOut -> State()) {               //  Adder carry ?
                    AComplement -> Set();               //  Yes.  Complement next
                    next.subscan = SUB_SCAN_MQ;         //  Set MQ latch
                    next.cycle = CYCLE_B;               //  B Cycle next
                    next.subcycle = 0;                  //  First kind.
                    return;
                }
                else {                                  //  No adder carry.
                    AComplement -> Reset();             //  True add next
                    next.subscan = SUB_SCAN_U;          //  Set Units latch
                    next.scan = SCAN_3;                 //  Set 3rd scan
                    next.cycle = CYCLE_A;               //  A Cycle next
                    next.subcycle = 1;                  //  Second kind
                    return;
                }
            }   //  End:    Complement

            if(MultiplyDivideLastLatch) {               //  Is MDL latch on?
                next.subscan = SUB_SCAN_MQ;             //  Set MQ Latch
                next.scan = SCAN_3;                     //  Set 3rd scan
                AComplement -> Reset();                 //  True add next
                next.cycle = CYCLE_B;                   //  B Cycle next
                next.subcycle = 0;                      //  First kind
                return;
            }
            else {                                      //  No MDL latch
                next.scan = SCAN_2;                     //  Set 2nd scan
                next.cycle = CYCLE_D;                   //  D Cycle next
                next.subcycle = 0;                      //  First kind
                return;
            }

        }   //  End: Extension

        assert(SubScanRing -> State() == SUB_SCAN_MQ);  //  Must be MQ

        if(AComplement -> State()) {                    //  Complement ?
            assert(CarryIn -> State());                 //  Carry s/b on too
            Adder(BCD_0,false,B_Reg -> Get(),BComplement -> State());   // ++B
            Store( AssemblyChannel -> Select (
                AssemblyChannel -> AsmChannelWMB,
                AssemblyChannel -> AsmChannelZonesB,    //  Should be no zones
                false,
                AssemblyChannel -> AsmChannelSignNone,
                AssemblyChannel -> AsmChannelNumAdder) );
            Cycle();
            if(CarryOut -> State()) {                   //  Adder carry?
                DivideOverflow -> Set();                //  Yes.  Set overflow!
                IRingControl = true;                    //  And be done.
                return;
            }
            CarryIn -> Set();                           //  No.  But set carry now
            next.subscan = SUB_SCAN_U;                  //  Set units latch
            next.scan = SCAN_3;                         //  Set 3rd scan
            AComplement -> Set();                       //  Complement Add next
            next.cycle = CYCLE_A;                       //  A cycle next
            next.subcycle = 1;                          //  Second kind.
            return;
        }   //  End: MQ, Complement

        assert(SubScanRing -> State() == SUB_SCAN_MQ && MultiplyDivideLastLatch);

        assert(!(AComplement -> State()));              //  Regen True Add
        Adder(BCD_9,false,B_Reg->Get(),BComplement -> State()); //  --B
        Store( AssemblyChannel -> Select (              //  Store sign in result
            AssemblyChannel -> AsmChannelWMB,
            AssemblyChannel -> AsmChannelZonesNone,
            false,
            AssemblyChannel -> AsmChannelSignLatch,
            AssemblyChannel -> AsmChannelNumAdder ) );
        Cycle();
        IRingControl = true;                            //  And be done.
        return;

    } //    End: B Cycle, subtype 0

    if(next.cycle == CYCLE_D) {
        *STAR = *D_AR;                                  //  D reg to RO B field
        Readout();
        Cycle();
        next.subscan = SUB_SCAN_U;                      //  Set units latch
        next.scan = SCAN_3;                             //  Set 3rd scan
        AComplement -> Set();                           //  Complement add
        CarryIn -> Set();                               //  Carry
        next.cycle = CYCLE_A;                           //  A Cycle next
        next.subcycle = 1;                              //  Second kind
        return;
    }

    if(next.cycle == CYCLE_A && next.subcycle == 1) {   //  A cycle
        if(SubScanRing -> State() == SUB_SCAN_U) {
            *STAR = *C_AR;                              //  Units RO via CAR
        }
        else {
            assert(SubScanRing -> State() == SUB_SCAN_B); //  Else s/b body
            *STAR = *A_AR;
        }
        Readout();
        Cycle();
        next.cycle = CYCLE_B;                           //  B Cycle next
        next.subcycle = 0;
        return;
    }

    assert(false);                                      //  OOPS
}

