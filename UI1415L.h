//---------------------------------------------------------------------------
#ifndef UI1415LH
#define UI1415LH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.StdCtrls.hpp>
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

//	1415 Light panel display

class TF1415L : public TForm
{
__published:	// IDE-managed Components
	TPageControl *I1415Display;
	TTabSheet *TSCPUStatus;
	TTabSheet *TSIOChannels;
	TTabSheet *TSSystemCheck;
	TTabSheet *TSPowerSystemControls;
	TPanel *PCPU;
	TPanel *PStatus;
	TLabel *LabelCPU;
	TPanel *PIRing;
	TLabel *Light_I_OP;
	TLabel *Light_I_1;
	TLabel *Light_I_2;
	TLabel *Light_I_3;
	TLabel *Light_I_4;
	TLabel *Light_I_5;
	TLabel *Light_I_6;
	TLabel *Light_I_7;
	TLabel *Light_I_8;
	TLabel *Light_I_9;
	TLabel *Light_I_10;
	TLabel *Light_I_11;
	TLabel *Light_I_12;
	TLabel *LabelIRing;
	TPanel *PAring;
	TLabel *LabelARing;
	TLabel *Light_A_1;
	TLabel *Light_A_2;
	TLabel *Light_A_3;
	TLabel *Light_A_4;
	TLabel *Light_A_5;
	TLabel *Light_A_6;
	TPanel *PClock;
	TLabel *LabelClock;
	TLabel *Light_Clk_A;
	TLabel *Light_Clk_B;
	TLabel *Light_Clk_C;
	TLabel *Light_Clk_D;
	TLabel *Light_Clk_E;
	TLabel *Light_Clk_F;
	TLabel *Light_Clk_G;
	TLabel *Light_Clk_H;
	TLabel *Light_Clk_J;
	TLabel *Light_Clk_K;
	TPanel *PScan;
	TLabel *LabelScan;
	TLabel *Light_Scan_N;
	TLabel *Light_Scan_1;
	TLabel *Light_Scan_2;
	TLabel *Light_Scan_3;
	TPanel *PSubScan;
	TLabel *LabelSubScan;
	TLabel *Light_Sub_Scan_U;
	TLabel *Light_Sub_Scan_B;
	TLabel *Light_Sub_Scan_E;
	TLabel *Light_Sub_Scan_MQ;
	TPanel *PCycle;
	TLabel *LabelCycle;
	TLabel *Light_Cycle_A;
	TLabel *Light_Cycle_B;
	TLabel *Light_Cycle_C;
	TLabel *Light_Cycle_D;
	TLabel *Light_Cycle_E;
	TLabel *Light_Cycle_F;
	TLabel *Light_Cycle_I;
	TLabel *Light_Cycle_X;
	TPanel *PArith;
	TLabel *LabelArith;
	TLabel *Light_Carry_In;
	TLabel *Light_Carry_Out;
	TLabel *Light_A_Complement;
	TLabel *Light_B_Complement;
	TLabel *LabelStatus;
	TLabel *Light_B_GT_A;
	TLabel *Light_B_EQ_A;
	TLabel *Light_B_LT_A;
	TLabel *Light_Overflow;
	TLabel *Light_Divide_Overflow;
	TLabel *Light_Zero_Balance;
	TPanel *PIOCh;
	TPanel *PIOCh1Status;
	TLabel *LabelCh1Status;
	TLabel *Light_Ch1_NotReady;
	TLabel *Light_Ch1_Busy;
	TLabel *Light_Ch1_DataCheck;
	TLabel *Light_Ch1_Condition;
	TLabel *Light_Ch1_WLRecord;
	TLabel *Light_Ch1_NoTransfer;
	TPanel *PIOCh1Control;
	TLabel *LabelCh1Control;
	TLabel *Light_Ch1_Interlock;
	TLabel *Light_Ch1_RBCInterlock;
	TLabel *Light_Ch1_Read;
	TLabel *Light_Ch1_Write;
	TLabel *Light_Ch1_Overlap;
	TLabel *Light_Ch1_NoOverlap;
	TPanel *PIOCh2Control;
	TLabel *LabelCh2Control;
	TLabel *Light_Ch2_Interlock;
	TLabel *Light_Ch2_RBCInterlock;
	TLabel *Light_Ch2_Read;
	TLabel *Light_Ch2_Write;
	TLabel *Light_Ch2_Overlap;
	TLabel *Light_Ch2_NoOverlap;
	TPanel *PIOChControl;
	TPanel *PIOChStatus;
	TPanel *PIOCh2Status;
	TLabel *LabelCh2Status;
	TLabel *Light_Ch2_NotReady;
	TLabel *Light_Ch2_Busy;
	TLabel *Light_Ch2_DataCheck;
	TLabel *Light_Ch2_Condition;
	TLabel *Light_Ch2_WLRecord;
	TLabel *Light_Ch2_NoTransfer;
	
