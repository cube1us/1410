//---------------------------------------------------------------------------
#ifndef UI1410CPUTH
#define UI1410CPUTH

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

typedef void (__closure *TInstructionExecuteRoutine)();

extern long ten_thousands[],thousands[],hundreds[],tens[];
extern long scan_mod[];
extern unsigned char sign_normalize_table[];
extern unsigned char sign_complement_table[];
extern unsigned char sign_negative_table[];
extern unsigned char numeric_value[];

struct OpCodeCommonLines {
	unsigned short ReadOut;
    unsigned short Operational;
    unsigned short Control;
};


extern struct OpCodeCommonLines OpCodeTable[64];

extern int IndexRegisterLookup [];

//	Table of execute routines.  These have to be closures to inherit
//	the object pointer (CPU).  Closures have to be initialized at runtime.
//	We do it in the contructor.

extern TInstructionExecuteRoutine InstructionExecuteRoutine[64];

//
//	Classes (types) used to implement the emulator, including the
//	final class defining what is in the CPU, T1410CPU.

//
//	Abstract class designed to build lists of objects affected by Program
//	Reset and Computer Reset
//

class TCpuObject : public TObject {

public:
	TCpuObject();						// Constructor to init data
	virtual void OnComputerReset() = 0;	// Called during Computer Reset
	virtual void OnProgramReset() = 0;	// Called during Program Reset

protected:
	bool DoesProgramReset;				// true if this is reset by P.R. button

public:
    TCpuObject *NextReset;
};


//
//	A second abstract class of objects that not only react to the Resets,
//	but also have entries on the display panel.
//

class TDisplayObject : public TCpuObject {

public:
	TDisplayObject();					// Constructor to init data.
	virtual void Display() = 0;			// Called to display this item
    virtual void LampTest(bool b) = 0;	// Called to start/end lamp test

public:
	TDisplayObject *NextDisplay;
};



//  Busy list object.  Each represents some device/unit that can
//  be busy for a while.  During it's constructor, device can request
//  an entry.  When a device wants to by busy for a while, it can
//  set it's counter.  We will pretend that the counter represents the
//  number of milliseconds the device wants to pretend it is busy.
//  But, for now, every "n" cycles, the CPU will go thru the list, and
//  decrement any non-zero ones.  A device can then check it's entry,
//  to know if it should still pretend it is busy, or not.  Eventually,
//  a real time element might well be introduced.

class TBusyDevice {

public:

    static TBusyDevice *FirstBusyDevice;    //  Pointer to first entry

private:

	TBusyDevice *NextBusyDevice;            //  Next entry in the list
    long BusyTime;                          //  How long to be busy for

public:

    TBusyDevice();                          //  Constructor.
    void SetBusy(long time) { BusyTime = time ? (time+1) : 0; };
    bool TestBusy() { return(BusyTime != 0); };
    static void BusyPass();             //  CLASS METHOD - pass the list
    static void Reset();                //  Resets list to all NOT busy
};

//	Class TDisplayObjects are indicators:  They just
//	display other things.  As a result, they need a pointer to
//	a function returning bool in order to decide what to do.

class TDisplayIndicator : public TDisplayObject {

protected:
	TLabel *lamp;
    bool (__closure *display)();

public:

	//	The constructor requires a pointer to a lamp and a pointer to
    //	a function that can calculate lamp state.

    //	We use a closure so that we don't have to pass a pointer to the
    //	stuff the display function needs (an object pointer is automatically
    //	embeeded in an __closure *)

	TDisplayIndicator(TLabel *l,bool (__closure *func)() ) {
    	lamp = l;
        display = func;
    }

	virtual void OnComputerReset() { ; }	// These have no state to reset
    virtual void OnProgramReset() { ; }		// These have no state to reset

    void Display() {
    	lamp -> Enabled = display();
        lamp -> Repaint();
    }

    void LampTest(bool b) {
    	lamp -> Enabled = (b ? true : display());
        lamp -> Repaint();
	}
};



