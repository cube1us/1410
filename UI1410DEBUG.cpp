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

#include <stdio.h>
#include "UI1410DEBUG.h"
//---------------------------------------------------------------------------
#pragma resource "*.dfm"

TF1410Debug *F1410Debug;
//---------------------------------------------------------------------------
__fastcall TF1410Debug::TF1410Debug(TComponent* Owner)
	: TForm(Owner)
{
	// WindowState = wsMinimized;
	fileName = L"";
	debugFd = NULL;

	fileNameLabel -> Caption = L"";
	fileButton -> Caption = L"File...";
}
//---------------------------------------------------------------------------

//	Routine to put out some debugging output onto the debugging panel

void TF1410Debug::DebugOut(char const *s)
{
    if(Debug -> Lines -> Capacity > 512) {
        Debug -> Lines -> Delete(0);
    }
	Debug -> Lines -> Add(s);
	if(debugFd != NULL) {
		debugFd -> Write(s,strlen(s));
		debugFd -> Write("\r\n",2);
    }
}

void TF1410Debug::Minimize() {
	WindowState = wsMinimized;
}

void __fastcall TF1410Debug::fileButtonClick(TObject *Sender)
{

	//  If we were already capturing, close that file and return.

	if(debugFd != NULL) {
		delete debugFd;
		debugFd = NULL;
		fileButton -> Caption = L"File...";
		return;
	}

	//  Otherwise, try and open a new one...

	if(FileOpenDialog -> Execute()) {
		fileName = FileOpenDialog -> FileName;
		try {
			debugFd = new TFileStream(fileName, fmCreate | fmOpenWrite |
				fmShareDenyWrite);
		} catch (EFOpenError &e) {
			debugFd = NULL;
			DEBUG("Debug File Open: file open failed");
			DEBUG("%s",AnsiString(e.Message).c_str());
			return;
		}

		fileButton -> Caption = L"Close...";
		DEBUG("Debug Log File now set to %ls",fileName.c_str());
		fileNameLabel -> Caption = ExtractFileName(fileName);
	}
}
//---------------------------------------------------------------------------

