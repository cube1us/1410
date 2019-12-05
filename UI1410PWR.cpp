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
#include "UI14101.h"
#include "UI1415IO.h"
#include "UI1410PWR.h"
//---------------------------------------------------------------------------
#pragma resource "*.dfm"

#include <assert.h>
#include <stdio.h>

#include "UI1410DEBUG.h"

//	These methods define the operation of the 1410 power panel

TFI1410PWR *FI1410PWR;
//---------------------------------------------------------------------------
__fastcall TFI1410PWR::TFI1410PWR(TComponent* Owner)
	: TForm(Owner)
{
	Mode -> ItemIndex = CPU -> MODE_RUN;
    Height = 522;
    Width = 327;
    Top = 0;
    Left = 600;
}
//---------------------------------------------------------------------------
void __fastcall TFI1410PWR::EmergencyOffClick(TObject *Sender)
{
    int i;

    for(i=0; i < MAXCHANNEL; ++i) {
        CPU -> Channel[i] -> IntEndofTransfer = true;
        CPU -> Channel[i] -> ExtEndofTransfer = true;
    }
	FI14101 -> Close();
    Application -> Terminate();
}
//---------------------------------------------------------------------------
void __fastcall TFI1410PWR::ComputerResetClick(TObject *Sender)
{
	TCpuObject *o;

	DEBUG("Computer Reset")
    for(o = CPU -> ResetList; o != 0; o = o -> NextReset) {
    	o -> OnComputerReset();
    }

    //	Handle any special case latches (which are reset in the above loop)

    CPU -> CompareBLTA -> Set();
    CPU -> I_AR -> Set(1);

    //	Hitting Computer Reset is like clicking STOP too

    StopClick(Sender);

    //	Reset any simple latches as needed

    CPU -> StopLatch = true;
    CPU -> DisplayModeLatch = false;
    CPU -> StorageWrapLatch = false;
    CPU -> ProcessRoutineLatch = false;
    CPU -> BranchTo1Latch = true;
    CPU -> BranchLatch = true;
    CPU -> IRingControl = true;
    CPU -> IndexLatches = 0;
    CPU -> InqReqLatch = false;
    CPU -> Channel[CHANNEL1] -> PriorityRequest &= ~PRINQUIRY;

    //  Reset the busy list

    TBusyDevice::Reset();

    //	Reset the console matrix

   	FI1415IO -> ResetMatrix();

    //	Do a display

    CPU -> Display();

    //	Reset the console

    FI1415IO -> NextLine();
    FI1415IO -> SetState(CONSOLE_IDLE);
}
//---------------------------------------------------------------------------
void __fastcall TFI1410PWR::ModeChange(TObject *Sender)
{
	if(Mode -> ItemIndex >= 0) {
		CPU -> Mode = (T1410CPU::TMode) Mode -> ItemIndex;
    }

    //	Moving the mode switch is equivalent to hitting STOP

    StopClick(Sender);

    CPU -> OffNormal -> Display();
    DEBUG("Mode Switch set to %d",CPU -> Mode)
}
//---------------------------------------------------------------------------
void __fastcall TFI1410PWR::ProgramResetClick(TObject *Sender)
{
	TCpuObject *o;

    DEBUG("Program Reset")
    for(o = CPU -> ResetList; o != 0; o = o -> NextReset) {
    	o -> OnProgramReset();
    }

    //	Handle any special cases

    CPU -> StopLatch = true;
    CPU -> BranchTo1Latch = true;		// Causes I Fetch to start at 00001
    CPU -> BranchLatch = true;
    CPU -> IndexLatches = 0;
    CPU -> InqReqLatch = false;
    CPU -> Channel[CHANNEL1] -> PriorityRequest &= ~PRINQUIRY;

    StopClick(Sender);

    CPU -> Display();
}
//---------------------------------------------------------------------------

void __fastcall TFI1410PWR::StartClick(TObject *Sender)
{
	CPU -> DoStartClick();
}
//---------------------------------------------------------------------------
void __fastcall TFI1410PWR::StopClick(TObject *Sender)
{
	CPU -> StopKeyLatch = true;
    CPU -> ProcessRoutineLatch = false;

   	FI1415IO -> DoDisplay(4);
}
//---------------------------------------------------------------------------