//
//	Class of TDisplayObjects that are latches:
//	that can be set, reset and their state read out.  Some are
//	reset by Program Reset (PR) some are not.
//

class TDisplayLatch : public TDisplayObject {

protected:
	bool state;							// Latches can be set or reset
    bool doprogramreset;				// Some are reset by PR, some are not.
    TLabel *lamp;						// Pointer to display lamp.

public:
	TDisplayLatch(TLabel *l);			// Constructor - Set up lamp
    TDisplayLatch(TLabel *l, bool progreset);  // Same, but inihibit PR

    virtual void OnComputerReset();		// Define Computer Reset behavior now
    virtual void OnProgramReset();		// Define Program Reset behavior now too

	void Display();						// Define display behavior
    void LampTest(bool b);				// Define lamp test behavior.

    //	All you can really do with latches is set/reset/test them

    inline void Reset() { state = false; }
    inline void Set() { state = true; }
    inline void Set(bool b) { state = b; }
    void SetStop(char const *msg);
    inline bool State() { return state; }
};



class TRingCounter : public TDisplayObject {
private:
	char state;							// Override "state" variable !!
    char max;							// Max number of entries
    TLabel *lastlamp;					// Last lamp to be displayed
    TLabel *lastlampCE;					// Last CE lamp to be displayed

public:
    TLabel **lamps;						// Ptr to array of lamps
    TLabel **lampsCE;					// Ptr to array of CE lamps (or 0)

public:
	TRingCounter(char n);				// Construct with # of entries
    									// Ring counters are alwayes reset by PR

    virtual __fastcall ~TRingCounter();	// Destructor for array of lamps

    //	Functions inherited from abstract base classes now need definition

    void OnComputerReset();
    void OnProgramReset();
    void Display();
	void LampTest(bool b);

    //	The real meat of the Ring Counter class

	inline void Reset() { state = 0; }
    inline char Set(char n) {return state = n; }
    inline char State() { return state; }
    char Next();
};



//	Data Registers.  All are stored as BCD, but we have special
//	set routines to set or clear special parts for those registers
//	that don't use all the bits (e.g. the Op register has no WM or
//	C bits.

//	Also, a Register can have an optional pointer to an error latch.
//	If so, then whenever the register is used, a parity check for ODD
//	parity is made, and if invalid, the error latch is set.  (This
//	includes use during assignment).

class TRegister : public TDisplayObject {

private:

	BCD	value;
    TLabel *lampER;		// If set, there is an error lamp
    TLabel **lamps;		// If set, they point to lamps for WM C B A 8 4 2 1
    					// Any lamp not present MUST be set to 0

public:

	TRegister() { value = BITC; DoesProgramReset = true; lampER = 0; lamps = 0; }
    TRegister(bool b) { value = BITC; DoesProgramReset = b; lampER = 0; lamps = 0; }
    TRegister(int i) { value = i; DoesProgramReset = true; lampER = 0; lamps = 0; }
	TRegister(int i, bool b) { value = i; DoesProgramReset = b; lampER = 0; lamps = 0; }

    inline void OnComputerReset() { Reset(); }
    inline void Reset() { value = BITC; }

    inline void Set(BCD bcd) { value = bcd; }

    inline BCD Get() { return value; }

    void operator=(TRegister &source);
    void Display();
    void LampTest(bool b);

    //	To set up a register to display, provide a pointer to an error
    //	lamp (if any), and an array of pointers to data lamps.  Data lamps
    //	for any given bit (e.g. WM) may or may not exist.  Order of lamps
    //	is 1 2 4 8 A B C WM (0 thru 7)

    void SetDisplay(TLabel *ler, TLabel **l) {
        lampER = ler;
        lamps = l;
    }

    void OnProgramReset() {
    	if(DoesProgramReset) {
        	Reset();
        }
    }
};

//	Address Registers.  For efficiency, we keep both binary and
//	the real 2-out-of-5 code representations.  If either one is
//	valid, and the other representation is requested, we convert
//	on the fly (and mark that representation valid).

