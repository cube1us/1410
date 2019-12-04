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
#include "ubcd.h"
#include "UI1410CPUT.h"
#include "UI1410INST.h"
#include "UIHOPPER.h"
#include "UI1410CHANNEL.h"
#include "UIREADER.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)

#include "UI1410DEBUG.h"
#include "UIPUNCH.h"
#include "UI1402.h"

//  TCardReader Class Implementation

//  Constructor.  Sets up the card reader

TCardReader::TCardReader(int devicenum, T1410Channel *Channel) :
    T1410IODevice(devicenum, Channel) {

    BusyEntry = new TBusyDevice();

	filename = L"";
    fd = NULL;

    ReadStation = NULL;
    CheckStation = NULL;
    StackStation = NULL;
    ReadBuffer = NULL;

    ready = eof = buffertransferred = false;
    readerstatus = column = 0;
}

//  IO Device Implementation

//  Select.  Returns 0 if successful, non 0 channel status otherwise

int TCardReader::Select() {

    int op;

    column = 1;                                 //  Reset column back to start

    //  Check that the card reader is ready, not busy, that we are doing
    //  either a read or unit control operation.  If not, return appropriate
    //  status.

    if(!IsReady()) {
        return(readerstatus = IOCHNOTREADY);
    }
    if(IsBusy()) {
        return(readerstatus = IOCHBUSY);
    }
    if(Channel -> ChWrite -> State()) {
        return(readerstatus = IOCHNOTREADY);
    }

    //  If this is a read operation (opcode M or L), and there is
    //  nothing in the read buffer, and eof is set, return IOCHCONDITION.

    //  If the eof switch is set, we can continue so long as there
    //  is at least a card at the Read Station.

    //  If the eof switch is not set, and there is no card at the
    //  check station, return not ready!

    //  If that is all OK, call SetUnit to fill the read buffer and start
    //  the card transport.

    //  Card transport for SSF is handled in the DoUnitControl method).

    readerstatus = 0;

    //  Nothing more to read, and EOF.  Stack the last card, go not ready,
    //  and return IOCHCONDITION status for M or L opcode.

    if(ReadStation == NULL && eof) {
        eof = false;
        if(StackStation != NULL) {
            StackStation -> Stack();
            delete StackStation;
            StackStation = NULL;
        }
        FI1402 -> ResetEOF();
        FI1402 -> SetReaderReady(false);
        ready = false;
        if((op = CPU -> Op_Reg -> Get().To6Bit()) == OP_IO_MOVE ||
            op == OP_IO_LOAD) {
            return(readerstatus = IOCHCONDITION);
        }
        return(readerstatus = IOCHNOTREADY);
    }

    //  If there is no card at the Check Station and the EOF switch isn't on,
    //  Go not ready, and return not ready.

    if(CheckStation == NULL && !eof) {
        FI1402 -> SetReaderReady(false);
        ready = false;
        return(readerstatus = IOCHNOTREADY);
    }

    //  ReadStation should not be null unless CheckStation is also null...

    assert(ReadStation != NULL);

    if(!SetUnit(Channel -> GetUnitNumber())) {
        return(readerstatus = IOCHNOTREADY);
    }

    //  Otherwise, AOK.  (Might be IOCHCONDITION from SetUnit() ).

    return(readerstatus);
}

//  Card reader unit control - used for Select Stacker and Feed instruction

void TCardReader::DoUnitControl(BCD opmod) {

    int hopper;

    hopper = opmod.To6Bit();

    switch(hopper) {

    case 0:
        hopper = HOPPER_R0;
        break;
    case 1:
        hopper = HOPPER_R1;
        break;
    case 2:
        hopper = HOPPER_R2;
        break;

	default:
        DEBUG("test without parameters");
        DEBUG("TCardReader::DoUnitControl invalid unit: %d",hopper);
        Channel -> UnitControlOverlapBusy = NULL;
        readerstatus |= IOCHNOTREADY;
        Channel -> ExtEndofTransfer = true;
        return;
    }

    TransportCard(hopper);                      //  Move cards along. Stack.
    buffertransferred = false;                  //  We now have data avail.
    Channel -> UnitControlOverlapBusy = BusyEntry;  //  Be busy for a while
    return;
}

