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
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "ubcd.h"
#include "UI1410CPUT.h"
#include "UIHOPPER.h"
#include "UI1410CHANNEL.h"
#include "UI1410INST.h"
#include "UIPRINTER.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)

#include "UI1403.h"
#include "UI1410DEBUG.h"

//  Printer Adapter Unit Implementation.  Follows I/O Device Interface.

//  Constructor.  Creates a printer!  (NOTE:  RIGHT NOW, CAN ONLY BE 1 !!)

TPrinter::TPrinter(int devicenum, T1410Channel *Channel) :
    T1410IODevice(devicenum,Channel) {

    BusyEntry = new TBusyDevice();

    ccfd = NULL;
    ccline[0] = '\0';
    SetCarriageDefault();                       //  Default CC tape

    FileName[0] = '\0';
    fd = NULL;

    SkipLines = SkipChannel = 0;
    CarriageAdvance = false;
    BufferPosition = 0;

    PrintStatus = 0;
    Start();
}

//  Start.  Called from initialization and when Start button on printer
//  user interface is pressed.

void TPrinter::Start() {
    if(CarriageCheck) {
        return;
    }
    Ready = true;
    FI1403 -> LightPrintReady -> Enabled = true;
}

//  Stop.  Called when Stop button on printer user interface is pressed.

void TPrinter::Stop() {
    Ready = false;
    FI1403 -> LightPrintReady -> Enabled = false;
}

//  CheckReset: You guessed it: Called when the Check Reset button on the
//  printer user interface is pressed.

void TPrinter::CheckReset() {
    CarriageCheck = false;
}

//  Carriage Restore:  You know the routine.

bool TPrinter::CarriageRestore() {
    return(CarriageSkip(0,1));
}

//  Carriage Space.  This one does a little more, because it is also
//  called during Carriage Skip operations.

bool TPrinter::CarriageSpace() {

    bool status;

    CarriageAdvance = false;
    status = FI1403 -> NextLine();
    if(!status) {
        PrintStatus |= IOCHCONDITION;
    }

    if(fd != NULL) {
        if(!FileCapturePrint('\r') || !FileCapturePrint('\n')) {
            PrintStatus |= IOCHCONDITION;
            status = false;
        }
    }

    if(++FormLine > FormLength) {
        FormLine = 1;
    }
    return(status);
}

//  Carriage Stop

void TPrinter::CarriageStop() {
    Stop();
    CarriageCheck = true;
    FI1403 -> LightFormsCheck -> Enabled = true;
}

//  Method to assign a name to the capture file

bool TPrinter::FileCaptureSet(char *filename) {
    if(strlen(filename) >= MAXPATH) {
        return(false);
    }
    strcpy(FileName,filename);
    return(true);
}

//  And, finally, open the capture file...

bool TPrinter::FileCaptureOpen() {
    if(strlen(FileName) == 0) {
        return(false);
    }
    fd = new TFileStream(FileName,fmCreate | fmOpenWrite);
    return(fd != NULL);
}


//  Method to close down the capture file.

void TPrinter::FileCaptureClose() {
    if(fd != NULL) {
        delete fd;
        fd = NULL;
    }
}

//  Now, for the I/O Device Interface.

//  Select is called at the beginning of an IO operation.

int TPrinter::Select() {

    if(!Ready) {
        return(IOCHNOTREADY);
    }
    if(BusyEntry -> TestBusy()) {
        return(IOCHBUSY);
    }
    BufferPosition = 0;
    if(Channel -> ChRead -> State() ||
       (Channel -> GetUnitNumber() != 0 && Channel -> GetUnitNumber() != 1) ) {
        return(IOCHNOTREADY);
    }
    return(PrintStatus = 0);
}

//  Status Sample is called at the end of an I/O operation.

int TPrinter::StatusSample() {
    if(Channel -> ChWrite -> State() && BufferPosition != PRINTPOSITIONS) {
        PrintStatus |= IOCHWLRECORD;
    }
    return(Channel -> GetStatus() | PrintStatus);
}

