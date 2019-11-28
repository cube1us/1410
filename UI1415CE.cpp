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

#include "ubcd.h"
#include "UI1410CPUT.h"
#include "UIHOPPER.h"
#include "UI1410CHANNEL.h"
#include "UI1415IO.h"
#include "UI1415CE.h"
//---------------------------------------------------------------------------
#pragma resource "*.dfm"

#include <stdio.h>

#include "UI1410DEBUG.h"

TFI1415CE *FI1415CE;
//---------------------------------------------------------------------------
__fastcall TFI1415CE::TFI1415CE(TComponent* Owner)
	: TForm(Owner)
{
    Height = 320;
    Width = 600;

	WindowState = wsMinimized;

    AddressEntry -> ItemIndex = CPU -> ADDR_ENTRY_I;
    StorageScan -> ItemIndex = CPU -> SSCAN_OFF;
    CycleControl -> ItemIndex = CPU -> CYCLE_OFF;
    CheckControl -> ItemIndex = CPU -> CHECK_STOP;
    DiskWrInhibit -> Checked = false;
    DensityCh1 -> ItemIndex = 0;
    DensityCh2 -> ItemIndex = 0;
    AsteriskInsert -> Checked = true;
    InhibitPrintOut -> Checked = false;
    SenseBit = 0;
    BitSenseC -> Checked = false;
    BitSenseB -> Checked = false;
    BitSenseA -> Checked = false;
    BitSense8 -> Checked = false;
    BitSense4 -> Checked = false;
    BitSense2 -> Checked = false;
    BitSense1 -> Checked = false;
    BitSenseWM -> Checked = false;
}
//---------------------------------------------------------------------------
void __fastcall TFI1415CE::AddressEntryChange(TObject *Sender)
{
	if(AddressEntry -> ItemIndex >= 0) {
		CPU -> AddressEntry =
			(T1410CPU::TAddressEntry)AddressEntry -> ItemIndex;
	}
    CPU -> OffNormal -> Display();
    DEBUG("Address Entry Switch set to %d",CPU -> AddressEntry)
}
//---------------------------------------------------------------------------
void __fastcall TFI1415CE::StorageScanChange(TObject *Sender)
{
	if(StorageScan -> ItemIndex >= 0) {
		CPU -> StorageScan = (T1410CPU::TStorageScan)StorageScan -> ItemIndex;
    }
    CPU -> OffNormal -> Display();
    DEBUG("Storage Scan Switch set to %d",CPU -> StorageScan)
}
//---------------------------------------------------------------------------
void __fastcall TFI1415CE::CycleControlChange(TObject *Sender)
{
	if(CycleControl -> ItemIndex >= 0) {
		CPU -> CycleControl =
			(T1410CPU::TCycleControl)CycleControl -> ItemIndex;
    }
    CPU -> OffNormal -> Display();
    DEBUG("Cycle Control Switch set to %d",CPU -> CycleControl)
}
//---------------------------------------------------------------------------
void __fastcall TFI1415CE::CheckControlChange(TObject *Sender)
{
	if(CheckControl -> ItemIndex >= 0) {
		CPU -> CheckControl =
			(T1410CPU::TCheckControl)CheckControl -> ItemIndex;
    }
    CPU -> OffNormal -> Display();
    DEBUG("Check Control Switch set to %d",CPU -> CheckControl)
}
//---------------------------------------------------------------------------
void __fastcall TFI1415CE::DiskWrInhibitClick(TObject *Sender)
{
	CPU -> DiskWrInhibit = DiskWrInhibit -> Checked;
    DEBUG("Disk Write Inhibit Switch set to %d",CPU -> DiskWrInhibit)
}
//---------------------------------------------------------------------------
void __fastcall TFI1415CE::DensityCh1Change(TObject *Sender)
{
	if(DensityCh1 -> ItemIndex >= 0) {
		CPU -> Channel[CHANNEL1] -> TapeDensity =
			(T1410Channel::TTapeDensity)DensityCh1 -> ItemIndex;
	}
	DEBUG("Channel 1 Tape Density Switch set to %d",
		CPU -> Channel[CHANNEL1] -> TapeDensity)
}
//---------------------------------------------------------------------------
void __fastcall TFI1415CE::DensityCh2Change(TObject *Sender)
{
	if(DensityCh2 -> ItemIndex >= 0) {
		CPU -> Channel[CHANNEL2] -> TapeDensity =
			(T1410Channel::TTapeDensity)DensityCh2 -> ItemIndex;
	}
    DEBUG("Channel 2 Tape Density Switch set to %d",
    	CPU -> Channel[CHANNEL2] -> TapeDensity)
}
//---------------------------------------------------------------------------
void __fastcall TFI1415CE::AsteriskInsertClick(TObject *Sender)
{
	CPU -> AsteriskInsert = AsteriskInsert -> Checked;
    CPU -> OffNormal -> Display();
    DEBUG("Asterisk Insert Switch set to %d",CPU -> AsteriskInsert)
}
//---------------------------------------------------------------------------
void __fastcall TFI1415CE::InhibitPrintOutClick(TObject *Sender)
{
	CPU -> InhibitPrintOut = InhibitPrintOut -> Checked;
    CPU -> OffNormal -> Display();
    DEBUG("Inhibit Print Out Switch set to %d",CPU -> InhibitPrintOut)
}
//---------------------------------------------------------------------------
void __fastcall TFI1415CE::BitSenseCClick(TObject *Sender)
{
	SetSense(BitSenseC -> Checked,BITC);
}
//---------------------------------------------------------------------------

void TFI1415CE::SetSense(bool b,int i)
{
	SenseBit = (b ? (SenseBit | i) : (SenseBit & ~i));
    CPU -> BitSwitches = BCD(SenseBit);
    DEBUG("Bit/Sense Switches now set to 0x%x",CPU -> BitSwitches.ToInt())
}
void __fastcall TFI1415CE::BitSenseBClick(TObject *Sender)
{
	SetSense(BitSenseB -> Checked,BITB);
}
//---------------------------------------------------------------------------
void __fastcall TFI1415CE::BitSenseAClick(TObject *Sender)
{
	SetSense(BitSenseA -> Checked,BITA);
}
//---------------------------------------------------------------------------
void __fastcall TFI1415CE::BitSense8Click(TObject *Sender)
{
	SetSense(BitSense8 -> Checked,BIT8);	
}
//---------------------------------------------------------------------------
void __fastcall TFI1415CE::BitSense4Click(TObject *Sender)
{
	SetSense(BitSense4 -> Checked,BIT4);	
}
//---------------------------------------------------------------------------
void __fastcall TFI1415CE::BitSense2Click(TObject *Sender)
{
	SetSense(BitSense2 -> Checked,BIT2);
}
//---------------------------------------------------------------------------
void __fastcall TFI1415CE::BitSense1Click(TObject *Sender)
{
	SetSense(BitSense1 -> Checked,BIT1);
}
//---------------------------------------------------------------------------
void __fastcall TFI1415CE::BitSenseWMClick(TObject *Sender)
{
	SetSense(BitSenseWM -> Checked,BITWM);
}
//---------------------------------------------------------------------------
void __fastcall TFI1415CE::StartPrintOutClick(TObject *Sender)
{
    if(!CPU -> InhibitPrintOut) {
		FI1415IO -> StopPrintOut('S');
    }
}
//---------------------------------------------------------------------------
