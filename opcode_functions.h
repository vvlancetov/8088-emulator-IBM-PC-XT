#pragma once
#include "custom_classes.h"

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

//декларирование функций обработчиков операций
void opcode_table_init();
void opcode_8087_table_init();
void op_code_unknown();			// Unknown operation
void op_code_8087_unknown();
void op_code_NOP();				// No operation
void segment_override_prefix(); //set override flag
inline uint16 mod_RM_Old(uint8 byte2);			//расчет адреса операнда
inline uint32 mod_RM_2(uint8 byte2);
void mod_RM_2_noret(uint8 byte2);
inline __int8 DispCalc8(uint8 data);				//преобразование смещения к знаку
inline __int16 DispCalc16(uint16 data);

//============== Data Transfer Group ===========

void mov_R_to_RM_8();	//move r/m
void mov_R_to_RM_16();	//move r/m
void mov_RM_to_R_8();	//move r/m
void mov_RM_to_R_16();	//move r/m

void IMM_8_to_RM();		//IMM_8 to R/M
void IMM_16_to_RM();	//IMM_16 to R/M

void IMM_8_to_R();		//IMM_8 to Register
void IMM_16_to_R();		//IMM_16 to Register

void M_8_to_ACC();		//Memory to Accumulator 8
void M_16_to_ACC();		//Memory to Accumulator 16
void ACC_8_to_M();		//Accumulator to Memory 8
void ACC_16_to_M();		//Accumulator to Memory 16

void RM_to_Segment_Reg();	//Register/Memory to Segment Register
void Segment_Reg_to_RM();	//Segment Register to Register/Memory

void Push_R();			//PUSH Register
void Push_SegReg();		//PUSH Segment Register

void Pop_RM();			//POP Register/Memory
void Pop_R();			//POP Register
void Pop_SegReg();		//POP Segment Register

void XCHG_8();			//Exchange Register/Memory with Register 8bit
void XCHG_16();			//Exchange Register/Memory with Register 16bit
void XCHG_ACC_R();		//Exchange Register with Accumulator

void In_8_to_ACC_from_port();	  //Input 8 to AL/AX AX from fixed PORT
void In_16_to_ACC_from_port();    //Input 16 to AL/AX AX from fixed PORT
void Out_8_from_ACC_to_port();    //Output 8 from AL/AX AX from fixed PORT
void Out_16_from_ACC_to_port();   //Output 16 from AL/AX AX from fixed PORT

void In_8_from_DX();		//In 8 from variable PORT
void In_16_from_DX();		//In 16 from variable PORT
void Out_8_to_DX();			//Out 8 to variable PORT
void Out_16_to_DX();		//Out 16 to variable PORT

void XLAT();		//Translate Byte to AL
void LEA();			//Load EA to Register
void LDS();			//Load Pointer to DS
void LES();			//Load Pointer to ES

void LAHF();			// Load AH with Flags
void SAHF();			// Store AH with Flags
void PUSHF();			// Push Flags
void POPF();			// Pop Flags

//============Arithmetic===================================

//ADD
void ADD_R_to_RM_8();		// INC R/M -> R/M 8bit
void ADD_R_to_RM_16();		// INC R/M -> R/M 16bit
void ADD_RM_to_R_8();		// INC R/M -> R 8bit
void ADD_RM_to_R_16();		// INC R/M -> R 16bit

void ADD_IMM_RM_16s();		// N operations
void ADD_IMM_to_ACC_8();	// ADD IMM -> ACC 8bit
void ADD_IMM_to_ACC_16();	// ADD IMM -> ACC 16bit 

//ADC

void ADC_R_to_RM_8();		// ADC R/M -> R/M 8bit
void ADC_R_to_RM_16();		// ADC R/M -> R/M 16bit
void ADC_RM_to_R_8();		// ADC R/M -> R 8bit
void ADC_RM_to_R_16();		// ADC R/M -> R 16bit

void ADC_IMM_to_ACC_8();	// ADC IMM->ACC 8bit
void ADC_IMM_to_ACC_16();	// ADC IMM->ACC 16bit 

//INC

void INC_RM_8();		// INC R/M 8bit
void INC_Reg();			// INC reg 16 bit

void AAA();				//  AAA = ASCII Adjust for Add
void DAA();				//  DAA = Decimal Adjust for Add