//	The Set() function effectively implements the Address Channel.

class TAddressRegister : public TCpuObject {

private:

	long i_value;			// Integer equivalent of register
    bool i_valid;			// True if integer rep. is valid
    bool set[5];			// True if corresponding digit is set
    TWOOF5 digits[5];		// Original 2 out of 5 code representation
    bool d_valid;			// True if digit rep. is valid
    char *name;

public:

    void OnComputerReset() { };     //  Do NOT reset on Computer Reset !!
    void OnProgramReset()  { };

	TAddressRegister();		// Constructor / initialization
	bool IsValid();			// Returns true if all digits set
    long Gate();			// Returns integer value if valid, -1 if not.
    BCD GateBCD(int i);		// Returns a single digit
    void Set(TWOOF5 digit,int index);	// Sets a digit
    void Set(long value);	// Sets whole register from binary (address mod)
    void Reset();			// Resets the register to blanks

    void operator=(TAddressRegister &source);	// Assignment

};



//	This class defines the A Channel - basically, it defines what
//	register is selected when the A channel is accessed.

class TAChannel : public TDisplayObject {

public:

	enum TAChannelSelect { A_Channel_None = 0, A_Channel_A = 1,
    	A_Channel_Mod = 2, A_Channel_E = 3, A_Channel_F = 4 };

private:

	enum TAChannelSelect AChannelSelect;
    TLabel *lamps[4];

public:

	TAChannel();							// Constructor

	void OnComputerReset() {
    	Reset();
    }

    void OnProgramReset() {
    	Reset();
    }

	// Old junk?  inline BCD Select(enum TAChannelSelect sel);	// Select input to A Channel
	// Old junk?  inline BCD Select();					// Use whatever was last selected

	BCD Select(enum TAChannelSelect sel);
	BCD Select();

    inline enum TAChannelSelect Selected() { return AChannelSelect; }

    inline void Reset() { AChannelSelect = A_Channel_None; }

    void Display();
    void LampTest(bool b);
};



//	The Assembly Channel: The 1410 Mix-Master!

class TAssemblyChannel : public TDisplayObject {

public:

    enum TAsmChannelZonesSelect {
    	AsmChannelZonesNone = 0, AsmChannelZonesB = 1, AsmChannelZonesA = 2
    };

    enum TAsmChannelWMSelect {
    	AsmChannelWMNone = 0, AsmChannelWMB = 1, AsmChannelWMA = 2,
        AsmChannelWMSet = 4
    };

    enum TAsmChannelNumericSelect {
    	AsmChannelNumNone = 0, AsmChannelNumB = 1, AsmChannelNumA = 2,
        AsmChannelNumAdder = 3, AsmChannelNumZero = 4
    };

    enum TAsmChannelSignSelect {
    	AsmChannelSignNone, AsmChannelSignB = 1, AsmChannelSignA = 2,
        AsmChannelSignLatch = 3
    };

private:

	BCD value;					// We remember value, for efficiency
    bool valid;					// True if value is set.  Reset each cycle!

    enum TAsmChannelZonesSelect AsmChannelZonesSelect;
    enum TAsmChannelWMSelect AsmChannelWMSelect;
    enum TAsmChannelNumericSelect AsmChannelNumericSelect;
    enum TAsmChannelSignSelect AsmChannelSignSelect;
    bool AsmChannelInvertSign;

    bool AsmChannelCharSet;     //  True if value set to a special char

	TLabel *AssmLamps[8];
    TLabel *AssmComplLamps[8];
    TLabel *AssmERLamp;

public:

	TAssemblyChannel();

    void OnComputerReset() { Reset(); }
    void OnProgramReset() { Reset(); }

    void Display();
    void LampTest(bool b);

    BCD Select();				// Uses last value and state

    BCD Select(
    	enum TAsmChannelWMSelect WMSelect,
        enum TAsmChannelZonesSelect ZoneSelect,
        bool InvertSign,
        enum TAsmChannelSignSelect SignSelect,
        enum TAsmChannelNumericSelect NumSelect
    );

