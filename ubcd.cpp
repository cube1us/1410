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
//	This Unit provides the implementation of bcd characters
//

#include "ubcd.h"

/*
	The following tables were copied from Joseph
	Newcomer's 1401 emulation, in order to provide
	consistent card, tape and print facilities.

	Jay Jaeger, 12/96
*/

/* The following table is given in the order of the 1401 BCD codes, and
	contains the equivalent ASCII codes for printout.
*/
char bcd_ascii[64] = {
			' ',	/* 0           - space */
			'1',	/* 1        1  - 1 */
			'2',	/* 2       2   - 2 */
			'3',	/* 3       21  - 3 */
			'4',	/* 4      4    - 4 */
			'5',	/* 5      4 1  - 5 */
			'6',    /* 6      42   - 6 */
			'7',	/* 7	  421  - 7 */
			'8',	/* 8     8     - 8 */
			'9',	/* 9     8  1  - 9 */
			'0',	/* 10    8 2   - 0 */
			'=',    /* 11    8 21  - number sign (#) or equal*/
			'\'',	/* 12    84    - at sign @ or quote */
			':',    /* 13    84 1  - colon */
			'>',	/* 14    842   - greater than */
			'û',	/* 15    8421  - radical */
			'b',    /* 16   A      - substitute blank */
			'/',	/* 17   A   1  - slash */
			'S',	/* 18   A  2   - S */
			'T',	/* 19   A  21  - T */
			'U',	/* 20   A 4    - U */
			'V',	/* 21   A 4 1  - V */
			'W',	/* 22   A 42   - W */
			'X',	/* 23   A 421  - X */
			'Y',	/* 24   A8     - Y */
			'Z',	/* 25   A8  1  - Z */
			'\174',	/* 26   A8 2   - record mark */
			',',	/* 27   A8 21  - comma */
			'(',	/* 28   A84    - percent % or paren */
			'^',	/* 29   A84 1  - word separator */
			'\\',	/* 30   A842   - left oblique */
			'×',    /* 31   A8421  - segment mark */
			'-',	/* 32  B       - hyphen */
			'J',	/* 33  B    1  - J */
			'K',	/* 34  B   2   - K */
			'L',	/* 35  B   21  - L */
			'M',	/* 36  B  4    - M */
			'N',	/* 37  B  4 1  - N */
			'O',	/* 38  B  42   - O */
			'P',	/* 39  B  421  - P */
			'Q',	/* 40  B 8     - Q */
			'R',	/* 41  B 8  1  - R */
			'!',	/* 42  B 8 2   - exclamation (-0) */
			'$',	/* 43  B 8 21  - dollar sign */
			'*',	/* 44  B 84    - asterisk */
			']',	/* 45  B 84 1  - right bracket */
			';',    /* 46  B 842   - semicolon */
			'\177', /* 47  B 8421  - delta */
			'+',    /* 48  BA      - ampersand or plus */
			'A',	/* 49  BA   1  - A */
			'B',    /* 50  BA  2   - B */
			'C',	/* 51  BA  21  - C */
			'D',	/* 52  BA 4    - D */
			'E',	/* 53  BA 4 1  - E */
			'F',	/* 54  BA 42   - F */
			'G',	/* 55  BA 421  - G */
			'H',	/* 56  BA8     - H */
			'I',	/* 57  BA8  1  - I */
			'?',	/* 58  BA8 2   - question mark */
			'.',	/* 59  BA8 21  - period */
			')',	/* 60  BA84    - lozenge or paren */
			'[',	/* 61  BA84 1  - left bracket */
			'<',	/* 62  BA842   - less than */
			'Î'		/* 63  BA8421  - group mark */
};


/*****************************************************************************
The following table is used to convert ASCII characters to BCD.

Note that it currently is not complete.

The following substitutions or alternate mappings are made:

		ASCII code	BCD		Notes
		----------  ---     -----
		"			N/A		illegal
		%			(		'H' character set representation
		&			+		'H' character set representation
		@			'		'H' character set representation
		#			=		'H' character set representation
		_			N/A		illegal
		^			^		substitute graphic for word-separator
		`			N/A		illegal
		a,c-z		A,C-Z	case folded
		b			b		substitute blank
		{			N/A		illegal
		}           N/A     illegal
        ~			N/A		illegal
		|			Ø		substitute for record mark

*****************************************************************************/

