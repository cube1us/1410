//---------------------------------------------------------------------------
#ifndef UI1415CEH
#define UI1415CEH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
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

class TFI1415CE : public TForm
{
__published:	// IDE-managed Components
	TComboBox *AddressEntry;
	TLabel *Label1;
	TComboBox *StorageScan;
	TLabel *Label2;
	TComboBox *CycleControl;
	TLabel *Label3;
	TComboBox *CheckControl;
	TLabel *Label4;
	TCheckBox *DiskWrInhibit;
	TComboBox *DensityCh1;
	TLabel *Label5;
	TComboBox *DensityCh2;
	TLabel *Label6;
	TButton *StartPrintOut;
	TCheckBox *Compat1401;
	TButton *CheckReset1401;
	TCheckBox *CheckStop1401;
	TButton *CheckTest1;
	TButton *CheckTest2;
	TButton *CheckTest3;
	TLabel *Label7;
	TLabel *Label8;
	TCheckBox *AsteriskInsert;
	TCheckBox *InhibitPrintOut;
	TCheckBox *BitSenseC;
	TCheckBox *BitSenseB;
	TCheckBox *BitSenseA;
	TCheckBox *BitSense8;
	TCheckBox *BitSense4;
	TCheckBox *BitSense2;
	TCheckBox *BitSense1;
	TCheckBox *BitSenseWM;
	TLabel *Label9;
	TLabel *Label10;
	TLabel *Label11;
	void __fastcall AddressEntryChange(TObject *Sender);

	void __fastcall StorageScanChange(TObject *Sender);
	void __fastcall CycleControlChange(TObject *Sender);
	void __fastcall CheckControlChange(TObject *Sender);
	void __fastcall DiskWrInhibitClick(TObject *Sender);
	void __fastcall DensityCh1Change(TObject *Sender);
	void __fastcall DensityCh2Change(TObject *Sender);
	void __fastcall AsteriskInsertClick(TObject *Sender);
	void __fastcall InhibitPrintOutClick(TObject *Sender);
	void __fastcall BitSenseCClick(TObject *Sender);
	void __fastcall BitSenseBClick(TObject *Sender);
	void __fastcall BitSenseAClick(TObject *Sender);
	void __fastcall BitSense8Click(TObject *Sender);
	void __fastcall BitSense4Click(TObject *Sender);
	void __fastcall BitSense2Click(TObject *Sender);
	void __fastcall BitSense1Click(TObject *Sender);
	void __fastcall BitSenseWMClick(TObject *Sender);
	void __fastcall StartPrintOutClick(TObject *Sender);
private:	// User declarations
	int SenseBit;
    void SetSense(bool b,int i);
public:		// User declarations
	__fastcall TFI1415CE(TComponent* Owner);
    void Minimize();
};
//---------------------------------------------------------------------------
extern TFI1415CE *FI1415CE;
//---------------------------------------------------------------------------
#endif
