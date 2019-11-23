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
#include <assert.h>
#include "UBCD.H"
#include "UI1410CPUT.H"
#include "UIHOPPER.H"
#include "UI1410CHANNEL.H"
#include "UIREADER.H"
#include "UIPUNCH.H"
#include "UI1402.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

#pragma resource "*.dfm"
TFI1402 *FI1402;

//---------------------------------------------------------------------------
__fastcall TFI1402::TFI1402(TComponent* Owner)
    : TForm(Owner)
{
    Width = 632;
    Left = 260;
    Top = 300;
    Height = 164;
   	WindowState = wsMinimized;
    current_hopper = 0;
    Channel = NULL;
}
//---------------------------------------------------------------------------

//  Method to reset card reader EOF status.  In a real 1402, this would
//  turn out the 1402 light (and enable the button).  Here, we just enable
//  the button.

void TFI1402::ResetEOF() {
    EOFButton -> Enabled = true;
}

//  Method to set or reset the Reader Check light

void TFI1402::SetReaderCheck(bool flag) {
    LightReaderCheck -> Enabled = flag;
}

//  Method to set or reset the Reader Validity Ligh

void TFI1402::SetReaderValidity(bool flag) {
    LightReaderValidity -> Enabled = flag;
}

//  Method to set or reset the Reader Ready Light

void TFI1402::SetReaderReady(bool flag) {
    LightReaderReady -> Enabled = flag;
    ReaderStart -> Enabled = !flag;
    ReaderStop -> Enabled = flag;
}

//  And another to set or reset the Punch Ready light

void TFI1402::SetPunchReady(bool flag) {
    LightPunchReady -> Enabled = flag;
    PunchStart -> Enabled = !flag;
    PunchStop -> Enabled = flag;
}

void __fastcall TFI1402::ReaderStartClick(TObject *Sender)
{
    if(LightReaderStop -> Enabled) {
        return;
    }
    if(ReaderIODevice -> DoStart()) {
        SetReaderReady(true);
    }
}
//---------------------------------------------------------------------------

void __fastcall TFI1402::ReaderStopClick(TObject *Sender)
{
    SetReaderReady(false);
    EOFButton -> Enabled = true;
    ReaderIODevice -> DoStop();
}
//---------------------------------------------------------------------------

void __fastcall TFI1402::EOFButtonClick(TObject *Sender)
{
    ReaderIODevice -> SetEOF();
    EOFButton -> Enabled = false;
}
//---------------------------------------------------------------------------

void __fastcall TFI1402::LoadReaderHopperClick(TObject *Sender)
{
    if(LightReaderReady -> Enabled) {
        return;
    }
    ReaderIODevice -> CloseFile();
    if(FileOpenDialog -> Execute() &&
        ReaderIODevice -> LoadFile(FileOpenDialog -> FileName.c_str())) {
        ReaderStart -> Enabled = true;
        EOFButton -> Enabled = true;
    }
}
//---------------------------------------------------------------------------

void __fastcall TFI1402::HopperSelectClick(TObject *Sender,
      TUDBtnType Button)
{
    static char *hopper_name[] = { "R0","R1","R2/P8","P4","P0" };

    current_hopper = HopperSelect -> Position;
    assert(current_hopper >= 0 && current_hopper <= 4);
    HopperNumber -> Caption = hopper_name[current_hopper];
    Display();
}
//---------------------------------------------------------------------------

void TFI1402::Display() {

    char count_string[32];

    if(Channel == NULL) {
        return;
    }

    assert(current_hopper >= 0 && current_hopper <= 4);
    sprintf(count_string,"%d",Channel -> Hopper[current_hopper] -> getCount());
    HopperCount -> SetTextBuf(count_string);
}


void __fastcall TFI1402::HopperLoadButtonClick(TObject *Sender)
{
    assert(current_hopper >= 0 && current_hopper <= 4);
    if(FileOpenDialog -> Execute() &&
       Channel -> Hopper[current_hopper] ->
        setFilename(FileOpenDialog -> FileName.c_str()) ) {
        Display();
    }
}
//---------------------------------------------------------------------------

void __fastcall TFI1402::PunchStartClick(TObject *Sender)
{
    if(LightPunchReady -> Enabled == true) {
        return;
    }
    if(PunchIODevice -> DoStart()) {
        LightPunchReady -> Enabled = true;
        PunchStart -> Enabled = false;
        PunchStop -> Enabled = true;
    }
}
//---------------------------------------------------------------------------

void __fastcall TFI1402::PunchStopClick(TObject *Sender)
{
    if(!LightPunchReady -> Enabled) {
        return;
    }
    if(PunchIODevice -> DoStop()) {
        LightPunchReady -> Enabled = false;
        PunchStart -> Enabled = true;
        PunchStop -> Enabled = false;
    }
}
//---------------------------------------------------------------------------

