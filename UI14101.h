//---------------------------------------------------------------------------
#ifndef UI14101H
#define UI14101H
#include <System.Classes.hpp>
#include <Vcl.Dialogs.hpp>
#include <Vcl.Menus.hpp>
//---------------------------------------------------------------------------
// #include <vcl\Classes.hpp>  (From Borland C++ 5)
#include <vcl.Controls.hpp>
#include <vcl.StdCtrls.hpp>
#include <vcl.Forms.hpp>
#include <vcl.ExtCtrls.hpp>
#include <vcl.Menus.hpp>
#include <vcl.Dialogs.hpp>
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

class TFI14101 : public TForm
{
__published:	// IDE-managed Components
	TMainMenu *MainMenu1;
	TMenuItem *File1;
	TMenuItem *Load1;
	TMenuItem *Dump1;
	TOpenDialog *LoadCoreDialog;
	TSaveDialog *DumpCoreDialog;
	void __fastcall Load1Click(TObject *Sender);
	void __fastcall Dump1Click(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TFI14101(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern TFI14101 *FI14101;
//---------------------------------------------------------------------------
#endif
