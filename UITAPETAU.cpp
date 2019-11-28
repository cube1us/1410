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

#include <dir.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include "ubcd.h"
#include "UI1410CPUT.h"
#include "UIHOPPER.h"
#include "UI1410CHANNEL.h"
#include "UITAPEUNIT.h"
#include "UITAPETAU.h"
#include "UI729TAPE.h"


//---------------------------------------------------------------------------
#pragma package(smart_init)

#include "UI1410DEBUG.h"

//  1410 Tape Adapter Unit Implementation.  Follows I/O Device Interface.

//  Constructor.  Creates tape drives!
//  NOTE:  This is expected to be called with the EVEN PARITY designation
//  for the device number ("U").  This constructor automatically adds the
//  ODD PARITY designation ("B")!

TTapeTAU::TTapeTAU(int devicenum,T1410Channel *Channel) :
    T1410IODevice(devicenum,Channel) {

    static char parity_table[128] = {
    	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
	    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
       	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
	    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1
    };

    int i;

    for(i=0; i < 10; ++i) {
        Unit[i] = new TTapeUnit(i);
    }

    TapeUnit = NULL;
    tapestatus = 0;
    chars_transferred = 0;
    tape_parity_table = parity_table;

    //  Create the odd parity device now...

    Channel -> AddIODevice(this,BCD(BCD::BCDConvert('B')).To6Bit());
}

//  Select.  If some other unit is selected, it gets deslected.
//  The unit comes from the CPU / Channel

int TTapeTAU::Select() {

    int u;

    if(TapeUnit != NULL) {
        TapeUnit -> Select(false);
        TapeUnit = NULL;
    }

    //  Find the tape unit.  Give up if none there.  Otherwise, select it.

    u = Channel -> GetUnitNumber();
    if((TapeUnit = GetUnit(u)) == NULL) {
        return(tapestatus = IOCHNOTREADY);
    }
    TapeUnit -> Select(true);

    //  If the unit isn't ready, say so

    if(!TapeUnit -> IsReady()) {
        return(tapestatus = IOCHNOTREADY);
    }

    //  If the unit is busy, say so

    if(TapeUnit -> IsBusy()) {
        return(tapestatus = IOCHBUSY);
    }

    //  Set 0 length record so far.

    chars_transferred = 0;

    //  If writing, and tape is write protected, set not ready and condition.
    //  Not sure if that is write - the diagnostics will probably figure it out
    //  for me...  8-)

    if(Channel -> ChWrite -> State() && TapeUnit -> IsFileProtected()) {
        return(tapestatus |= (IOCHNOTREADY | IOCHCONDITION));
    }

    return(tapestatus = 0);
}

//  Return a pointer to a given tape drive, NULL if invalid.

TTapeUnit *TTapeTAU::GetUnit(int u) {

    if(u < -0 || u > 9) {
        return(NULL);
    }
    return(Unit[u]);
}

//  DoOutput: Accept an output character from the channel.

void TTapeTAU::DoOutput() {

    //  If no (or invalid) unit selected, just return not ready.

    if(TapeUnit == NULL) {
        tapestatus |= IOCHNOTREADY;
        Channel -> ExtEndofTransfer = true;
        return;
    }

    //  If file protected, return not ready as well.  (Not sure if this is
    //  correct, but hopefully the diagnostics will test it)

    if(TapeUnit -> IsFileProtected()) {
        tapestatus |= IOCHNOTREADY;
        Channel -> ExtEndofTransfer = true;
        return;
    }

    //  Get the character from the channel.  Strip down to 6 bits for tape.

    ch_char = Channel -> ChR2 -> Get();
    tape_char = ch_char.To6Bit();

    //  If we are in load mode, and the character either has a word mark or is
    //  itself a word separator, write out an extra word separator character to
    //  tape.

    if(Channel -> LoadMode && (ch_char.TestWM() ||
        tape_char == (BCD_WS & 0x3f)) ) {
        if(!DoOutputWrite(BCD_WS)) {
            tapestatus |= IOCHCONDITION;
            Channel -> ExtEndofTransfer = true;
            return;
        }
    }

    //  If the incoming character has invalid parity, flag a datacheck.  But
    //  we will still write what we can, in valid parity, to tape.

    if(!ch_char.CheckParity()) {
        tapestatus |= IOCHDATACHECK;
    }

    //  Finally, write out the chracter.


    if(!DoOutputWrite(tape_char)) {
        tapestatus |= IOCHCONDITION;
        Channel -> ExtEndofTransfer = true;
        return;
    }

    Channel -> OutputRequest = true;
    Channel -> CycleRequired = true;

    ++chars_transferred;
}

