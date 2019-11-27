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

#include "UI1410DATA.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)

#include <assert.h>
#include <dir.h>
#include <stdio.h>

#include "ubcd.h"
#include "UI1410CPUT.h"
#include "UIHOPPER.h"
#include "UI1410CHANNEL.h"
#include "UI1410DEBUG.h"
#include "UI1410INST.h"

int edit_char_flag [64] = {
			SZ_SUPPRESS_IF_ZERO |
            EDIT_UNITS_SPECIAL |
            EDIT_BODY_SPECIAL |
            EDIT_SUPPRESS_IF_NS,    /* 0           - space */
			SZ_SIGNIFICANT_DIGIT,	/* 1        1  - 1 */
			SZ_SIGNIFICANT_DIGIT,	/* 2       2   - 2 */
			SZ_SIGNIFICANT_DIGIT,	/* 3       21  - 3 */
			SZ_SIGNIFICANT_DIGIT,   /* 4      4    - 4 */
			SZ_SIGNIFICANT_DIGIT,	/* 5      4 1  - 5 */
			SZ_SIGNIFICANT_DIGIT,   /* 6      42   - 6 */
			SZ_SIGNIFICANT_DIGIT,   /* 7	  421  - 7 */
			SZ_SIGNIFICANT_DIGIT,   /* 8     8     - 8 */
			SZ_SIGNIFICANT_DIGIT,	/* 9     8  1  - 9 */
			SZ_SUPPRESS_IF_ZERO |
            EDIT_UNITS_SPECIAL |
            EDIT_BODY_SPECIAL |
            EDIT_SUPPRESS_IF_NS,    /* 10    8 2   - 0 */
			0,                      /* 11    8 21  - number sign (#) or equal*/
			0,                  	/* 12    84    - at sign @ or quote */
			0,                      /* 13    84 1  - colon */
			0,                      /* 14    842   - greater than */
			0,                      /* 15    8421  - radical */
			0,                      /* 16   A      - substitute blank */
			0,                      /* 17   A   1  - slash */
			0,                      /* 18   A  2   - S */
			0,                      /* 19   A  21  - T */
			0,                      /* 20   A 4    - U */
			0,                      /* 21   A 4 1  - V */
			0,                      /* 22   A 42   - W */
			0,                      /* 23   A 421  - X */
			0,                      /* 24   A8     - Y */
			0,                      /* 25   A8  1  - Z */
			0,                      /* 26   A8 2   - record mark */
			SZ_SUPPRESS_IF_ZERO |
            EDIT_EXT_SPECIAL |
            EDIT_SUPPRESS_IF_NS,   	/* 27   A8 21  - comma */
			0,                      /* 28   A84    - percent % or paren */
			0,                      /* 29   A84 1  - word separator */
			0,                      /* 30   A842   - left oblique */
			0,                      /* 31   A8421  - segment mark */
			SZ_INCLUDE_IF_ZERO |
            EDIT_SUPPRESS_IF_PLUS |
            EDIT_UNITS_SPECIAL |
            EDIT_EXT_SPECIAL |
            EDIT_SUPPRESS_IF_NS,  	/* 32  B       - hyphen */
			0,                      /* 33  B    1  - J */
			0,                      /* 34  B   2   - K */
			0,                      /* 35  B   21  - L */
			0,                      /* 36  B  4    - M */
			0,                      /* 37  B  4 1  - N */
			0,                      /* 38  B  42   - O */
			0,                      /* 39  B  421  - P */
			0,                      /* 40  B 8     - Q */
			EDIT_SUPPRESS_IF_PLUS |
            EDIT_UNITS_SPECIAL |
            EDIT_EXT_SPECIAL,       /* 41  B 8  1  - R */
			0,                      /* 42  B 8 2   - exclamation (-0) */
			EDIT_BODY_SPECIAL,      /* 43  B 8 21  - dollar sign */
			EDIT_BODY_SPECIAL,      /* 44  B 84    - asterisk */
			0,                      /* 45  B 84 1  - right bracket */
			0,                      /* 46  B 842   - semicolon */
			0,                      /* 47  B 8421  - delta */
			EDIT_UNITS_SPECIAL |
            EDIT_EXT_SPECIAL |
            EDIT_BODY_SPECIAL,      /* 48  BA      - ampersand or plus */
			0,                      /* 49  BA   1  - A */
			0,                      /* 50  BA  2   - B */
			EDIT_SUPPRESS_IF_PLUS |
            EDIT_UNITS_SPECIAL |
            EDIT_EXT_SPECIAL,       /* 51  BA  21  - C */
			0,                      /* 52  BA 4    - D */
			0,                      /* 53  BA 4 1  - E */
			0,                      /* 54  BA 42   - F */
			0,                      /* 55  BA 421  - G */
			0,                      /* 56  BA8     - H */
			0,                      /* 57  BA8  1  - I */
			0,                      /* 58  BA8 2   - question mark */
			SZ_INCLUDE_IF_ZERO |
            EDIT_SUPPRESS_IF_NS,    /* 59  BA8 21  - period */
			0,                      /* 60  BA84    - lozenge or paren */
			0,                      /* 61  BA84 1  - left bracket */
			0,                      /* 62  BA842   - less than */
			0,                      /* 63  BA8421  - group mark */
};