    BCD Get();

    void Set(BCD v) { value = v; valid = true; AsmChannelCharSet = true; }

    void Reset();

    bool SpecialChar() { return AsmChannelCharSet; };

    BCD GateAChannelToAssembly(BCD v);

    BCD GateBRegToAssembly(BCD v);

};



//  Forward declaration of Channel, so we can put that in its
//  own header file

class T1410Channel;


//	This class defines what is actually inside the CPU.

#define MAXCHANNEL 2
#define CHANNEL1 0
#define CHANNEL2 1

#define STORAGE 80000

#define I_RING_OP 0
#define I_RING_1 1
#define I_RING_2 2
#define I_RING_3 3
#define I_RING_4 4
#define I_RING_5 5
#define I_RING_6 6
#define I_RING_7 7
#define I_RING_8 8
#define I_RING_9 9
#define I_RING_10 10
#define I_RING_11 11
#define I_RING_12 12

#define A_RING_1 0
#define A_RING_2 1
#define A_RING_3 2
#define A_RING_4 3
#define A_RING_5 4
#define A_RING_6 5

#define CLOCK_A 0
#define CLOCK_B 1
#define CLOCK_C 2
#define CLOCK_D 3
#define CLOCK_E 4
#define CLOCK_F 5
#define CLOCK_G 6
#define CLOCK_H 7
#define CLOCK_J 8
#define CLOCK_K 9

#define SCAN_N 0
#define SCAN_1 1
#define SCAN_2 2
#define SCAN_3 3

#define SUB_SCAN_NONE 0
#define SUB_SCAN_U 1
#define SUB_SCAN_B 2
#define SUB_SCAN_E 3
#define SUB_SCAN_MQ 4

#define CYCLE_A 0
#define CYCLE_B 1
#define CYCLE_C 2
#define CYCLE_D 3
#define CYCLE_E 4
#define CYCLE_F 5
#define CYCLE_I 6
#define CYCLE_X 7

class T1410CPU {

private:

	BCD core[STORAGE];

public:

	//	Wiring list

	TCpuObject *ResetList;					// List of latches.
	TDisplayObject *DisplayList;			// List of displayable things

    //	Data Registers

    TRegister *A_Reg, *B_Reg, *Op_Reg, *Op_Mod_Reg;

    //	Address Registers

    TAddressRegister *STAR;					// Storage Address Register
    										// AKA MAR (Memory Address Register)

    TAddressRegister *A_AR, *B_AR, *C_AR, *D_AR, *E_AR, *F_AR;

    TAddressRegister *I_AR;					// Instruction Counter

    TAddressRegister *TOD;                  //  Time of Day clock - 5 digits


    //	Channels

    T1410Channel *Channel[MAXCHANNEL];		// 2 I/O Channels.

    TAChannel *AChannel;					// A Channel
	TAssemblyChannel *AssemblyChannel;		// Assembly Channel

    //	Indicators

	TDisplayIndicator *OffNormal;			// OFF NORMAL Indicator

    //	Ring Counters

    TRingCounter *IRing;					// Instruction decode ring
    TRingCounter *ARing;					// Address decode ring
    TRingCounter *ClockRing;				// Cycle Clock
    TRingCounter *ScanRing;					// Address Modification Mode
    TRingCounter *SubScanRing;				// Arithmetic Scan type
    TRingCounter *CycleRing;				// CPU Cycle type

    //	Latches with Indicators

    TDisplayLatch *CarryIn;					// Carry latch
    TDisplayLatch *CarryOut;				// Adder has generated carry
    TDisplayLatch *AComplement;				// A channel complement
    TDisplayLatch *BComplement;				// B channel complement
    TDisplayLatch *CompareBGTA;				// B > A
    TDisplayLatch *CompareBEQA;				// B = A
    TDisplayLatch *CompareBLTA;				// B < A  NOTE:  On after C. Reset.
    TDisplayLatch *Overflow;				// Arithmetic Overflow
    TDisplayLatch *DivideOverflow;			// Divide Overflow
    TDisplayLatch *ZeroBalance;				// Zero arithmetic result
    TDisplayLatch *PriorityAlert;           // Priority Alert mode