//  Here is where the real work gets done...

void TPrinter::DoOutput() {

    BCD ch_char;

    //  If not ready, say so.

    if(!Ready) {
        PrintStatus |= IOCHNOTREADY;
        Channel -> ExtEndofTransfer = true;
        return;
    }

    //  If we have not advanced carriage since last print, do so now.
    //  (The real 1410 would do this at the end of the transfer, but
    //  that would require more buffer coordination in the simulator --
    //  a real printer would have already printed the data)

    if(CarriageAdvance) {
        CarriageSpace();
    }

    //  Get character from the channel silo

    ch_char = Channel -> ChR2 -> Get();

    //  If we have too many characters, indicate the problem

    if(BufferPosition >= PRINTPOSITIONS) {
        PrintStatus |= IOCHWLRECORD;
        Channel -> ExtEndofTransfer = true;
        return;
    }

    //  Check the parity of the character coming from memory

    if(!ch_char.CheckParity()) {
        PrintStatus |= IOCHDATACHECK;
    }

    //  In Load Mode, we turn Wordmarks into Word Separators.
    //  The addition of the word separator makes a Wrong Length Record very
    //  very likely!

    if(Channel -> LoadMode && ch_char.TestWM()) {
        DoOutputChar(BCD_WS);
        if(++BufferPosition >= PRINTPOSITIONS) {
            PrintStatus |= IOCHWLRECORD;
            Channel -> ExtEndofTransfer;
            return;
        }
    }

    //  Having handled Load Mode, we proceed.  For the normal print unit (%20),
    //  we now print the character.  If we are in Move Mode and have the
    //  alternate print unit (%21), we print a space, because we have already
    //  stripped the WM.  (Not sure this is how it really worked, but the
    //  Principles of Operation indicates that a L%21 prints a blank line).

    if(Channel -> GetUnitNumber() == 0) {
        DoOutputChar(ch_char);
    }
    else {  // %21
        DoOutputChar(Channel -> MoveMode && ch_char.TestWM() ?
            BCD_1 : BCD_SPACE);
    }
    ++BufferPosition;

    //  Check for end of transfer.  If we have need more characters,
    //  let the Channel know.  Otherwise, let the Channel know we have
    //  had enough.

    //  Also, this is normally how we will know we are ready to print the
    //  line.  (A Wrong Length Record can suppress output, according to the
    //  Principles of Operation).  It is easier to do it here than wait to
    //  do it a StatusSample time - which has to service both Print and
    //  Carriage Control operations.  After we print the line, we might
    //  also have a deferred Carriage Control operation to do.

    if(BufferPosition == PRINTPOSITIONS) {
        EndofLine();
        if(SkipLines || SkipChannel) {
            CarriageSkip(SkipLines, SkipChannel);
            SkipLines = SkipChannel = 0;
        }
        else {
            CarriageAdvance = true;                 //  No space -- yet
        }
        BusyEntry -> SetBusy(2);
    }

    //  Minor kludge.  We ask for another character, even if we just
    //  printed.  If there is one, then we will detect WLR.

    Channel -> OutputRequest = true;
    Channel -> CycleRequired = true;
}

//  Send the output character to the appropriate device.

void TPrinter::DoOutputChar(BCD c) {

    c = c & (BIT_NUM | BIT_ZONE);
    if(!FI1403 -> SendBCD(c)) {
        PrintStatus |= IOCHCONDITION;
    }
    if(fd != NULL) {
        if(!FileCapturePrint(c.ToAscii())) {
            PrintStatus |= IOCHCONDITION;
        }
    }
}

//  Send the word to the appropriate device that the line is complete.

void TPrinter::EndofLine() {

    if(!FI1403 -> EndofLine()) {
        PrintStatus |= IOCHCONDITION;
    }

    if(fd != NULL && !FileCapturePrint('\r')) {
        PrintStatus |= IOCHCONDITION;
    }

}

