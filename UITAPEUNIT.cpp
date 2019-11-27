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
#include "UITAPEUNIT.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)

#include "UI1410DEBUG.h"

#define TAPEDEBUG   1

//  Tape Unit Implementation.

//  Constructor

TTapeUnit::TTapeUnit(int u) {

    fd = NULL;
    BusyEntry = new TBusyDevice();                  //  Create a busy list entry
    Init(u);                                        //  Let common init take over
}

//  Initialization (not sure if anyone else will ever use this)

void TTapeUnit::Init(int u) {

    if(fd != NULL) {
        delete fd;
    }

    unit = u;
    fd = NULL;
    loaded = fileprotect = tapeindicate = ready = selected = bot = false;
    write_irg = irg_read = modified = false;
    highdensity = true;
    filename[0] = '\0';
    record_number = 0;
    BusyEntry -> SetBusy(0);                        //  Set not busy.
    return;
}

//  Method to reset file, if open, and reset flags.  Typically called
//  after an error of some sort.

void TTapeUnit::ResetFile() {

    if(fd != NULL) {
        delete fd;
    }

    fd = NULL;
    ready = loaded = fileprotect = bot = false;
    irg_read = write_irg = modified = false;
    record_number = 0;
    return;
}


//  Methods that interface to user interface buttons

bool TTapeUnit::Reset() {
    ready = false;
    return(true);
}

//  Load the tape (file) (if not already loaded) and rewind.

bool TTapeUnit::LoadRewind() {

    if(ready) {                                     //  Inop if drive is ready
        return(false);
    }

    if(modified && write_irg) {                     //  Write a closing 0 + IRG
        assert(fd != NULL);
        Write(0);
        modified = false;
    }

    if(fd != NULL) {                                //  If loaded, just rewind

        try {
            fd -> Seek(0,soFromBeginning);
            irg_read = modified = false;
            write_irg = true;
            record_number = 0;
            return(bot = loaded = true);
        }

        catch(char *dummy) {
            DEBUG("LoadRewind: Seek on failed on tape unit %d",unit);
            ResetFile();
            return(false);
        }

    }
    else if(strlen(filename) == 0) {
        return(false);
    }

    //  Open the file.  First try RW.  If that fails, try RO and set fileprot.

    try {
        fd = new TFileStream(filename,fmOpenReadWrite);
        fileprotect = false;
    }

    catch(EFOpenError &e) {

        try {
            fd = new TFileStream(filename,fmOpenRead);
            fileprotect = true;                         //  Read Only was OK
        }

        catch(EFOpenError &e) {
            DEBUG("LoadRewind: open failed on tape unit %d",unit);
            DEBUG(e.Message.c_str(),0);
            ResetFile();
            return(false);
        }
    }

    irg_read = modified = false;
    write_irg = true;
    record_number = 0;
    return(bot = loaded = true);
}

//  Unload the tape (file)

bool TTapeUnit::Unload() {

    if(ready || !loaded) {                          //  If ready, ignore.
        return(false);
    }

    ResetFile();                                    //  Handles most of the work
    tapeindicate = false;
    return(true);
}

//  Mount a tape on the drive (associate a file)

bool TTapeUnit::Mount(char *fname) {

    if(ready || loaded) {                           //  If ready or already
        return(false);                              //  loaded, ignore it.
    }

    if(strlen(fname) == 0 || strlen(fname)+1 > sizeof(filename)) {
        return(false);
    }
    assert(fd == NULL);
    strcpy(filename,fname);
    irg_read = write_irg = fileprotect = tapeindicate = bot = modified = false;

    return(true);
}

//  Start Button

bool TTapeUnit::Start() {

    if(ready || !loaded) {                          //  If ready or not loaded
        return(false);                              //  can't help you!
    }

    assert(fd != NULL);

    return(ready = true);
}

bool TTapeUnit::ChangeDensity() {
    if(ready) {
        return(false);
    }
    highdensity = !highdensity;
    return(true);
}

//  Methods that interface with the Tape Adapter Unit (TAU)

bool TTapeUnit::Select(bool b) {
    selected = b;
#ifdef TAPEDEBUG
    if(fd != NULL && !BusyEntry -> TestBusy()) {
        DEBUG("TTapeUnit unit %d selected",unit);
        DEBUG("TTapeUnit Current file offset is %d",fd -> Position);
    }
#endif
    return(true);
}

//  Rewind to beginning of tape (file)

