#ifndef UBCDH
#define UBCDH

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

//
//	Definition of character representation
//

#define BITWM 0x80
#define BIT1 1
#define BIT2 2
#define BIT4 4
#define BIT8 8
#define BITA 0x10
#define BITB 0x20
#define BITC 0x40

#define	BIT_NUM  0x0f
#define BIT_ZONE 0x30

#define BCD_0	(10 | BITC)
#define BCD_1   (1)
#define	BCD_9	(9 | BITC)

#define BCD_AMPERSAND   (48 | BITC)
#define BCD_SPACE       (0 | BITC)
#define BCD_COMMA       (27 | BITC)
#define BCD_DOLLAR      (43 | BITC)
#define BCD_ASTERISK    44
#define BCD_MINUS       (32 | BITC)
#define BCD_PERIOD      59

#define BCD_WS          29
#define BCD_TM          15

extern char bcd_ascii[];
extern int ascii_bcd[];
extern int bcd_to_two_of_five_table[];
extern int two_of_five_to_bin_table[];
extern int bin_to_two_of_five_table[];
extern int parity_table[];
extern int odd_parity_table[];

enum bcd_char_type { BCD_NN = 1, BCD_SC = 2, BCD_AN = 3 };

extern bcd_char_type bcd_char_type_table[];

extern char collating_table[];

//
//	These are defined so that keyboard input may recognize them
//

#define ASCII_RECORD_MARK	0174
#define ASCII_GROUP_MARK    0316
#define ASCII_SEGMENT_MARK	0327
#define ASCII_RADICAL		0373
#define ASCII_ALT_BLANK		'b'
#define ASCII_WORD_SEPARATOR '^'
#define ASCII_DELTA			0177

class BCD {

private:
	int c;							// WM C B A 8 4 2 1

public:

	BCD() { c = 0; }				// Default constructor

    BCD(int i) { c = i; }

	void Set(int i) { c = i; }

	static inline int BCDConvert(int ch) {
        return( (ascii_bcd[ch] < 0) ?
                ascii_bcd[ASCII_ALT_BLANK] :
   		        ascii_bcd[ch] );
    }

    static inline int BCDCheck(int ch) {
    	return(ascii_bcd[ch]);
    }

    //	Ascii translation ignores wordmark and parity bits.

	inline char ToAscii() {
    		return bcd_ascii[c & 077];
    }

    inline int ToInt() {
    	return c;
    }

    inline int To6Bit() {
        return c & 0x3f;
    }

    inline bool IsMinus() {
    	return((c & BIT_ZONE) == BITB);
    }

    inline bool TestWM() {
    	return((c & BITWM) != 0);
    }

    inline bool TestRM() {
        return((c & 0x3f) == 26);
    }

    inline bool TestGMWM() {
        return((c & (0x3f | BITWM)) == (63 | BITWM));
    }

    inline bool TestCheck() {
    	return((c & BITC) != 0);
    }

    inline bool TestChar(int ch) {
        return((c & 0x3f) == (ch & 0x3f));
    }

    inline void SetWM() {
    	c |= BITWM;
    }

    inline void ClearWM() {
    	c &= ~BITWM;
    }

    inline void SetCheck() {
    	c |= BITC;
    }

    inline void ComplementCheck() {
    	c ^= BITC;
    }

    inline void ClearCheck() {
    	c &= ~BITC;
    }

    inline bool CheckParity() {
    	return(odd_parity_table[c]);
    }

    inline void SetOddParity() {
    	if(!odd_parity_table[c]) {
        	c ^= BITC;
        }
    }

    //	Routine to test the parity of a BCD character
	//	Returns 0 for Even Parity, 1 for Odd Parity
	//	The test includes the WM bit, but does NOT itself
	//	include the check bit.

    inline int GetParity() { return(parity_table[c]); }

    //	Overloaded standard bit operator functions

    BCD operator&(BCD bcd) {
    	return(BCD(c & bcd.c));
    }

    BCD operator&(int i) {
    	return(BCD(c & i));
    }

    BCD operator|(BCD bcd) {
    	return(BCD(c | bcd.c));
    }

    BCD operator|(int i) {
    	return(BCD(c | i));
    }

    BCD operator<<(int shift) {
    	return(BCD(c << shift));
    }

    BCD operator>>(int shift) {
    	return(BCD(c >> shift));
    }

    bool operator== (BCD bcd) {
    	return(c == bcd.c);
    }

    bool operator != (BCD bcd) {
    	return(c != bcd.c);
    }

    bool operator== (int i) {
    	return(c == i);
    }

    bool operator!= (int i) {
    	return(c != i);
    }

    enum bcd_char_type GetType() {
    	return bcd_char_type_table[c & 0x3f];
    }

};

//	Class for the 2 of 5 code used in address registers.  The "0 bit"
//	(really a parity bit) is encoded as 16.

#define	TWOOF5_1	1
#define TWOOF5_2	2
#define TWOOF5_4	4
#define TWOOF5_8	8
#define TWOOF5_0	16

class TWOOF5 {

private:
	int b;

public:

	TWOOF5() { b = 0; }
    TWOOF5(BCD bcd) { b = bcd_to_two_of_five_table[bcd.ToInt() & 0x7f]; }
    TWOOF5(int i) { b = bin_to_two_of_five_table[i]; }
	inline int ToInt() { return two_of_five_to_bin_table[b]; }
    inline int ToBCD() {
    	return(ascii_bcd['0' + ToInt()]);
    };
};

#endif

