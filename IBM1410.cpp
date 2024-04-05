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
#include <tchar.h>
//---------------------------------------------------------------------------
USEFORM("UERROR.cpp", FError);
USEFORM("UI729TAPE.cpp", FI729);
USEFORM("UI1402.cpp", FI1402);
USEFORM("UI1403.cpp", FI1403);
USEFORM("UI1410DEBUG.cpp", F1410Debug);
USEFORM("UI1410PWR.cpp", FI1410PWR);
USEFORM("UI1415CE.cpp", FI1415CE);
USEFORM("UI1415IO.cpp", FI1415IO);
USEFORM("UI1415L.cpp", F1415L);
USEFORM("UI14101.cpp", FI14101);
//---------------------------------------------------------------------------
#include <dir.h>
#include <stdio.h>

#include "ubcd.h"
#include "UI14101.h"
#include "UI1410CPUT.h"
#include "UIHOPPER.h"
#include "UI1410CHANNEL.h"
#include "UI1410DEBUG.h"
#include "UI1410CPU.h"
#include "UI1415IO.h"
#include "UI1410PWR.h"

int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{

	try
	{
		Application->Initialize();
		Application->CreateForm(__classid(TFI14101), &FI14101);
		Application->CreateForm(__classid(TF1410Debug), &F1410Debug);
		Application->CreateForm(__classid(TFI1415IO), &FI1415IO);
		Application->CreateForm(__classid(TF1415L), &F1415L);
		Application->CreateForm(__classid(TFI1410PWR), &FI1410PWR);
		Application->CreateForm(__classid(TFI1415CE), &FI1415CE);
		Application->CreateForm(__classid(TFI729), &FI729);
		Application->CreateForm(__classid(TFI1403), &FI1403);
		Application->CreateForm(__classid(TFI1402), &FI1402);
		Application->CreateForm(__classid(TFError), &FError);
		// Application->CreateForm(__classid(TFI729), &FI729);
		// Application->CreateForm(__classid(TFI1402), &FI1402);
		// Application->CreateForm(__classid(TF1410Debug), &F1410Debug);
		// Application->CreateForm(__classid(TFI1415IO), &FI1415IO);
		// Application->CreateForm(__classid(TF1415L), &F1415L);
		// Application->CreateForm(__classid(TFI14101), &FI14101);
		Init1410();

		Application->Run();
	}
	catch (Exception &exception)
	{
		Application -> MessageBox(L"Main Program Caught Exception.",
			L"",MB_OK);
		Application->ShowException(&exception);
	}
	return 0;
}
//---------------------------------------------------------------------------
