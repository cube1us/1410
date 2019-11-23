//---------------------------------------------------------------------------
#ifndef UI1402H
#define UI1402H
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

#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Buttons.hpp>
#include <Dialogs.hpp>
#include <ComCtrls.hpp>
//---------------------------------------------------------------------------
class TFI1402 : public TForm
{
__published:	// IDE-managed Components
    TBitBtn *ReaderStart;
    TBitBtn *ReaderStop;
    TBitBtn *EOFButton;
    TBitBtn *PunchStart;
    TBitBtn *PunchStop;
    TLabel *LightPunchReady;
    TLabel *LightPunchCheck;
    TLabel *LightPunchStop;
    TLabel *LightChips;
    TLabel *LightReaderValidity;
    TLabel *LightFuse;
    TLabel *LightPower;
    TLabel *LightTransport;
    TLabel *LightStacker;
    TLabel *LightReaderReady;
    TLabel *LightReaderCheck;
    TLabel *LightReaderStop;
    TButton *LoadReaderHopper;
    TOpenDialog *FileOpenDialog;
    TButton *HopperLoadButton;
    TUpDown *HopperSelect;
    TLabel *HopperNumber;
    TLabel *HopperCount;
    void __fastcall ReaderStartClick(TObject *Sender);
    void __fastcall ReaderStopClick(TObject *Sender);
    void __fastcall EOFButtonClick(TObject *Sender);
    void __fastcall LoadReaderHopperClick(TObject *Sender);
    void __fastcall HopperSelectClick(TObject *Sender, TUDBtnType Button);

    void __fastcall HopperLoadButtonClick(TObject *Sender);
    void __fastcall PunchStartClick(TObject *Sender);
    void __fastcall PunchStopClick(TObject *Sender);
private:	// User declarations
    int current_hopper;

public:		// User declarations
    __fastcall TFI1402(TComponent* Owner);

    void ResetEOF();
    void SetReaderCheck(bool flag);
    void SetReaderValidity(bool flag);
    void SetReaderReady(bool flag);
    void SetPunchReady(bool flag);
    void Display();

    TCardReader *ReaderIODevice;
    TPunch *PunchIODevice;
    T1410Channel *Channel;
};
//---------------------------------------------------------------------------
extern PACKAGE TFI1402 *FI1402;
//---------------------------------------------------------------------------
#endif
