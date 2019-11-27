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

//	This Unit provides the functionality behind some of my private
//	types for the 1410 emulator

//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <assert.h>
#include <dir.h>
#include <stdlib.h>
#include <stdio.h>

#include "ubcd.h"
#include "UI1410CPUT.h"
#include "UIHOPPER.h"
#include "UI1410CHANNEL.h"
#include "UI14101.h"
#include "UI1415L.h"
#include "UI1415IO.h"
#include "UI1415CE.h"
#include "UITAPEUNIT.h"
#include "UITAPETAU.h"
#include "UI729TAPE.h"
#include "UIPRINTER.h"
#include "UI1403.h"
#include "UIREADER.h"
#include "UIPUNCH.h"
#include "UI1402.h"
#include "UI1410PWR.h"
#include "UI1410DEBUG.h"
#include "UI1410CPU.h"
#include "UI1410INST.h"


//---------------------------------------------------------------------------


//	Declarations for Borland VCL controls

//	#include <vcl\Classes.hpp>  (From Borland C++ 5)
#include <vcl.Controls.hpp>
#include <vcl.StdCtrls.hpp>

//	Table of execute routines.  These have to be closures to inherit
//	the object pointer (CPU).  Closures have to be initialized at runtime.
//	We do it in the contructor.

TInstructionExecuteRoutine InstructionExecuteRoutine[64];


//	Core pre-load (gives this virtual 1410 a 7010-like load capability
//	Just hit Computer Reset then Start to boot from tape

char *core_load_chars = "AL%B000012$N..";
char core_load_wm[] = { 0,1,0,0,0,0,0,0,0,0,0,1,1,1 };

//	Tables which pre-multiply integers by decimal numbers, for efficiency.

long ten_thousands[] = {
	0,10000,20000,30000,40000,50000,60000,70000,80000,90000
};

long thousands[] = {
	0,1000,2000,3000,4000,5000,6000,7000,8000,9000
};

long hundreds[] = {
	0,100,200,300,400,500,600,700,800,900
};

long tens[] = { 0,10,20,30,40,50,60,70,80,90 };

//  Numeric equivalencies for low 4 bits.  Used mostly by arithmetic
//  Basically, for numbers > 9, the 8 bit gets ignored.

unsigned char numeric_value[] = { 0,1,2,3,4,5,6,7,8,9,0,3,4,5,6,7 };

//	Storage Scan modification values
//	Correspond to the values in the enum StorageScan

long scan_mod[] = { 0, -1, +1, -1 };

//	Tables for processing signs (shifted right 4 bits!)

//	Table to take a sign bit configuration (BA) and normalize it.
//	Plus sign is normalized to BA, Minus sign is normalized to just A

unsigned char sign_normalize_table[] = { 3, 3, 2, 3 };

//	Same thing, but inverts the sign in the process

unsigned char sign_complement_table[] = { 2, 2, 3, 2 };

//	Same thing, but indictes negative by 1

unsigned char sign_negative_table[] = { 0, 0, 1, 0 };

//  Busy Device List initialization:

TBusyDevice *TBusyDevice::FirstBusyDevice = NULL;

//	Implementation of TCpuObject (Abstract Base Class)

//	Everything in the CPU is on the reset list.
//	Everything is reset by Computer Reset
//	NOT Everything is reset by Program Reset!

TCpuObject::TCpuObject()
{
	NextReset = CPU -> ResetList;
    CPU -> ResetList = this;
}

//  Implementation of TBusyDevice - busy devices list


//  Constructor -- adds itself to the list

TBusyDevice::TBusyDevice() {
    NextBusyDevice = FirstBusyDevice;
    FirstBusyDevice = this;
    BusyTime = 0;
}


//  Class method to make a pass thru the busy list

void TBusyDevice::BusyPass() {
    TBusyDevice *p;
    for(p = FirstBusyDevice; p != NULL; p = p -> NextBusyDevice) {
        if(p -> BusyTime != 0) {
            --(p -> BusyTime);
        }
    }
}

//  Class method to reset all of the busy times to 0

void TBusyDevice::Reset() {
    TBusyDevice *p;
    for(p = FirstBusyDevice; p != NULL; p = p -> NextBusyDevice) {
        p -> BusyTime = 0;
    }
}


//	Implementation of TDisplayObject (Abstract Base Class)

//	What is special about these objects is that they are on the display list.

TDisplayObject::TDisplayObject()
{
	NextDisplay = CPU -> DisplayList;
    CPU -> DisplayList = this;
}



//	Implementation of TDisplayLatch (Display Latch Base Class)

//	The constructor initializes the latch and sets the pointer to a lamp.
//	A pointer to a lamp is required.

//	Default is to be reset by a Program Reset, but this can be overridden
//	when the constructor is called, if necessary.

TDisplayLatch::TDisplayLatch(TLabel *l)
{
	state = false;
    doprogramreset = true;
    lamp = l;
}

//	The second constructor does the same thing, but is passed a variable
//	which indicates whether or not the latch should be reset by Program
//	Reset.

TDisplayLatch::TDisplayLatch(TLabel *l, bool progreset)
{
	state = false;
    doprogramreset = progreset;
    lamp = l;
}

//	All Displayable Latches are reset on a COMPUTER RESET

void TDisplayLatch::OnComputerReset()
{
	Reset();
}

//	Whether or not a displayable latch is reset by program reset depends
//	on the latch.

void TDisplayLatch::OnProgramReset()
{
	if(doprogramreset) {
    	Reset();
    }
}

//	Routine to set a latch and set the CPU Stop Latch at the same time
//	Typically used for error latches.

void TDisplayLatch::SetStop(char *msg) {
    	state = true;
        CPU -> StopLatch = true;
        if(msg != 0) {
        	F1410Debug -> DebugOut(msg);
        }
}

//	When the Display routine is called, it sets or resets the lamp,
//	depending on the current state.

//  Change 2/1/99: Repaint only if state changes (save time)

void TDisplayLatch::Display()
{
    if(lamp -> Enabled != state) {
        lamp -> Enabled = state;
        lamp -> Repaint();
    }
}

//	On a lamp test, light all the lamp.
//	On reset of a lamp test, display the current state.

void TDisplayLatch::LampTest(bool b)
{
	lamp -> Enabled = (b ? true : state);
    lamp -> Repaint();
}



//	Implementation of TRingCounter (Ring Counter Class)

//	The constructor sets the max state of the ring, and resets the ring
//	It also allocates an array of pointers to the lamps.  The creator must
//	fill in that array, however.  Initially, the lamp pointers are empty,
//	(which means no lamp is attached to that state).

TRingCounter::TRingCounter(char n)
{
	int i;

	state = 0;
	max = n;
    lastlamp = 0;
    lastlampCE = 0;
    lampsCE = 0;

    lamps = new TLabel*[n];
    for(i=0; i < n; ++i) {
    	lamps[i] = 0;
    }
}

//	The destructor frees up the array of lamps.  Probably will never use.

__fastcall TRingCounter::~TRingCounter()
{
	delete[] lamps;
    if(lampsCE != 0) {
    	delete[] lampsCE;
    }
}

//	All Ring counters are reset on PROGRAM and COMPUTER RESET

void TRingCounter::OnComputerReset()
{
	state = 0;
}

void TRingCounter::OnProgramReset()
{
	state = 0;
}

//	When the Display routine is called, it resets the lamp corresponding
//	to the state when it last displayed, and then displays the current state
//	If there is no lamp associated with a state (lamp pointer is null),
//	then don't display any lamp.

void TRingCounter::Display()
{
    if(lastlamp == lamps[state] &&
        (lampsCE == 0 || lastlampCE == lampsCE[state])) {
        return;
    }

	if(lastlamp) {
    	lastlamp -> Enabled = false;
        lastlamp -> Repaint();
    }
    if(lamps[state] == 0) {
    	lastlamp = 0;
    	return;
    }
    lamps[state] -> Enabled = true;
    lamps[state] -> Repaint();
    lastlamp = lamps[state];

    if(lampsCE != 0) {
    	if(lastlampCE) {
        	lastlampCE -> Enabled = false;
            lastlampCE -> Repaint();
        }
        if(lampsCE[state] == 0) {
        	lastlampCE = 0;
            return;
        }
        lampsCE[state] -> Enabled = true;
        lampsCE[state] -> Repaint();
        lastlampCE = lampsCE[state];
    }
}

//	On a lamp test, light all the associated lamps.
//	On reset of a lamp test, clear them all, then display the current state.