//  Card Readers don't do ouptut very well, do they....

void TCardReader::DoOutput() {
    readerstatus |= IOCHNOTREADY;
}

//  Status sample is pretty simple for this device...

int TCardReader::StatusSample() {
    return(Channel -> GetStatus() | readerstatus);
}

//  The real meat - input!

void TCardReader::DoInput() {

    bool wm = false;                                //  True if wm in progress
    BCD c;                                          //  Column as BCD char
    int card_input_char;                            //  Column as BCD char too

    //  If the read buffer is empty, no transfer.
    //  (really, this should never happen!)

    if(ReadBuffer == NULL) {
        readerstatus |= IOCHNOTRANSFER;
        Channel -> ExtEndofTransfer = true;
        return;
    }

    //  Read the next column.  If return value is < 0, then something unusual
    //  happened.  channel status should already be set, so just return.

    card_input_char = DoInputColumn();              //  Get next column of data
    if(card_input_char < 0) {
        return;
    }

    //  Handle load mode

    if(Channel -> LoadMode && (card_input_char & 0x3f) == (BCD_WS & 0x3f)) {
        wm = true;
        card_input_char = DoInputColumn();          //  Read char after ws
        if(card_input_char < 0) {                   //  Ooops -- off the end
            return;
        }
        if((card_input_char & 0x3f) == (BCD_WS & 0x3f)) {    //  Another WS?
            wm = false;                             //  Yes.  Throw away WM
        }
    }

    //  Convert the character to BCD.  The DoInputColumn routine already
    //  sets IOCHDATACHECK for invalid characters...

    assert(card_input_char >= 0 && card_input_char <= 63);
    c = BCD(card_input_char);
    c.SetOddParity();
    if(wm) {
        c.SetWM();
        c.ComplementCheck();
    }

    //  Tell the channel we have something for it!

    Channel -> ChannelStrobe(c);
}

//  Method to handle setting the unit during the select process for Read

bool TCardReader::SetUnit(int u) {

    assert(ReadStation != NULL);

    switch(u) {

    case 0:
        u = HOPPER_R0;
        break;
    case 1:
        u = HOPPER_R1;
        break;
    case 2:
        u = HOPPER_R2;
        break;

    case 9:
        if(buffertransferred) {
            readerstatus |= IOCHNOTRANSFER;
            return(true);
        }
        ReadBuffer = ReadStation;
        return(true);

    default:
        return(false);
    }

    ReadBuffer = ReadStation;
    TransportCard(u);
    return(true);

}

//  Method to transport the cards in the card reader.  The card in the stacking
//  station gets stacked (and deleted).  The card that was just read gets its
//  hopper selected, and moves to the Stacking Station.
//  The card in the Check Station goes to the Reader Station, and we fill the
//  Check Station with the next card (NULL if none).

void TCardReader::TransportCard(int hopper) {

    if(StackStation != NULL) {
       StackStation -> Stack();
       delete StackStation;
       StackStation = NULL;
    }

    //  If there is a card to stack, stack it.  For the reader, the array
    //  indices for the hoppers (see T1410CHANNEL) match up.

    if(ReadStation != NULL) {
        ReadStation -> SelectStacker(Channel -> Hopper[hopper]);
    }

    FI1402 -> SetReaderCheck(false);
    FI1402 -> SetReaderValidity(false);
    BusyEntry -> SetBusy(2);
    StackStation = ReadStation;
    ReadStation = CheckStation;

    if(ready) {
        CheckStation = FeedCard();
    }
    else {
        CheckStation = NULL;
    }
}

//  Method to feed a card from the input hopper.  Returns NULL if there is
//  an EOF or I/O Error

