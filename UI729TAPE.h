//---------------------------------------------------------------------------
#ifndef UI729TAPEH
#define UI729TAPEH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
#include <Dialogs.hpp>
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

class TFI729 : public TForm
{
__published:	// IDE-managed Components
    TLabel *Unit;
    TUpDown *UnitDial;
    TButton *LoadRewind;
    TButton *Start;
    TButton *ChangeDensity;
    TButton *Unload;
    TButton *Reset;
    TLabel *Ready;
    TLabel *Select;
    TLabel *FileProtect;
    TLabel *TapeIndicate;
    TButton *ChannelSelect;
    TLabel *Label1;
    TLabel *HighDensity;
    TButton *Mount;
    TOpenDialog *FileMountDialog;
    TLabel *Filename;
    TLabel *RecordNum;
    void __fastcall UnitDialClick(TObject *Sender, TUDBtnType Button);
    void __fastcall LoadRewindClick(TObject *Sender);
    void __fastcall StartClick(TObject *Sender);
    void __fastcall ChangeDensityClick(TObject *Sender);
    void __fastcall UnloadClick(TObject *Sender);
    void __fastcall ResetClick(TObject *Sender);
    void __fastcall ChannelSelectClick(TObject *Sender);
    void __fastcall MountClick(TObject *Sender);
private:	// User declarations

    int current_channel;
    int current_unit;
    TTapeTAU *TAU[MAXCHANNEL];
    TTapeUnit *TapeUnit;

public:		// User declarations

    __fastcall TFI729(TComponent* Owner);
    void SetTAU(TTapeTAU *TAU,int channel);
    void Display();
};
//---------------------------------------------------------------------------
extern PACKAGE TFI729 *FI729;
//---------------------------------------------------------------------------
#endif
