//---------------------------------------------------------------------------
#ifndef UI1410CHANNELH
#define UI1410CHANNELH
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

//	This class defines what is in an I/O Channel

#define	IOCHNOTREADY	1
#define IOCHBUSY		2
#define IOCHDATACHECK	4
#define IOCHCONDITION	8
#define IOCHNOTRANSFER	16
#define IOCHWLRECORD	32

#define IOLAMPNOTREADY	0
#define IOLAMPBUSY		1
#define IOLAMPDATACHECK	2
#define IOLAMPCONDITION	3
#define IOLAMPNOTRANSFER 4
#define IOLAMPWLRECORD	5

#define PROVERLAP       1
#define PRIOUNIT        2
#define PRINQUIRY       4
#define PROUTQUIRY      8
#define PRSEEK          16
#define PRATTENTION     32

//  Forward declaration of I/O Device class, which T1410Channel uses.

class T1410IODevice;

class T1410Channel : public TDisplayObject {

public:

    void Reset();

    //	Functions inherited from abstract base class classes now need definition

    void OnComputerReset();
    void OnProgramReset();
    void Display();
    void LampTest(bool b);

private:

	// Channel information

	int	ChStatus;							// Channel status (see defines)
    TLabel *ChStatusDisplay[6];				// Channel status lights
    bool R1Status,R2Status;                 // State of R1, R2.  True if full
    class T1410IODevice *Devices[64];       // Pointer to devices

public:

	TRegister *ChOp;
    TRegister *ChUnitType;
    TRegister *ChUnitNumber;
    TRegister *ChR1, *ChR2;
    TAddressRegister *ChAddr;               //  Points to EAR or FAR

    TDisplayLatch *ChInterlock;
    TDisplayLatch *ChRBCInterlock;
    TDisplayLatch *ChRead;
    TDisplayLatch *ChWrite;
    TDisplayLatch *ChOverlap;
    TDisplayLatch *ChNotOverlap;

    bool MoveMode, LoadMode;                // Mode: Without/With WM
    bool IntEndofTransfer,ExtEndofTransfer; // Transfer end flags
    bool CycleRequired;                     // Request for input transfer
    bool InputRequest;                      // Not in real machine - for input
    bool OutputRequest;                     // Not in real machine - for output
                                            // (co-routines)
    bool LastInputCycle;                    // Indicate last cycle of input
    bool EndofRecord;                       // Indicate record has ended
    bool TapeIndicate;                      // If a tape operation set TI
    int PriorityRequest;                    // Not 0 implies Interrupt Request
    bool ChNOP;                             // True for I/O NOP instructions

    T1410IODevice *CurrentDevice;           // Ptr to device doing transfer
    TBusyDevice *UnitControlOverlapBusy;    // Busy counter for WTM use.
    THopper *Hopper[5];                     // Read/Punch Hoppers

    enum TTapeDensity {
    	DENSITY_200_556 = 0, DENSITY_200_800 = 1, DENSITY_556_800 = 2
    } TapeDensity;


	// Methods

    T1410Channel(							// Constructor
        TAddressRegister *Addr,
    	TLabel *LampInterlock,
        TLabel *LampRBCInterlock,
        TLabel *LampRead,
        TLabel *LampWRite,
        TLabel *LampOverlap,
        TLabel *LampNotOverlap,
        TLabel *LampNotRead,
        TLabel *LampBusy,
        TLabel *LampDataCheck,
        TLabel *LampCondition,
        TLabel *LampWLRecord,
        TLabel *LampNoTransfer
    );

    //  The following routine is called from T1410IODevice constructors
    //  to add devices into the Channel's device table.

    void AddIODevice(T1410IODevice *iodevice, int devicenumber);

    //  Methods that do most of the Channel's real work.

    inline int SetStatus(int i) { return ChStatus = i; }
    inline int GetStatus() { return ChStatus; }
    bool IsTapeIndicate() { return TapeIndicate; }
    void SetTapeIndicate() { TapeIndicate = true; }
    void ResetTapeIndicate() { TapeIndicate = false; }

    inline int GetDeviceNumber() {
        return ChUnitType -> Get().ToInt() & 0x3f;
    }

    T1410IODevice *SetCurrentDevice() {
        return CurrentDevice = Devices[GetDeviceNumber()];
    }

    T1410IODevice *GetCurrentDevice() { return CurrentDevice; };
    inline int GetUnitNumber() {
        return ChUnitNumber -> Get().ToAscii() - '0';
    }

    T1410IODevice *GetIODevice(int devicenumber);

    void DoOutput(TAddressRegister *addr);      //  Channel to Device
    void DoInput(TAddressRegister *addr);       //  Device to Channel - transfer
    bool ChannelStrobe(BCD ch);                 //  Device to Channel - Request
    void DoUnitControl(BCD opmod);              //  Unit control
    void DoOverlap();                           //  Overlap cycle processing

    //  Channel register methods

    inline bool GetR1Status() { return R1Status; };
    inline bool GetR2Status() { return R2Status; };
    inline void ResetR1() { R1Status = false; };
    inline void ResetR2() { R2Status = false; };
    BCD SetR1(BCD b);
    BCD SetR2(BCD b);
    BCD MoveR1R2();
};

//  Class declaration for IO Devices.  This is an *abstract* class,
//  which is used to derive a class for each kind of IO device (console,
//  reader, punch, tape, disk, etc.  The Unit Control function is the
//  only one that is not pure virtual.  We provide a default for it because
//  most devices don't implement unit control

class T1410IODevice : public TObject {

protected:
    T1410Channel *Channel;                  //  Channel device is attached to

public:
    T1410IODevice(int devicenum,T1410Channel *Channel); //  Constructor
    virtual int Select() = 0;                           //  Start an operation
    virtual int StatusSample() = 0;                     //  Return device status
    virtual void DoOutput() = 0;                        //  Channel -> Device
    virtual void DoInput() = 0;                         //  Device -> Channel
    virtual void DoUnitControl(BCD opmod);              //  Unit Control
};

#endif
