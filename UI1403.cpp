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

//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <dir.h>
#include <stdio.h>

#include "ubcd.h"
#include "UI1410CPUT.h"
#include "UIHOPPER.h"
#include "UI1410CHANNEL.h"
#include "UIPRINTER.h"
#include "UI1403.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

#include "UI1410DEBUG.h"

TFI1403 *FI1403;
//---------------------------------------------------------------------------
__fastcall TFI1403::TFI1403(TComponent* Owner)
    : TForm(Owner)
{
    Width = 699;
    Left = 260;
    Top = 0;
    Height = 296;
    PrintPosition = 0;
    PrintData = false;
    Line = 1;

   	WindowState = wsMinimized;
}
//---------------------------------------------------------------------------

bool TFI1403::SendBCD(BCD c) {

    if(PrintPosition < 0 || PrintPosition >= PRINTPOSITIONS) {
        return(false);
    }
    PrintBuffer[PrintPosition++] = c.ToAscii();
    PrintBuffer[PrintPosition] = '\0';
    return(true);
}

bool TFI1403::EndofLine() {

    if(PrintPosition != 0) {
        PrintData = true;
    }
    PrintPosition = 0;
    return(true);
}

bool TFI1403::NextLine() {

    if(!PrintData || PrintBuffer[0] == '\0') {
        sprintf(PrintBuffer,"<%d>",Line);
    }

    if(Paper -> Lines -> Capacity > PRINTMAXLINES) {
        Paper -> Lines -> Delete(0);
    }
	Paper -> Lines -> Add(PrintBuffer);
    ++Line;
    PrintPosition = 0;
    PrintBuffer[0] = '\0';
    PrintData = false;
    return(true);
}

void __fastcall TFI1403::StartClick(TObject *Sender)
{
    if(LightPrintCheck -> Enabled || LightEndofForms -> Enabled ||
       LightFormsCheck -> Enabled || LightSyncCheck -> Enabled) {
       return;
    }
    PrinterIODevice -> Start();                 //  If OK, will light READY
}
//---------------------------------------------------------------------------

void __fastcall TFI1403::CheckResetClick(TObject *Sender)
{
    LightPrintCheck -> Enabled = false;
    LightEndofForms -> Enabled = false;
    LightFormsCheck -> Enabled = false;
    LightSyncCheck -> Enabled = false;
    PrinterIODevice -> CheckReset();
}
//---------------------------------------------------------------------------

void __fastcall TFI1403::StopClick(TObject *Sender)
{
    PrinterIODevice -> Stop();                      //  Will unlight READY
}
//---------------------------------------------------------------------------

void __fastcall TFI1403::SpaceClick(TObject *Sender)
{
    if(PrinterIODevice -> IsReady()) {
        return;
    }
    PrinterIODevice -> CarriageSpace();
}
//---------------------------------------------------------------------------

void __fastcall TFI1403::CarriageRestoreClick(TObject *Sender)
{
    if(PrinterIODevice -> IsReady()) {
        return;
    }
    PrinterIODevice -> CarriageRestore();
}
//---------------------------------------------------------------------------

void __fastcall TFI1403::CarriageStopClick(TObject *Sender)
{
    PrinterIODevice -> CarriageStop();
}
//---------------------------------------------------------------------------

void __fastcall TFI1403::FileButtonClick(TObject *Sender)
{

    //  If the button says "Disable", we have to close out the existing file.

    if(EnableFile -> Enabled &&
        strcmp(EnableFile -> Caption.c_str(),"Close") == 0) {
        PrinterIODevice -> FileCaptureClose();
        EnableFile -> Caption = "Enable";
        EnableFile -> Enabled = false;
    }

    //  Now, send the file name off to the printer to have and to hold

    if(FileCaptureDialog -> Execute() &&
       PrinterIODevice -> FileCaptureSet(FileCaptureDialog -> FileName.c_str())) {
        EnableFile -> Caption = "Enable";
        EnableFile -> Enabled = true;
    }
}
//---------------------------------------------------------------------------


void __fastcall TFI1403::EnableFileClick(TObject *Sender)
{
    if(strcmp(EnableFile -> Caption.c_str(),"Close") == 0) {
        PrinterIODevice -> FileCaptureClose();
        EnableFile -> Caption = "Enable";
    }
    else if(PrinterIODevice -> FileCaptureOpen()) {
        EnableFile -> Caption = "Close";
    }
}
//---------------------------------------------------------------------------

void __fastcall TFI1403::CarriageTapeClick(TObject *Sender)
{
    int rc;

    if(FileCaptureDialog -> Execute()) {
        rc =  PrinterIODevice ->
            SetCarriageTape(FileCaptureDialog -> FileName.c_str());
        if(rc < 0) {
            DEBUG("Carriage Tape File Error, line %d",-rc);
        }
    }
}
//---------------------------------------------------------------------------

