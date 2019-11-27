//---------------------------------------------------------------------------
#ifndef UI1403H
#define UI1403H
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Buttons.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.Dialogs.hpp>
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

#define PRINTPOSITIONS  132
#define PRINTMAXLINES   100

class TFI1403 : public TForm
{
__published:	// IDE-managed Components
    TMemo *Paper;
    TBitBtn *CheckReset;
    TBitBtn *Start;
    TBitBtn *Stop;
    TBitBtn *Space;
    TBitBtn *CarriageRestore;
    TBitBtn *SingleCycle;
    TLabel *LightPrintReady;
    TLabel *LightPrintCheck;
    TLabel *LightEndofForms;
    TBitBtn *CarriageStop;
    TLabel *LightFormsCheck;
    TLabel *LightSyncCheck;
    TButton *FileButton;
    TButton *EnableFile;
    TButton *Button1;
    TButton *EnablePrinter;
    TOpenDialog *FileCaptureDialog;
    TButton *CarriageTape;
    void __fastcall StartClick(TObject *Sender);
    void __fastcall CheckResetClick(TObject *Sender);
    void __fastcall StopClick(TObject *Sender);
    void __fastcall SpaceClick(TObject *Sender);
    void __fastcall CarriageRestoreClick(TObject *Sender);
    void __fastcall CarriageStopClick(TObject *Sender);
    void __fastcall FileButtonClick(TObject *Sender);

    void __fastcall EnableFileClick(TObject *Sender);
    void __fastcall CarriageTapeClick(TObject *Sender);
private:	// User declarations

    int PrintPosition;
    bool PrintData;
    char PrintBuffer[PRINTPOSITIONS + 1];
    int Line;

public:		// User declarations
    __fastcall TFI1403(TComponent* Owner);
    bool SendBCD(BCD c);
    bool EndofLine();
    bool NextLine();

    T1403Printer *PrinterIODevice;

};
//---------------------------------------------------------------------------
extern PACKAGE TFI1403 *FI1403;
//---------------------------------------------------------------------------
#endif
