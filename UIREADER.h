//---------------------------------------------------------------------------
#ifndef UIREADERH
#define UIREADERH
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

#define READER_IO_DEVICE 1

//  Class to implement the card reader interface and buffer



class TCardReader : public T1410IODevice {

protected:

	TCard *ReadStation;                         //  Station to read the card
	TCard *CheckStation;                        //  Where feed goes to
	TCard *StackStation;                        //  Card after read
	TCard *ReadBuffer;                          //  Where the read card goes

	String filename;			                //  Input hopper, if you will
	TFileStream *fd;                            //  Input hopper file stream

	int column;                                 //  Current column.  1-80, 81

    bool ready;                                 //  True if reader ready
    bool eof;                                   //  True if EOF switch on
    bool buffertransferred;                     //  True if no data at Check Stn

    int unit;                                   //  Hopper number: 0, 1, 2

    int readerstatus;                           //  Channel status value
    TBusyDevice *BusyEntry;

public:

    //  Implement the I/O device interface standard

    TCardReader(int devicenum, T1410Channel *Channel);
    virtual int Select();
    virtual void DoOutput();
    virtual void DoInput();
    virtual int StatusSample();
    virtual void DoUnitControl(BCD opmod);

    //  State and status methods

    inline bool IsReady() { return ready; }
    inline void SetEOF() { eof = true; }
    inline void ResetEOF() { eof = false; }
    inline bool IsBusy() { return BusyEntry -> TestBusy(); }
    inline String GetFileName() { return filename; }
    bool SetUnit(int u);                        //  Check unit validity, set
    inline int GetUnit() { return unit; }

	bool LoadFile(String s);		            //  Call to open input file
    void CloseFile();                           //  Force a file close if open
    bool DoStart();                             //  Process Start Button
	void DoStop();                              //  Process Stop Button

private:

    void TransportCard(int hopper);             //  Transport card to hopper
    TCard *FeedCard();                          //  Feed card from input file
    int DoInputColumn();                        //  Process one card column

};



#endif