	TPanel *PSysCheck;
	TPanel *PProcess;
	TLabel *LabelProcess;
	TLabel *Light_Check_AChannel;
	TLabel *Light_Check_BChannel;
	TLabel *Light_Check_AssemblyChannel;
	TLabel *Light_Check_AddressChannel;
	TLabel *Light_Check_AddressExit;
	TLabel *Light_Check_ARegisterSet;
	TPanel *PProgram;
	TLabel *LabelProgram;
	TLabel *Light_Check_IOInterlock;
	TLabel *Light_Check_AddressCheck;
	TLabel *Light_Check_RBCInterlock;
	TLabel *Light_Check_InstructionCheck;
	TPanel *PSystemCheck;
	TLabel *Light_Check_BRegisterSet;
	TLabel *Light_Check_OpRegisterSet;
	TLabel *Light_Check_OpModifierSet;
	TLabel *Light_Check_ACharacterSelect;
	TLabel *Light_Check_BCharacterSelect;
	TPanel *LabelPower;
	TPanel *PPower;
	TLabel *Light_Thermal;
	TLabel *Light_CB_Trip;
	TLabel *Light_IO_Offline;
	TLabel *Light_Tape_Offline;
	TLabel *Light_Disk_Offline;
	TPanel *Lable_SystemControls;
	TPanel *P_System_Controls;
	TLabel *Light_1401_Compat;
	TLabel *Light_Priority_Alert;
	TLabel *Light_Off_Normal;
	TLabel *Light_Stop;
	TTabSheet *TSCEPanel;
	TPanel *PCEReg;
	TPanel *PCECycle;
	TLabel *Light_CE_Cyc_A;
	TLabel *Light_CE_Cyc_B;
	TLabel *Light_CE_Cyc_C;
	TLabel *Light_CE_Cyc_D;
	TLabel *Light_CE_Cyc_E;
	TLabel *Light_CE_Cyc_I;
	TLabel *Light_CE_Cyc_X;
	TLabel *Light_CE_Cyc_F;
	TPanel *PCEAChSel;
	TLabel *Light_CE_ACh_A;
	TLabel *Light_CE_ACh_d;
	TLabel *Light_CE_ACh_E;
	TLabel *Light_CE_ACh_F;
	TPanel *PCEAReg;
	TLabel *Light_CE_A_ER;
	TLabel *Light_CE_A_WM;
	TLabel *Light_CE_A_C;
	TLabel *Light_CE_A_B;
	TLabel *Light_CE_A_2;
	TLabel *Light_CE_A_1;
	TLabel *Light_CE_A_A;
	TLabel *Light_CE_A_8;
	TLabel *Light_CE_A_4;
	TPanel *PCEAssm;
	TLabel *Light_CE_Assm_ER;
	TLabel *Light_CE_Assm_WM;
	TLabel *Light_CE_Assm_C;
	TLabel *Light_CE_Assm_B;
	TLabel *Light_CE_Assm_2;
	TLabel *Light_CE_Assm_1;
	TLabel *Light_CE_Assm_A;
	TLabel *Light_CE_Assm_8;
	TLabel *Light_CE_Assm_4;
	TLabel *Light_CE_Assm_NWM;
	TLabel *Light_CE_Assm_NC;
	TLabel *Light_CE_Assm_NB;
	TLabel *Light_CE_Assm_NA;
	TLabel *Light_CE_Assm_N8;
	TLabel *Light_CE_Assm_N4;
	TLabel *Light_CE_Assm_N2;
	TLabel *Light_CE_Assm_N1;
	TPanel *PCEBReg;
	TLabel *Light_CE_B_ER;
	TLabel *Light_CE_B_WM;
	TLabel *Light_CE_B_C;
	TLabel *Light_CE_B_B;
	TLabel *Light_CE_B_2;
	TLabel *Light_CE_B_1;
	TLabel *Light_CE_B_A;
	TLabel *Light_CE_B_8;
	TLabel *Light_CE_B_4;
	TPanel *PCEOP;
	TLabel *Light_CE_OP_C;
	TLabel *Light_CE_OP_B;
	TLabel *Light_CE_OP_2;
	TLabel *Light_CE_OP_1;
	TLabel *Light_CE_OP_A;
	TLabel *Light_CE_OP_8;
	TLabel *Light_CE_OP_4;
	TPanel *PCEOPMod;
	TLabel *Light_CE_Mod_C;
	TLabel *Light_CE_Mod_B;
	TLabel *Light_CE_Mod_2;
	TLabel *Light_CE_Mod_1;
	TLabel *Light_CE_Mod_A;
	TLabel *Light_CE_Mod_8;
	TLabel *Light_CE_Mod_4;
	TPanel *PCEAddr;
	TLabel *Light_CE_Addr_2;
	TLabel *Light_CE_Addr_1;
	TLabel *Light_CE_Addr_0;
	TLabel *Light_CE_Addr_8;
	TLabel *Light_CE_Addr_4;
	TLabel *Light_CE_Addr_ER;
	TPanel *PCERegHdr;
	TLabel *LabelCECYC;
	TLabel *LabelCEAChSel;
	TLabel *LabelCEB;
	TLabel *LabelCEA;
	TLabel *LabelCEAssm;
	TLabel *LabelCEOP;
	TLabel *LabelCEMod;
	TLabel *LabelCEAddr;
private:	// User declarations
public:		// User declarations
	__fastcall TF1415L(TComponent* Owner);

    void DisplayAddrChannel(TWOOF5 a,bool err);

};
//---------------------------------------------------------------------------
extern TF1415L *F1415L;
//---------------------------------------------------------------------------
#endif