//SUB

void SUB_RM_from_RM_8();		// SUB R/M -> R/M 8bit
void SUB_RM_from_RM_16();		// SUB R/M -> R/M 16bit
void SUB_RM_from_R_8();			// SUB R/M -> R 8bit
void SUB_RM_from_R_16();		// SUB R/M -> R 16bit

void SUB_IMM_from_ACC_8();		// SUB ACC  8bit - IMM -> ACC
void SUB_IMM_from_ACC_16();		// SUB ACC 16bit - IMM -> ACC

//SBB

void SBB_RM_from_RM_8();		// SBB R/M -> R/M 8bit
void SBB_RM_from_RM_16();		// SBB R/M -> R/M 16bit
void SBB_RM_from_R_8();			// SBB R/M -> R 8bit
void SBB_RM_from_R_16();		// SBB R/M -> R 16bit

void SBB_IMM_from_ACC_8();		// SBB ACC  8bit - IMM -> ACC
void SBB_IMM_from_ACC_16();		// SBB ACC 16bit - IMM -> ACC

//DEC
void DEC_Reg();			//  DEC reg 16 bit
//CMP
void CMP_Reg_RM_8();	//  CMP Reg with R/M 8 bit
void CMP_Reg_RM_16();	//  CMP Reg with R/M 16 bit
void CMP_RM_Reg_8();	//  CMP R/M with Reg 8 bit
void CMP_RM_Reg_16();	//  CMP R/M with Reg 16 bit

void CMP_IMM_with_ACC_8();		// CMP IMM  8bit - ACC
void CMP_IMM_with_ACC_16();		// CMP IMM 16bit - ACC

void AAS(); //AAS = ASCII Adjust for Subtract
void DAS(); //DAS = Decimal Adjust for Subtract
void AAM(); //AAM = ASCII Adjust for Multiply
void AAD(); //AAD = ASCII Adjust for Divide
void CBW(); //CBW = Convert Byte to Word
void CWD(); //CWD = Convert Word to Double Word

//============Logic========================================

void Invert_RM_8();			// NOT R/M 8bit
void Invert_RM_16();		// NOT R/M 16bit

void SHL_ROT_8();			// Shift Logical / Arithmetic Left / 8bit / once
void SHL_ROT_16();			// Shift Logical / Arithmetic Left / 16bit / once
void SHL_ROT_8_mult();		// Shift Logical / Arithmetic Left / 8bit / CL
void SHL_ROT_16_mult();		// Shift Logical / Arithmetic Left / 16bit / CL

//ROL = Rotate Left
//ROR = Rotate Right
//RCL = Rotate through Carry Left
//RCR = Rotate through Carry Right

//AND
void AND_RM_8();			// AND R + R/M -> R/M 8bit
void AND_RM_16();			// AND R + R/M -> R/M 16bit
void AND_RM_R_8();			// AND R + R/M -> R 8bit
void AND_RM_R_16();			// AND R + R/M -> R 16bit

void AND_IMM_ACC_8();		//AND IMM to ACC 8bit
void AND_IMM_ACC_16();		//AND IMM to ACC 16bit

//TEST
void TEST_8();				//TEST = AND Function to Flags
void TEST_16();				//TEST = AND Function to Flags
void TEST_IMM_8();			//TEST = AND Function to Flags
void TEST_IMM_16();			//TEST = AND Function to Flags
void TEST_IMM_ACC_8();		//TEST = AND Function to Flags
void TEST_IMM_ACC_16();     //TEST = AND Function to Flags

//OR = Or
void OR_RM_8();		// OR R + R/M -> R/M 8bit
void OR_RM_16();		// OR R + R/M -> R/M 16bit
void OR_RM_R_8();		// OR R + R/M -> R 8bit
void OR_RM_R_16();		// OR R + R/M -> R 16bit

void OR_IMM_ACC_8();    //OR IMM to ACC 8bit
void OR_IMM_ACC_16();   //OR IMM to ACC 16bit

//XOR = Exclusive OR
void XOR_RM_8();		// XOR R + R/M -> R/M 8bit
void XOR_RM_16();		// XOR R + R/M -> R/M 16bit
void XOR_RM_R_8();		// XOR R + R/M -> R 8bit
void XOR_RM_R_16();		// XOR R + R/M -> R 16bit