int ascii_bcd[256] = {
	-1,-1,-1,-1,-1,-1,-1,-1,	/* 00 - 07 illegal */
	-1,-1,-1,-1,-1,-1,-1,-1,	/* 010 - 017 illegal */
	-1,-1,-1,-1,-1,-1,-1,-1,	/* 020 - 027 illegal */
	-1,-1,-1,-1,-1,-1,-1,-1,	/* 030 - 037 illegal */

	0,				/* 040 space */
	42,				/* 041 ! */
	-1,				/* 042 " illegal */
	11,				/* 043 # */
	43,				/* 044 $ */
	28,				/* 045 % also ( */
	48,				/* 046 & also + */
	12,				/* 047 ' also @ */

	28,				/* 050 ( also % */
	60,				/* 051 ) also lozenge */
	44,				/* 052 * */
	48,				/* 053 + also & */
	27,				/* 055 , */
	32,				/* 055 - */
	59,				/* 056 . */
	17,				/* 057 / */

	10,				/* 060 0 */
	1,				/* 061 1 */
	2,				/* 062 2 */
	3,				/* 063 3 */
	4,				/* 064 4 */
	5,				/* 065 5 */
	6,				/* 066 6 */
	7,				/* 067 7 */

	8,				/* 070 8 */
	9,				/* 071 9 */
	13,				/* 072 : */
	46,				/* 073 ; */
	62,				/* 074 < */
	11,				/* 075 = also # */
	14,				/* 076 > */
	58,				/* 077 ? */

	12,				/* 0100 @ */
	49,				/* 0101 A */
	50,				/* 0102 B */
	51,				/* 0103 C */
	52,				/* 0104 D */
	53,				/* 0105 E */
	54,				/* 0106 F */
	55,				/* 0107 G */

	56,				/* 0110 H */
	57,				/* 0111 I */
    33,				/* 0112 J */
	34,				/* 0113 K */
	35,				/* 0114 L */
	36,				/* 0115 M */
	37,				/* 0116 N */
	38,				/* 0117 O */

	39,				/* 0120 P */
	40,				/* 0121 Q */
	41,				/* 0122 R */
	18,				/* 0123 S */
	19,				/* 0124 T */
	20,				/* 0125 U */
	21,				/* 0126 V */
	22,				/* 0127 W */

	23,				/* 0130 X */
	24,				/* 0131 Y */
	25,				/* 0132 Z */
	61,				/* 0133 [ */
	30,				/* 0134 \ */
	45,				/* 0135 ] */
	29,				/* 0136 ^ word separator */
	-1,				/* 0137 _ illegal */

	-1,				/* 0140 ` illegal */
	49,				/* 0141 a is A */
	16,				/* 0142 b is substitute blank */
	51,				/* 0143 c is C */
	52,				/* 0144 d is D */
	53,				/* 0145 e is E */
	54,				/* 0146 f is F */
	55,				/* 0147 g is G */

	56,				/* 0150 h is H */
	57,				/* 0151 i is I */
	33,				/* 0152 j is J */
	34,				/* 0153 k is K */
	35,				/* 0154 l is L */
	36,				/* 0155 m is M */
	37,				/* 0156 n is N */
	38,				/* 0157 o is O */

	39,				/* 0160 p is P */
	40,				/* 0161 q is Q */
	41,				/* 0162 r is R */
	18,				/* 0163 s is S */
	19,				/* 0164 t is T */
	20,				/* 0165 u is U */
	21,				/* 0166 v is V */
	22,				/* 0167 w is W */

	23,				/* 0170 x is X */
	24,				/* 0171 y is Y */
	25,				/* 0172 z is Z */
	-1,				/* 0173 { illegal */
	26,				/* 0174 | substitute record mark */
	-1,				/* 0175	} illegal */
	-1,				/* 0176 ~ illegal */
	47,				/* 0177  delta */

	-1,-1,-1,-1,-1,-1,-1,-1,	/* 0200-0207 illegal */
	-1,-1,-1,-1,-1,-1,-1,-1,	/* 0210-0217 illegal */
	-1,-1,-1,-1,-1,-1,-1,-1,	/* 0220-0227 illegal */
	-1,-1,-1,-1,-1,-1,-1,-1,	/* 0230-0237 illegal */
	-1,-1,-1,-1,-1,-1,-1,-1,	/* 0240-0247 illegal */
	-1,-1,-1,-1,-1,-1,-1,-1,	/* 0250-0257 illegal */
	-1,-1,-1,-1,-1,-1,-1,-1,	/* 0260-0267 illegal */
	-1,-1,-1,-1,-1,-1,-1,-1,	/* 0270-0277 illegal */
	-1,-1,-1,-1,-1,-1,-1,-1,	/* 0300-0307 illegal */

	-1,-1,-1,-1,-1,-1,			/* 0310-0315 illegal */
	63,							/* 0316 group mark   */
	-1,							/* 0317 illegal */

	-1,-1,-1,-1,-1,-1,-1,		/* 0320-0326 illegal */
	31,							/* 0327 segment mark */

	26,							/* 0330 record mark  */
	-1,-1,-1,-1,-1,-1,-1,		/* 0331-0337 illegal */

	-1,-1,-1,-1,-1,-1,-1,-1,	/* 0340-0347 illegal */
	-1,-1,-1,-1,-1,-1,-1,-1,	/* 0350-0357 illegal */
	-1,-1,-1,-1,-1,-1,-1,-1,	/* 0360-0367 illegal */
	-1,-1,-1,					/* 0370-0372 illegal */
	15,							/* 373 radical       */
	-1,-1,-1,-1};				/* 0374-0377 illegal */

