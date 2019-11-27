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
#include <stdlib.h>
#include <stdio.h>
#include "ubcd.h"
#include "UI1410CPUT.h"
#include "UIHOPPER.h"
#include "UI1410CHANNEL.h"
#include "UI1410INST.h"
#include "UIPUNCH.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)

#include "UI1410DEBUG.h"

//  Constructor

TPunch::TPunch(int devicenum, T1410Channel *Channel) :
    T1410IODevice(devicenum,Channel) {

    BusyEntry = new TBusyDevice();
    PunchBuffer = new TCard();
    Ready = false;
    column = 1;
}

//  User interface methods

bool TPunch::DoStart() {
    Ready = true;
    return(true);
}

bool TPunch::DoStop() {
    Ready = false;
    return(true);
}

//  IO Device Interface

//  Select is called at the beginning of an IO operation

int TPunch::Select() {

    if(!Ready) {
        return(PunchStatus = IOCHNOTREADY);
    }
    if(IsBusy()) {
        return(PunchStatus = IOCHBUSY);
    }
    if(Channel -> ChRead -> State()) {
        return(PunchStatus = IOCHNOTREADY);
    }
    if(!SetUnit(Channel -> GetUnitNumber())) {
        return(PunchStatus = IOCHNOTREADY);
    }

    column = 1;
    return(PunchStatus = 0);
}

//  Method to validate unit, and directed (eventual) card to correct hopper.

bool TPunch::SetUnit(int u) {

    switch(u) {

    case 0:
        u = HOPPER_P0;
        break;
    case 4:
        u = HOPPER_P4;
        break;
    case 8:
        u = HOPPER_P8;
        break;
    default:
        return(false);
    }

    PunchBuffer -> SelectStacker(Channel -> Hopper[u]);
    return(true);
}

//  Status Sample is called at the end of an I/O operation

int TPunch::StatusSample() {
    if(Channel -> ChWrite -> State() && column != 81) {
        PunchStatus |= IOCHWLRECORD;
    }
    return(Channel -> GetStatus() | PunchStatus);
}

//  Here is where we really get "punchy"

void TPunch::DoOutput() {

    BCD ch_char;

    //  If not ready, say so.

    if(!Ready) {
        PunchStatus |= IOCHNOTREADY;
        Channel -> ExtEndofTransfer = true;
        return;
    }

    //  Get a character from the channel silo

    ch_char = Channel -> ChR2 -> Get();

    //  If we have too many characters, indicate the problem

    if(column > 80) {
        PunchStatus |= IOCHWLRECORD;
        Channel -> ExtEndofTransfer = true;
        return;
    }

    //  Check the parity of the character coming from memory, set the DC
    //  flag, but continue.

    if(!ch_char.CheckParity()) {
        PunchStatus |= IOCHDATACHECK;
    }

	//  In Load Mode, we have to turn Wordmarks into Word Separators.  The
	//  addition of the Word Separator makes a Wrong Length Record quite
    //  likely!

    if(Channel -> LoadMode && ch_char.TestWM()) {
        DoOutputChar(BCD_WS);
        if(++column > 80) {
            PunchStatus |= IOCHWLRECORD;
            Channel -> ExtEndofTransfer;
            return;
        }
    }

    //  Now punch the character itself...

    DoOutputChar(ch_char);
    ++column;

    //  Check for end of transfer.  If we need more characters, let the
    //  Channel know.  Otherwise, let the Channel know we have had enough.

    //  Also, this is how we will know we are ready to punch and stack the
    //  card image.  We could do it at status sample time, but we do it
    //  here to be consistent with the Printer code.

    if(column == 81) {
        PunchBuffer -> Stack();                     //  Hopper code makes it QED
        BusyEntry -> SetBusy(2);                    //  Go busy for a while.
    }

    //  Same kludge as for printer.  We ask for another character, even if
    //  we don't really want it.  If we get one, then we will detect WLR

    Channel -> OutputRequest = true;
    Channel -> CycleRequired = true;
}

//  Bugs Says:  Punches don't weed vewy well, do dey?

void TPunch::DoInput() {
    PunchStatus |= IOCHNOTREADY;
    Channel -> ExtEndofTransfer = true;
}

//  The Punch also doesn't support any unit control instructions

void TPunch::DoUnitControl(BCD opmod) {
    PunchStatus |= IOCHNOTREADY;
    Channel -> ExtEndofTransfer = true;
}

//  And, now, for the work of actually putting a character on the card image

void TPunch::DoOutputChar(BCD c) {

    if(column < 1 || column > 80) {
        PunchStatus |= IOCHCONDITION;               //  Should never happen!
        DEBUG("TPunch::DoOutputChar: Invalid column: %d",column);
        return;
    }

    PunchBuffer -> image[column-1] = c.ToAscii();   //  Also QED.
}
