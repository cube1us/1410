//---------------------------------------------------------------------------
#ifndef UIPRINTERH
#define UIPRINTERH
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

#define PRINTMAXFORM    1024
#define PRINTMAXTOKENS  80
#define PRINTCCMAXLINE  256

#define PRINTER_IO_DEVICE   2

//  Printer Adapter Unit (1414)

class TPrinter : public T1410IODevice {

protected:

    int PrintStatus;                            //  Printer Status for Channel
    bool Ready;                                 //  True if ready to go
    bool CarriageCheck;                         //  True if runaway forms
    TBusyDevice *BusyEntry;                     //  Used to set delays
    int BufferPosition;                         //  Current output column

    int SkipLines;                              //  Deferred Skip in lines
    int SkipChannel;                            //  Deferred Skip to Channel
    bool CarriageAdvance;                       //  True to auto advance carriage

    int FormLength;                             //  Length of current form
    int FormLine;                               //  Current line in form

    TFileStream *ccfd;                          //  Carriage Control File
    int CarriageTape[PRINTMAXFORM];             //  Carriage tape data
    char ccline[PRINTCCMAXLINE];

    char FileName[MAXPATH];                     //  File name, if to file
    TFileStream *fd;                            //  File FDs for print, CC file

public:

    TPrinter(int devicenum, T1410Channel *Channel);

    inline bool IsReady() { return(Ready); }
    inline bool IsBusy() { return BusyEntry -> TestBusy(); }

    void Start();                               //  Called from UI Start Button
    void Stop();                                //  Called from UI Stop Button
    void CheckReset();                          //  Called from UI Check Reset

    bool CarriageRestore();                     //  Carriage to Channel 1
    void CarriageStop();                        //  Called from UI Carriage Stop
    bool CarriageSpace();                       //  Space to next line

    virtual int Select();                       //  Channel Select
    virtual void DoOutput();                    //  Character to output
    virtual void DoInput();                     //  NOP on this device
    virtual int StatusSample();                 //  End of I/O status sample
    virtual void DoUnitControl(BCD opmod);      //  CC operation

    void ControlCarriage(BCD opmod);

    void DoOutputChar(BCD c);                   //  Send one char of output
    void EndofLine();                           //  Flush line to device.

    int SetCarriageTape(char *tapefile);        //  Set up carriage control
    void SetCarriageDefault();                  //  Set default CC tape
    bool CarriageChannelTest(int cchannel);     //  True if this line has chan.

    void FileCaptureClose();                    //  Terminate file capture
    bool FileCaptureSet(char *filename);        //  Set capture file name
    bool FileCaptureOpen();                     //  Open capture filename
    bool FileCapturePrint(char c);              //  Print a character to file

private:

    bool CarriageSkip(int lines, int channel);
    bool GetCarriageLine();
    void ParseCarriageLine(char *line, char **elements);
    int CarriageTapeError(int rc);              //  Cleans up after tape errors

};

#endif