//
//	Parity table.  Encode the 64 BCD characters.  Does not itself
//	include the check bit, so the 2nd 64 are the same as the first
//	64.  Does include the WM bit, so the next 128 entries are the
//	opposite of the first 128.
//

int parity_table[256] = {
	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,

	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,

	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,

	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1
};

//	This table is similar to the one above, but is used for checking
//	for correct odd parity.  As a result, the 2nd 64 are the complement
//	of the first 64 (becuase the Check bit is 0x40), and the 2nd 128
//	are the complement of the 1st 128 (because the WM bit is 0x80).

//	Note that you can get a similar thing for even parity by using
//	a logical ! operator on the table below.

int odd_parity_table[256] = {
	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,

   	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,

	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,

	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0
};

//	This table translates valide BCD numerics (odd parity, including
//	check bit).  It assumes that any WM bit has already been stripped.
//	It also assumes that the character has the correct (ODD) parity!
//	Note that the Two Out of Five code "0" bit is here coded as 16.

int bcd_to_two_of_five_table[] = {
	0,17,18,0,20,0,0,12,24,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,3,0,5,6,0,0,9,10,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

int two_of_five_to_bin_table[] = {
	-1,-1,-1,3,-1,5,6,-1,-1,9,0,-1,7,-1,-1,-1,
   	-1,1,2,-1,4,-1,-1,-1,8,-1,-1,-1,-1,-1,-1,-1
};

int bin_to_two_of_five_table[] = {
	10,17,18,3,20,5,6,12,24,9
};

enum bcd_char_type bcd_char_type_table[64] = {
			BCD_NN,	/* 0           - space */
			BCD_AN,	/* 1        1  - 1 */
			BCD_AN,	/* 2       2   - 2 */
			BCD_AN,	/* 3       21  - 3 */
			BCD_AN,	/* 4      4    - 4 */
			BCD_AN,	/* 5      4 1  - 5 */
			BCD_AN, /* 6      42   - 6 */
			BCD_AN,	/* 7	  421  - 7 */
			BCD_AN,	/* 8     8     - 8 */
			BCD_AN,	/* 9     8  1  - 9 */
			BCD_AN,	/* 10    8 2   - 0 */
			BCD_SC, /* 11    8 21  - number sign (#) or equal*/
			BCD_SC,	/* 12    84    - at sign @ or quote */
			BCD_SC, /* 13    84 1  - colon */
			BCD_SC,	/* 14    842   - greater than */
			BCD_SC,	/* 15    8421  - radical */
			BCD_NN, /* 16   A      - substitute blank */
			BCD_SC,	/* 17   A   1  - slash */
			BCD_AN,	/* 18   A  2   - S */
			BCD_AN,	/* 19   A  21  - T */
			BCD_AN,	/* 20   A 4    - U */
			BCD_AN,	/* 21   A 4 1  - V */
			BCD_AN,	/* 22   A 42   - W */
			BCD_AN,	/* 23   A 421  - X */
			BCD_AN,	/* 24   A8     - Y */
			BCD_AN,	/* 25   A8  1  - Z */
			BCD_AN,	/* 26   A8 2   - record mark */
			BCD_SC,	/* 27   A8 21  - comma */
			BCD_SC,	/* 28   A84    - percent % or paren */
			BCD_SC,	/* 29   A84 1  - word separator */
			BCD_SC,	/* 30   A842   - left oblique */
			BCD_SC, /* 31   A8421  - segment mark */
			BCD_NN,	/* 32  B       - hyphen */
			BCD_AN,	/* 33  B    1  - J */
			BCD_AN,	/* 34  B   2   - K */
			BCD_AN,	/* 35  B   21  - L */
			BCD_AN,	/* 36  B  4    - M */
			BCD_AN,	/* 37  B  4 1  - N */
			BCD_AN,	/* 38  B  42   - O */
			BCD_AN,	/* 39  B  421  - P */
			BCD_AN,	/* 40  B 8     - Q */
			BCD_AN,	/* 41  B 8  1  - R */
			BCD_AN,	/* 42  B 8 2   - exclamation (-0) */
			BCD_SC,	/* 43  B 8 21  - dollar sign */
			BCD_SC,	/* 44  B 84    - asterisk */
			BCD_SC,	/* 45  B 84 1  - right bracket */
			BCD_SC, /* 46  B 842   - semicolon */
			BCD_SC, /* 47  B 8421  - delta */
			BCD_NN, /* 48  BA      - ampersand or plus */
			BCD_AN,	/* 49  BA   1  - A */
			BCD_AN,    /* 50  BA  2   - B */
			BCD_AN,	/* 51  BA  21  - C */
			BCD_AN,	/* 52  BA 4    - D */
			BCD_AN,	/* 53  BA 4 1  - E */
			BCD_AN,	/* 54  BA 42   - F */
			BCD_AN,	/* 55  BA 421  - G */
			BCD_AN,	/* 56  BA8     - H */
			BCD_AN,	/* 57  BA8  1  - I */
			BCD_AN,	/* 58  BA8 2   - question mark */
			BCD_SC,	/* 59  BA8 21  - period */
			BCD_SC,	/* 60  BA84    - lozenge or paren */
			BCD_SC,	/* 61  BA84 1  - left bracket */
			BCD_SC,	/* 62  BA842   - less than */
			BCD_SC	/* 63  BA8421  - group mark */
};

//  NOTE:  The following collating sequence table IS NOT ACTUALLY USED
//  in the simulator EXCEPT to verify proper operation of the comparator

char collating_table[] = {
			00, /* 0           - space */
			55,	/* 1        1  - 1 */
			56,	/* 2       2   - 2 */
			57,	/* 3       21  - 3 */
			58,	/* 4      4    - 4 */
			59,	/* 5      4 1  - 5 */
			60, /* 6      42   - 6 */
			61,	/* 7	  421  - 7 */
			62,	/* 8     8     - 8 */
			63,	/* 9     8  1  - 9 */
			54,	/* 10    8 2   - 0 */
			20, /* 11    8 21  - number sign (#) or equal*/
			21,	/* 12    84    - at sign @ or quote */
			22, /* 13    84 1  - colon */
			23,	/* 14    842   - greater than */
			24,	/* 15    8421  - radical */
			19, /* 16   A      - substitute blank */
			13,	/* 17   A   1  - slash */
			46,	/* 18   A  2   - S */
			47,	/* 19   A  21  - T */
			48,	/* 20   A 4    - U */
			49,	/* 21   A 4 1  - V */
			50,	/* 22   A 42   - W */
			51,	/* 23   A 421  - X */
			52,	/* 24   A8     - Y */
			53,	/* 25   A8  1  - Z */
			45,	/* 26   A8 2   - record mark */
			14,	/* 27   A8 21  - comma */
			15,	/* 28   A84    - percent % or paren */
			16,	/* 29   A84 1  - word separator */
			17,	/* 30   A842   - left oblique */
			18, /* 31   A8421  - segment mark */
			12,	/* 32  B       - hyphen */
			36,	/* 33  B    1  - J */
			37,	/* 34  B   2   - K */
			38,	/* 35  B   21  - L */
			39,	/* 36  B  4    - M */
			40,	/* 37  B  4 1  - N */
			41,	/* 38  B  42   - O */
			42,	/* 39  B  421  - P */
			43,	/* 40  B 8     - Q */
			44,	/* 41  B 8  1  - R */
			35,	/* 42  B 8 2   - exclamation (-0) */
			07,	/* 43  B 8 21  - dollar sign */
			8,	/* 44  B 84    - asterisk */
			9,	/* 45  B 84 1  - right bracket */
			10, /* 46  B 842   - semicolon */
			11, /* 47  B 8421  - delta */
			06, /* 48  BA      - ampersand or plus */
			26,	/* 49  BA   1  - A */
			27, /* 50  BA  2   - B */
			28,	/* 51  BA  21  - C */
			29,	/* 52  BA 4    - D */
			30,	/* 53  BA 4 1  - E */
			31,	/* 54  BA 42   - F */
			32,	/* 55  BA 421  - G */
			33,	/* 56  BA8     - H */
			34,	/* 57  BA8  1  - I */
			25,	/* 58  BA8 2   - question mark */
			01, /* 59  BA8 21  - period */
			02,	/* 60  BA84    - lozenge or paren */
			03,	/* 61  BA84 1  - left bracket */
			04,	/* 62  BA842   - less than */
			05	/* 63  BA8421  - group mark */
};

