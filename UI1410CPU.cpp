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

//
//	This Unit is the main part of the 1410 emulator
//

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
#include "UI1410CPU.h"
#include "UI1415IO.h"
#include "UI1415CE.h"
//---------------------------------------------------------------------------


#include <assert.h>

//	We have to predefine CPU, because the CPU object's children need a
//	pointer to the CPU to set up the various lists.

T1410CPU *CPU = 0;

//	Construct and initialize the CPU.

void Init1410()
{
	TRegister *TestA,*TestB;
    int a_temp,b_temp;
    char debug_msg[80];
	bool quit_comparator_test = false;

	FI1415CE -> Minimize();
	F1410Debug -> Minimize();

	DEBUG("Creating CPU Object");

	new T1410CPU;

    // The following sets are for testing the Computer Reset button...

	CPU -> IRing -> Set(I_RING_2);
    CPU -> SubScanRing -> Set(SUB_SCAN_MQ);
    CPU -> CycleRing -> Set(CYCLE_F);
    CPU -> CarryIn -> Set();
    CPU -> BComplement -> Set();
    CPU -> CompareBLTA -> Reset();
    CPU -> CompareBEQA -> Set();
    CPU -> InstructionCheck -> Set();

    //	Before we run more tests, display the CPU

    FI1415IO -> SetState(CONSOLE_IDLE);

    DEBUG("A Register is %d",CPU -> A_Reg -> Get().ToInt())

    CPU -> Display();

    //	Test Address register code

    DEBUG("STAR is initially %s",CPU -> STAR -> IsValid() ? "Valid" : "Invalid");
	DEBUG("Setting STAR from an int");
    CPU -> STAR -> Set(12345);
    DEBUG("STAR is now %s",CPU -> STAR -> IsValid() ? "Valid" : "Invalid");
	DEBUG("STAR (int) value is now %ld",CPU -> STAR -> Gate());
	DEBUG("Resetting STAR");
    CPU -> STAR -> Reset();
    DEBUG("STAR is now %s",CPU -> STAR -> IsValid() ? "Valid" : "Invalid");
    CPU -> STAR -> Set(1,1);
    CPU -> STAR -> Set(2,2);
    CPU -> STAR -> Set(3,3);
    CPU -> STAR -> Set(4,4);
    CPU -> STAR -> Set(5,5);
    DEBUG("STAR is now %s",CPU -> STAR -> IsValid() ? "Valid" : "Invalid");
	DEBUG("STAR (int) value is now %ld",CPU -> STAR->Gate());

    //	Test register assignment

    TestA = new TRegister(1,false);
    TestB = new TRegister(2,true);
    *TestB = *TestA;

	//	Test assembly channel

	DEBUG("Begin Assembly Channel Test")

    CPU -> A_Reg -> Set(0x15);
    CPU -> B_Reg -> Set(0xaa);
    CPU -> AChannel -> Select(CPU -> AChannel -> A_Channel_A);

    DEBUG("A Channel is now %x",CPU -> AChannel -> Select().ToInt())
    DEBUG("B Register is now %x",CPU -> B_Reg -> Get().ToInt())

    CPU -> AssemblyChannel -> Reset();
	DEBUG("Assembly Channel from A Channel yields %x",
		CPU -> AssemblyChannel -> Select(
    		CPU -> AssemblyChannel -> AsmChannelWMA,
            CPU -> AssemblyChannel -> AsmChannelZonesA, false,
            CPU -> AssemblyChannel -> AsmChannelSignNone,
			CPU -> AssemblyChannel -> AsmChannelNumA).ToInt() )

	CPU -> AssemblyChannel -> Reset();
	DEBUG("Assembly Channel from B Channel yields %x",
		CPU -> AssemblyChannel -> Select(
			CPU -> AssemblyChannel -> AsmChannelWMB,
			CPU -> AssemblyChannel -> AsmChannelZonesB, false,
			CPU -> AssemblyChannel -> AsmChannelSignNone,
			CPU -> AssemblyChannel -> AsmChannelNumB).ToInt() )

	DEBUG("Assembly Channel WM from B, rest from A yields %x",
		CPU -> AssemblyChannel -> Select(
    		CPU -> AssemblyChannel -> AsmChannelWMB,
            CPU -> AssemblyChannel -> AsmChannelZonesA, false,
            CPU -> AssemblyChannel -> AsmChannelSignNone,
			CPU -> AssemblyChannel -> AsmChannelNumA).ToInt() )

	DEBUG("Assembly Channel Normalized Sign from A yields %x",
		CPU -> AssemblyChannel -> Select(
    		CPU -> AssemblyChannel -> AsmChannelWMA,
            CPU -> AssemblyChannel -> AsmChannelZonesNone, false,
            CPU -> AssemblyChannel -> AsmChannelSignA,
			CPU -> AssemblyChannel -> AsmChannelNumA).ToInt() )

	DEBUG("Assembly Channel Normalized Sign from B yelds %x",
		CPU -> AssemblyChannel -> Select(
    		CPU -> AssemblyChannel -> AsmChannelWMB,
            CPU -> AssemblyChannel -> AsmChannelZonesNone, false,
            CPU -> AssemblyChannel -> AsmChannelSignB,
			CPU -> AssemblyChannel -> AsmChannelNumB).ToInt() )

	DEBUG("Assembly Channel Inverted Sign from A yields %x",
		CPU -> AssemblyChannel -> Select(
    		CPU -> AssemblyChannel -> AsmChannelWMA,
            CPU -> AssemblyChannel -> AsmChannelZonesNone, true,
            CPU -> AssemblyChannel -> AsmChannelSignA,
			CPU -> AssemblyChannel -> AsmChannelNumA).ToInt() )

	DEBUG("Assembly Channel Inverted Sign from B yelds %x",
		CPU -> AssemblyChannel -> Select(
    		CPU -> AssemblyChannel -> AsmChannelWMB,
            CPU -> AssemblyChannel -> AsmChannelZonesNone, true,
            CPU -> AssemblyChannel -> AsmChannelSignB,
            CPU -> AssemblyChannel -> AsmChannelNumB).ToInt() )

	//	Test the Adder

    DEBUG("Adder 'A' + 'F' (CarryIn Set) yields: %x ",
    	CPU -> Adder(BCD::BCDConvert('A'),false,BCD::BCDConvert('F'),false).ToInt() )
	CPU -> CarryIn -> Reset();

    DEBUG("Adder 'G' + 'G' (CarryIn Reset) yields: %x ",
    	CPU -> Adder(BCD::BCDConvert('G'),false,BCD::BCDConvert('G'),false).ToInt() )

	CPU -> CarryIn -> Set(CPU -> CarryOut -> State());

    //	Note, the following test result depends on the carry latch being set
    //	already.

    DEBUG("Adder -2 + 'F' (CarryIn Set) yields: %x ",
    	CPU -> Adder(BCD::BCDConvert('2'),true,BCD::BCDConvert('F'),false).ToInt() )
	CPU -> CarryIn -> Set(CPU -> CarryOut -> State());

    //  The loop below is an exhaustive comaprator test!

    /*

    TestComparator('b',' ',"0 0 1");        //  Current testing case

    TestComparator('-','L',"1 0 0");        //  NN < AN
    TestComparator(',','L',"1 0 0");        //  SC < AN

    TestComparator('<',',',"1 0 0");        //  SC : SC  B Zones higher (binary)
    TestComparator(',','<',"0 0 1");        //  SC : SC  A Zones higher (binary)
    TestComparator('$','*',"1 0 0");        //  SC : SC  Same zones, B < A
    TestComparator('*','$',"0 0 1");        //  SC : SC  Same zones, B > A
    TestComparator('*','*',"0 1 0");        //  SC : SC  B = A

    TestComparator('B','!',"1 0 0");        //  AN : AN  B Zones higher (binary)
    TestComparator('U','J',"0 0 1");        //  AN : AN  A Zones higher (binary)
    TestComparator('5','3',"0 0 1");        //  AN : AN  Same zones, Carry
    TestComparator('3','7',"1 0 0");        //  AN : AN  Same zones, No Carry
    TestComparator('6','6',"0 1 0");        //  AN : AN  Quinary 8, Binary 2
    TestComparator('8','9',"1 0 0");        //  AN : AN  Quinary 8, Binary 1
    TestComparator('5','5',"0 1 0");        //  AN : AN  Quinary 8, Binary 3

    TestComparator('3',' ',"0 0 1");        //  AN : NN
    TestComparator('3','/',"0 0 1");        //  AN : SC

    TestComparator(' ','+',"1 0 0");        //  NN : NN  B Zones higher (binary)
    TestComparator(' ',' ',"0 1 0");        //  NN : NN  Equal
    TestComparator('-','+',"0 0 1");        //  NN : NN  A Zones higher (binary)

    TestComparator('-','>',"1 0 0");        //  NN : SC  B Zones higher (binary)
    TestComparator('-','<',"0 0 1");        //  NN : SC  A Zones higher
    TestComparator(' ','>',"1 0 0");        //  NN : SC  Blank special case
    TestComparator('-','*',"0 0 1");        //  NN : SC  Same Zones

    TestComparator('<','-',"1 0 0");        //  SC : NN  B Zones higher (binary)
    TestComparator(',','+',"0 0 1");        //  SC : NN  A Zones higher
    TestComparator('>',' ',"0 0 1");        //  SC : NN  Blank special case
    TestComparator('<','+',"1 0 0");        //  SC : NN  Same Zones

    */

    //  Now, do an exhaustive comparator test.  What it does is check out
    //  the logic by comparing the results obtained from a simple collating
    //  sequence table.

    CPU -> SubScanRing -> Set(SUB_SCAN_U);

	DEBUG("Begin Comparator test...");

    for(b_temp = 0; !quit_comparator_test && b_temp < 64; ++b_temp) {
        for(a_temp = 0; !quit_comparator_test && a_temp < 64; ++a_temp) {
            CPU -> B_Reg -> Set(b_temp);
            CPU -> A_Reg -> Set(a_temp);
            CPU -> Comparator();
            if(CPU -> CompareBLTA -> State() &&
               collating_table[b_temp] >= collating_table[a_temp]) {
                sprintf(debug_msg,"Comparator failure %d : %d == %d : %d BLTA",
                    b_temp,a_temp,
                    CPU -> B_Reg -> Get().ToInt(),
                    CPU -> A_Reg -> Get().ToInt());
                DEBUG(debug_msg,0);
                quit_comparator_test = false;
                break;
            }
            else if(CPU -> CompareBGTA -> State() &&
                    collating_table[b_temp] <= collating_table[a_temp]) {
                sprintf(debug_msg,"Comparator failure %d : %d == %d : %d BGTA",
                    b_temp,a_temp,
                    CPU -> B_Reg -> Get().ToInt(),
                    CPU -> A_Reg -> Get().ToInt());
                DEBUG(debug_msg,0);
                quit_comparator_test = false;
                break;
            }
            else if(CPU -> CompareBEQA -> State() && b_temp != a_temp) {
                sprintf(debug_msg,"Comparator failure %d : %d BEQA",
                    b_temp,a_temp);
                DEBUG(debug_msg,0);
                quit_comparator_test = false;
                break;
            }
        }
    }

    DEBUG("End Comparator Test...");

	//	Redisplay (in case a test changed indicators)

    CPU -> Display();

    //	Set breakpoint to examine values here

	DEBUG("Waiting for User Interface")


}

void TestComparator(char b,char a,char *expect)
{
    char debug_msg[80];
    //  Test comparator

    CPU -> SubScanRing -> Set(SUB_SCAN_U);      //  So we can get equal
    CPU -> B_Reg -> Set(BCD::BCDConvert(b));
    CPU -> A_Reg -> Set(BCD::BCDConvert(a));
    CPU -> Comparator();
    sprintf(debug_msg,"Comparator: %c:%c %d %d %d expect %s",
        b,a,CPU -> CompareBLTA -> State(),CPU -> CompareBEQA -> State(),
        CPU -> CompareBGTA -> State(),expect);
    DEBUG(debug_msg,0);
}