bool TPrinter::FileCapturePrint(char c) {

    if(fd == NULL) {
        return(false);
    }
    if(fd -> Write(&c,1) != 1) {
        DEBUG("TPrinter::FileCapturePrint Error",0);
        return(false);
    }
    return(true);
}

//  Carriage Control operations.  Basically pretty simple stuff.

void TPrinter::DoUnitControl(BCD opmod) {

    Channel -> UnitControlOverlapBusy = NULL;
    if(!Ready) {
        PrintStatus |= IOCHNOTREADY;
    }
    else {
        ControlCarriage(opmod);
        BusyEntry -> SetBusy(2);
    }
    Channel -> ExtEndofTransfer = true;
}

void TPrinter::ControlCarriage(BCD opmod) {

    int dint;
    int num;

    dint = opmod.To6Bit();
    num = dint & BIT_NUM;

    //  A '-' suppresses the automatic space operation.

    if(dint == OP_MOD_SYMBOL_MINUS) {
        CarriageAdvance = false;
    }

    //  Single spaces get handled by default (may change later)

    if(num == 0) {
        return;
    }

    SkipChannel = SkipLines = 0;

    //  0 - 9, #, @: Skip to carriage tape channels 1 - 12

    if((dint & BIT_ZONE) == 0) {
        if(num > 12) {
            return;
        }
        CarriageSkip(0,num);
    }

    //  A -I, ? . <lozenge>:  Skip to channels 1-2 *after* printing

    else if((dint & BIT_ZONE) == BIT_ZONE) {
        if(num <= 12) {
            SkipChannel = num;
        }
    }

    //  J, K, L: Immediate space of 1, 2 or 3 lines

    else if((dint & BIT_ZONE) == BITB) {
        if(num > 3) {
            return;
        }
        CarriageSkip(num,0);
    }

    //  /, S, T: Skip 1, 2 or 3 lines *after* printing

    else {
        if(num > 3) {
            return;
        }
        SkipLines = num;
    }
}

//  Printers ignore input.  (Actually, this should never get called, because
//  select will return Not Ready on an attempt to select for input).

void TPrinter::DoInput() {
    PrintStatus |= IOCHNOTREADY;
    return;
}

//  PRIVATE routine to skip carriage to given channel or given number of lines.

bool TPrinter::CarriageSkip(int spaces, int chan) {

    int line;

    if(spaces < 0 || spaces > 4 || chan < 0 || chan > 12) {
        CarriageStop();
        return(false);
    }

    if(spaces > 0 && spaces < 4) {
        while(spaces--) {
            CarriageSpace();
        }
        return(true);
    }

    if(chan == 0) {
        return(true);
    }

    for(line=0; line <= FormLength; ++line) {
        CarriageSpace();
        if(CarriageChannelTest(chan)) {
            return(true);
        }
    }

    CarriageStop();
    return(false);
}

bool TPrinter::CarriageChannelTest(int ch) {
    return((CarriageTape[FormLine] & (1 << (ch-1))) != 0);
}

//  The rmaining code has to do with setting up the carriage control tape.
//  A carriage control tape is read in from a file using the same format
//  as the Newcomer 1401 simulator.

//  * beginning a line indicates a comment
//  LENGTH ###
//  CHAN # = # [[+]#] ...

//  Method to set up a default carriage tape, during construction

void TPrinter::SetCarriageDefault() {

    int line;

    for(line=0; line < PRINTMAXFORM; ++line) {
        CarriageTape[line] = 0;
    }

    FormLength = 66;
    FormLine = 1;
    CarriageTape[4] = 1 | 2 | 4 | 8 | 16 | 32 | 64 | 128 | 512 | 1024;
    //  Channesl      1   2   3   4    5    6    7    8     10     11

    CarriageTape[61] = 256;     //  Channel 9
    CarriageTape[63] = 2048;    //  Channel 12
}

//  Main method for carriage tape - sets up carriage tape from a file
//  Returns 0 if tape is OK.  Returns - value if an error.  The value
//  is in fact the line number.

