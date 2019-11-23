//---------------------------------------------------------------------------
#ifndef UITAPETAUH
#define UITAPETAUH
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

#define TAPE_IO_DEVICE  20

#define UNIT_BACKSPACE  50
#define UNIT_SKIP       53
#define UNIT_REWIND     41
#define UNIT_REWIND_UNLOAD     20
#define UNIT_WTM        36
#define UNIT_SPACE      49

//  1410 Tape Adapter Unit

class TTapeTAU : public T1410IODevice {

protected:

    TTapeUnit *Unit[10];                            //  Units 0 thru 9
    TTapeUnit *TapeUnit;                            //  Current selected unit

    int tapestatus;                                 //  Channel status
    BCD ch_char;                                    //  Channel character

    char tape_char;                                 //  Tape Unit character
    int tape_read_char;                             //  Before checking status

    long chars_transferred;                         //  Storage characters
    char *tape_parity_table;


public:

    //  Implement the I/O device interface standard

    TTapeTAU(int devicenum, T1410Channel *Channel);
    virtual int Select();
    virtual void DoOutput();
    virtual void DoInput();
    virtual int StatusSample();
    virtual void DoUnitControl(BCD opmod);

    //  State and status methods

    TTapeUnit *GetUnit(int unit);

private:

    //  Internal methods.

    bool DoOutputWrite(char c);
    int DoInputRead();
};


#endif