    //	Check Latches

    TDisplayLatch *AChannelCheck;			// A Channel parity error
    TDisplayLatch *BChannelCheck;			// B Channel parity error
    TDisplayLatch *AssemblyChannelCheck;	// Assembly Channel parity error
    TDisplayLatch *AddressChannelCheck;		// Address Channel parity error
    TDisplayLatch *AddressExitCheck;		// Validity error at address reg.
    TDisplayLatch *ARegisterSetCheck;		// A register failed to reset
    TDisplayLatch *BRegisterSetCheck;		// B register failed to reset
    TDisplayLatch *OpRegisterSetCheck;		// Op register failed to set
    TDisplayLatch *OpModifierSetCheck;		// Op modifier failed to set
	TDisplayLatch *ACharacterSelectCheck;	// Incorrect A channel gating
    TDisplayLatch *BCharacterSelectCheck;	// Incorrect B channel geting

    TDisplayLatch *IOInterlockCheck;		// Program did not check I/O
    TDisplayLatch *AddressCheck;			// Program gave bad address
    TDisplayLatch *RBCInterlockCheck;		// Program did not check RBC
    TDisplayLatch *InstructionCheck;		// Program issued invalide op

    //	Switches

    enum TMode {								// Mode switch, values must match
		MODE_RUN = 0, MODE_DISPLAY = 1, MODE_ALTER = 2,
    	MODE_CE = 3, MODE_IE = 4, MODE_ADDR = 5
    } Mode;

    enum TAddressEntry {
    	ADDR_ENTRY_I = 0, ADDR_ENTRY_A = 1, ADDR_ENTRY_B = 2,
        ADDR_ENTRY_C = 3, ADDR_ENTRY_D = 4, ADDR_ENTRY_E = 5,
        ADDR_ENTRY_F = 6
	} AddressEntry;

	//	The entry below is for the state of the 1415 Storage Scan SWITCH
    //	(The storage scan modification mode is in the Ring ScanRing)

    enum TStorageScan {
    	SSCAN_OFF = 0, SSCAN_LOAD_1 = 1, SSCAN_LOAD_0 = 2,
        SSCAN_REGEN_0 = 3, SSCAN_REGEN_1 = 4
    } StorageScan;

    enum TCycleControl {
    	CYCLE_OFF = 0, CYCLE_LOGIC = 1, CYCLE_STORAGE = 2
    } CycleControl;

    enum TCheckControl {
    	CHECK_STOP = 0, CHECK_RESTART = 1, CHECK_RESET = 2
    } CheckControl;

    bool DiskWrInhibit;
    bool AsteriskInsert;
    bool InhibitPrintOut;

    BCD BitSwitches;

    //	CPU  state latches

    bool StopLatch;							// True to stop CPU
    bool StopKeyLatch;						// True if STOP button pressed
    bool DisplayModeLatch;					// True if we are displaying storage
    bool ProcessRoutineLatch;				// True if we are in Run or IE mode
    bool BranchLatch;						// True if we are to branch
    bool BranchTo1Latch;					// True to branch to 1
    bool LastInstructionReadout;			// True at end of Instruction fetch
    bool IRingControl;						// Set to start Instruction Fetch

    bool SignLatch;                         // Set if Minus sign at certain pts.
    bool ZeroSuppressLatch;                 // Set if supressing zeroes.
    bool FloatingDollarLatch;               // Used in Edit
    bool AsteriskFillLatch;                 // Used in Edit
    bool DecimalControlLatch;               // Used in Edit

    bool StorageWrapLatch;                  //  True if storage wrapped

    int IOChannelSelect;					//	One if  if channel 2 op.
    bool IOOverlapSelect;                   //  True if overlap (A bit)
    bool InqReqLatch;                       //  True if outstanding inquiry

