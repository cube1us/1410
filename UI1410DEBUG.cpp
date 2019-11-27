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
	WindowState = wsMinimized;
}
//---------------------------------------------------------------------------

//	Routine to put out some debugging output onto the debugging panel

void TF1410Debug::DebugOut(char *s)
{
    if(Debug -> Lines -> Capacity > 512) {
        Debug -> Lines -> Delete(0);
    }
	Debug -> Lines -> Add(s);
}