bool TTapeUnit::Rewind() {

    if(!selected || !loaded || !ready) {            //  Must be ready to go...
        DEBUG("TTapeUnit::Rewind: Unit %d not selected or not ready",unit);
        return(false);
    }

#ifdef TAPEDEBUG
    DEBUG("Rewind unit %d",unit);
#endif

    assert(fd != NULL);

    if(modified && write_irg) {                           //  Mark end of record
        if(!Write(0)) {
            return(false);
        }
        write_irg = modified = false;
    }

    try {
        fd -> Seek(0,soFromBeginning);
    }

    catch(char *dummy) {
        DEBUG("Rewind: Seek failed, unit %d",unit);
        ResetFile();
        return(false);
    }

    irg_read = modified = false;
    write_irg = true;
    BusyEntry -> SetBusy(5*record_number);            //  Act like we are busy
    record_number = 0;
    return(bot = true);
}

//  Rewind and unload the tape (close the file)

bool TTapeUnit::RewindUnload() {

    if(!Rewind()) {                                     //  If rewind fails...
        ResetFile();
        return(false);
    }

    ResetFile();
    tapeindicate = false;
    return(true);
}

//  Skip and blank tape, does nothing for now (until we have measured tape)

bool TTapeUnit::Skip() {

#ifdef TAPEDEBUG
    DEBUG("Write IRG (Skip) unit %d",unit);
#endif

    if(!selected || !loaded || !ready) {
        return(false);
    }
    write_irg = true;
    return(true);
}

//  Space forward.  (d-character is "A" - not in my Principles of Operation!)
//  Basically pretty easy: all we have to do is call read until we get a
//  negative return code, which will happen at EOF or IRG or an error.
//  We can let the Tape Adapter Unit figure out the status.

int TTapeUnit::Space() {

    int rc;

#ifdef TAPEDEBUG
    DEBUG("Space unit %d",unit);
#endif

    while((rc = Read()) >= 0) {
        //  Do nothing.
    }
    BusyEntry -> SetBusy(2);                    //  Must go busy for a while
    return(rc);
}

//  Backspace.  This one is a pain.  To do it, we take two steps back, one
//  forward.  Repeatedly.  Slow.  Oh well....

bool TTapeUnit::Backspace() {

    if(!selected || !ready || !loaded) {
        DEBUG("TTapeUnit::Backspace: Unit %d not selected or not ready",unit);
        return(false);
    }

    assert(fd != NULL);

#ifdef TAPEDEBUG
    DEBUG("Backspace start: %d",fd -> Position);
#endif

    if(bot) {                                           //  If at BOT, a NOP
        record_number = 0;
        return(true);
    }

    BusyEntry -> SetBusy(2);                            //  If not a BOT, go busy

    //  If we just ended a record, write out its IRG, then back up before it.

    if(modified && write_irg) {
        if(!Write(0)) {
            ResetFile();
            return(false);
        }
        modified = write_irg = irg_read = false;

        try {
            fd -> Seek(-1,soFromCurrent);
        }

        catch(char *dummy) {
            DEBUG("Backspace: Seek over EOR failed on unit %d",unit);
            ResetFile();
            return(false);
        }

    }

    //  Now, go into the two steps back, one step forward routine.

    while(true) {                                       //  Start the dance...

        //  Seek back 2 characters

        try {
            fd -> Seek(-2,soFromCurrent);
        }

        catch(char *dummy) {
            DEBUG("Backspace: Seek failed on unit %d",unit);
            ResetFile();
            return(false);
        }

        //  If beginning of file, done! (special case)

        if(fd -> Position == 0) {
            irg_read = false;
            bot = write_irg = true;
            record_number = 0;
#ifdef TAPEDEBUG
            DEBUG("Backspace end at BOT",0);
#endif
            return(true);
        }

        //  Read forward 1 character.  Quit on error or EOF

        if(fd -> Read(&tape_buffer,1) != 1) {
            DEBUG("Backspace: Read failed: unexpected eof on unit %d",unit);
            return(false);
        }

        //  If we find an IRG bit on, we are done!  Have to leave the IRG
        //  bit on the character, though.

        if(tape_buffer & TAPE_IRG) {
            irg_read = write_irg = true;
            --record_number;
#ifdef TAPEDEBUG
            DEBUG("Backspace end: %d",fd -> Position);
#endif
            return(true);
        }
    }
}

//  Method to write a character.  Note that by this time any cute stuff
//  (like parity, changing wordmarks in to word separators, changing
//  word separators into two word separators, etc. should have already been
//  handled in the TAU

