//---------------------------------------------------------------------------
#ifndef UIPUNCHH
#define UIPUNCHH
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

#define PUNCH_IO_DEVICE 4

class TPunch : public T1410IODevice {

protected:

    int PunchStatus;                                //  Punch status for channel
    bool Ready;

    TCard *PunchBuffer;                             //  Current output card
    int column;                                     //  Current output column

    TBusyDevice *BusyEntry;                         //  Used to set delays

public:

    //  Implement the I/O device interface standrad

    TPunch(int devicenum, T1410Channel *Channel);
    virtual int Select();
    virtual void DoOutput();
    virtual void DoInput();
    virtual int StatusSample();
    virtual void DoUnitControl(BCD opmod);

    //  State and status methods

    inline bool IsReady() { return Ready; }
    inline bool IsBusy() { return BusyEntry -> TestBusy(); }

    bool SetUnit(int u);                            //  Check unit validity, set

    bool DoStart();                                 //  Process Start button
    bool DoStop();                                  //  Process Stop button
    void DoOutputChar(BCD c);                       //  Punch a column
};


#endif