    //	Index latches are rolled up into an int.  The BA zones from the
    //	tens and hundreds positions are shifted and the 4 bits together
    //	generate a value from 0 to 15.

    int	IndexLatches;

	//	Methods

    T1410CPU();								// Constructor
    void Display();							// Run thru the display list
    void Cycle();							// Used for common CPU Cycles

    //	The 1410 Adder accepts BCD inputs, a Carry Latch and complement
    //	flags and returns a sum in BCD, possible setting Carry Out.

    BCD Adder(BCD A,int Complement_A,BCD B,int Complement_B);

    BCD AdderResult;						// For the Assembly Channel


    //  The 1410 Comparator compares the B and A character registers,
    //  setting the CompareB(GT/EQ/LT)A latches appropriately.

    void Comparator();

private:

    int AdderBinaryResult,AdderQuinaryResult;   //  For the Comparator

private:

	//	Indicator Routines
    //	Normally, these will be accessed via a bool (__closure *func)()

    bool IndicatorOffNormal();

public:

	//	Core operations  (using STAR/MAR and B Data Register)

    void Readout(); 			//	Reads out one storage character
    void Store(BCD bcd);		//	Store data
    void SetScan(char s);		//	Sets Scan Modification value

    long STARScan();			//	Applies Scan CTRL modification to STAR,
    							//	and returns the results - suitable to assign

    long STARMod(int mod);		//	Similar, but provides direct +1/0/-1 mod

    bool StorageWrapCheck(int mod);    //  Call to check for storage wrap
                                        //  Mod should be 1 or -1.

	void LoadCore(String file);	//	Loads core from a file
	void DumpCore(String file);	//	Dumps core to a file


public:

	//	Instruction operations

    unsigned short OpReadOutLines;
    unsigned short OpOperationalLines;
    unsigned short OpControlLines;

	void DoStartClick();			//	START pressed (moved from UI1410PWR)
    void InstructionDecodeStart();	//	Starts instruction decode processing
    void InstructionDecode();		//	Remainder of instruction decode
    void InstructionDecodeIARAdvance();		//	Conditionally advance IAR

    void InstructionIndexStart();	//	Starts up indexing operation
    void InstructionIndex();		//	Does the actual indexing

    void InstructionExecuteInvalid();	//	Default instruction execute routine

    void InstructionArith();		        //	Add and Subtract
    void InstructionZeroArith();	        //	Zero and Add, Zero and Subtract
    void InstructionMultiply();             //  Multiply
    void InstructionDivide();               //  Divide
    void InstructionMove();                 //  Move
    void InstructionMoveSuppressZeros();    //  Move and Suppress Zeros
    void InstructionEdit();                 //  Edit
    void InstructionCompare();              //  Compare
    void InstructionTableLookup();          //  Table Lookup
    void InstructionBranchCond();   //  J Branch (unconditional & conditional)
    void InstructionBranchChannel1();       //  Branch on Channel 1 Condition
    void InstructionBranchChannel2();       //  Branch on Channel 2 condition
    void InstructionBranchChannel(int channel); //  Branch on channel condition
    void InstructionBranchCharEqual();      //  Branch if Character Equal
    void InstructionBranchBitEqual();       //  Branch if Bit Equal
    void InstructionBranchZoneWMEqual();    //  Branch if WM or Zone Equal
    void InstructionStoreAddressRegister(); //  S.A.R.
    void InstructionDoWordMark();           //  Set or Clear Word Mark
    void InstructionClearStorage();         //  Clear Storage / CS & Branch
    void InstructionHalt();                 //  Halt / Halt & Branch
    void InstructionIO();                   //  Move and Load mode IO
    void InstructionUnitControl();          //  Unit Control Instruction
    void InstructionPriorityBranch();       //  Priority Alert Special Feature
    void InstructionCarriageControl();      //  Carriage Control (F / 2)
    void InstructionSelectStacker();        //  Select Stacker and Feed (K/4)

};

extern T1410CPU *CPU;

//---------------------------------------------------------------------------
#endif