void XOR_OR_IMM_RM_8();		//XOR/OR IMM to Register/Memory 8bit
void XOR_OR_IMM_RM_16();	//XOR/OR IMM to Register/Memory 16bit
void XOR_IMM_ACC_8();		//XOR IMM to ACC 8bit
void XOR_IMM_ACC_16();		//XOR IMM to ACC 16bit

//============String Manipulation=========================

void REPNE();		//REP = Repeat_while ZF=0   [F2]
void REP();			//REP = Repeat				[F3]

void MOVES_8();    //MOVS = Move String 8bit	[A4]
void MOVES_16();   //MOVS = Move String 16bit	[A5]

void CMPS_8();     //CMPS = Compare String		[A6]
void CMPS_16();    //CMPS = Compare String		[A7]

void SCAS_8();     //SCAS = Scan String			[AE]
void SCAS_16();     //SCAS = Scan String		[AF]

void LODS_8();     //LODS = Load String			[AC]
void LODS_16();    //LODS = Load String			[AD]

void STOS_8();     //STOS = Store String		[AA]
void STOS_16();    //STOS = Store String		[AB]

//============ Control Transfer============================

// Call + Jump
void Call_Near();					//Direct Call within Segment
void Call_Jump_Push();				//N operations
void Call_dir_interseg();			//Direct Intersegment Call

void Jump_Near_8();					//Direct jump within Segment-Short
void Jump_Near_16();				//Direct jump within Segment-Short
void Jump_Far();					//Direct Intersegment Jump

//RET

void RET_Segment();					//Return Within Segment
void RET_Segment_IMM_SP();			//Return Within Segment Adding Immediate to SP
void RET_Inter_Segment();			//Return Intersegment
void RET_Inter_Segment_IMM_SP();	//Return Intersegment Adding Immediate to SP

// Conditional Jumps
void JE_JZ();			// JE/JZ = Jump on Equal/Zero
void JL_JNGE();			// JL/JNGE = Jump on Less/Not Greater, or Equal
void JLE_JNG();			// JLE/JNG = Jump on Less, or Equal/Not Greater
void JB_JNAE();			// JB/JNAE = Jump on Below/Not Above, or Equal
void JBE_JNA();			// JBE/JNA = Jump on Below, or Equal/Not Above
void JP_JPE();			// JP/JPE = Jump on Parity/Parity Even
void JO();				// JO = Jump on Overflow
void JS();				// JS = Jump on Sign
void JNE_JNZ();			// JNE/JNZ = Jump on Not Equal/Not Zero
void JNL_JGE();			// JNL/JGE = Jump on Not Less/Greater, or Equal
void JNLE_JG();			//JNLE/JG = Jump on Not Less, or Equal/Greater
void JNB_JAE();			//JNB/JAE = Jump on Not Below/Above, or Equal
void JNBE_JA();			//JNBE/JA = Jump on Not Below, or Equal/Above
void JNP_JPO();			//JNP/JPO = Jump on Not Parity/Parity Odd
void JNO();				//JNO = Jump on Not Overflow
void JNS();				//JNS = Jump on Not Sign
void LOOP();			//LOOP = Loop CX Times
void LOOPZ_LOOPE();		//LOOPZ/LOOPE = Loop while Zero/Equal
void LOOPNZ_LOOPNE();	//LOOPNZ/LOOPNE = Loop while Not Zero/Not Equal
void JCXZ();			//JCXZ = Jump on CX Zero

//========Conditional Transfer-===============

void INT_N();			//INT = Interrupt
void INT_3();			//INT = Interrupt Type 3
void INT_O();			//INTO = Interrupt on Overflow
void I_RET();			//Interrupt Return

//==========Processor Control=================
void CLC();					//Clear Carry
void STC();					//Set Carry
void CMC();					//Complement Carry
void CLD();					//Clear Direction
void STD();					//Set Direction
void CLI();					//Clear Interrupt
void STI();					//Set Interrupt
void HLT();					//Halt
void Wait();				//Wait
void Lock();				//Bus lock prefix
void Esc_8087();			//Call 8087
void CALC();				//Undoc

//=============== 8087 =================