void TRingCounter::LampTest(bool b)
{
	int i;

   	for(i = 0; i < max; ++i) {
    	if(lamps[i] != 0) {
	       	lamps[i] -> Enabled = b;
    	    lamps[i] -> Repaint();
        }
        if(lampsCE != 0 && lampsCE[i] != 0) {
        	lampsCE[i] -> Enabled = b;
            lampsCE[i] -> Repaint();
        }
    }
    if(!b) {
    	lastlamp = 0;
        lastlampCE = 0;
    	this -> Display();
    }
}

//	Next advances to the next state (or back to the start if appropriate)
//	Returns current state.  This is only useful for true Ring counters.

char TRingCounter::Next()
{
	if(++state >= max) {
    	state = 0;
    }
    return(state);
}



//	Implementation of Simple Registers

//	Assignment:  Just assign the value, NOT the things from TCpuObject!!
//	(Those need to stay unchanged as they should be invariant once the
//	register is created: it's position on the reset list and whether or
//	not it is affected by Program Reset, for example.

void TRegister::operator=(TRegister &source)
{
	value = source.value;
}

//	Display and LampTest.  These only do anything if the display variables
//	are actually set.

void TRegister::Display()
{
	int bitmask = 0x1;
    int i;

    for(i=0; i < 8; ++i) {
    	if(lamps != 0 && lamps[i] != 0) {
        	lamps[i] -> Enabled = ((value.ToInt() & bitmask) != 0);
            lamps[i] -> Repaint();
        }
        bitmask <<= 1;
    }

    if(lampER != 0) {
    	lampER -> Enabled = (!value.CheckParity());
        lampER -> Repaint();
    }
}

void TRegister::LampTest(bool b)
{
	int i;

	if(b) {
    	for(i=0; i < 8; ++i) {
        	if(lamps != 0 && lamps[i] != 0) {
            	lamps[i] -> Enabled = true;
                lamps[i] -> Repaint();
            }
        }
        if(lampER != 0) {
        	lampER -> Enabled = true;
            lampER -> Repaint();
        }
    }
    else {
    	Display();
    }
}



//	Implementation of Address Registers

//	The constructor just initializes things so that we know that the
//	address register contains an invalid value.

TAddressRegister::TAddressRegister()
{
	i_valid = false;
    i_value = 0;
    d_valid = false;
    set[0] = set[1] = set[2] = set[3] = set[4] = false;
    DoesProgramReset = false;
}

//	A function to determine whether or not the address register contains
//	a valid value.

bool TAddressRegister::IsValid()
{
	if(i_valid || d_valid ||
    	(set[0] && set[1] && set[2] && set[3] && set[4]) ) {
        return(true);
    }
    return(false);
}

//	A routine to reset an address register: to binary 0.  Note that this
//	means it is invalid (i.e. will cause an Address Exit Check if you try
//	and actually use the value except to print it out on the console.

void TAddressRegister::Reset()
{
	TWOOF5 zero;

	i_valid = false;
    d_valid = false;
    set[0] = set[1] = set[2] = set[3] = set[4] = false;
    digits[0] = digits[1] = digits[2] = digits[3] = digits[4] = zero;
    i_value = 0;
}

//	A routine to get the value of an address register.  Note that if
//  the value is invalid, the result is an Address Exit Check.

long TAddressRegister::Gate()
{
    if(i_valid) {
    	return(i_value);
    }
	else if(IsValid()) {
    	i_value = ten_thousands[digits[0].ToInt()] +
        	thousands[digits[1].ToInt()] +
            hundreds[digits[2].ToInt()] +
            tens[digits[3].ToInt()] + digits[4].ToInt();
        i_valid = true;
        d_valid = true;
        return(i_value);
    }
    else {
		CPU -> AddressExitCheck ->
        	SetStop("Address Exit Check during Gate()");
        return(-1);
    }
}

//	Get a single character from an address register.  This is valid even
//	if the value in the register is invalid.

BCD TAddressRegister::GateBCD(int i)
{
	if(d_valid) {
    	if(digits[i-1].ToInt() < 0) {
        	CPU -> AddressExitCheck ->
            	SetStop("Address Exit Check during GateBCD(#1)");
            F1415L -> DisplayAddrChannel(digits[i-1],true);
            return(0);
        }
    	return(digits[i-1].ToBCD());
    }
    else if(i_valid) {
    	digits[4] = i_value % 10;
        digits[3] = (i_value % 100)/10;
        digits[2] = (i_value % 1000)/100;
        digits[1] = (i_value % 10000)/1000;
        digits[0] = i_value /10000;
        d_valid = true;
        set[0] = set[1] = set[2] = set[3] = set[4] = true;
       	if(digits[i-1].ToInt() < 0) {
        	CPU -> AddressExitCheck ->
            	SetStop("Address Exit Check during GateBCD(#2)");
            return(0);
        }
        return(digits[i-1].ToBCD());
    }
    else {

    	//	Register is not set, but it is still valid to read out a single
        //	digit.  This should not cause an error.

/*    	CPU -> AddressExitCheck ->
        	SetStop("Address Exit Check during GateBCD(Contents not valid)");
        F1415L -> DisplayAddrChannel(TWOOF5(0),true);
    	return(BCD(0));
*/

		if(set[i-1] && digits[i-1].ToInt() >= 0) {
        	return(digits[i-1].ToBCD());
        }
        else {
        	return(0);
        }
    }
}

//	A routine to set a single digit of an address register.
//	This effectively implements the address channel.

void TAddressRegister::Set(TWOOF5 digit,int i)
{
	if(digit.ToInt() == -1) {
    	CPU -> AddressChannelCheck -> 
        	SetStop("Address Channel Check while setting address register");
        F1415L -> DisplayAddrChannel(digit,true);
    }

	digits[i-1] = digit;
    set[i-1] = true;
    if(i == 5 && set[0] && set[1] && set[2] && set[3]) {
    	d_valid = true;
    }
}

//	A routine to set an address register from a binary value.  Should
//	really only be used to reset IAR to 1 during a reset operation.

void TAddressRegister::Set(long i)
{
	d_valid = false;
    set[0] = set[1] = set[2] = set[3] = set[4] = false;
    i_valid = true;
    i_value = i;
}

//	Assignment:  Just assign the value, NOT the things from TCpuObject!!
//	(Those need to stay unchanged as they should be invariant once the
//	register is created: it's position on the reset list and whether or
//	not it is affected by Program Reset, for example.

//	Note that an attempt to assign from an invalid register does do the
//	set, but also sets the Address Exit Check error in the CPU

void TAddressRegister::operator=(TAddressRegister &source)
{
	int i;

    i_value = source.i_value;
    i_valid = source.i_valid;
    d_valid = source.d_valid;
    if(!source.IsValid()) {
    	CPU -> AddressExitCheck ->
        	SetStop("Address Exit Check assigning - from register invalid");
    }
    for(i=0; i < 5; ++i) {
    	digits[i] = source.digits[i];
        set[i] = source.set[i];
    }
}



//	Implementation of A Channel

//	Constructor.  There is only 1 a channel, so it sets pointers to lamps
//	as well as initializing

TAChannel::TAChannel()
{
	AChannelSelect = A_Channel_None;

    lamps[0] = F1415L -> Light_CE_ACh_A;
    lamps[1] = F1415L -> Light_CE_ACh_d;
    lamps[2] = F1415L -> Light_CE_ACh_E;
    lamps[3] = F1415L -> Light_CE_ACh_F;
}

//	Routine to return what is currently selected on A Channel

BCD TAChannel::Select()
{
	return Select(AChannelSelect);
}

//	Routine to set what the A Channel will select.  It also returns that
//	selected value, for convenience.

BCD TAChannel::Select(enum TAChannelSelect sel)
{
	AChannelSelect = sel;

    switch(AChannelSelect) {

    case A_Channel_None:
    	return(0);
    case A_Channel_A:
    	return(CPU -> A_Reg -> Get());
    case A_Channel_Mod:
    	return(CPU -> Op_Mod_Reg -> Get());
    case A_Channel_E:
    	return(CPU -> Channel[CHANNEL1] -> ChR2 -> Get());
    case A_Channel_F:
    	if(MAXCHANNEL > 1) {
        	return(CPU -> Channel[CHANNEL2] -> ChR2 -> Get());
        }
        else {
        	CPU -> ACharacterSelectCheck ->
            	SetStop("A Character Select Check: Channel 2, only 1 configured");
            return(0);
        }
    default:
    	CPU -> ACharacterSelectCheck ->
        	SetStop("A Character Select Check: Invalid selection");
        return(0);
    }
}

