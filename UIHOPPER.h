//---------------------------------------------------------------------------
#ifndef UIHOPPERH
#define UIHOPPERH
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

#define HOPPER_R0   0
#define HOPPER_R1   1
#define HOPPER_R2   2
#define HOPPER_P8   2
#define HOPPER_P4   3
#define HOPPER_P0   4

class THopper;                                      //  Forward declearation.

//  Very simple class (almost a "struct") to represent a card image

class TCard : public TObject {

private:

    THopper *Hopper;

public:

    TCard();                                    //  Constructor
    unsigned char image[80];                    //  80 columns of data
    void SelectStacker(THopper *h);             //  Select Stacker
    bool Stack();                               //  Stack card to hopper
};

//  Class to represent a card Hopper on a Reader/Punch

class THopper: public TObject {

private:

    int Count;                                          //  Count of cards
    String Filename;                                    //  Corresponding file
    TFileStream *fd;                                    //  File to write to

public:

    THopper();                                          //  Constructor

    inline void resetCount() { Count = 0; }
    inline void incCount() { ++Count; }
    inline int getCount() { return Count; }

	bool setFilename(String s);
	String getFilename() { return Filename; }

    bool Stack(TCard *card);
};

#endif