void Esc_8087_001_000_Load();  //LOAD int/real to ST0
void Esc_8087_011_000_Load();	//LOAD int/real to ST0
void Esc_8087_101_000_Load();	//LOAD int/real to ST0
void Esc_8087_111_000_Load();	//LOAD int/real to ST0

void Esc_8087_111_101_Load();  //LOAD long INT to ST0
void Esc_8087_011_101_Load();	//LOAD temp real to ST0
void Esc_8087_111_100_Load();	//LOAD BCD to ST0

void Esc_8087_001_010_Store();	//STORE ST0 to int/real
void Esc_8087_011_010_Store();	//STORE ST0 to int/real
void Esc_8087_101_010_Store();	//STORE ST0 to int/real
void Esc_8087_111_010_Store();	//STORE ST0 to int/real

void Esc_8087_001_011_StorePop(); //FSTP = Store and Pop
void Esc_8087_011_011_StorePop(); //FSTP = Store and Pop
void Esc_8087_101_011_StorePop(); //FSTP = Store and Pop + ST0 to STi 
void Esc_8087_111_011_StorePop(); //FSTP = Store and Pop

void Esc_8087_111_111_StorePop(); //StorePop ST0 Long Int to MEM
void Esc_8087_011_111_StorePop(); //StorePop ST0 to TMP real mem
void Esc_8087_111_110_StorePop(); //StorePop ST0 to BCD mem

void Esc_8087_001_001_FXCH();	  //EXchange ST0 - STi

void Esc_8087_000_010_FCOM();		//FCOM = Compare + STi to ST0
void Esc_8087_010_010_FCOM();		//FCOM = Compare
void Esc_8087_100_010_FCOM();		//FCOM = Compare
void Esc_8087_110_010_FCOM();		//FCOM = Compare

void Esc_8087_000_011_FCOM();		//FcomPop + STi to ST0
void Esc_8087_010_011_FCOM();		//FcomPop
void Esc_8087_100_011_FCOM();		//FcomPop
void Esc_8087_110_011_FCOM();		//FcomPop

void Esc_8087_001_100_TEST();		//FTST/FXAM

void Esc_8087_000_000_FADD();		//FADD
void Esc_8087_010_000_FADD();		//FADD
void Esc_8087_100_000_FADD();		//FADD
void Esc_8087_110_000_FADD();		//FADD

void Esc_8087_000_100_FSUB();		//FSUB R = 0
void Esc_8087_000_101_FSUB();		//FSUB R = 1
void Esc_8087_010_100_FSUB();		//FSUB R = 0
void Esc_8087_010_101_FSUB();		//FSUB R = 1
void Esc_8087_100_100_FSUB();		//FSUB R = 0
void Esc_8087_100_101_FSUB();		//FSUB R = 1
void Esc_8087_110_100_FSUB();		//FSUB R = 0
void Esc_8087_110_101_FSUB();		//FSUB R = 1

void Esc_8087_000_001_FMUL();		//FMUL
void Esc_8087_010_001_FMUL();		//FMUL
void Esc_8087_100_001_FMUL();		//FMUL
void Esc_8087_110_001_FMUL();		//FMUL

void Esc_8087_000_110_FDIV();		//FDIV R = 0
void Esc_8087_000_111_FDIV();		//FDIV R = 1
void Esc_8087_010_110_FDIV();		//FDIV R = 0
void Esc_8087_010_111_FDIV();		//FDIV R = 1
void Esc_8087_100_110_FDIV();		//FDIV R = 0
void Esc_8087_100_111_FDIV();		//FDIV R = 1
void Esc_8087_110_110_FDIV();		//FDIV R = 0
void Esc_8087_110_111_FDIV();		//FDIV R = 1

void Esc_8087_001_111_FSQRT();		//FSQRT/FSCALE/FPREM/FRNDINIT
void Esc_8087_001_110_FXTRACT();	//FXTRACT
void Esc_8087_001_101_FLDZ();		//FLDZ/FLD1/FLOPI/FLOL2T
void Esc_8087_011_100_FINIT();		//FINIT/FENI/FDISI
void Esc_8087_101_111_FSTSW();		//FSTSW
void Esc_8087_101_110_FSAVE();		//FSAVE
void Esc_8087_101_100_FRSTOR();	//FRSTOR