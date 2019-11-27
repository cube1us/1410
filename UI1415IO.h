//---------------------------------------------------------------------------
#ifndef UI1415IOH
#define UI1415IOH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.StdCtrls.hpp>
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

//	1415 Console User Interface

#include <vcl.ExtCtrls.hpp>

#define CONSOLE_IDLE	1			// Console is idle
#define CONSOLE_NORMAL	2			// Normal Read Mode input
#define CONSOLE_LOAD	3			// Normal Load Mode input
#define CONSOLE_ALTER	4			// Console is in alter mode loading memory
#define CONSOLE_ADDR	5			// Console is in address set or display mode
#define CONSOLE_DISPLAY	6			// Console is displaying register or mem
#define CONSOLE_OUTPUT  7           // Console is doing output.

#define CONSOLE_MATRIX_HOME	0		// Console matrix home position

#define CONSOLE_IO_DEVICE   19      // Console is device 'T'

class T1415Console : public T1410IODevice {

private:
    bool IOInProgress;

public:

    T1415Console(int devicenum,T1410Channel *Channel);
    virtual int Select();
    virtual int StatusSample();
    virtual void DoOutput();
    virtual void DoInput();

    bool ConsoleIOInProgress() { return IOInProgress; }
};


class TFI1415IO : public TForm
{
__published:	// IDE-managed Components
	TMemo *I1415IO;
	TLabel *KeyboardLock;
	TButton *InqReq;
	TButton *InqRlse;
	TButton *InqCancel;
	TButton *WordMark;
	TButton *MarginRelease;
	TTimer *KeyboardLockReset;
    TTimer *BusyTimer;
	void __fastcall I1415IOKeyPress(TObject *Sender, char &Key);

	void __fastcall I1415IOKeyDown(TObject *Sender, WORD &Key, TShiftState Shift);

	void __fastcall WordMarkClick(TObject *Sender);
	void __fastcall MarginReleaseClick(TObject *Sender);

	void __fastcall KeyboardLockResetTimer(TObject *Sender);

    void __fastcall InqReqClick(TObject *Sender);
    void __fastcall InqCancelClick(TObject *Sender);
    void __fastcall InqRlseClick(TObject *Sender);
    void __fastcall BusyTimerTimer(TObject *Sender);
private:	// User declarations

    int state;
    int matrix;

    bool WMCtrlLatch;
    bool DisplayWMCtrlLatch;

    bool AlterFullLineLatch;
    bool AlterWMConditionLatch;

    TAddressRegister *Console_AR;

    bool DoWordMark();
    void LockKeyboard();
    void UnlockKeyboard();

public:		// User declarations
    __fastcall TFI1415IO(TComponent* Owner);

    void SendBCDTo1415(BCD bcd);
    bool SetState(int s);
	void NextLine();

    inline void SetMatrix(int i) { matrix = i; }
    void SetMatrix(int x, int y) { matrix = 6*(x-1) + y; }
	inline int GetMatrix() { return matrix; }
    inline int GetMatrixX() { return (matrix-1)/6 + 1; }
    inline int GetMatrixY() { return (matrix-1)%6 + 1; }
    void StepMatrix() { ++matrix; }
    void ResetMatrix() { matrix = CONSOLE_MATRIX_HOME; }
    void DoMatrix();

    void StopPrintOut(char c);
    void DoAddressEntry();
    void DoDisplay(int phase);
    void DoAlter();

    T1415Console *ConsoleIODevice;

};
//---------------------------------------------------------------------------
extern TFI1415IO *FI1415IO;
//---------------------------------------------------------------------------

//	Keyboard representations of some unusual BCD characters

#define	KBD_RADICAL			022
#define KBD_RECORD_MARK		'|'
#define KBD_ALT_BLANK		002
#define KBD_WORD_SEPARATOR	'^'
#define KBD_SEGMENT_MARK	023
#define KBD_DELTA			004
#define KBD_GROUP_MARK		007
#define KBD_WORD_MARK		0x1b


#endif