bool TTapeUnit::Write(char c) {

    if(!loaded || !ready || !selected || fd == NULL) {
        DEBUG("TapeUnit::Write: Unit %d not ready or selected",unit);
        return(false);
    }

    if(fileprotect) {
        DEBUG("TapeUnit::Write: Attempt to write when file protected, unit %d",
            unit);
        return(false);
    }

    //  If we have an irg left over from a previous read, back up over it.

    if(irg_read) {
        try {
            DEBUG("Write seeking back over EOR from: %d",fd -> Position);
            fd -> Seek(-1,soFromCurrent);
            DEBUG("Write seeking back over EOR to: %d",fd -> Position);
            irg_read = false;
            write_irg = modified = true;           //  Set modified to write IRG
        }

        catch(char *dummy) {
            DEBUG("Write: Seek over EOR failed on unit %d",unit);
            ResetFile();
            return(false);
        }
    }

    //  If we have an IRG left to do from a previous write,
    //  or, if the last operation was a read, set the IRG bit.

    if(write_irg) {
        c |= TAPE_IRG;
    }


    if(fd -> Write(&c,1) != 1) {
        DEBUG("TapeUnit::Write: File I/O error writing on unit %d",unit);
        ResetFile();
        tapeindicate = true;
        return(false);
    }

    irg_read = write_irg = false;

    return(true);
}

//  Mark end of record.  Called at the end of a transfer by the TAU.
//  All this does is set the IRG flag for the start of the next record.

void TTapeUnit::WriteIRG() {
    write_irg = modified = true;
    bot = irg_read = false;
    ++record_number;

#ifdef TAPEDEBUG
    DEBUG("TTapeUnit::WriteIRG unit %d",unit);
    if(fd != NULL) {
        DEBUG("TTapeUnit Current file position is %d",fd -> Position);
    }
#endif

    return;
}

//  Write tape mark.  Just calls write to do the dirty work...
//  Note that tape marks are *always* even parity.  Since this writes
//  an IRG and flushes, it also clears the modified flag.

bool TTapeUnit::WriteTM() {
    bool status;

    ++record_number;

#ifdef TAPEDEBUG
    DEBUG("Write TM %d",unit);
#endif

    if(!Write(TAPE_TM | TAPE_IRG)) {
        return(false);
    }
    irg_read = modified = bot = false;
    status = Write(TAPE_TM);
    write_irg = modified = true;

#ifdef TAPEDEBUG
    DEBUG("Write TM end: %d",fd -> Position);
#endif

    BusyEntry -> SetBusy(2);                       //  Go busy

    return(status);
}

//  Read a character.

int TTapeUnit::Read() {

    int rc;

    if(!loaded || !ready || !selected || fd == NULL) {
        DEBUG("TapeUnit::Read: Unit not ready or selected: %d",unit);
        return(TAPEUNITNOTREADY);
    }

    //  Read a character, unless the last read resulted in an IRG, in
    //  which case it is already in the buffer.
    //  Interesting statuses are negative, just bubble them on up.

    if(!irg_read) {
        if((rc = ReadNextChar()) < 0) {
            return(rc);
        }
        tape_buffer = (char) rc;
    }

    //  If we are at bot or an IRG, strip the IRG bit from the char.
    //  We will then just return that char.

    //  A Tape Mark is only a Tape Mark as the first character.  Also,
    //  when we first see it at the end of a record, it just denotes the
    //  IRG.  (Which means we need to save it in tape_buffer, too)

    if(bot || irg_read) {
        tape_buffer &= (char) (~TAPE_IRG);
        if((tape_buffer & 0x3f) == TAPE_TM) {
            tapeindicate = true;
        }
        bot = irg_read = false;
    }

    //  If we hit the end of the record, then set irg now, and return such

    if(tape_buffer & TAPE_IRG) {
#ifdef TAPEDEBUG
        DEBUG("TTapeUnit::Read: Found IRG character at %d", fd -> Position);
#endif
        irg_read = true;
        bot = false;
        ++record_number;
        return(TAPEUNITIRG);
    }

    //  Aw shucks, just return the blinkin' character already!

    //  Clear irg at this point too, becuase if we had an IRG bit,
    //  it got handled in the preceeding IF.  If we hit EOF, the value
    //  in the character is a tape mark with an IRG.

    irg_read = false;
    return(tape_buffer);
}

//  Utility method to read a character from the file, and handle a few odds
//  and ends.  Keeps us from having to do it all more than once...

int TTapeUnit::ReadNextChar() {

    unsigned char c;

    write_irg = true;                           //  Next write must write IRG

    if(fd -> Read(&c,1) != 1) {
        DEBUG("TapeUnit::ReadNextChar: Error or EOF in Read, unit %d",unit);
        return(TAPE_TM | TAPE_IRG);
    }
    return(c);
}