void T1410CPU::InstructionMove()
{

    static struct {
        char cycle;
        char subcycle;
        char scan;
        char subscan;
    } next;

    static int op_mod_bin;
    BCD b_temp;

    enum TAssemblyChannel::AsmChannelNumericSelect AsmChannelNumericSelect;
    enum TAssemblyChannel::AsmChannelZonesSelect AsmChannelZonesSelect;
    enum TAssemblyChannel::AsmChannelWMSelect AsmChannelWMSelect;

    if(LastInstructionReadout) {
        op_mod_bin = Op_Mod_Reg -> Get().ToInt();
        next.scan = (op_mod_bin & BIT8) ? SCAN_2 : SCAN_1;
        ScanRing -> Set(next.scan);
        next.subscan = SUB_SCAN_U;
        SubScanRing -> Set(next.subscan);
        next.cycle = CYCLE_A;
        next.subcycle = 0;
    }

    //  Do common items for all cycles

    CycleRing -> Set(next.cycle);
    ScanRing -> Set(next.scan);
    SubScanRing -> Set(next.subscan);

    //  The "if" statements use the next variable for easy coding

    if(next.cycle == CYCLE_A && next.subcycle == 0) {
        *STAR = *A_AR;
        Readout();                                      //  Readout A field char
        Cycle();
        next.cycle = CYCLE_B;                           //  B Cycle next
        next.subcycle = 0;
        return;
    }

    if(next.cycle  == CYCLE_B && next.subcycle == 0) {
        *STAR = *B_AR;
        Readout();                                      //  Readout B field char
        b_temp = B_Reg -> Get();                        //  Save B before move
        AsmChannelNumericSelect = (op_mod_bin & BIT1) ?
            AssemblyChannel -> AsmChannelNumA :
            AssemblyChannel -> AsmChannelNumB;
        AsmChannelZonesSelect = (op_mod_bin & BIT2) ?
            AssemblyChannel -> AsmChannelZonesA :
            AssemblyChannel -> AsmChannelZonesB;
        AsmChannelWMSelect = (op_mod_bin & BIT4) ?
            AssemblyChannel -> AsmChannelWMA :
            AssemblyChannel -> AsmChannelWMB;
        Store( AssemblyChannel -> Select (              //  Store the result
            AsmChannelWMSelect,
            AsmChannelZonesSelect,
            false,
            AssemblyChannel -> AsmChannelSignNone,
            AsmChannelNumericSelect ) );

        Cycle();
        next.cycle = CYCLE_A;                           //  For now, assume we
        next.subcycle = 0;                              //  will continue on
        next.subscan = SUB_SCAN_B;                      //  next will be body
        //  Regen Scan Ctrl (1 or 2)

        //  The next bits set IRingControl if we should stop here...

        if( !(op_mod_bin & (BIT8 | BITA | BITB)) ) {    //  None of 8, A, B
            IRingControl = true;                        //  single char - done
        }
        else if( !(op_mod_bin & (BIT8)) ) {             //  No 8 bit - R to L
            if( ((op_mod_bin & BITA) && A_Reg -> Get().TestWM()) ||
                ((op_mod_bin & BITB) && b_temp.TestWM()) ) {
                IRingControl = true;                    //  Stop on approp. WM
            }
        }
        else if( !(op_mod_bin & (BITA | BITB)) ) {      //  8, not A or B
            IRingControl = (A_Reg -> Get().TestWM() || b_temp.TestWM());
        }
        else if(op_mod_bin & (BITA | BITB)) {
            IRingControl = ((op_mod_bin & BITA) && A_Reg -> Get().TestRM() ) ||
                ((op_mod_bin & BITB) && A_Reg -> Get().TestGMWM() );
        }
        else {
            assert(false);                              //  Sould not get here
        }

        return;
    }   //  End B Cycle, subtype 0

}

