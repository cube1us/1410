//---------------------------------------------------------------------------
#ifndef UI1410PWRH
#define UI1410PWRH
//---------------------------------------------------------------------------
#include <vcl\Classes.hpp>
#include <vcl\Controls.hpp>
#include <vcl\StdCtrls.hpp>
#include <vcl\Forms.hpp>
#include <vcl\ExtCtrls.hpp>
#include <vcl\Buttons.hpp>
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

//	1410 Power panel

class TFI1410PWR : public TForm
{
__published:	// IDE-managed Components
	TPanel *Panel1;
	TLabel *EMERGENCY;
	TLabel *OFF;
	TBitBtn *EmergencyOff;
	TBitBtn *ComputerReset;
	TBitBtn *DCOff;
	TPanel *READY;
	TBitBtn *PowerOff;
	TPanel *Panel2;
	TComboBox *Mode;
	TLabel *MODELABEL;
	TBitBtn *Start;
	TBitBtn *Stop;
	TBitBtn *ProgramReset;
	void __fastcall EmergencyOffClick(TObject *Sender);
	void __fastcall ComputerResetClick(TObject *Sender);
	void __fastcall ModeChange(TObject *Sender);
	void __fastcall ProgramResetClick(TObject *Sender);
	void __fastcall StartClick(TObject *Sender);
	void __fastcall StopClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TFI1410PWR(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern TFI1410PWR *FI1410PWR;
//---------------------------------------------------------------------------
#endif
