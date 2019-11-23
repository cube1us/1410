//---------------------------------------------------------------------------
#ifndef UITAPEUNITH
#define UITAPEUNITH
//---------------------------------------------------------------------------

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

//  Define constants returned by Read()

#define TAPEUNITIRG     (-1)
#define TAPEUNITEOF     (-2)
#define TAPEUNITNOTREADY    (-3)
#define TAPEUNITERROR   (-4)

#define TAPE_IRG        0x80
#define TAPE_TM         0x0f

class TTapeUnit : public TObject {

protected:

    int unit;                                       //  Unit number.

    char filename[MAXPATH];                         //  Path to tape file
    TFileStream *fd;                                //  File Descripter for I/O
    char tape_buffer;                               //  Char read from file

    int record_number;                              //  Record number (0=bot)

    bool loaded;                                    //  State flags
    bool fileprotect;
    bool ready;
    bool selected;
    bool tapeindicate;
    bool highdensity;
    bool bot;

    bool irg_read;                                  //  True if tape_buffer full
    bool write_irg;                                 //  Write IRG on next write
    bool modified;

    TBusyDevice *BusyEntry;

private:

    int ReadNextChar();                             //  Factored I/O call
    void ResetFile();                               //  Close file, reset flags

public:

    //  Functions to make part of state visible to the outside

    inline bool Selected() { return selected; }
    inline bool IsReady() { return ready; }
    inline bool IsFileProtected() { return fileprotect; }
    inline bool TapeIndicate() { return tapeindicate; }
    inline void ResetIndicate() { tapeindicate = false; }
    inline bool HighDensity() { return highdensity; }
    inline char *GetFileName() { return filename; }
    inline bool IsLoaded() { return loaded; }
    inline bool IsBusy() { return BusyEntry -> TestBusy(); }
    TBusyDevice *GetBusyDevice() { return BusyEntry; }
    inline bool IsAtBot() { return bot; }
    inline int GetRecordNumber() { return record_number; }

    //  Interface functions for the User Interface buttons

    bool LoadRewind();
    bool Reset();
    bool Unload();
    bool Start();
    bool ChangeDensity();
    bool Mount(char *filename);

    //  Interface functions for the Tape Adapter Unit (TAU) to use

    TTapeUnit(int u);                               //  Constructor
    void Init(int u);                               //  Initialization

    bool Select(bool b);                            //  Select or De-select unit
    bool Rewind();
    bool RewindUnload();
    bool Backspace();
    bool Skip();                                    //  Skip and blank tape
    void WriteIRG();                                //  Mark end of record
    bool WriteTM();                                 //  Write Tape Mark (EOF)
    int Space();                                    //  Space fwd 1 record

    int Read();                                     //  Returns char or -value
    bool Write(char c);                             //  Write file character
};

#endif
