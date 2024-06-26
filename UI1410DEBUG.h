//---------------------------------------------------------------------------
#ifndef UI1410DEBUGH
#define UI1410DEBUGH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Dialogs.hpp>
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

//  NOTE:  I tried using __VA_OPT__(,) below, but Embarcadero's
//  "legacy" Borland C++ style compiler did not handle it.

#define DEBUG(string, ...) \
	sprintf(F1410Debug->line,string, ##__VA_ARGS__);	\
	F1410Debug -> DebugOut(F1410Debug -> line);


class TF1410Debug : public TForm
{
__published:	// IDE-managed Components
	TMemo *Debug;
	TButton *fileButton;
	TLabel *fileNameLabel;
	TOpenDialog *FileOpenDialog;
	void __fastcall fileButtonClick(TObject *Sender);
private:	// User declarations
	String fileName;
    TFileStream *debugFd;

public:		// User declarations
	__fastcall TF1410Debug(TComponent* Owner);
    TF1410Debug();
    char line[256];
	void DebugOut(char const *s);
    void Minimize();
};
//---------------------------------------------------------------------------
extern TF1410Debug *F1410Debug;
//---------------------------------------------------------------------------
#endif