void T1410CPU::InstructionMoveSuppressZeros()
{

    static struct {
        char cycle;
        char subcycle;
        char scan;
        char subscan;
    } next;

    enum TAssemblyChannel::AsmChannelNumericSelect AsmChannelNumericSelect;
    enum TAssemblyChannel::AsmChannelWMSelect AsmChannelWMSelect;
    enum TAssemblyChannel::AsmChannelZonesSelect AsmChannelZonesSelect;

    int sz_char_flags;

    if(LastInstructionReadout) {
        next.scan = SCAN_1;
        ScanRing -> Set(next.scan);
        next.subscan = SUB_SCAN_U;
        SubScanRing -> Set(next.subscan);
        next.cycle = CYCLE_A;
        next.subcycle = 0;
        ZeroSuppressLatch = false;
    }

    //  Do common items for all cycles

    CycleRing -> Set(next.cycle);
    ScanRing -> Set(next.scan);
    SubScanRing -> Set(next.subscan);

    //  The "if" statements use the "next" variable for easy coding

    if(next.cycle == CYCLE_A && next.subcycle == 0) {
        *STAR = *A_AR;
        Readout();                                  //  Read out A field char
        Cycle();
        next.cycle = CYCLE_B;
        next.subcycle = 0;
        return;
    }   //  End A Cycle

    if(next.cycle == CYCLE_B && next.subcycle == 0) {
        *STAR = *B_AR;
        Readout();                                  //  Read out B field char

        if(SubScanRing -> State() == SUB_SCAN_U) {    //  Units?
            AsmChannelZonesSelect = AssemblyChannel -> AsmChannelZonesNone;
            AsmChannelWMSelect = AssemblyChannel -> AsmChannelWMSet;
            ZeroSuppressLatch = true;
        }
        else {                                      //  No (body)
            assert(SubScanRing -> State() == SUB_SCAN_B);
            AsmChannelZonesSelect = AssemblyChannel -> AsmChannelZonesA;
            AsmChannelWMSelect = AssemblyChannel -> AsmChannelWMNone;
        }

        AsmChannelNumericSelect = AssemblyChannel -> AsmChannelNumA;

        Store( AssemblyChannel -> Select (
            AsmChannelWMSelect,
            AsmChannelZonesSelect,
            false,
            AssemblyChannel -> AsmChannelSignNone,
            AsmChannelNumericSelect ) );

        Cycle();

        if(A_Reg -> Get().TestWM()) {               //  AChannel WM?
            next.cycle = CYCLE_B;                   //  Yes.  Start 2nd scan
            next.subcycle = 1;
            next.scan = SCAN_2;
            next.subscan = SUB_SCAN_MQ;             //  Set MQ latch for Skid
            return;
        }
        else {                                      //  No A Channel WM
            next.cycle = CYCLE_A;                   //  A cycle next
            next.subcycle = 0;                      //  First scan
            assert(ScanRing -> State() == SCAN_1);  //  Regen 1st scan
            next.subscan = SUB_SCAN_B;              //  Set Body Latch
            return;
        }
    }   //  End B Cycle, subtype 0

    if(next.cycle == CYCLE_B && next.subcycle == 1) {
        *STAR = *B_AR;
        Readout();                                  //  RO B field char
        AssemblyChannel -> Reset();                 //  Clear Asm. channel flags

        if(SubScanRing -> State() == SUB_SCAN_MQ) { //  Skid Cycle (MQ) ?
            next.subscan = SUB_SCAN_E;              //  Yes. Extension next
            Store( AssemblyChannel -> Select (
                AssemblyChannel -> AsmChannelWMB,   //  B Ch WM
                AssemblyChannel -> AsmChannelZonesB, // B Character
                false,
                AssemblyChannel -> AsmChannelSignNone,
                AssemblyChannel -> AsmChannelNumB) );
            Cycle();
            assert(ScanRing -> State() == SCAN_2);  //  Regen 2nd scan
            return;                                 //  Next cycle same kind
        }

        sz_char_flags = edit_char_flag[B_Reg -> Get().ToInt() & 0x3f];
        if(sz_char_flags & SZ_SIGNIFICANT_DIGIT) {
            ZeroSuppressLatch = false;
        }
        else if(!(sz_char_flags & (SZ_INCLUDE_IF_ZERO | SZ_SUPPRESS_IF_ZERO))) {
            //  NOT:  , 0 sp . -
            ZeroSuppressLatch = true;
        }
        else if(sz_char_flags & SZ_INCLUDE_IF_ZERO) {
            //  . or - will just Store B w/o WM (later)
        }
        else {
            assert(sz_char_flags & SZ_SUPPRESS_IF_ZERO);    //  Must be sp , 0
                if(ZeroSuppressLatch) {
                    AssemblyChannel -> Set(BITC);           //  Special - blank
                }
        }

        if(AssemblyChannel -> SpecialChar()) {
            Store(AssemblyChannel -> Select());             //  Store special
        }
        else {
            Store( AssemblyChannel -> Select (
                AssemblyChannel -> AsmChannelWMNone,        //  Store, no WM
                AssemblyChannel -> AsmChannelZonesB,        //  B Char
                false,
                AssemblyChannel -> AsmChannelSignNone,
                AssemblyChannel -> AsmChannelNumB) );
        }

        Cycle();

        if(B_Reg -> Get().TestWM()) {                       //  Did org. B WM?
            IRingControl = true;                            //  If so, done
            return;
        }
        else {
            assert(SubScanRing -> State() == SUB_SCAN_E);   //  Regen Extension
            assert(ScanRing -> State() == SCAN_2);          //  Regen 2nd scan
            //  Next cycle same kind.
            return;
        }
    }   //  End B Cycle, subtype 1

}   //  End, Move and Suppress Zeros



