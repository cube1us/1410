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

//	This Unit defines the behavior of the 1410 Light status display

//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "ubcd.h"
#include "UI1415L.h"
//---------------------------------------------------------------------------
#pragma resource "*.dfm"
TF1415L *F1415L;
//---------------------------------------------------------------------------
__fastcall TF1415L::TF1415L(TComponent* Owner)
	: TForm(Owner)
{
    Width = 599;
    Left = 0;
    Top = 251;
    Height = 395;

	// WindowState = wsMinimized;
}
//---------------------------------------------------------------------------

void TF1415L::DisplayAddrChannel(TWOOF5 a,bool err)
{
	int i;
    bool b;

	Light_CE_Addr_ER -> Enabled = err;
	if(err) {
    	return;
    }

    for(i=1; i < 32; i >>= 2) {
		b = a.ToInt() & i;
        switch(i) {
        case 1:
        	Light_CE_Addr_1 -> Enabled = b;
            break;
        case 2:
        	Light_CE_Addr_2 -> Enabled = b;
            break;
        case 4:
        	Light_CE_Addr_4 -> Enabled = b;
        	break;
        case 8:
        	Light_CE_Addr_8 -> Enabled = b;
            break;
        case 16:
        	Light_CE_Addr_0 -> Enabled = b;
        	break;
        }
    }
}