//  Method to tell channel when we have data.

void TTapeTAU::DoInput() {

    bool wm = false;
    BCD b;

    if(TapeUnit == NULL) {
        tapestatus |= IOCHNOTREADY;
        Channel -> ExtEndofTransfer = true;
        return;
    }

    //  Get a character from the tape unit.  If an interesting status, just
    //  return.  The utility method will have already set the right status.
    //  (That is why the utility method exists!)

    tape_read_char = DoInputRead();
    if(tape_read_char < 0) {
        return;
    }

    ++chars_transferred;

    //  If we are reading in load mode, and we got a word separator, read the
    //  next character.  Then we will either set a WM on it, or, if it too is
    //  a word separator, just store the word separator.  If we have a problem
    //  reading the next char, just return -- the read routine will have set
    //  status -- that is why it is there!  8-)

    if(Channel -> LoadMode && (tape_read_char & 0x3f) == (BCD_WS & 0x3f)) {
        wm = true;
        tape_read_char = DoInputRead();
        if(tape_read_char < 0) {
            return;
        }
        if((tape_read_char & 0x3f) == BCD_WS) {
            wm = false;
        }
    }

    b = BCD(tape_read_char);

    //  The character we just got is in the correct TAPE parity.  If we
    //  are reading tape in EVEN parity, we have to FLIP THE CHECK BIT,
    //  so that the character gets returned in ODD parity.

    //  Note that, as a result, if we read a tape mark from tape in odd
    //  parity, it will cause a data check, because it is *always* even
    //  parity on tape.

    //  Furthermore, if we are left with just an A bit, we change the character
    //  to a blank.  (In some modes of writing a tape, writing a frame with no
    //  bits would mess it up).


    if((Channel -> ChUnitType -> Get().ToInt() & 2) == 0) { //  B as 2 bit - ODD
        //  NO 2 bit -- EVEN!
        b.ComplementCheck();
        if(b.To6Bit() == BITA) {
            b = BITC;
        }
    }

    //  If we set the wm flag earlier, turn it on (and flip the check bit)

    if(wm) {
        b.SetWM();
        b.ComplementCheck();
    }

    //  Finally, tell the channel we have something to eat.

    Channel -> ChannelStrobe(b);
}

//  Method for unit control.  Mostly, just passes it off to the tape unit.
//  Also sets ExtEndofTransfer, indicating we are done.  (Will probably have
//  to work in delays someday.  Oh well)

//  Note that, interestingly, the TAU returns busy for many successful
//  unit control operations.