void T1410CPU::InstructionEdit()
{
    static struct {
        char cycle;
        char subcycle;
        char scan;
        char subscan;
    } next;

    bool FirstBFieldZero = false;
    int edit_char_flags;
    int first_scan_store_type = 0;
    BCD b_temp;

    enum TAssemblyChannel::AsmChannelZonesSelect AsmChannelZonesSelect;
    enum TAssemblyChannel::AsmChannelWMSelect AsmChannelWMSelect;

    if(LastInstructionReadout) {
        next.scan = SCAN_1;
        ScanRing -> Set(next.scan);
        next.subscan = SUB_SCAN_U;
        SubScanRing -> Set(next.subscan);
        next.cycle = CYCLE_A;
        next.subcycle = 0;
        SignLatch = false;
        FloatingDollarLatch = false;
        AsteriskFillLatch = false;
        DecimalControlLatch = false;
        ZeroSuppressLatch = false;
    }

    //  Do common items for all cycles

    CycleRing -> Set(next.cycle);
    ScanRing -> Set(next.scan);
    SubScanRing -> Set(next.subscan);

    //  The "if" statements use the next variable for easy coding

    if(next.cycle == CYCLE_A && next.subcycle == 0) {
        *STAR = *A_AR;
        Readout();                                      //  Readout A char
        if(SubScanRing -> State() == SUB_SCAN_U) {
            SignLatch = B_Reg -> Get().IsMinus();       //  Determine sign
                                                        //  While in B_Reg
        }
        Cycle();
        next.cycle = CYCLE_B;
        next.subcycle = 0;
        return;
    }   //  End A Cycle, subtype 0

    if(next.cycle == CYCLE_B && next.subcycle == 0) {
        *STAR = *B_AR;
        Readout();                                      //  Readout B field char
        b_temp = B_Reg -> Get();
        AssemblyChannel -> Reset();                     //  Init to not special

        if(b_temp.TestChar(BCD_0) && !ZeroSuppressLatch) {
            FirstBFieldZero = ZeroSuppressLatch = true;
        }

        edit_char_flags = edit_char_flag[b_temp.ToInt() & 0x3f];

        if(SubScanRing -> State() == SUB_SCAN_U) {      //  Units
            if(edit_char_flags & EDIT_UNITS_SPECIAL) {
                if(((edit_char_flags & EDIT_SUPPRESS_IF_PLUS) && !SignLatch) ||
                    b_temp.TestChar(BCD_AMPERSAND) ) {
                        first_scan_store_type = 2;
                }
                else if(b_temp.TestChar(BCD_SPACE) || b_temp.TestChar(BCD_0)) {
                    first_scan_store_type = 4;
                }
                else {
                    assert((edit_char_flags & EDIT_SUPPRESS_IF_PLUS) && SignLatch);
                    first_scan_store_type = 1;
                }
            }   //  End Units Special
            else {
                first_scan_store_type = 1;
            }
        }   //  End Units

        else if(SubScanRing -> State() == SUB_SCAN_E) {   //  Extension
            if(edit_char_flags & EDIT_EXT_SPECIAL) {
                if(((edit_char_flags & EDIT_SUPPRESS_IF_PLUS) && !SignLatch) ||
                    b_temp.TestChar(BCD_AMPERSAND) ||
                    b_temp.TestChar(BCD_COMMA) ) {
                    first_scan_store_type = 2;
                }
                else {
                    assert((edit_char_flags & EDIT_SUPPRESS_IF_PLUS) && SignLatch);
                    first_scan_store_type = 1;
                }
            }   //  End Ext. Special
            else {
                first_scan_store_type = 1;
            }
        }   //  End Extension

        else {
            assert(SubScanRing -> State() == SUB_SCAN_B);   //  Body
            if(edit_char_flags & EDIT_BODY_SPECIAL) {
                if(b_temp.TestChar(BCD_AMPERSAND)) {
                    first_scan_store_type = 2;
                }
                else if(b_temp.TestChar(BCD_0) || b_temp.TestChar(BCD_SPACE)) {
                    first_scan_store_type = 3;
                }
                else {
                    assert(b_temp.TestChar(BCD_DOLLAR) ||
                        b_temp.TestChar(BCD_ASTERISK));
                    if(ZeroSuppressLatch && !FloatingDollarLatch &&
                        !AsteriskFillLatch) {
                        FloatingDollarLatch = b_temp.TestChar(BCD_DOLLAR);
                        AsteriskFillLatch = b_temp.TestChar(BCD_ASTERISK);
                    }
                    first_scan_store_type = 3;
                }

            }   //  End Body Special
            else {
                first_scan_store_type = 1;
            }
        }   //  End Body

        if(first_scan_store_type == 1 || first_scan_store_type == 2) {

            if(first_scan_store_type == 2) {        //  Point 2: Store a blank
                AssemblyChannel -> Set(BCD_SPACE);  //  Generate a space
                Store(AssemblyChannel -> Select()); //  Store the space
            }
            else {
                assert(first_scan_store_type == 1); //  Point 1: Store B NO WM!
                Store( AssemblyChannel -> Select (
                    TAssemblyChannel::AsmChannelWMNone,
                    TAssemblyChannel::AsmChannelZonesB,
                    false,
                    TAssemblyChannel::AsmChannelSignNone,
                    TAssemblyChannel::AsmChannelNumB) );
            }

            Cycle();
            if(b_temp.TestWM()) {                   //  B Channel WM? Yes...
                if(ZeroSuppressLatch) {             //  Zeros found?  Yes...
                    next.scan = SCAN_2;             //  Enter 2nd Scan
                    next.subscan = SUB_SCAN_MQ;     //  Enter MQ (skid) cycle
                    next.cycle = CYCLE_B;           //  B Cycle Next
                    next.subcycle = 1;              //  Second kind.
                }
                else {
                    IRingControl = True;            //  No Zeros found. All done
                }
            }
            else {                                  //  Not B Channel WM
                assert(ScanRing -> State() == SCAN_1);      //  Regen 1st scan
                next.cycle = CYCLE_B;               //  More B Cycles
                next.subcycle = 0;                  //  Still 1st scan
            }
            return;
        }   //  End flowchart symbols 1 and 2

        else if(first_scan_store_type == 3 || first_scan_store_type == 4) {
            AsmChannelWMSelect = FirstBFieldZero ?
                TAssemblyChannel::AsmChannelWMSet :
                TAssemblyChannel::AsmChannelWMNone;
            AsmChannelZonesSelect = (first_scan_store_type == 3) ?
                TAssemblyChannel::AsmChannelZonesA :    //  Point 3: Store A Chr
                TAssemblyChannel::AsmChannelZonesNone;  //  Point 4: Store A Num
            Store( AssemblyChannel -> Select (
                AsmChannelWMSelect,
                AsmChannelZonesSelect,
                false,
                TAssemblyChannel::AsmChannelSignNone,
                TAssemblyChannel::AsmChannelNumA ) );
            Cycle();
            if(b_temp.TestWM()) {                       //  B field WM? Yes...
                if(ZeroSuppressLatch) {                 //  Zero Found? Yes...
                    next.cycle = CYCLE_B;               //  Start 2nd scan
                    next.subcycle = 1;
                    next.scan = SCAN_2;
                    next.subscan = SUB_SCAN_MQ;         //  Set for skid cycle
                }
                else {                                  //  No zero found
                    IRingControl = true;                //  All done
                }
            }
            else {                                      //  No B field WM
                assert(ScanRing -> State() == SCAN_1);  //  Regen 1st scan
                if(A_Reg -> Get().TestWM()) {           //  A field WM?  Yes..
                    next.subscan = SUB_SCAN_E;          //  Set Extension
                    next.cycle = CYCLE_B;               //  Continue 1st scan
                    next.subcycle = 0;
                }
                else {                                  //  No A field WM
                    next.subscan = SUB_SCAN_B;          //  Set Body
                    next.cycle = CYCLE_A;               //  A Cycle next
                    next.subcycle = 0;                  //  First scan
                }
            }
            return;
        }   //  End flowchart symbols 3 and 4

        else {
            assert(false);                              //  Logic error (sb 1-4)
        }

    }   //  End B Cycle, subtype 0

    if(next.cycle == CYCLE_B && next.subcycle == 1) {

        *STAR = *B_AR;
        Readout();                                      //  Read out B Field chr
        b_temp = B_Reg -> Get();
        AssemblyChannel -> Reset();                     //  Init not spcl char
        edit_char_flags = edit_char_flag[b_temp.ToInt() & 0x3f];

        if(SubScanRing -> State() == SUB_SCAN_MQ) {     //  In MQ (skid) cycle?
            next.cycle = CYCLE_B;                       //  B Cycle Next
            next.subcycle = 1;                          //  Same station
            next.subscan = SUB_SCAN_E;                  //  Set Extension
            Store( AssemblyChannel -> Select (          //  Store B Field char
                TAssemblyChannel::AsmChannelWMB,
                TAssemblyChannel::AsmChannelZonesB,
                false,
                TAssemblyChannel::AsmChannelSignNone,
                TAssemblyChannel::AsmChannelNumB) );
            Cycle();
            assert(ScanRing -> State() == SCAN_2);      //  Regen 2nd scan
            return;
        }   //  End MQ (Skid) cycle

        if(edit_char_flags & SZ_SIGNIFICANT_DIGIT) {
            ZeroSuppressLatch = false;
        }
        else if(!(edit_char_flags & EDIT_SUPPRESS_IF_NS)) {
            if(!DecimalControlLatch) {
                ZeroSuppressLatch = true;
            }
        }
        else if(b_temp.TestChar(BCD_MINUS)) {
        }
        else if(b_temp.TestChar(BCD_PERIOD)) {
            if(ZeroSuppressLatch) {
                DecimalControlLatch = true;
            }
        }
        else {
            assert(b_temp.TestChar(BCD_SPACE) ||
                b_temp.TestChar(BCD_COMMA) || b_temp.TestChar(BCD_0));
            if(ZeroSuppressLatch && !DecimalControlLatch) {
                AssemblyChannel -> Set(AsteriskFillLatch ? BCD_ASTERISK : BCD_SPACE);
            }
        }

        if(AssemblyChannel -> SpecialChar()) {
            Store(AssemblyChannel -> Select());
        }
        else {
            Store( AssemblyChannel -> Select (
                TAssemblyChannel::AsmChannelWMNone,
                TAssemblyChannel::AsmChannelZonesB,
                false,
                TAssemblyChannel::AsmChannelSignNone,
                TAssemblyChannel::AsmChannelNumB ) );
        }

        Cycle();

        if(!b_temp.TestWM()) {
            assert(SubScanRing -> State() == SUB_SCAN_E);   //  Regen Ext. Latch
            assert(ScanRing -> State() == SCAN_2);          //  Regen 2nd Scan
            next.cycle = CYCLE_B;                           //  B Cycle next
            next.subcycle = 1;                              //  2nd scan
            return;
        }

        if(!FloatingDollarLatch && !DecimalControlLatch) {
            IRingControl = true;
            return;
        }

        if(FloatingDollarLatch ||
            (ZeroSuppressLatch && !(edit_char_flags & SZ_SIGNIFICANT_DIGIT))) {
            next.scan = SCAN_3;                             //  Set 3rd scan
            next.subscan = SUB_SCAN_MQ;                     //  Set MQ for skid
            next.cycle = CYCLE_B;                           //  B Cycle next
            next.subcycle = 2;                              //  3rd scan
            return;
        }

        if(ZeroSuppressLatch && (edit_char_flags & SZ_SIGNIFICANT_DIGIT)) {
            IRingControl = true;
            return;
        }

        assert(!FloatingDollarLatch && !ZeroSuppressLatch);
        if(b_temp.TestChar(BCD_0)) {
            IRingControl = true;

              /* InstructionCheck ->
                SetStop("Instruction Check: Edit B WM 0 ZeroSuppress OFF");
              */
           
                return;
        }
        else {
            IRingControl = true;
            return;
        }

        assert(false);                                      //  Can't get here
    }   //  End B Cycle, subtype 1

    if(next.cycle == CYCLE_B && next.subcycle == 2) {

        *STAR = *B_AR;
        Readout();                                          //  Read out B Char
        b_temp = B_Reg -> Get();
        AssemblyChannel -> Reset();                         //  Init not special

        if(SubScanRing -> State() == SUB_SCAN_MQ) {         //  Skid cycle?
            next.cycle = CYCLE_B;                           //  B Cycle Next
            next.subcycle = 2;                              //  Same station
            next.subscan = SUB_SCAN_E;                      //  Set Extension
            Store( AssemblyChannel -> Select (              //  Store B Char
                TAssemblyChannel::AsmChannelWMB,
                TAssemblyChannel::AsmChannelZonesB,
                false,
                TAssemblyChannel::AsmChannelSignNone,
                TAssemblyChannel::AsmChannelNumB ) );
            Cycle();
            assert(ScanRing -> State() == SCAN_3);          //  Regen 3rd scan
            return;
        }   //  End Skid cycle

        else if(b_temp.TestChar(BCD_SPACE)) {
            if(AsteriskFillLatch) {
                AssemblyChannel -> Set(BCD_ASTERISK);
            }
            else if(FloatingDollarLatch) {
                AssemblyChannel -> Set(BCD_DOLLAR);
                IRingControl = true;                        //  Stop after $
            }
            //  Else, leave B field char as a blank
        }
        else if(b_temp.TestChar(BCD_0) ||
               (b_temp.TestChar(BCD_PERIOD) && DecimalControlLatch) ) {
            if(ZeroSuppressLatch) {
                AssemblyChannel -> Set( AsteriskFillLatch ? BCD_ASTERISK : BCD_SPACE);
                IRingControl = b_temp.TestChar(BCD_PERIOD); //  Done if period
            }
            else if(b_temp.TestChar(BCD_PERIOD)) {
                IRingControl = true;
            }
            //  Otherwise, just store the 0
        }   //  End 0 or . w decimal control
        else if(b_temp.TestChar(BCD_PERIOD)) {
            assert(!DecimalControlLatch);
            //  Just store the period back
        }
        else {
            assert(!b_temp.TestChar(BCD_SPACE) && !b_temp.TestChar(BCD_0) &&
                !b_temp.TestChar(BCD_PERIOD) );
            //  Just store other characters back.
        }

        if(AssemblyChannel -> SpecialChar()) {
            Store(AssemblyChannel -> Select());
        }
        else {
            Store( AssemblyChannel -> Select (
                TAssemblyChannel::AsmChannelWMNone,
                TAssemblyChannel::AsmChannelZonesB,
                false,
                TAssemblyChannel::AsmChannelSignNone,
                TAssemblyChannel::AsmChannelNumB ) );
        }

        Cycle();
        if(!IRingControl) {
            assert(ScanRing -> State() == SCAN_3);          //  Regen 3rd scan
        }
        return; //  Perpahs with IRingControl Set
    }   //  End B Cycle, subtype 2

}   //  End Edit Instruction


