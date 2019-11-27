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

//	This Unit defines the behavior of the 1410 emulator main form

//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <dir.h>

#include "ubcd.h"
#include "UI1410CPUT.h"
#include "UIHOPPER.h"
#include "UI1410CHANNEL.h"
#include "UI14101.h"
//---------------------------------------------------------------------------
#pragma resource "*.dfm"

TFI14101 *FI14101;
//---------------------------------------------------------------------------
__fastcall TFI14101::TFI14101(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TFI14101::Load1Click(TObject *Sender)
{
	if(LoadCoreDialog -> Execute()) {
    	CPU -> LoadCore(LoadCoreDialog -> FileName.c_str());
    }
}
//---------------------------------------------------------------------------
void __fastcall TFI14101::Dump1Click(TObject *Sender)
{
	if(DumpCoreDialog -> Execute()) {
    	CPU -> DumpCore(DumpCoreDialog -> FileName.c_str());
    }
}
//---------------------------------------------------------------------------