void TTapeTAU::DoUnitControl(BCD opmod) {

    int d;
    int rc;

    d = opmod.To6Bit();

    if(TapeUnit == NULL) {
        tapestatus |= IOCHNOTREADY;
        Channel -> ExtEndofTransfer = true;
        return;
    }

    DEBUG("TTapeTAU: Unit operation %d",d);
    
    switch(d) {

    case UNIT_BACKSPACE:
        if(!TapeUnit -> Backspace()) {
            tapestatus |= IOCHNOTREADY;
        }
        //  tape indicate does NOT cause CONDITION in backspace operation!
        break;

    case UNIT_SKIP:
        if(!TapeUnit -> Skip()) {
            tapestatus |= IOCHNOTREADY;
        }
        break;

    case UNIT_REWIND:
        if(!TapeUnit -> Rewind()) {
            tapestatus |= IOCHNOTREADY;
        }
        break;

    case UNIT_REWIND_UNLOAD:
        if(!TapeUnit -> Unload()) {
            tapestatus |= IOCHNOTREADY;
        }
        break;

    case UNIT_WTM:
        if(!TapeUnit -> WriteTM()) {
            tapestatus |= IOCHNOTREADY;
        }
        //  Write TM in odd parity sets Data Check
        if(Channel -> ChUnitType -> Get().ToInt() & 2) {
            tapestatus |= IOCHDATACHECK;
        }
        break;

    case UNIT_SPACE:
        rc = TapeUnit -> Space();
        switch(rc) {
        case TAPEUNITEOF:
            tapestatus |= IOCHCONDITION;
            break;
        case TAPEUNITNOTREADY:
            tapestatus |= IOCHNOTREADY;
            break;
        case TAPEUNITERROR:
            tapestatus |= (IOCHNOTREADY | IOCHCONDITION);
            break;
        default:
            break;
        }
        break;

    default:
        tapestatus |= IOCHNOTREADY;
        break;
    }

    FI729 -> Display();

    //  Possibly set up for forlapped operation.  This only affects WTM

    Channel -> UnitControlOverlapBusy = NULL;
    if(d != UNIT_WTM || Channel -> ChNotOverlap -> State()) {
        Channel -> ExtEndofTransfer = true;
    }
    else {
        Channel -> UnitControlOverlapBusy = TapeUnit -> GetBusyDevice();
    }

    return;
}

//  Private utility methods...

//  Utility method to write a character to the drive and collect status.

bool TTapeTAU::DoOutputWrite(char c) {

    int wanted_parity;

    if(TapeUnit == NULL) {
        tapestatus |= IOCHNOTREADY;
        Channel -> ExtEndofTransfer = true;
        return(false);
    }

    c &= 0x3f;                                  //  Bye bye WM & Check Bit.

    //  Check the unit type.  B, which has a "2" bit, means odd.

    wanted_parity = ((Channel -> ChUnitType -> Get().ToInt() & 2) != 0); // 1 Odd

    if(tape_parity_table[c] != wanted_parity) {
        c ^= BITC;
    }

    //  Even parity blanks would have no bits.  The tape drive couldn't do
    //  that, so it gets changed to C+A.  C+A turns back to blank when it
    //  is read back in.

    if(c == 0) {
        assert(wanted_parity == 0);
        c = BITC | BITA;
    }

    //  Finally, tell the drive to write the character.

    return(TapeUnit -> Write(c));
}

//  Utility method to read a character from a tape drive.  Sets appropriate
//  status if the drive returns something other than an ordinary character.

int TTapeTAU::DoInputRead() {

    int c;
    bool wanted_parity;

    c = TapeUnit -> Read();

    if(c < 0) {
        Channel -> ExtEndofTransfer = true;
        if(c == TAPEUNITNOTREADY  || c == TAPEUNITERROR) {
            tapestatus |= IOCHNOTREADY;
            return(c);
        }
        else if(c == TAPEUNITEOF) {
            tapestatus |= IOCHCONDITION;
            c = BCD_TM;                     //  Continue on with parity check
        }
        else {
            return(c);
        }
    }

    //  Check what parity we want.  We want odd parity if the device in the
    //  instruction was "B" (has a 2 bit).

    wanted_parity = ((Channel -> ChUnitType -> Get().ToInt() & 2) != 0);

    assert(c >= 0 && c < 0x80);
    if(tape_parity_table[c] != wanted_parity) {
        tapestatus |= IOCHDATACHECK;
    }

    return(c);
}

//  Return status at end of operation

int TTapeTAU::StatusSample() {
    if(TapeUnit != NULL) {
        FI729 -> Display();
    }

    if(Channel -> ChWrite -> State()) {
        if(TapeUnit == NULL) {
            tapestatus |= IOCHNOTREADY;
        }
        else {
            DEBUG("TAU Initiating WriteIRG");
            TapeUnit -> WriteIRG();
        }
    }

    if(TapeUnit != NULL &&
       ((tapestatus & IOCHCONDITION) || TapeUnit -> TapeIndicate()) ) {
        tapestatus |= IOCHCONDITION;
        TapeUnit -> ResetIndicate();
        Channel -> SetTapeIndicate();
    }
    return(Channel -> GetStatus() | tapestatus);
}