void T1410CPU::InstructionCompare()
{
    static struct {
        char cycle;
        char subcycle;
        char scan;
        char subscan;
    } next;

    if(LastInstructionReadout) {
        next.scan = SCAN_1;
        ScanRing -> Set(next.scan);
        next.subscan = SUB_SCAN_U;
        SubScanRing -> Set(next.subscan);
        next.cycle = CYCLE_A;
        next.subcycle = 0;
        LastInstructionReadout = false;
    }

    CycleRing -> Set(next.cycle);
    ScanRing -> Set(next.scan);
    SubScanRing -> Set(next.subscan);

    if(next.cycle == CYCLE_A) {
        *STAR = *A_AR;
        Readout();
        Cycle();
        next.cycle = CYCLE_B;
        next.subcycle = 0;
        return;
    }

    assert(next.cycle == CYCLE_B && next.subcycle == 0);

    *STAR = *B_AR;
    Readout();
    Cycle();

    //  If the A field WM reached before B field WM, then B > A

    if(A_Reg -> Get().TestWM() && !(B_Reg -> Get().TestWM()) ) {
        CompareBGTA -> Set();
        CompareBLTA -> Reset();
        CompareBEQA -> Reset();
        IRingControl = true;
        return;
    }

    Comparator();                                   //  Sets CompareBxxA Latches

    if(B_Reg -> Get().TestWM()) {
        IRingControl = true;
        return;
    }

    next.subscan = SUB_SCAN_B;                      //  Body after 1st B char
    next.cycle = CYCLE_A;                           //  A cycle next
    next.subcycle = 0;
    return;
}   //  End Compare Instruction


