//---------------------------------------------------------------------------
#ifndef UI1410INSTH
#define UI1410INSTH
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

//	Op code table Instruction Readout lines values: OpReadoutLines

#define OP_PERCENTTYPE	1
#define	OP_NOTPERCENTTYPE	2
#define OP_ADDRDBL		4
#define	OP_NOTADDRDBL	8
#define OP_1ADDRPLUSMOD	16
#define	OP_2ADDRNOMOD	32
#define	OP_2ADDRPLUSMOD	64
#define	OP_2ADDRESS		128
#define	OP_ADDRTYPE		256
#define	OP_2CHARONLY	512
#define	OP_CCYCLE		1024
#define	OP_NOCORDCY		2048
#define	OP_NODCYIRING6	4096
#define	OP_NOINDEXON1STADDR	8192

//	Op code table Operational lines values: OpOperationalLines

#define	OP_RESETTYPE	1
#define	OP_ADDORSUBT	2
#define	OP_MPYORDIV		4
#define	OP_ADDTYPE		8
#define	OP_ARITHTYPE	16
#define	OP_EORZ			32
#define OP_COMPARETYPE	64
#define	OP_BRANCHTYPE	128
#define	OP_NOBRANCH		256
#define	OP_WORDMARK		512
#define	OP_MORL			1024

//	Op code table Control lines values: OpControlLines

#define	OP_1STSCANFIRST	1
#define	OP_ACYFIRST		2
#define	OP_STDACYCLE	4
#define	OP_BCYFIRST		8
#define	OP_AREGTOACHONBCY	16
#define	OP_OPMODTOACHONBCY	32
#define	OP_LOADMEMONBCY	64
#define	OP_RGENMEMONBCY	128
#define	OP_STOPATFONBCY	256
#define	OP_STOPATHONBCY	512
#define	OP_STOPATJONBCY	1024
#define	OP_ROBARONSCANBCY	2048
#define	OP_ROAARONACY	4096

#define	OP_INVALID		65535

#define	OP_SUBTRACT		18
#define OP_TABLESEARCH	19
#define	OP_NOP			37
#define	OP_ZERO_SUB		42
#define	OP_ADD			49
#define OP_SAR_G		55
#define	OP_ZERO_ADD		58
#define OP_MULTIPLY     12
#define OP_DIVIDE       28
#define OP_MOVE         52
#define OP_MCS          25
#define OP_EDIT         53
#define OP_COMPARE      51

#define OP_BRANCHCOND   33
#define OP_BRANCH_CH_1  41
#define OP_BRANCH_CH_2  23
#define OP_BRANCH_CE    50
#define OP_BRANCH_BE    22
#define OP_BRANCH_ZWM   21
#define OP_BRANCH_PR    24

#define OP_STORE_AR     55
#define OP_SETWM        27
#define OP_CLEARWM      60
#define OP_CLEAR_STORAGE 17
#define OP_HALT         59

#define OP_IO_MOVE      36
#define OP_IO_LOAD      35
#define OP_IO_UNIT      20
#define OP_IO_CARRIAGE_1 54
#define OP_IO_CARRIAGE_2 02
#define OP_IO_SSF_1     34
#define OP_IO_SSF_2     04

#define OP_MOD_SYMBOL_BLANK 00
#define OP_MOD_SYMBOL_1     1
#define OP_MOD_SYMBOL_2     2
#define OP_MOD_SYMBOL_9     9
#define OP_MOD_SYMBOL_AT    12
#define OP_MOD_SYMBOL_SLASH 17
#define OP_MOD_SYMBOL_S     18
#define OP_MOD_SYMBOL_T     19
#define OP_MOD_SYMBOL_U     20
#define OP_MOD_SYMBOL_V     21
#define OP_MOD_SYMBOL_W     22
#define OP_MOD_SYMBOL_X     23
#define OP_MOD_SYMBOL_Z     25
#define OP_MOD_SYMBOL_RM    26
#define OP_MOD_SYMBOL_MINUS 32
#define OP_MOD_SYMBOL_K     34
#define OP_MOD_SYMBOL_M     36
#define OP_MOD_SYMBOL_N     37
#define OP_MOD_SYMBOL_Q     40
#define OP_MOD_SYMBOL_R     41
#define OP_MOD_SYMBOL_DOLLAR 43
#define OP_MOD_SYMBOL_ASTERISK 44
#define OP_MOD_SYMBOL_A     49
#define OP_MOD_SYMBOL_B     50
#define OP_MOD_SYMBOL_E     53
#define OP_MOD_SYMBOL_F     54

#endif
