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
#include "ubcd.h"
#include "UI1410CPUT.h"
#include "UIHOPPER.h"
#include "UI1410CHANNEL.h"
#include "UIREADER.h"
#include "UIHOPPER.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)

#include "UIPUNCH.h"
#include "UI1402.h"
#include "UI1410DEBUG.h"

//  TCard Class Implementation

TCard::TCard() {
    Hopper = NULL;
}

bool TCard::Stack() {

    return(Hopper -> Stack(this));
}

void TCard::SelectStacker(THopper *h) {
    Hopper = h;
}

//  THopper Implementation

//  Constructor

THopper::THopper() {

    Count = 0;
    fd = NULL;
    Filename[0] = '\0';
}

//  Method to stack a card

bool THopper::Stack(TCard *card) {

    char *cp;
    char temp[83];

    incCount();
    if(fd == NULL) {
        FI1402 -> Display();
        return(true);
    }
    memcpy(temp,card->image,80);

    //  Remove trailing blanks, and stick on CRLF

    for(cp = temp+79; cp >= temp && *cp == ' '; --cp) {
    }
    *++cp = '\r';
    *++cp = '\n';
    *++cp = '\0';

    try {
        fd -> WriteBuffer(temp,strlen(temp));
    }
    catch(EWriteError &e) {
        delete fd;
        fd = NULL;
        resetCount();
        DEBUG("THopper::Stack Write Error. Closing file.",0);
        return(false);
    }

    FI1402 -> Display();
    return(true);
}

//  Method to associate (and open) a file with a hopper...

bool THopper::setFilename(char *s) {

    resetCount();
    if(fd != NULL) {
        delete fd;
        fd = NULL;
    }
    if(s == NULL) {
        return(false);
    }

    try {
        fd = new TFileStream(s,fmCreate);
    }
    catch(EFOpenError &e) {
        return(false);
    }

    strncpy(Filename,s,MAXPATH);
    return(true);
}




