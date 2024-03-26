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

#include "ubcd.h"
#include "UI1410CPUT.h"
#include "UIHOPPER.h"
#include "UI1410CHANNEL.h"
#include "UITAPEUNIT.h"
#include "UITAPETAU.h"

#include "UI729TAPE.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

TFI729 *FI729;
//---------------------------------------------------------------------------
__fastcall TFI729::TFI729(TComponent* Owner)
    : TForm(Owner)
{
    int i;

    Height = 215;
	Width = 400;
	Left = 930;
    Top = 470;

  	// WindowState = wsMinimized;

    current_unit = 0;
    current_channel = CHANNEL1;
    TapeUnit = NULL;

    for(i=0; i < MAXCHANNEL; ++i) {
        TAU[i] = NULL;
    }

}
//---------------------------------------------------------------------------
void __fastcall TFI729::UnitDialClick(TObject *Sender, TUDBtnType Button)
{
    char unit_string[2];

    //  Click of up/down dial

    current_unit = UnitDial -> Position;
    assert(current_unit >= 0 && current_unit <= 9);

    sprintf(unit_string,"%1.1d",current_unit);
    Unit -> Caption = unit_string;
    assert(TAU[current_channel] != NULL);
    TapeUnit = TAU[current_channel] -> GetUnit(current_unit);

    //  Display resulting unit

    Display();
}
//---------------------------------------------------------------------------
void __fastcall TFI729::LoadRewindClick(TObject *Sender)
{
    if(TapeUnit != NULL) {
        TapeUnit -> LoadRewind();
        Display();
    }
}
//---------------------------------------------------------------------------
void __fastcall TFI729::StartClick(TObject *Sender)
{
    if(TapeUnit != NULL) {
        TapeUnit -> Start();
        Display();
    }
}
//---------------------------------------------------------------------------
void __fastcall TFI729::ChangeDensityClick(TObject *Sender)
{
    if(TapeUnit != NULL) {
        TapeUnit -> ChangeDensity();
        Display();
    }
}
//---------------------------------------------------------------------------
void __fastcall TFI729::UnloadClick(TObject *Sender)
{
    if(TapeUnit != NULL) {
        TapeUnit -> Unload();
        Display();
    }
}
//---------------------------------------------------------------------------
void __fastcall TFI729::ResetClick(TObject *Sender)
{
    if(TapeUnit != NULL) {
        TapeUnit -> Reset();
        Display();
    }
}
//---------------------------------------------------------------------------
void __fastcall TFI729::ChannelSelectClick(TObject *Sender)
{
    char channel_string[2];

    if(++current_channel >= MAXCHANNEL) {
        current_channel = CHANNEL1;
    }
    sprintf(channel_string,"%1.1d",current_channel+1);
    ChannelSelect -> Caption = channel_string;
    TapeUnit = TAU[current_channel] -> GetUnit(current_unit);
    Display();
}
//---------------------------------------------------------------------------

//  Display method: Displays current unit

void TFI729::Display()
{
	wchar_t record_number_string[32];

	if(TapeUnit == NULL) {
		if(TAU[current_channel] == NULL) {
			return;
		}
		TapeUnit = TAU[current_channel] -> GetUnit(current_unit);
		if(TapeUnit == NULL) {
			return;
		}
	}

	Ready -> Enabled = TapeUnit -> IsReady();
	Select -> Enabled = TapeUnit -> Selected();
	FileProtect -> Enabled = TapeUnit -> IsFileProtected();
	TapeIndicate -> Enabled = TapeUnit -> TapeIndicate();
	HighDensity -> Enabled = TapeUnit -> HighDensity();
	Filename -> SetTextBuf(TapeUnit -> GetFileName().w_str());

	swprintf(record_number_string,
		sizeof(record_number_string)/sizeof(record_number_string[0]),
		L"%d",TapeUnit -> GetRecordNumber() );
    RecordNum -> SetTextBuf(record_number_string);

    if(TapeUnit -> IsReady()) {
        Mount -> Enabled = false;
        LoadRewind -> Enabled = false;
        Start -> Enabled = false;
        Unload -> Enabled = false;
        Reset -> Enabled = true;
    }
    else if(TapeUnit -> IsLoaded()) {
        Mount -> Enabled = false;
        LoadRewind -> Enabled = true;
        Start -> Enabled = true;
        Unload -> Enabled = true;
        Reset -> Enabled = false;
    }
	else if((TapeUnit -> GetFileName()).Length() > 0) {
        Mount -> Enabled = true;
        LoadRewind -> Enabled = true;
        Start -> Enabled = false;
        Unload -> Enabled = false;
        Reset -> Enabled = false;
    }
    else {
        Mount -> Enabled = true;
        LoadRewind -> Enabled = false;
        Start -> Enabled = false;
        Unload -> Enabled = false;
        Reset -> Enabled = false;
    }

    Refresh();
}

//  Method to set the TAU array

void TFI729::SetTAU(TTapeTAU *T, int ch)
{
    assert(ch >= 0 && ch < MAXCHANNEL);
    TAU[ch] = T;
}

void __fastcall TFI729::MountClick(TObject *Sender)
{
    if(TapeUnit != NULL && FileMountDialog -> Execute()) {
		Filename -> Caption = FileMountDialog -> FileName;
		TapeUnit -> Mount(Filename -> Caption /* .c_str() */ );
        Display();
    };
}
//---------------------------------------------------------------------------