//	Routine to display the current A Channel Select status

void TAChannel::Display()
{
	int i;

    for(i=0; i < 4; ++i) {
    	lamps[i] -> Enabled = false;
    }

	switch(AChannelSelect) {

    case A_Channel_None:
    	break;
	case A_Channel_A:
    	lamps[0] -> Enabled = true;
        break;
    case A_Channel_Mod:
    	lamps[1] -> Enabled = true;
        break;
    case A_Channel_E:
    	lamps[2] -> Enabled = true;
        break;
    case A_Channel_F:
    	if(MAXCHANNEL > 1) {
        	lamps[3] -> Enabled = true;
        }
        break;
    }

    for(i = 0; i < 4; ++i) {
    	lamps[i] -> Repaint();
    }
}

//	Lamp Test

void TAChannel::LampTest(bool b)
{
	int i;

    if(b) {
    	for(i = 0; i < 4; ++i) {
        	lamps[i] -> Enabled = true;
            lamps[i] -> Repaint();
        }
    }
    else {
    	Display();
    }
}



//	Implementation of Assembly Channel

//	Private tables for assembly channel.  These are bitmasks.  In general,
//	these arrays have elements in the order of their respective enums.
//	Each element has 3 entries, one byte each for the A Channel, the B Channel/
//	Register and the Adder

struct AssmMask {
	int AChannelMask;
    int BChannelMask;
    int AdderMask;
};

static struct AssmMask AssmWMMask[] = {
	{ 0x00, 0x00, 0x00 },		//	No WM
    { 0x00, 0x80, 0x00 },		//	B WM
    { 0x80, 0x00, 0x00 },		//	A WM
};

static struct AssmMask AssmZonesMask[] = {
	{ 0x00, 0x00, 0x00 },		//	No Zones
    { 0x00, 0x30, 0x00 },		//	B Zones
    { 0x30, 0x00, 0x00 }		//	A Zones
};

static struct AssmMask AssmNumMask[] = {
	{ 0x00, 0x00, 0x00 },		//	No Numerics
    { 0x00, 0x0f, 0x00 },		//	B Numerics
    { 0x0f, 0x00, 0x00 },		//	A Numerics
    { 0x00, 0x00, 0x0f },		//	Adder Numerics
    { 0x00, 0x00, 0x00 }        //  Numeric Zero
};

static struct AssmMask AssmSignMask[] = {
	{ 0x00, 0x00, 0x00 },		//	No Sign
    { 0x00, 0x30, 0x00 },		//	B Sign
    { 0x30, 0x00, 0x00 },		//	A Sign
    { 0x00, 0x00, 0x30 }		//	Sign Latch
};

//	Constructor

TAssemblyChannel::TAssemblyChannel()
{
	AssmLamps[0] = F1415L -> Light_CE_Assm_1;
    AssmLamps[1] = F1415L -> Light_CE_Assm_2;
    AssmLamps[2] = F1415L -> Light_CE_Assm_4;
    AssmLamps[3] = F1415L -> Light_CE_Assm_8;
    AssmLamps[4] = F1415L -> Light_CE_Assm_A;
    AssmLamps[5] = F1415L -> Light_CE_Assm_B;
    AssmLamps[6] = F1415L -> Light_CE_Assm_C;
    AssmLamps[7] = F1415L -> Light_CE_Assm_WM;

	AssmComplLamps[0] = F1415L -> Light_CE_Assm_N1;
    AssmComplLamps[1] = F1415L -> Light_CE_Assm_N2;
    AssmComplLamps[2] = F1415L -> Light_CE_Assm_N4;
    AssmComplLamps[3] = F1415L -> Light_CE_Assm_N8;
    AssmComplLamps[4] = F1415L -> Light_CE_Assm_NA;
    AssmComplLamps[5] = F1415L -> Light_CE_Assm_NB;
    AssmComplLamps[6] = F1415L -> Light_CE_Assm_NC;
    AssmComplLamps[7] = F1415L -> Light_CE_Assm_NWM;

    AssmERLamp = F1415L -> Light_CE_Assm_ER;

    Reset();
}

//	Display and Lamptest have to display both positive and complement.
//	Otherwise, very similar to a TRegister

void TAssemblyChannel::Display()
{
	int bitmask = 0x1;
    int i;

    for(i=0; i < 8; ++i) {
    	AssmLamps[i] -> Enabled = ((value.ToInt() & bitmask) != 0);
        AssmComplLamps[i] -> Enabled = !AssmLamps[i] -> Enabled;
    	AssmLamps[i] -> Repaint();
        AssmComplLamps[i] -> Repaint();
        bitmask <<= 1;
    }

    AssmERLamp -> Enabled = (!valid || !value.CheckParity());
    AssmERLamp -> Repaint();
}

void TAssemblyChannel::LampTest(bool b)
{
	int i;

	if(b) {
    	for(i=0; i < 8; ++i) {
        	AssmLamps[i] -> Enabled = true;
            AssmComplLamps[i] -> Enabled = true;
            AssmLamps[i] -> Repaint();
            AssmComplLamps[i] -> Repaint();
        }
        AssmERLamp -> Enabled = true;
        AssmERLamp -> Repaint();
    }
    else {
    	Display();
    }
}

//	Select sets the Assembly Channel selection variables and then
//	returns the result

BCD TAssemblyChannel::Select(
    enum TAsmChannelWMSelect WMSelect,
    enum TAsmChannelZonesSelect ZoneSelect,
    bool InvertSign,
    enum TAsmChannelSignSelect SignSelect,
    enum TAsmChannelNumericSelect NumSelect)
{
    BCD Zones, WM, Numerics;

    //	Remember the selection, for next time

    AsmChannelWMSelect = WMSelect;
    AsmChannelZonesSelect = ZoneSelect;
    AsmChannelInvertSign = InvertSign;
    AsmChannelSignSelect = SignSelect;
    AsmChannelNumericSelect = NumSelect;
    AsmChannelCharSet = false;

    //	Can't select both sign and zones!

    if(AsmChannelZonesSelect != AsmChannelZonesNone &&
       AsmChannelSignSelect != AsmChannelSignNone) {
       CPU -> AssemblyChannelCheck ->
       		SetStop("Assembly Channel Check: Selected both Sign and Zones");
       return(0);
    }

    if(AsmChannelWMSelect == AsmChannelWMSet) {
        WM = BITWM;
    }
    else {
    	WM =
           	(CPU -> AChannel -> Select() & AssmWMMask[AsmChannelWMSelect].AChannelMask) |
    	    (CPU -> B_Reg -> Get() & AssmWMMask[AsmChannelWMSelect].BChannelMask);
    }

    Numerics =
    	(CPU -> AChannel -> Select() & AssmNumMask[AsmChannelNumericSelect].AChannelMask) |
    	(CPU -> B_Reg -> Get() & AssmNumMask[AsmChannelNumericSelect].BChannelMask) |
        (CPU -> AdderResult & AssmNumMask[AsmChannelNumericSelect].AdderMask);

    if(AsmChannelNumericSelect == AsmChannelNumZero) {
        Numerics = (BCD_0) & BIT_NUM;
    }

    if(AsmChannelSignSelect != AsmChannelSignNone) {
    	if(AsmChannelSignSelect == AsmChannelSignLatch) {
        	Zones = (CPU -> SignLatch ? 0x20 : 0x30);   // True ==> Minus
        }
        else {
        	Zones =
            	(CPU -> AChannel -> Select() & AssmSignMask[AsmChannelSignSelect].AChannelMask) |
                (CPU -> B_Reg -> Get() & AssmSignMask[AsmChannelSignSelect].BChannelMask);
        }
        Zones = Zones >> 4;		//	Prepare to normalize
        Zones = BCD( (AsmChannelInvertSign ?
        	sign_complement_table[Zones.ToInt()] :
            sign_normalize_table[Zones.ToInt()]) << 4 );
    }
	else {
    	Zones =
        	(CPU -> AChannel -> Select() & AssmZonesMask[AsmChannelZonesSelect].AChannelMask) |
            (CPU -> B_Reg -> Get() & AssmZonesMask[AsmChannelZonesSelect].BChannelMask);
    }

    value = WM | Zones | Numerics;
    value.SetOddParity();
    valid = true;
    return(value);
}

//	Same thing, only we use the existing values.  If the value is already
//	valid, we can take a shortcut.