void T1410CPU::InstructionTableLookup()
{
    static struct {
        char cycle;
        char subcycle;
        char scan;
        char subscan;
    } next;

    if(LastInstructionReadout) {
        if(Op_Mod_Reg -> Get().ToInt() & (BIT8 | BITA | BITB)) {
            InstructionCheck ->
                SetStop("Instruction Check: Invalid Table Lookup d-char");
            return;
        }
        CompareBLTA -> Set();
        CompareBEQA -> Reset();
        CompareBGTA -> Reset();
        next.scan = SCAN_1;
        ScanRing -> Set(next.scan);
        next.subscan = SUB_SCAN_U;
        SubScanRing -> Set(next.subscan);
        next.cycle = CYCLE_A;
        next.subcycle = 0;
        LastInstructionReadout = false;
    }

    CycleRing -> Set(next.cycle);
    ScanRing -> Set(next.scan);
    SubScanRing -> Set(next.subscan);

    if(next.cycle == CYCLE_A) {
        *STAR = (SubScanRing -> State() == SUB_SCAN_U) ? *C_AR : *A_AR;
        Readout();                                  //  Readout from CAR or AAR
        Cycle();
        next.cycle = CYCLE_B;
        next.subcycle = 0;
        return;
    }

    assert(next.cycle == CYCLE_B && next.subcycle == 0);

    *STAR = *B_AR;
    Readout();
    Cycle();

    Comparator();                                   //  Set compare latches

    if(!(A_Reg -> Get().TestWM())) {                //  No A WM?
        if(B_Reg -> Get().TestWM()) {               //  B WM? -- End of table
            CompareBGTA -> Set();
            CompareBLTA -> Reset();
            CompareBEQA -> Reset();
            IRingControl = true;
            return;
        }
        next.cycle = CYCLE_A;                       //  No B WM -- body
        next.subcycle = 0;
        next.subscan = SUB_SCAN_B;
        next.scan = SCAN_1;                         //  Regen 1st scan
        return;
    }   //  End no A WM

    //  Get here -- we have an A WM.  Check if we are done - found equal
    //  when looking for equal, high when looking for high or low when looking
    //  for low.

    if((CompareBEQA -> State() && ((Op_Mod_Reg -> Get() & BIT2) != 0)) ||
       (CompareBGTA -> State() && ((Op_Mod_Reg -> Get() & BIT4) != 0)) ||
       (CompareBLTA -> State() && ((Op_Mod_Reg -> Get() & BIT1) != 0)) ) {
        IRingControl = true;
        return;
    }

    if(B_Reg -> Get().TestWM()) {                   //  B WM ? Back to Units
        next.cycle = CYCLE_A;
        next.subcycle = 0;
        next.subscan = SUB_SCAN_U;
        next.scan = SCAN_1;                         //  Regen 1st scan
        return;
    }
    else {
        next.cycle = CYCLE_B;
        next.subcycle = 0;
        next.subscan = SUB_SCAN_E;                  //  In extension (no match)
        next.scan = SCAN_1;                         //  Regen 1st scan
        return;
    }

}   //  End Table Lookup Instruction