int TPrinter::SetCarriageTape(char *filename) {

    char *elements[PRINTMAXTOKENS];               //  Up to 80 fields per line.
    int line, chan, i, n;

    for(line=0; line < PRINTMAXFORM; ++line) {
        CarriageTape[line] = 0;
    }

    //  Passing us no file is OK: Means they want the default carriage tape.

    if(filename == NULL || strlen(filename) == 0) {
        SetCarriageDefault();
        return(0);
    }

    assert(ccfd == NULL);
    line = 0;

    //  Try and open the file.  If it fails, report an error on line 1.

    try {
        ccfd = new TFileStream(filename,fmOpenRead);
    }
    catch(EFOpenError &e) {
        return(CarriageTapeError(-1));
    }

    //  Skip any leading comment lines.  Error if we hit EOF.

    do {
        if(!GetCarriageLine()) {
            return(CarriageTapeError(-line));
        }
        ++line;
    } while(ccline[0] == '*');

    //  Parse the line.  It should have 2 elements.  LENGTH and the number

    ParseCarriageLine(ccline,elements);
    if(strcmp(elements[0],"LENGTH") != 0) {
        return(CarriageTapeError(-line));
    }
    FormLength = atoi(elements[1]);
    if(FormLength < 1 || FormLength > PRINTMAXFORM) {
        return(CarriageTapeError(-line));
    }

    //  Now process the carriage control lines themselves.

    n = 0;
    while(GetCarriageLine()) {
        ++line;
        if(ccline[0] == '*') {                      //  * for comment
            continue;
        }
        ParseCarriageLine(ccline,elements);
        if(strcmp(elements[0],"CHAN") != 0 ||       //  Check CHAN # =
           strcmp(elements[2],"=") != 0) {
           return(CarriageTapeError(-line));
        }
        chan = atoi(elements[1]);
        if(chan < 1 || chan > 12) {                 //  Validate channel number
            return(CarriageTapeError(-line));
        }
        n = 0;
        for(i=3; elements[i] != NULL; ++i) {        //  Process the form lines
            if(*elements[i] == '+') {
                n += atoi(elements[i]+1);
            }
            else {
                n = atoi(elements[i]);
            }
            if(n < 1 || n > FormLength) {
                CarriageTapeError(-line);
            }
            CarriageTape[n] |= (1 << (chan - 1));
        }
    }

    FormLine = 1;
    delete ccfd;
    ccfd = NULL;
    return(0);
}

//  Utility method to clean up after carriage tape errors

int TPrinter::CarriageTapeError(int rc) {

    if(ccfd != NULL) {
        delete ccfd;
    }
    ccfd = NULL;
    SetCarriageDefault();
    return(rc);
}

//  Method to get one line of input.  (Why Borland didn't have this kind of
//  method as part of their file stream object I have *no* idea!

bool TPrinter::GetCarriageLine() {

    char *cp;

    if(ccfd == NULL) {
        return(false);
    }

    for(cp = ccline; cp - ccline < PRINTCCMAXLINE; ++cp) {
        if(ccfd -> Read(cp,1) != 1) {
            return(false);
        }
        if(*cp == '\n') {
            *cp = 0;
            return(true);
        }
    }

    ccline[PRINTCCMAXLINE] = 0;
    return(false);
}

//  Method to set pointers to the tokens in a line.

void TPrinter::ParseCarriageLine(char *line, char **element) {

    int i=0;
    element[0] = NULL;

    //  Ignore leading white space to find first token

    while(*line && isspace(*line)) {
        ++line;
    }

    //  Process tokens one at a time until end of line.

    while(*line && i < PRINTMAXTOKENS-1) {
        element[i] = line;
        element[i+1] = NULL;
        while(*++line && !isspace(*line)) {
            //  Skip over rest of the token
        }
        if(!*line) {
            break;
        }
        *line = 0;                              //  Terminate token with '\0'
        while(*++line && isspace(*line)) {
            //  Skip over white space
        }
        ++i;                                    //  Bump to next element
    }
}