BCD TAssemblyChannel::Select()
{
	if(valid) {
    	return(value);
    }

    return(Select(AsmChannelWMSelect,AsmChannelZonesSelect,
    	AsmChannelInvertSign,AsmChannelSignSelect,AsmChannelNumericSelect));
}

//	Get really does the same thing as select, except that it flags an
//	error if there is not a valid value already.

BCD TAssemblyChannel::Get()
{
	if(valid) {
    	return(value);
    }

    CPU -> AssemblyChannelCheck ->
    	SetStop("Assembly Channel Check: Value not valid at this time");
    return(0);
}

//	Method to reset the assembly channel

void TAssemblyChannel::Reset() {
    	value = BITC;
        valid = true;
        AsmChannelWMSelect = AsmChannelWMNone;
        AsmChannelZonesSelect = AsmChannelZonesNone;
        AsmChannelInvertSign = false;
        AsmChannelSignSelect = AsmChannelSignNone;
        AsmChannelNumericSelect = AsmChannelNumNone;
        AsmChannelCharSet = false;
    }

//	Common operation: Gate A Channel to Assembly Channel

BCD TAssemblyChannel::GateAChannelToAssembly(BCD v) {
    	AsmChannelWMSelect = AsmChannelWMA;
        AsmChannelZonesSelect = AsmChannelZonesA;
        AsmChannelInvertSign = false;
        AsmChannelSignSelect = AsmChannelSignNone;
        AsmChannelNumericSelect = AsmChannelNumA;
        AsmChannelCharSet = false;
        value = v;		// Usually this will be an AChannel call return val
        valid = true;
        return(v);
    }

BCD TAssemblyChannel::GateBRegToAssembly(BCD v) {
        AsmChannelWMSelect = AsmChannelWMB;
        AsmChannelZonesSelect = AsmChannelZonesB;
        AsmChannelInvertSign = false;
        AsmChannelSignSelect = AsmChannelSignNone;
        AsmChannelNumericSelect = AsmChannelNumB;
        AsmChannelCharSet = false;
        value = v;		// Usually this will be a B_Reg call return val
        valid = true;
        return(v);
    }



//	CPU object constructor.  Essentially this method "wires" the 1410.