TCard *TCardReader::FeedCard() {

    TCard *card;
    char temp[82];
    char *cp;
    int i;

    //  If the file isn't open, or we cannot allocate a card, return NULL

    if(fd == NULL || (card = new TCard()) == NULL) {
        return(NULL);
    }

    //  Read up to 80 columns, looking for newline.  If we hit EOF first,
    //  throw away the card.  Replace the newline with a blank.  We have
    //  to read up to 82 columns to get the newline.

    for(cp = temp; cp - temp < 82; ++cp) {
        if(fd -> Read(cp,1) != 1) {
            delete card;
            delete fd;
            fd = NULL;
            return(card = NULL);
        }
        if(*cp == '\n') {
            break;
        }
    }

    if(*cp != '\n') {
        DEBUG("TCardReader::FeedCard: No newline found within 82 characters");
    }

    //  If the character before the newline was a carriage return, throw it
    //  away as well.

    if(cp > temp && *(cp-1) == '\r') {
        *(cp-1) = ' ';
    }

    //  Change the newline and any trailing garbage to blank.

    for(; cp - temp < 80; ++cp) {
        *cp = ' ';
    }

    //  Finally, transfer the card image to the card object.

    for(i=0, cp=temp; i < 80; ++i, ++cp) {
        card -> image[i] = *cp;
    }
    return(card);
}

//  Method to read one card column.  Returns BCD character as an integer, so
//  that it also has a way to return EOF/Error status

int TCardReader::DoInputColumn() {

    int ch;

    //  If nothing in the buffer, say so.

    if(ReadBuffer == NULL) {
        readerstatus |= IOCHNOTRANSFER;
        Channel -> ExtEndofTransfer = true;
        return(-1);
    }

    //  If column is invalid, write to log, and take the reader offline.

    if(column < 1 || column > 81) {
        DEBUG("TCardReader::DoInputColumn: Invalid column: %d",column);
        ready = false;
        readerstatus |= IOCHNOTREADY;
        FI1402 -> SetReaderReady(false);
        Channel -> ExtEndofTransfer = true;
        return(-1);
    }

    //  If we have read all of the card, set end of transfer flag

    if(column == 81) {
        ++column;                               //  If it tries again, bad dog.
        Channel -> ExtEndofTransfer = true;
        return(-1);
    }

    //  Otherwise, grab a character from the card image.  Check that it is
    //  a valid character, and if not, signal a data check, and set the
    //  reader check light.

    assert(column > 0 && column <= 80);
    ch = ReadBuffer -> image[column-1];
    ++column;
    if(BCD::BCDCheck(ch) < 0) {
        readerstatus |= IOCHDATACHECK;
        ready = false;
        FI1402 -> SetReaderCheck(true);
        FI1402 -> SetReaderValidity(true);
    }
    ch = BCD::BCDConvert(ch);                       //  Invalid turns to alt b
    return(ch);
}

//  Interface Methods for User Interface

//  Method to load the card file

bool TCardReader::LoadFile(String s) {

	if(fd != NULL) {
		delete fd;
		fd = NULL;
	}

	if(s.Length() == 0) {
		return(false);
    }

    try {
        fd = new TFileStream(s /* .c_str() */,fmOpenRead);
    }
    catch(EFOpenError &e) {
        return(false);
    }

	// strncpy(filename,s,MAXPATH);
	filename = s;
    return(true);
}

//  Forced file close.  Necessary because (apparently) the file open
//  dialog checks and notices that the file is already open.  This prevents
//  reloading the same card deck twice in a row if we didn't have this method.

//  It also stacks all of the remaining cards in hopper 0.

void TCardReader::CloseFile() {

    if(fd != NULL) {
        delete fd;
        fd = NULL;
    }

    while(StackStation != NULL) {
        StackStation -> SelectStacker(Channel -> Hopper[0]);
        StackStation -> Stack();
        TransportCard(0);
    }
}

//  Method to handle the STOP button

void TCardReader::DoStop() {
    ready = false;
    eof = false;
}

//  Method to handle the START button

bool TCardReader::DoStart() {

    ready = false;

    //  Feed a card into the Check station if it is empty.

    if(CheckStation == NULL) {
        CheckStation = FeedCard();
    }

    //  If there is no card at the Read Station, and there is (now) a card
    //  at the Check Station, move it to the Read Station, and
    //  feed another card.

    if(ReadStation == NULL && (ReadStation = CheckStation) != NULL) {
        CheckStation = FeedCard();
    }

    //  If there is a card at the Check Station, or, if EOF is pressed,
    //  there is at least a card at the Read Station, all is OK.

    if(CheckStation != NULL || (eof && ReadStation != NULL)) {
        ready = true;
    }

    return(ready);
}