T1410CPU::T1410CPU()
{
	long i;
    TLabel *tmper,**tmpl;

	CPU = this;

	//	Build the op code execute table

    for(i=0; i < 64; ++i) {
    	InstructionExecuteRoutine[i] = this -> InstructionExecuteInvalid;
    }

    InstructionExecuteRoutine[OP_ADD] = InstructionArith;
    InstructionExecuteRoutine[OP_SUBTRACT] = InstructionArith;
    InstructionExecuteRoutine[OP_ZERO_ADD] = InstructionZeroArith;
    InstructionExecuteRoutine[OP_ZERO_SUB] = InstructionZeroArith;
    InstructionExecuteRoutine[OP_MULTIPLY] = InstructionMultiply;
    InstructionExecuteRoutine[OP_DIVIDE] = InstructionDivide;
    InstructionExecuteRoutine[OP_MOVE] = InstructionMove;
    InstructionExecuteRoutine[OP_MCS] = InstructionMoveSuppressZeros;
    InstructionExecuteRoutine[OP_EDIT] = InstructionEdit;
    InstructionExecuteRoutine[OP_COMPARE] = InstructionCompare;
    InstructionExecuteRoutine[OP_TABLESEARCH] = InstructionTableLookup;
    InstructionExecuteRoutine[OP_BRANCHCOND] = InstructionBranchCond;
    InstructionExecuteRoutine[OP_BRANCH_CH_1] =
        InstructionBranchChannel1;
    InstructionExecuteRoutine[OP_BRANCH_CH_2] =
        InstructionBranchChannel2;
    InstructionExecuteRoutine[OP_BRANCH_CE] = InstructionBranchCharEqual;
    InstructionExecuteRoutine[OP_BRANCH_BE] = InstructionBranchBitEqual;
    InstructionExecuteRoutine[OP_BRANCH_ZWM] = InstructionBranchZoneWMEqual;
    InstructionExecuteRoutine[OP_STORE_AR] = InstructionStoreAddressRegister;
    InstructionExecuteRoutine[OP_SETWM] = InstructionDoWordMark;
    InstructionExecuteRoutine[OP_CLEARWM] = InstructionDoWordMark;
    InstructionExecuteRoutine[OP_CLEAR_STORAGE] = InstructionClearStorage;
    InstructionExecuteRoutine[OP_HALT] = InstructionHalt;
    InstructionExecuteRoutine[OP_IO_MOVE] = InstructionIO;
    InstructionExecuteRoutine[OP_IO_LOAD] = InstructionIO;
    InstructionExecuteRoutine[OP_IO_UNIT] = InstructionUnitControl;
    InstructionExecuteRoutine[OP_BRANCH_PR] = InstructionPriorityBranch;
    InstructionExecuteRoutine[OP_IO_CARRIAGE_1] = InstructionCarriageControl;
    InstructionExecuteRoutine[OP_IO_CARRIAGE_2] = InstructionCarriageControl;
    InstructionExecuteRoutine[OP_IO_SSF_1] = InstructionSelectStacker;
    InstructionExecuteRoutine[OP_IO_SSF_2] = InstructionSelectStacker;


    //	Clear out the lists

	DisplayList = 0;
    ResetList = 0;

    //	Set switches to initial states

    Mode = MODE_RUN;
    AddressEntry = ADDR_ENTRY_I;
    StorageScan = SSCAN_OFF;
    CycleControl = CYCLE_OFF;
    CheckControl = CHECK_STOP;
    DiskWrInhibit = false;
    AsteriskInsert = true;
    InhibitPrintOut = false;
    BitSwitches = BCD(0);

    //	Build the various displayable components of the CPU

	IRing = new TRingCounter(13);
    assert(F1415L -> Light_I_OP != 0);
    IRing -> lamps[0] = F1415L -> Light_I_OP;
    IRing -> lamps[1] = F1415L -> Light_I_1;
    IRing -> lamps[2] = F1415L -> Light_I_2;
    IRing -> lamps[3] = F1415L -> Light_I_3;
    IRing -> lamps[4] = F1415L -> Light_I_4;
    IRing -> lamps[5] = F1415L -> Light_I_5;
    IRing -> lamps[6] = F1415L -> Light_I_6;
    IRing -> lamps[7] = F1415L -> Light_I_7;
    IRing -> lamps[8] = F1415L -> Light_I_8;
    IRing -> lamps[9] = F1415L -> Light_I_9;
    IRing -> lamps[10] = F1415L -> Light_I_10;
    IRing -> lamps[11] = F1415L -> Light_I_11;
    IRing -> lamps[12] = F1415L -> Light_I_12;

    ARing = new TRingCounter(6);
    ARing -> lamps[0] = F1415L -> Light_A_1;
    ARing -> lamps[1] = F1415L -> Light_A_2;
    ARing -> lamps[2] = F1415L -> Light_A_3;
    ARing -> lamps[3] = F1415L -> Light_A_4;
    ARing -> lamps[4] = F1415L -> Light_A_5;
    ARing -> lamps[5] = F1415L -> Light_A_6;

    ClockRing = new TRingCounter(10);
    ClockRing -> lamps[0] = F1415L -> Light_Clk_A;
    ClockRing -> lamps[1] = F1415L -> Light_Clk_B;
    ClockRing -> lamps[2] = F1415L -> Light_Clk_C;
    ClockRing -> lamps[3] = F1415L -> Light_Clk_D;
    ClockRing -> lamps[4] = F1415L -> Light_Clk_E;
    ClockRing -> lamps[5] = F1415L -> Light_Clk_F;
    ClockRing -> lamps[6] = F1415L -> Light_Clk_G;
    ClockRing -> lamps[7] = F1415L -> Light_Clk_H;
    ClockRing -> lamps[8] = F1415L -> Light_Clk_J;
    ClockRing -> lamps[9] = F1415L -> Light_Clk_K;

    ScanRing = new TRingCounter(4);
    ScanRing -> lamps[0] = F1415L -> Light_Scan_N;
    ScanRing -> lamps[1] = F1415L -> Light_Scan_1;
    ScanRing -> lamps[2] = F1415L -> Light_Scan_2;
    ScanRing -> lamps[3] = F1415L -> Light_Scan_3;

    SubScanRing = new TRingCounter(5);
    //	NOTE:  State 0 is "OFF" - no flip flops set
    SubScanRing -> lamps[1] = F1415L -> Light_Sub_Scan_U;
    SubScanRing -> lamps[2] = F1415L -> Light_Sub_Scan_B;
    SubScanRing -> lamps[3] = F1415L -> Light_Sub_Scan_E;
    SubScanRing -> lamps[4] = F1415L -> Light_Sub_Scan_MQ;

    CycleRing = new TRingCounter(8);
    CycleRing -> lamps[0] = F1415L -> Light_Cycle_A;
    CycleRing -> lamps[1] = F1415L -> Light_Cycle_B;
    CycleRing -> lamps[2] = F1415L -> Light_Cycle_C;
    CycleRing -> lamps[3] = F1415L -> Light_Cycle_D;
    CycleRing -> lamps[4] = F1415L -> Light_Cycle_E;
    CycleRing -> lamps[5] = F1415L -> Light_Cycle_F;
    CycleRing -> lamps[6] = F1415L -> Light_Cycle_I;
    CycleRing -> lamps[7] = F1415L -> Light_Cycle_X;

    //	The Cycle Ring also displays on the CE panel - special case

    CycleRing -> lampsCE = new TLabel*[8];
    CycleRing -> lampsCE[0] = F1415L -> Light_CE_Cyc_A;
    CycleRing -> lampsCE[1] = F1415L -> Light_CE_Cyc_B;
    CycleRing -> lampsCE[2] = F1415L -> Light_CE_Cyc_C;
    CycleRing -> lampsCE[3] = F1415L -> Light_CE_Cyc_D;
    CycleRing -> lampsCE[4] = F1415L -> Light_CE_Cyc_E;
    CycleRing -> lampsCE[5] = F1415L -> Light_CE_Cyc_F;
    CycleRing -> lampsCE[6] = F1415L -> Light_CE_Cyc_I;
    CycleRing -> lampsCE[7] = F1415L -> Light_CE_Cyc_X;


    //	Build the various latches.  Most of these are not
    //	reset during Program Reset, but some are.

    StopLatch = true;
    F1415L -> Light_Stop -> Enabled = true;
    StopKeyLatch = false;
    DisplayModeLatch = false;
	StorageWrapLatch = false;
    ProcessRoutineLatch = false;
    BranchLatch = true;
    BranchTo1Latch = true;
    LastInstructionReadout = false;
    IRingControl = true;

    SignLatch = false;
    ZeroSuppressLatch = false;
    FloatingDollarLatch = false;
    AsteriskFillLatch = false;
    DecimalControlLatch = false;

    IOChannelSelect = false;
    IndexLatches = 0;
    InqReqLatch = false;

    CarryIn = new TDisplayLatch(F1415L -> Light_Carry_In,false);
    CarryOut = new TDisplayLatch(F1415L -> Light_Carry_Out,false);
    AComplement = new TDisplayLatch(F1415L -> Light_A_Complement,false);
    BComplement = new TDisplayLatch(F1415L -> Light_B_Complement,false);

    CompareBGTA = new TDisplayLatch(F1415L -> Light_B_GT_A,false);
    CompareBEQA = new TDisplayLatch(F1415L -> Light_B_EQ_A,false);
    CompareBLTA = new TDisplayLatch(F1415L -> Light_B_LT_A,false);
    Overflow = new TDisplayLatch(F1415L -> Light_Overflow,false);
    DivideOverflow = new TDisplayLatch(F1415L -> Light_Divide_Overflow,false);
    ZeroBalance = new TDisplayLatch(F1415L -> Light_Zero_Balance,false);
    PriorityAlert = new TDisplayLatch(F1415L -> Light_Priority_Alert,false);

    //	Build the various check latches.  Program Reset does reset these

    AChannelCheck = new TDisplayLatch(F1415L -> Light_Check_AChannel);
    BChannelCheck = new TDisplayLatch(F1415L -> Light_Check_BChannel);
    AssemblyChannelCheck = new TDisplayLatch(F1415L -> Light_Check_AssemblyChannel);
    AddressChannelCheck = new TDisplayLatch(F1415L -> Light_Check_AddressChannel);
    AddressExitCheck = new TDisplayLatch(F1415L -> Light_Check_AddressExit);
    ARegisterSetCheck = new TDisplayLatch(F1415L -> Light_Check_ARegisterSet);
    BRegisterSetCheck = new TDisplayLatch(F1415L -> Light_Check_BRegisterSet);
    OpRegisterSetCheck = new TDisplayLatch(F1415L -> Light_Check_OpRegisterSet);
    OpModifierSetCheck = new TDisplayLatch(F1415L -> Light_Check_OpModifierSet);
    ACharacterSelectCheck = new TDisplayLatch(F1415L -> Light_Check_ACharacterSelect);
    BCharacterSelectCheck = new TDisplayLatch(F1415L -> Light_Check_BCharacterSelect);

    IOInterlockCheck = new TDisplayLatch(F1415L -> Light_Check_IOInterlock);
    AddressCheck = new TDisplayLatch(F1415L -> Light_Check_AddressCheck);
    RBCInterlockCheck = new TDisplayLatch(F1415L -> Light_Check_IOInterlock);
    InstructionCheck = new TDisplayLatch(F1415L -> Light_Check_InstructionCheck);

    //	Build the Data Registers

    A_Reg = new TRegister();

    tmper = F1415L -> Light_CE_A_ER;
    tmpl = new TLabel*[8];
    tmpl[0] = F1415L -> Light_CE_A_1;
    tmpl[1] = F1415L -> Light_CE_A_2;
    tmpl[2] = F1415L -> Light_CE_A_4;
    tmpl[3] = F1415L -> Light_CE_A_8;
    tmpl[4] = F1415L -> Light_CE_A_A;
    tmpl[5] = F1415L -> Light_CE_A_B;
    tmpl[6] = F1415L -> Light_CE_A_C;
    tmpl[7] = F1415L -> Light_CE_A_WM;
    A_Reg -> SetDisplay(tmper,tmpl);

    B_Reg = new TRegister();

    tmper = F1415L -> Light_CE_B_ER;
    tmpl = new TLabel*[8];
    tmpl[0] = F1415L -> Light_CE_B_1;
    tmpl[1] = F1415L -> Light_CE_B_2;
    tmpl[2] = F1415L -> Light_CE_B_4;
    tmpl[3] = F1415L -> Light_CE_B_8;
    tmpl[4] = F1415L -> Light_CE_B_A;
    tmpl[5] = F1415L -> Light_CE_B_B;
    tmpl[6] = F1415L -> Light_CE_B_C;
    tmpl[7] = F1415L -> Light_CE_B_WM;
    B_Reg -> SetDisplay(tmper,tmpl);

    Op_Reg = new TRegister();

    DEBUG("B_Reg is at %x",B_Reg)
    DEBUG("OP Reg is at %x",Op_Reg)

    tmpl = new TLabel*[8];
    tmpl[0] = F1415L -> Light_CE_OP_1;
    tmpl[1] = F1415L -> Light_CE_OP_2;
    tmpl[2] = F1415L -> Light_CE_OP_4;
    tmpl[3] = F1415L -> Light_CE_OP_8;
    tmpl[4] = F1415L -> Light_CE_OP_A;
    tmpl[5] = F1415L -> Light_CE_OP_B;
    tmpl[6] = F1415L -> Light_CE_OP_C;
    tmpl[7] = 0;
    Op_Reg -> SetDisplay(0,tmpl);

    Op_Mod_Reg = new TRegister();

    tmpl = new TLabel*[8];
    tmpl[0] = F1415L -> Light_CE_Mod_1;
    tmpl[1] = F1415L -> Light_CE_Mod_2;
    tmpl[2] = F1415L -> Light_CE_Mod_4;
    tmpl[3] = F1415L -> Light_CE_Mod_8;
    tmpl[4] = F1415L -> Light_CE_Mod_A;
    tmpl[5] = F1415L -> Light_CE_Mod_B;
    tmpl[6] = F1415L -> Light_CE_Mod_C;
    tmpl[7] = 0;
    Op_Mod_Reg -> SetDisplay(0,tmpl);

    //	Build the Address Registers

    STAR = new TAddressRegister();
    STAR -> Reset();
    A_AR = new TAddressRegister();
    A_AR -> Reset();
	B_AR = new TAddressRegister();
    B_AR -> Reset();
    C_AR = new TAddressRegister();
    C_AR -> Reset();
    D_AR = new TAddressRegister();
    D_AR -> Reset();
    E_AR = new TAddressRegister();
    E_AR -> Reset();
    F_AR = new TAddressRegister();
    F_AR -> Reset();
    I_AR = new TAddressRegister();
    I_AR -> Reset();
    TOD = new TAddressRegister();
    TOD -> Reset();

    //	Build the channels

    Channel[CHANNEL1] = new T1410Channel(
        E_AR,
    	F1415L -> Light_Ch1_Interlock,
        F1415L -> Light_Ch1_RBCInterlock,
        F1415L -> Light_Ch1_Read,
        F1415L -> Light_Ch1_Write,
        F1415L -> Light_Ch1_Overlap,
        F1415L -> Light_Ch1_NoOverlap,
        F1415L -> Light_Ch1_NotReady,
        F1415L -> Light_Ch1_Busy,
        F1415L -> Light_Ch1_DataCheck,
        F1415L -> Light_Ch1_Condition,
        F1415L -> Light_Ch1_WLRecord,
        F1415L -> Light_Ch1_NoTransfer
    );

    Channel[CHANNEL2] = new T1410Channel(
        F_AR,
    	F1415L -> Light_Ch2_Interlock,
        F1415L -> Light_Ch2_RBCInterlock,
        F1415L -> Light_Ch2_Read,
        F1415L -> Light_Ch2_Write,
        F1415L -> Light_Ch2_Overlap,
        F1415L -> Light_Ch2_NoOverlap,
        F1415L -> Light_Ch2_NotReady,
        F1415L -> Light_Ch2_Busy,
        F1415L -> Light_Ch2_DataCheck,
        F1415L -> Light_Ch2_Condition,
        F1415L -> Light_Ch2_WLRecord,
        F1415L -> Light_Ch2_NoTransfer
    );

    //	Build the A Channel and Assembly Channel

    AChannel = new TAChannel();
    AssemblyChannel = new TAssemblyChannel();

    //	Some latches are set after power on...

    CompareBLTA -> Set();
    I_AR -> Set(1);
    AdderResult = 0;

    //	Initialize core-load

    for(i=0; i < STORAGE; ++i) {
        core[i] = BITC;
    }

    for(i=0; i < strlen(core_load_chars); ++i) {
    	core[i] = BCD::BCDConvert(core_load_chars[i]);
        if(core_load_wm[i]) {
        	core[i].SetWM();
        }
        core[i].SetOddParity();
    }

    //	Set up the indicators

    OffNormal = new TDisplayIndicator(F1415L -> Light_Off_Normal,
    	&(CPU -> IndicatorOffNormal));

    //  Finally, create any I/O devices

    FI1415IO -> ConsoleIODevice =
        new T1415Console(CONSOLE_IO_DEVICE,CPU -> Channel[CHANNEL1]);

    FI1403 -> PrinterIODevice =
        new TPrinter(PRINTER_IO_DEVICE, CPU -> Channel[CHANNEL1]);

    CPU -> Channel[CHANNEL1] -> Hopper[0] = new THopper();
    CPU -> Channel[CHANNEL1] -> Hopper[1] = new THopper();
    CPU -> Channel[CHANNEL1] -> Hopper[2] = new THopper();
    CPU -> Channel[CHANNEL1] -> Hopper[3] = new THopper();
    CPU -> Channel[CHANNEL1] -> Hopper[4] = new THopper();

    FI1402 -> ReaderIODevice =
        new TCardReader(READER_IO_DEVICE, CPU -> Channel[CHANNEL1]);
    FI1402 -> PunchIODevice =
        new TPunch(PUNCH_IO_DEVICE, CPU -> Channel[CHANNEL1]);
    FI1402 -> Channel = CPU -> Channel[CHANNEL1];

    FI729 -> SetTAU(new TTapeTAU(TAPE_IO_DEVICE,CPU -> Channel[CHANNEL1]),
        CHANNEL1);
    if(MAXCHANNEL > 1) {
        FI729 -> SetTAU(new TTapeTAU(TAPE_IO_DEVICE,CPU -> Channel[CHANNEL2]),
        CHANNEL2);
    }
    FI729 -> Display();

}


//	CPU Object display - run thru the display list

void T1410CPU::Display()
{
	TDisplayObject *l;

    for(l = DisplayList; l != 0; l = l -> NextDisplay) {
    	l -> Display();
    }

}

//	Off Normal Indicator routine

bool T1410CPU::IndicatorOffNormal()
{
	return(CPU -> InhibitPrintOut ||
    	!(CPU -> AsteriskInsert) ||
		CPU -> CycleControl != CPU -> CYCLE_OFF ||
        CPU -> CheckControl != CPU -> CHECK_STOP ||
	    (CPU -> Mode == CPU -> MODE_CE &&
        	CPU -> StorageScan  != CPU -> SSCAN_OFF) ||
        CPU -> AddressEntry != CPU -> ADDR_ENTRY_I );
}

//	Cycle routine.  Does typical kinds of CPU cycles

void T1410CPU::Cycle()
{
	switch(CycleRing -> State()) {

    case CYCLE_I:

    	*A_Reg = *B_Reg;
        AssemblyChannel -> GateAChannelToAssembly(
        	AChannel -> Select(AChannel -> A_Channel_A));
        break;

    case CYCLE_X:
    	//	This one is tricky, because it depends on opcode type.
        //	So this is handled in the indexing routines

        break;

    case CYCLE_A:
    	*A_Reg = *B_Reg;
		AChannel -> Select(AChannel -> A_Channel_A);
        A_AR -> Set(STARScan());
        break;

    case CYCLE_B:
    	B_AR -> Set(STARScan());
        break;

    case CYCLE_C:
        *A_Reg = *B_Reg;
        AChannel -> Select(AChannel -> A_Channel_A);
        A_AR -> Set(STARScan());
        break;

    case CYCLE_D:
        D_AR -> Set(STARScan());
        break;

    case CYCLE_E:
    case CYCLE_F:
        //  E/F Cycle register advance handled in channel code.
        break;

    default:
    	break;
    }
}

//	Storage routine to read out (access) core storage.
//	(A real core storage unit would also have to store it back)
//	Address is in the STAR (aka MAR)

void T1410CPU::Readout()
{
	long i;

    //	Get the contents of STAR: The Memory Address Register

    i = STAR -> Gate();			//  Hee hee  ;-)  A Punny
    StorageWrapLatch = false;

    //	This is really a debugging statement, but it's useful

    if(i < 0) {
        AddressCheck ->
        	SetStop("Address Check: STAR value not valid");
        AddressExitCheck ->
        	SetStop("Address Exit Check: STAR value not valid");
        B_Reg -> Set(0);		// Force a B Channel Check
        return;
    }

    if(i >= STORAGE) {
        AddressCheck ->
        	SetStop("Address Check: STAR value > storage size");
        B_Reg -> Set(0);
        return;
    }

    B_Reg -> Set(core[i]);
}

//	Storage routine to store what is in the B Data Register

void T1410CPU::Store(BCD bcd)
{
	long i;

    //	Get the contents of STAR: The Memory Address Register

    i = STAR -> Gate();

    //    if(i == 8233 && bcd.To6Bit() != 12 && bcd.To6Bit() != 28) {
    //        DEBUG("Storing location 8233 value %d",bcd.To6Bit());
    //    }

    //	This is really a debugging statement, but it's useful

    if(i < 0) {
        AddressCheck ->
        	SetStop("Address Check: STAR value not valid during store");
        AddressExitCheck ->
        	SetStop("Address Exit Check: STAR value not valid during store");
        return;
    }

    if(i >= STORAGE) {
        AddressCheck ->
        	SetStop("Address Check: Star value > size of storage during store");
        return;
    }

    if(!bcd.CheckParity()) {
    	BChannelCheck ->
        	SetStop("B Channel Channel Check: Invalid parity during store");
        AssemblyChannelCheck ->
            SetStop("Assembly Channel Check: Invalid parity during store");
    }

    core[i] = bcd;
}

//	Set Storage Scan Mode

void T1410CPU::SetScan(char i)
{
	ScanRing -> Set(i);
}

//	Apply storage scan value to STAR and return result.
//	Essentially this gates the ScanControl Ring to the Address Modification
//	circuitry

long T1410CPU::STARScan()
{
    long temp;

	temp = STAR -> Gate() + scan_mod[ScanRing -> State()];
    if(temp >= STORAGE) {
        temp = 0;
        StorageWrapLatch = true;
    }
    else if(temp < 0) {
        temp = 99999;
        StorageWrapLatch = true;
    }
    return(temp);
}

//	Apply +1 / 0 / -1 modification value to STAR and return result.
//	This amounts to direct use of the address modification circuitry when
//	not using SCAN CTRL (e.g. when doing Instruction readout)

long T1410CPU::STARMod(int mod)
{
    long temp;

	temp = STAR -> Gate() + mod;
    if(temp >= STORAGE) {
        temp = 0;
        StorageWrapLatch = true;
    }
    else if(temp < 0) {
        temp = 99999;
        StorageWrapLatch = true;
    }
    return temp;
}

//  Storage wrap check.  Typically this will be called BEFORE the
//  actual STARMod / STARScan call to set the Storage Wrap flag in
//  time for it to be used in code.

bool T1410CPU::StorageWrapCheck(int mod) {

    switch(mod) {

    case 1:
        if(STAR -> Gate() == STORAGE-1) {
            return(StorageWrapLatch = true);
        }
        break;

    case -1:
        if(STAR -> Gate() == 0) {
            return(StorageWrapLatch = true);
        }
        break;
    }
    return(StorageWrapLatch = false);
}

//	Method to load core from a file

void T1410CPU::LoadCore(char *filename)
{
	FILE *fd;
    long coresize,loc;
    char file_coresize[6];
    int file_core[STORAGE];

    //	First open the file.

    if((fd = fopen(filename,"rb")) == NULL) {
    	Application -> MessageBox("Unable to open file to load core.",
        	"Load Core Error",MB_OK);
        return;
    }

    //	Next, read in the (5 digit) core size

    file_coresize[sizeof(file_coresize)-1] = '\0';
    if(fread(&file_coresize,1,5,fd) != 5) {
    	Application -> MessageBox("Error reading dump core size from file.",
        	"Error Reading Dump",MB_OK);
        fclose(fd);
        return;
    }

    //	Convert from decimal to long, and validate.

    coresize = atol(file_coresize);
    if(coresize < 0) {
    	Application -> MessageBox("Dump contained negative value for core size.",
        	"Negative Core Size in File",MB_OK);
        fclose(fd);
        return;
    }

    if(coresize > STORAGE) {
    	Application -> MessageBox("Dump contained core size greater than simlator's.",
        	"Large Core Size in File",MB_OK);
        fclose(fd);
        return;
    }

    //	Read in the data from the dump file.

    if(fread(&file_core,sizeof(int),coresize,fd) != coresize) {
    	Application -> MessageBox("Error loading core data from file",
        	"Error Loading Core",MB_OK);
        fclose(fd);
    }

    //	Finally, copy the data to core.

    for(loc=0; loc < coresize; ++loc) {
    	core[loc].Set(file_core[loc]);
    }

    fclose(fd);
    Application -> MessageBox("Core Loaded!","Core Loaded",MB_OK);
}

void T1410CPU::DumpCore(char *filename)
{
	FILE *fd;
    long loc;
    int file_core[STORAGE];

    //	First, prepare a file.

    if((fd = fopen(filename,"wb")) == NULL) {
    	Application -> MessageBox("Unable to open file to dump core.",
        	"Dump Core Error",MB_OK);
    }

    //	Then, copy core to an integer array

    for(loc=0; loc < STORAGE; ++loc){
    	file_core[loc] = core[loc].ToInt();
    }

    //	Write out the core size to the file

    if(fprintf(fd,"%05d",STORAGE) != 5) {
    	Application -> MessageBox("Error writing out core size.",
        	"Core Size Write Error",MB_OK);
        fclose(fd);
        return;
    }

    //	Then write out the integer array

    if(fwrite(&file_core,sizeof(int),STORAGE,fd) != STORAGE) {
    	Application -> MessageBox("Error writing out core file.",
        	"File Write Error",MB_OK);
        fclose(fd);
    }

    Application -> MessageBox("Core Dumped!","Core Dumped",MB_OK);
    fclose(fd);
}

//	The 1410 Adder.  It works by translating the numeric part into
//	QuiBinary code.  The real 1410 used logic gates to add, we use
//	a table.

//  The adder also provides the Quinary and Binary results individually
//  for the Comparator.

//	Here is the code

#define ADDER_BINARY_0	0
#define ADDER_BINARY_1	1
#define ADDER_QUINARY_0 0
#define ADDER_QUINARY_2 1
#define ADDER_QUINARY_4 2
#define ADDER_QUINARY_6 3
#define ADDER_QUINARY_8 4

//	This table translates the numeric part of a BCD character (8421) to
//	obtain the quinary part

static int num_to_true_quinary[] = {
	ADDER_QUINARY_0,				//	0 (no bits)
    ADDER_QUINARY_0,				//  1
    ADDER_QUINARY_2,				//	2
    ADDER_QUINARY_2,				//  3 (2 + 1)
    ADDER_QUINARY_4,				//	4
    ADDER_QUINARY_4,				//	5 (4 + 1)
    ADDER_QUINARY_6,				//	6
    ADDER_QUINARY_6,				//	7 (6 + 1)
    ADDER_QUINARY_8,				//	8
    ADDER_QUINARY_8,				//	9 (8 + 1)
    ADDER_QUINARY_0,				//	0 (8 + 2)
    ADDER_QUINARY_2,				//  3 (2 + 1) (8 is ignored)
    ADDER_QUINARY_4,				//	4 (8 is ignored)
    ADDER_QUINARY_4,				//	5 (4+ 1) (8 is ignored)
    ADDER_QUINARY_6,				//  6 (8 is ingored)
	ADDER_QUINARY_6					//	7 (6 + 1) (8 is ignored)
};

//	This table does the same thing, but generated the complement value,
//	used for subtraction and negative numbers.  Note that these entries
//	are 8 - (the entry from the above table).  But we are generated *indexes*
//	here, not actual values, so we couldn't just subtract!

static int num_to_complement_quinary[] = {
	ADDER_QUINARY_8,ADDER_QUINARY_8,ADDER_QUINARY_6,ADDER_QUINARY_6,
    ADDER_QUINARY_4,ADDER_QUINARY_4,ADDER_QUINARY_2,ADDER_QUINARY_2,
    ADDER_QUINARY_0,ADDER_QUINARY_0,ADDER_QUINARY_8,ADDER_QUINARY_6,
    ADDER_QUINARY_4,ADDER_QUINARY_4,ADDER_QUINARY_2,ADDER_QUINARY_2
};

//	Adder Quinary Matrix.  Combines the two quinary values, returning
//	a quinary result, and a carry value.  Indexed as [B][A], but the
//	table is symmetric (addition is complementary, even in quinary matrices!)

struct adder_matrix {
	int quinary;
    bool carry;
} adder_matrix[5][5] = {
	{ { ADDER_QUINARY_0, false }, { ADDER_QUINARY_2, false },	// 0-0, 0-2
    { ADDER_QUINARY_4, false }, { ADDER_QUINARY_6, false },		// 0-4, 0-6
    { ADDER_QUINARY_8, false } },								// 0-8

    { { ADDER_QUINARY_2, false }, { ADDER_QUINARY_4, false },	// 2-0, 2-2
    { ADDER_QUINARY_6, false },	{ ADDER_QUINARY_8, false }, 	// 2-4, 2-6
    { ADDER_QUINARY_0, true } },								// 2-8

    { { ADDER_QUINARY_4, false }, { ADDER_QUINARY_6, false},	// 4-0, 4-2
    { ADDER_QUINARY_8, false }, { ADDER_QUINARY_0, true },		// 4-4, 4-6
    { ADDER_QUINARY_2, true  } },								// 4-8

    { { ADDER_QUINARY_6, false }, { ADDER_QUINARY_8, false },	// 6-0, 6-2
    { ADDER_QUINARY_0, true }, { ADDER_QUINARY_2, true  }, 		// 6-4, 6-6
    { ADDER_QUINARY_4, true } },								// 6-8

    { { ADDER_QUINARY_8, false }, { ADDER_QUINARY_0, true },	// 8-0, 8-2
    { ADDER_QUINARY_2, true  }, { ADDER_QUINARY_4, true },		// 8-4, 8-6
    { ADDER_QUINARY_6, true } }									// 8-8
};

//	Adder result table.  Indexed via [quinary value][bshift value]

int adder_result_table[5][4] = {
	{ 0, 1, 2, 3 },
    { 2, 3, 4, 5 },
    { 4, 5, 6, 7 },
    { 6, 7, 8, 9 },
    { 8, 9, 0, 1 }
};

//	Adder.

//	Note: eventually we can make this more efficient by just generating
//	a 16x16 table for all of the possibilities during the CPU constructor!

BCD T1410CPU::Adder(BCD A,int Complement_A,BCD B,int Complement_B)
{
    struct adder_matrix *qmatrix;
    int bin;

    int bcd_a,bcd_b;
    int quinary_a,quinary_b;
    int binary_a,binary_b;

    bcd_a = A.ToInt();
    bcd_b = B.ToInt();

    //	Based on the complement requests, translate BCD to QuiBinary.

    if(Complement_A) {
    	quinary_a = num_to_complement_quinary[bcd_a & BIT_NUM];
        binary_a = 1 - (bcd_a & 1);
    }
    else {
        quinary_a = num_to_true_quinary[bcd_a & BIT_NUM];
        binary_a = bcd_a & 1;
    }

    if(Complement_B) {
    	quinary_b = num_to_complement_quinary[bcd_b & BIT_NUM];
        binary_b = 1 - (bcd_b & 1);
    }
    else {
        quinary_b = num_to_true_quinary[bcd_b & BIT_NUM];
        binary_b = bcd_b & 1;
    }

	//	Handle the binary parts by adding them together along with the
    //	CarryIn latch.  This results in a number from 0 to 3.

	AdderBinaryResult = binary_a + binary_b + (CarryIn -> State());

    //	Add the Quinary parts together according to the matrix

    qmatrix = &adder_matrix[quinary_a][quinary_b];
    AdderQuinaryResult = qmatrix -> quinary;

    //	Set the carry out state...

    CarryOut -> Set(qmatrix -> carry ||
    	(qmatrix -> quinary == ADDER_QUINARY_8 && AdderBinaryResult > 1));

	//	Calculate the binary result

    bin = adder_result_table[qmatrix -> quinary][AdderBinaryResult];

    //	Finally, translate the binary result to BCD.  Since we *know* that
    //	the bin value is range 0-9, we just cheat, and use the ascii to bcd
    //	table

    AdderResult = BCD::BCDConvert(bin + '0');
	return(AdderResult);
}

//  Comparator.  The comparator uses a fairly convoluted system of
//  comparing character types (see bcd_char_type_table), zones and
//  if all of that is equal, uses the adder to finish the job.  As
//  with the Adder, I have used a table to accomplish what logic gates
//  do in the real machine.  (Note that I could have simply put a
//  collating sequence table into the emulator, but that would not have
//  been nearly has perverse -- or as much fun!!

//  Note that the collating sequence is in _reverse_ order of zone bits:
//  BA < B < A < no zones (except for BLANK, which is a special case)

//  For a given set of zones, NN is GREATER than SC, except for blank,
//  which is, of course, less than anything.

//  Also note that the BEQA latch is only set during the units position,
//  and that the comparator only sets any latches during units or body.
//  After that, subsequent characters either leave it alone (are equal),
//  or set GT or LT.

//  The constants below are B Type _ A Type

enum comparator_status {
    COMPARATOR_INVALID, COMPARATOR_NNSC_AN, COMPARATOR_SCAN_SCAN,
    COMPARATOR_AN_NNSC, COMPARATOR_NN_NN, COMPARATOR_NN_SC,
    COMPARATOR_SC_NN };

//  The table below is [B][A] (each row is for one type of B Character)

static enum comparator_status comparator_table[4][4] = {
    {COMPARATOR_INVALID,COMPARATOR_INVALID,COMPARATOR_INVALID,COMPARATOR_INVALID},
    {COMPARATOR_INVALID,COMPARATOR_NN_NN,COMPARATOR_NN_SC,COMPARATOR_NNSC_AN},
    {COMPARATOR_INVALID,COMPARATOR_SC_NN,COMPARATOR_SCAN_SCAN,COMPARATOR_NNSC_AN},
    {COMPARATOR_INVALID,COMPARATOR_AN_NNSC,COMPARATOR_AN_NNSC,COMPARATOR_SCAN_SCAN}
};

void T1410CPU::Comparator()
{
    BCD a_temp,b_temp;
    int a_zones,b_zones;
    enum comparator_status type_status;
    enum Tbvsa {BGTA, BEQA, BLTA} bvsa;

    //  The comparator only sets the latches if units or body.

    if(SubScanRing -> State() != SUB_SCAN_U &&
       SubScanRing -> State() != SUB_SCAN_B) {
        return;
    }

    a_temp = AChannel -> Select() & 0x3f;
    b_temp = B_Reg -> Get() & 0x3f;
    a_zones = a_temp.ToInt() & BIT_ZONE;
    b_zones = b_temp.ToInt() & BIT_ZONE;
    type_status = comparator_table[(int)b_temp.GetType()][(int)a_temp.GetType()];
    assert(type_status != COMPARATOR_INVALID);

    CarryOut -> Reset();
    CarryIn -> Set();

    switch(type_status) {

    //  NN or SC are always less than AN
    case COMPARATOR_NNSC_AN:
        bvsa = BLTA;
        break;

    //  Comparing SC:SC or AN:AN.  Check zones first.  If equal, user adder
    case COMPARATOR_SCAN_SCAN:
        if(b_zones > a_zones) {
            bvsa = BLTA;
        }
        else if(b_zones < a_zones) {
            bvsa = BGTA;
        }
        else {                          //  Zones equal.  Add -A and B
            Adder(a_temp,true,b_temp,false);
            if(AdderQuinaryResult != ADDER_QUINARY_8) {
                if(CarryOut -> State()) {
                    bvsa = BGTA;
                }
                else {
                    bvsa = BLTA;
                }
            }                           //  Quinary 8 - more to check
            else {
                switch(AdderBinaryResult) {
                case 0:
                    assert(false);      //  Cannot happen (Carry In!)
                    break;
                case 1:
                    bvsa = BLTA;
                    break;
                case 2:                 //  Set Equal ONLY DURING UNITS!!
                    bvsa = BEQA;
                    break;
                case 3:
                    bvsa = BGTA;
                    break;
                default:
                    assert(false);
                }   //  End switch to check binary result
            }   //  End adder, Quinary 8 result
        }   //  End compare using adder
        break;

    //  AN is always greater than NN or SC
    case COMPARATOR_AN_NNSC:                       //  AN > NN or SC
        bvsa = BGTA;
        break;

    //  Comparing NN to NN.  Since NN have numeric parts of zeros,
    //  just check the zones.  EXCEPT FOR NO ZONES (blank), which is less
    //  than everything
    case COMPARATOR_NN_NN:
        if(b_zones == a_zones) {
            bvsa = BEQA;
        }
        else if(b_zones == 0) {
            bvsa = BLTA;
        }
        else if(a_zones == 0) {
            bvsa = BGTA;
        }
        else if(b_zones > a_zones) {
            bvsa = BLTA;
        }
        else {
            assert(a_zones > b_zones);
            bvsa = BGTA;
        }
        break;

    //  Comparing B:NN to A:SC.  First check zones.  If they are different,
    //  base decison on zones.  If they are the same, NN is > SC.
    //  EXCEPT that if NN is a space (no zones), NN < SC
    case COMPARATOR_NN_SC:                      //  NN:SC  Check Z
        if(a_zones == b_zones) {
            bvsa = (b_zones != 0) ? BGTA : BLTA;
        }
        else if(b_zones == 0) {
            bvsa = BLTA;
        }
        else if(b_zones > a_zones) {
            bvsa = BLTA;
        }
        else {
            assert(a_zones > b_zones);
            bvsa = BGTA;
        }
        break;

    //  Comparing B:SC to A:NN, similar situation to above
    case COMPARATOR_SC_NN:
        if(b_zones == a_zones) {
            bvsa = (a_zones != 0) ? BLTA : BGTA;
        }
        else if(a_zones == 0) {
            bvsa = BGTA;
        }
        else if(b_zones > a_zones) {
            bvsa = BLTA;
        }
        else {
            assert(a_zones > b_zones);
            bvsa = BGTA;
        }
        break;

    default:
        assert(false);
        break;
    }

    //  Now that we know how the compare turned out, set up the latches

    switch(bvsa) {

    case BLTA:
        CompareBLTA -> Set();
        CompareBGTA -> Reset();
        CompareBEQA -> Reset();
        break;

    case BGTA:
        CompareBGTA -> Set();
        CompareBLTA -> Reset();
        CompareBEQA -> Reset();
        break;

    case BEQA:
        //  Equal can only be set during units!
        if(SubScanRing -> State() == SUB_SCAN_U) {
            CompareBEQA -> Set();
            CompareBLTA -> Reset();
            CompareBGTA -> Reset();
        };
        break;

    default:
        assert(false);
        break;
    }

}
