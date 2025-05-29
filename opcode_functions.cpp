#include "opcode_functions.h"
#include "custom_classes.h"
#include "video.h"

#include <Windows.h>
#include <iostream>
#include <string>
#include <conio.h>
#include <bitset>
#include <sstream>

#define DEBUG
typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

//Доп функции
template< typename T >
extern std::string int_to_hex(T i, int w);

// переменные из основного файла программы
using namespace std;
extern HANDLE hConsole;

extern uint8 exeption;

extern FDD_mon_device FDD_monitor;

//регистры процессора
extern uint16 AX; // AX
extern uint16 BX; // BX
extern uint16 CX; // CX
extern uint16 DX; // DX

//указатели на регистры
extern uint16* ptr_AX;
extern uint16* ptr_BX;
extern uint16* ptr_CX;
extern uint16* ptr_DX;
extern uint16* ptr_SP;
extern uint16* ptr_BP;
extern uint16* ptr_SI;
extern uint16* ptr_DI;

//половинки регистров
extern uint8* ptr_AL;
extern uint8* ptr_AH;
extern uint8* ptr_BL;
extern uint8* ptr_BH;
extern uint8* ptr_CL;
extern uint8* ptr_CH;
extern uint8* ptr_DL;
extern uint8* ptr_DH;

//таблицы указателей
extern uint16* ptr_r16[8];
extern uint8* ptr_r8[8];

//названия регистров
std::string reg8_name[8];		//8 бит
std::string reg16_name[8];		//16 бит

extern uint16 Status_Flags;	//Flags register
extern bool Flag_OF;	//Overflow flag
extern bool Flag_DF;	//Direction flag
extern bool Flag_IF;	//Int Enable flag
extern bool Flag_TF;	//Trap SS flag
extern bool Flag_SF;	//Signflag
extern bool Flag_ZF;	//Zeroflag
extern bool Flag_AF;	//Aux Carry flag
extern bool Flag_PF;	//Parity flag
extern bool Flag_CF;	//Carry flag

extern uint16 Stack_Pointer;			//указатель стека
extern uint16 Instruction_Pointer;		//указатель текущей команды
extern uint16 Base_Pointer;
extern uint16 Source_Index;
extern uint16 Destination_Index;

//указатели сегментов
extern uint16* CS;	//Code Segment
extern uint16* DS;	//Data Segment
extern uint16* SS;	//Stack Segment
extern uint16* ES;	//Extra Segment

//указатели сегментов
extern uint16 CS_data;			//Code Segment
extern uint16 DS_data;			//Data Segment
extern uint16 SS_data;			//Stack Segment
extern uint16 ES_data;			//Extra Segment

uint16* ptr_segreg[4];			//указатели
std::string segreg_name[4];		//имена сегментов

extern bool Interrupts_enabled; //разрешение прерываний
extern bool log_to_console_FDD;
extern bool cont_exec;

//временные регистры
extern uint16 temp_ACC_16;
extern uint8 temp_ACC_8;
extern uint16 temp_Addr;

//вспомогательные переменные для рассчета адресов операндов
uint8 additional_IPs;
uint32 New_Addr;
string OPCODE_comment;

uint16 operand_RM_seg;
uint16 operand_RM_offset;

//вспомогательные регистры для расчетов

uint16 Src = 0;
uint16 Dst = 0;
uint16* ptr_Src = &Src;
uint8* ptr_Src_L = (uint8*)ptr_Src;
uint8* ptr_Src_H = ptr_Src_L + 1;
uint16* ptr_Dst = &Dst;
uint8* ptr_Dst_L = (uint8*)ptr_Dst;
uint8* ptr_Dst_H = ptr_Dst_L + 1;

//префикс замены сегмента
extern uint8 Flag_segment_override;

//флаг аппаратных прерываний
extern bool Flag_hardware_INT;

extern Mem_Ctrl memory;
extern uint8 memory_2[1024 * 1024]; //память 2.0

extern SoundMaker speaker;
extern IO_Ctrl IO_device;
extern IC8259 int_ctrl;

//extern uint16 Stack_Pointer;

extern uint16 registers[8];
extern bool parity_check[256];

extern bool log_to_console;
extern bool step_mode;

extern string regnames[8];
extern string pairnames[4];
extern Video_device monitor;

extern void (*op_code_table[256])();
extern void (*backup_table[256])();

extern bool test_mode;
extern bool repeat_test_op;
extern bool negate_IDIV;
int last_INT = 0; //номер последнего прерывания (для отладки)

//=============
__int16 DispCalc16(uint16 data); //декларация
__int8 DispCalc8(uint8 data);
//==============Service functions===============

//инициализация таблицы функций
void opcode_table_init()
{
	for (int i = 0; i < 256; i++) op_code_table[i] = &op_code_unknown; //заполняем таблицу пустой функцией

	op_code_table[0b00100110] = &segment_override_prefix;	//override prefix
	op_code_table[0b00101110] = &segment_override_prefix;	//override prefix
	op_code_table[0b00110110] = &segment_override_prefix;	//override prefix
	op_code_table[0b00111110] = &segment_override_prefix;	//override prefix

	op_code_table[0b10001000] = &mov_R_to_RM_8;				//move R-R/M
	op_code_table[0b10001001] = &mov_R_to_RM_16;			//move R-R/M
	op_code_table[0b10001010] = &mov_RM_to_R_8;				//move R-R/M
	op_code_table[0b10001011] = &mov_RM_to_R_16;			//move R-R/M

	op_code_table[0b11000110] = &IMM_8_to_RM;				//IMM_8 to R/M
	op_code_table[0b11000111] = &IMM_16_to_RM;				//IMM_16 to R/M

	op_code_table[0b10110000] = &IMM_8_to_R;				//IMM_8 to Register
	op_code_table[0b10110001] = &IMM_8_to_R;				//IMM_8 to Register
	op_code_table[0b10110010] = &IMM_8_to_R;				//IMM_8 to Register
	op_code_table[0b10110011] = &IMM_8_to_R;				//IMM_8 to Register
	op_code_table[0b10110100] = &IMM_8_to_R;				//IMM_8 to Register
	op_code_table[0b10110101] = &IMM_8_to_R;				//IMM_8 to Register
	op_code_table[0b10110110] = &IMM_8_to_R;				//IMM_8 to Register
	op_code_table[0b10110111] = &IMM_8_to_R;				//IMM_8 to Register

	op_code_table[0b10111000] = &IMM_16_to_R;				//IMM_16 to Register
	op_code_table[0b10111001] = &IMM_16_to_R;				//IMM_16 to Register
	op_code_table[0b10111010] = &IMM_16_to_R;				//IMM_16 to Register
	op_code_table[0b10111011] = &IMM_16_to_R;				//IMM_16 to Register
	op_code_table[0b10111100] = &IMM_16_to_R;				//IMM_16 to Register
	op_code_table[0b10111101] = &IMM_16_to_R;				//IMM_16 to Register
	op_code_table[0b10111110] = &IMM_16_to_R;				//IMM_16 to Register
	op_code_table[0b10111111] = &IMM_16_to_R;				//IMM_16 to Register

	op_code_table[0b10100000] = &M_8_to_ACC;				//Memory to Accumulator 8
	op_code_table[0b10100001] = &M_16_to_ACC;				//Memory to Accumulator 16
	op_code_table[0b10100010] = &ACC_8_to_M;				//Accumulator to Memory 8
	op_code_table[0b10100011] = &ACC_16_to_M;				//Accumulator to Memory 16
	op_code_table[0b10001110] = &RM_to_Segment_Reg;			//Register/Memory to Segment Register
	op_code_table[0b10001100] = &Segment_Reg_to_RM;			//Segment Register to Register/Memory

	op_code_table[0b01010000] = &Push_R;					//PUSH Register
	op_code_table[0b01010001] = &Push_R;					//PUSH Register
	op_code_table[0b01010010] = &Push_R;					//PUSH Register
	op_code_table[0b01010011] = &Push_R;					//PUSH Register
	op_code_table[0b01010100] = &Push_R;					//PUSH Register
	op_code_table[0b01010101] = &Push_R;					//PUSH Register
	op_code_table[0b01010110] = &Push_R;					//PUSH Register
	op_code_table[0b01010111] = &Push_R;					//PUSH Register

	op_code_table[0b00000110] = &Push_SegReg;				//PUSH Segment Register
	op_code_table[0b00001110] = &Push_SegReg;				//PUSH Segment Register
	op_code_table[0b00010110] = &Push_SegReg;				//PUSH Segment Register
	op_code_table[0b00011110] = &Push_SegReg;				//PUSH Segment Register

	op_code_table[0b10001111] = &Pop_RM;					//POP Register/Memory

	op_code_table[0b01011000] = &Pop_R;						//POP Register
	op_code_table[0b01011001] = &Pop_R;						//POP Register
	op_code_table[0b01011010] = &Pop_R;						//POP Register
	op_code_table[0b01011011] = &Pop_R;						//POP Register
	op_code_table[0b01011100] = &Pop_R;						//POP Register
	op_code_table[0b01011101] = &Pop_R;						//POP Register
	op_code_table[0b01011110] = &Pop_R;						//POP Register
	op_code_table[0b01011111] = &Pop_R;						//POP Register

	op_code_table[0b00000111] = &Pop_SegReg;				//POP Segment Register
	op_code_table[0b00001111] = &Pop_SegReg;				//POP Segment Register
	op_code_table[0b00010111] = &Pop_SegReg;				//POP Segment Register
	op_code_table[0b00011111] = &Pop_SegReg;				//POP Segment Register

	op_code_table[0b10000110] = &XCHG_8;				//Exchange Register/Memory with Register 8bit
	op_code_table[0b10000111] = &XCHG_16;				//Exchange Register/Memory with Register 16bit
	op_code_table[0b10010000] = &XCHG_ACC_R;			//Exchange Register with Accumulator
	op_code_table[0b10010001] = &XCHG_ACC_R;			//Exchange Register with Accumulator
	op_code_table[0b10010010] = &XCHG_ACC_R;			//Exchange Register with Accumulator
	op_code_table[0b10010011] = &XCHG_ACC_R;			//Exchange Register with Accumulator
	op_code_table[0b10010100] = &XCHG_ACC_R;			//Exchange Register with Accumulator
	op_code_table[0b10010101] = &XCHG_ACC_R;			//Exchange Register with Accumulator
	op_code_table[0b10010110] = &XCHG_ACC_R;			//Exchange Register with Accumulator
	op_code_table[0b10010111] = &XCHG_ACC_R;			//Exchange Register with Accumulator

	op_code_table[0b11100100] = &In_8_to_ACC_from_port;	  //Input 8 to AL/AX AX from fixed PORT
	op_code_table[0b11100101] = &In_16_to_ACC_from_port;  //Input 16 to AL/AX AX from fixed PORT
	op_code_table[0b11100110] = &Out_8_from_ACC_to_port;  //Output 8 from AL/AX AX from fixed PORT
	op_code_table[0b11100111] = &Out_16_from_ACC_to_port; //Output 16 from AL/AX AX from fixed PORT
	op_code_table[0b11101100] = &In_8_from_DX;			  //In 8 from variable PORT
	op_code_table[0b11101101] = &In_16_from_DX;			  //In 16 from variable PORT
	op_code_table[0b11101110] = &Out_8_to_DX;			  //Out 8 to variable PORT
	op_code_table[0b11101111] = &Out_16_to_DX;			  //Out 16 to variable PORT

	op_code_table[0b11010111] = &XLAT;		//Translate Byte to AL
	op_code_table[0b10001101] = &LEA;		//Load EA to Register
	op_code_table[0b11000101] = &LDS;		//Load Pointer to DS
	op_code_table[0b11000100] = &LES;		//Load Pointer to ES

	op_code_table[0b10011111] = &LAHF;		// Load AH with Flags
	op_code_table[0b10011110] = &SAHF;		// Store AH with Flags
	op_code_table[0b10011100] = &PUSHF;		// Push Flags
	op_code_table[0b10011101] = &POPF;		// Pop Flags

	//============Arithmetic===================================

	//ADD (также в XOR_OR_IMM_RM_8 и XOR_OR_IMM_RM_16, &ADD_IMM_RM_16s)
	op_code_table[0b00000000] = &ADD_R_to_RM_8;		// ADD R/M -> R/M 8bit
	op_code_table[0b00000001] = &ADD_R_to_RM_16;		// ADD R/M -> R/M 16bit
	op_code_table[0b00000010] = &ADD_RM_to_R_8;			// ADD R/M -> R 8bit
	op_code_table[0b00000011] = &ADD_RM_to_R_16;		// ADD R/M -> R 16bit
	op_code_table[0b10000011] = &ADD_IMM_RM_16s;		// ADD IMM -> R/M 16 bit sign ext.
	op_code_table[0b00000100] = &ADD_IMM_to_ACC_8;		// ADD IMM->ACC 8bit
	op_code_table[0b00000101] = &ADD_IMM_to_ACC_16;		// ADD IMM->ACC 16bit 

	//ADC (также в XOR_OR_IMM_RM_8 и XOR_OR_IMM_RM_16, &ADD_IMM_RM_16s)
	op_code_table[0b00010000] = &ADC_RM_to_RM_8;		// ADC R/M -> R/M 8bit
	op_code_table[0b00010001] = &ADC_RM_to_RM_16;		// ADC R/M -> R/M 16bit
	op_code_table[0b00010010] = &ADC_RM_to_R_8;			// ADC R/M -> R 8bit
	op_code_table[0b00010011] = &ADC_RM_to_R_16;		// ADC R/M -> R 16bit
	op_code_table[0b00010100] = &ADC_IMM_to_ACC_8;		// ADC IMM->ACC 8bit
	op_code_table[0b00010101] = &ADC_IMM_to_ACC_16;		// ADC IMM->ACC 16bit 

	//INC
	op_code_table[0b11111110] = &INC_RM_8;		// INC R/M 8bit
	op_code_table[0b01000000] = &INC_Reg;		//  INC reg 16 bit
	op_code_table[0b01000001] = &INC_Reg;		//  INC reg 16 bit
	op_code_table[0b01000010] = &INC_Reg;		//  INC reg 16 bit
	op_code_table[0b01000011] = &INC_Reg;		//  INC reg 16 bit
	op_code_table[0b01000100] = &INC_Reg;		//  INC reg 16 bit
	op_code_table[0b01000101] = &INC_Reg;		//  INC reg 16 bit
	op_code_table[0b01000110] = &INC_Reg;		//  INC reg 16 bit
	op_code_table[0b01000111] = &INC_Reg;		//  INC reg 16 bit

	//SUB (также в XOR_OR_IMM_RM_8 и XOR_OR_IMM_RM_16, &ADD_IMM_RM_16s)
	op_code_table[0b00101000] = &SUB_RM_from_RM_8;		// SUB R/M -> R/M 8bit
	op_code_table[0b00101001] = &SUB_RM_from_RM_16;		// SUB R/M -> R/M 16bit
	op_code_table[0b00101010] = &SUB_RM_from_R_8;		// SUB R/M -> R 8bit
	op_code_table[0b00101011] = &SUB_RM_from_R_16;		// SUB R/M -> R 16bit
	op_code_table[0b00101100] = &SUB_IMM_from_ACC_8;		// SUB ACC  8bit - IMM -> ACC
	op_code_table[0b00101101] = &SUB_IMM_from_ACC_16;		// SUB ACC 16bit - IMM -> ACC

	//SBB (также в XOR_OR_IMM_RM_8 и XOR_OR_IMM_RM_16, &ADD_IMM_RM_16s)
	op_code_table[0b00011000] = &SBB_RM_from_RM_8;		// SBB R/M -> R/M 8bit
	op_code_table[0b00011001] = &SBB_RM_from_RM_16;		// SBB R/M -> R/M 16bit
	op_code_table[0b00011010] = &SBB_RM_from_R_8;		// SBB R/M -> R 8bit
	op_code_table[0b00011011] = &SBB_RM_from_R_16;		// SBB R/M -> R 16bit
	op_code_table[0b00011100] = &SBB_IMM_from_ACC_8;		// SBB ACC  8bit - IMM -> ACC
	op_code_table[0b00011101] = &SBB_IMM_from_ACC_16;		// SBB ACC 16bit - IMM -> ACC

	//DEC
	op_code_table[0b01001000] = &DEC_Reg;		//  DEC reg 16 bit
	op_code_table[0b01001001] = &DEC_Reg;		//  DEC reg 16 bit
	op_code_table[0b01001010] = &DEC_Reg;		//  DEC reg 16 bit
	op_code_table[0b01001011] = &DEC_Reg;		//  DEC reg 16 bit
	op_code_table[0b01001100] = &DEC_Reg;		//  DEC reg 16 bit
	op_code_table[0b01001101] = &DEC_Reg;		//  DEC reg 16 bit
	op_code_table[0b01001110] = &DEC_Reg;		//  DEC reg 16 bit
	op_code_table[0b01001111] = &DEC_Reg;		//  DEC reg 16 bit

	//NEG (Invert_RM_8 и Invert_RM_16)

	//CMP (также в XOR_OR_IMM_RM_8, XOR_OR_IMM_RM_16, ADD_IMM_RM_16s)
	op_code_table[0b00111000] = &CMP_Reg_RM_8;	//  CMP Reg with R/M 8 bit
	op_code_table[0b00111001] = &CMP_Reg_RM_16;	//  CMP Reg with R/M 16 bit
	op_code_table[0b00111010] = &CMP_RM_Reg_8;	//  CMP R/M with Reg 8 bit
	op_code_table[0b00111011] = &CMP_RM_Reg_16;	//  CMP R/M with Reg 16 bit
	op_code_table[0b00111100] = &CMP_IMM_with_ACC_8;		// CMP IMM  8bit - ACC
	op_code_table[0b00111101] = &CMP_IMM_with_ACC_16;		// CMP IMM 16bit - ACC

	//Двоично-десятичная ботва

	op_code_table[0b00110111] = &AAA; //  AAA = ASCII Adjust for Add
	op_code_table[0b00100111] = &DAA; //  DAA = Decimal Adjust for Add
	op_code_table[0b00111111] = &AAS; //AAS = ASCII Adjust for Subtract
	op_code_table[0b00101111] = &DAS; //DAS = Decimal Adjust for Subtract
	op_code_table[0b11010100] = &AAM; //AAM = ASCII Adjust for Multiply
	op_code_table[0b11010101] = &AAD; //AAD = ASCII Adjust for Divide
	op_code_table[0b10011000] = &CBW; //CBW = Convert Byte to Word
	op_code_table[0b10011001] = &CWD; //CWD = Convert Word to Double Word

	//============Logic========================================

	op_code_table[0b11110110] = &Invert_RM_8;		// NOT/NEG/MUL/DIV R/M 8bit
	op_code_table[0b11110111] = &Invert_RM_16;		// NOT/NEG/MUL/DIV R/M 16bit

	op_code_table[0b11010000] = &SHL_ROT_8;			// Shift Logical / Arithmetic Left / 8bit / once
	op_code_table[0b11010001] = &SHL_ROT_16;		// Shift Logical / Arithmetic Left / 16bit / once
	op_code_table[0b11010010] = &SHL_ROT_8_mult;	// Shift Logical / Arithmetic Left / 8bit / CL
	op_code_table[0b11010011] = &SHL_ROT_16_mult;	// Shift Logical / Arithmetic Left / 16bit / CL

	//AND (также в XOR_OR_IMM_RM_8 и XOR_OR_IMM_RM_16)
	op_code_table[0b00100000] = &AND_RM_8;			// AND R + R/M -> R/M 8bit
	op_code_table[0b00100001] = &AND_RM_16;			// AND R + R/M -> R/M 16bit
	op_code_table[0b00100010] = &AND_RM_R_8;		// AND R + R/M -> R 8bit
	op_code_table[0b00100011] = &AND_RM_R_16;		// AND R + R/M -> R 16bit
	op_code_table[0b00100100] = &AND_IMM_ACC_8;		//AND IMM to ACC 8bit
	op_code_table[0b00100101] = &AND_IMM_ACC_16;    //AND IMM to ACC 16bit

	//TEST (также в Invert_RM_8 и Invert_RM_16)
	op_code_table[0b10000100] = &TEST_8;			//TEST = AND Function to Flags
	op_code_table[0b10000101] = &TEST_16;			//TEST = AND Function to Flags
	op_code_table[0b10101000] = &TEST_IMM_ACC_8;	//TEST = AND Function to Flags
	op_code_table[0b10101001] = &TEST_IMM_ACC_16;    //TEST = AND Function to Flags

	//OR = Or
	op_code_table[0b00001000] = &OR_RM_8;		// OR R + R/M -> R/M 8bit
	op_code_table[0b00001001] = &OR_RM_16;		// OR R + R/M -> R/M 16bit
	op_code_table[0b00001010] = &OR_RM_R_8;		// OR R + R/M -> R 8bit
	op_code_table[0b00001011] = &OR_RM_R_16;	// OR R + R/M -> R 16bit

	op_code_table[0b00001100] = &OR_IMM_ACC_8;    //OR IMM to ACC 8bit
	op_code_table[0b00001101] = &OR_IMM_ACC_16;   //OR IMM to ACC 16bit

	//XOR = Exclusive OR
	op_code_table[0b00110000] = &XOR_RM_8;		// XOR R + R/M -> R/M 8bit
	op_code_table[0b00110001] = &XOR_RM_16;		// XOR R + R/M -> R/M 16bit
	op_code_table[0b00110010] = &XOR_RM_R_8;	// XOR R + R/M -> R 8bit
	op_code_table[0b00110011] = &XOR_RM_R_16;	// XOR R + R/M -> R 16bit

	op_code_table[0b10000000] = &XOR_OR_IMM_RM_8;    //XOR IMM to Register/Memory 8bit
	op_code_table[0b10000010] = &XOR_OR_IMM_RM_8;    //UNDOC Alias
	op_code_table[0b10000001] = &XOR_OR_IMM_RM_16;   //XOR IMM to Register/Memory 16bit

	op_code_table[0b00110100] = &XOR_IMM_ACC_8;    //XOR IMM to ACC 8bit
	op_code_table[0b00110101] = &XOR_IMM_ACC_16;   //XOR IMM to ACC 16bit

	//============String Manipulation=========================

	op_code_table[0b11110010] = &REPNE;	 //REP = Repeat_while ZF=0
	op_code_table[0b11110011] = &REP;	 //REP = Repeat

	op_code_table[0b10100100] = &MOVES_8;    //MOVS = Move String 8bit
	op_code_table[0b10100101] = &MOVES_16;   //MOVS = Move String 16bit

	op_code_table[0b10100110] = &CMPS_8;     //CMPS = Compare String
	op_code_table[0b10100111] = &CMPS_16;    //CMPS = Compare String

	op_code_table[0b10101110] = &SCAS_8;     //SCAS = Scan String
	op_code_table[0b10101111] = &SCAS_16;     //SCAS = Scan String

	op_code_table[0b10101100] = &LODS_8;     //LODS = Load String
	op_code_table[0b10101101] = &LODS_16;    //LODS = Load String

	op_code_table[0b10101010] = &STOS_8;     //STOS = Store String
	op_code_table[0b10101011] = &STOS_16;    //STOS = Store String

	//============ Control Transfer============================

	// Call + Jump
	op_code_table[0b11101000] = &Call_Near;					//Direct Call within Segment
	op_code_table[0b11111111] = &Call_Jump_Push;			//(5 operations)
	op_code_table[0b10011010] = &Call_dir_interseg;			//Direct Intersegment Call

	op_code_table[0b11101011] = &Jump_Near_8;				//Direct jump within Segment-Short
	op_code_table[0b11101001] = &Jump_Near_16;				//Direct jump within Segment-Short
	op_code_table[0b11101010] = &Jump_Far;					//Direct Intersegment Jump

	// RET
	op_code_table[0b11000011] = &RET_Segment;				//Return Within Segment
	op_code_table[0b11000001] = &RET_Segment;				//(UNDOC)
	op_code_table[0b11000010] = &RET_Segment_IMM_SP;		//Return Within Segment Adding Immediate to SP
	op_code_table[0b11000000] = &RET_Segment_IMM_SP;		//(UNDOC)
	op_code_table[0b11001011] = &RET_Inter_Segment;			//Return Intersegment
	op_code_table[0b11001001] = &RET_Inter_Segment;			//(UNDOC)
	op_code_table[0b11001010] = &RET_Inter_Segment_IMM_SP;	//Return Intersegment Adding Immediate to SP
	op_code_table[0b11001000] = &RET_Inter_Segment_IMM_SP;	//(UNDOC)

	// Conditional Jumps
	op_code_table[0b01110100] = &JE_JZ;			// JE/JZ = Jump on Equal/Zero
	op_code_table[0b01100100] = &JE_JZ;			// JE/JZ = Jump on Equal/Zero  (UNDOC)
	op_code_table[0b01111100] = &JL_JNGE;		// JL/JNGE = Jump on Less/Not Greater, or Equal
	op_code_table[0b01101100] = &JL_JNGE;		// JL/JNGE = Jump on Less/Not Greater, or Equal (UNDOC)
	op_code_table[0b01111110] = &JLE_JNG;		// JLE/JNG = Jump on Less, or Equal/Not Greater
	op_code_table[0b01101110] = &JLE_JNG;		// JLE/JNG = Jump on Less, or Equal/Not Greater (UNDOC)
	op_code_table[0b01110010] = &JB_JNAE;		// JB/JNAE = Jump on Below/Not Above, or Equal
	op_code_table[0b01100010] = &JB_JNAE;		// JB/JNAE = Jump on Below/Not Above, or Equal  (UNDOC)
	op_code_table[0b01110110] = &JBE_JNA;		// JBE/JNA = Jump on Below, or Equal/Not Above
	op_code_table[0b01100110] = &JBE_JNA;		// JBE/JNA = Jump on Below, or Equal/Not Above  (UNDOC)
	op_code_table[0b01111010] = &JP_JPE;		// JP/JPE = Jump on Parity/Parity Even
	op_code_table[0b01101010] = &JP_JPE;		// JP/JPE = Jump on Parity/Parity Even  (UNDOC)
	op_code_table[0b01110000] = &JO;			// JO = Jump on Overflow
	op_code_table[0b01100000] = &JO;			// JO = Jump on Overflow (UNDOC)
	op_code_table[0b01111000] = &JS;			// JS = Jump on Sign
	op_code_table[0b01101000] = &JS;			// JS = Jump on Sign  (UNDOC)
	op_code_table[0b01110101] = &JNE_JNZ;		// JNE/JNZ = Jump on Not Equal/Not Zero
	op_code_table[0b01100101] = &JNE_JNZ;		// JNE/JNZ = Jump on Not Equal/Not Zero (UNDOC)
	op_code_table[0b01111101] = &JNL_JGE;		// JNL/JGE = Jump on Not Less/Greater, or Equal
	op_code_table[0b01101101] = &JNL_JGE;		// JNL/JGE = Jump on Not Less/Greater, or Equal (UNDOC)
	op_code_table[0b01111111] = &JNLE_JG;		//JNLE/JG = Jump on Not Less, or Equal/Greater
	op_code_table[0b01101111] = &JNLE_JG;		//JNLE/JG = Jump on Not Less, or Equal/Greater (UNDOC)
	op_code_table[0b01110011] = &JNB_JAE;		//JNB/JAE = Jump on Not Below/Above, or Equal
	op_code_table[0b01100011] = &JNB_JAE;		//JNB/JAE = Jump on Not Below/Above, or Equal  (UNDOC)
	op_code_table[0b01110111] = &JNBE_JA;		//JNBE/JA = Jump on Not Below, or Equal/Above
	op_code_table[0b01100111] = &JNBE_JA;		//JNBE/JA = Jump on Not Below, or Equal/Above (UNDOC)
	op_code_table[0b01111011] = &JNP_JPO;		//JNP/JPO = Jump on Not Parity/Parity Odd
	op_code_table[0b01101011] = &JNP_JPO;		//JNP/JPO = Jump on Not Parity/Parity Odd (UNDOC)
	op_code_table[0b01110001] = &JNO;			//JNO = Jump on Not Overflow
	op_code_table[0b01100001] = &JNO;			//JNO = Jump on Not Overflow (UNDOC)
	op_code_table[0b01111001] = &JNS;			//JNS = Jump on Not Sign
	op_code_table[0b01101001] = &JNS;			//JNS = Jump on Not Sign  (UNDOC)
	op_code_table[0b11100010] = &LOOP;			//LOOP = Loop CX Times
	op_code_table[0b11100001] = &LOOPZ_LOOPE;	//LOOPZ/LOOPE = Loop while Zero/Equal
	op_code_table[0b11100000] = &LOOPNZ_LOOPNE;	//LOOPNZ/LOOPNE = Loop while Not Zero/Not Equal
	op_code_table[0b11100011] = &JCXZ;			//JCXZ = Jump on CX Zero

	//========Conditional Transfer-===============

	op_code_table[0b11001101] = &INT_N;			//INT = Interrupt
	op_code_table[0b11001100] = &INT_3;			//INT = Interrupt Type 3
	op_code_table[0b11001110] = &INT_O;			//INTO = Interrupt on Overflow
	op_code_table[0b11001111] = &I_RET;			//Interrupt Return

	//==========Processor Control=================
	op_code_table[0b11111000] = &CLC;					//Clear Carry
	op_code_table[0b11111001] = &STC;					//Set Carry
	op_code_table[0b11110101] = &CMC;					//Complement Carry
	op_code_table[0b10010000] = &op_code_NOP;			//NOP
	op_code_table[0b11111100] = &CLD;					//Clear Direction
	op_code_table[0b11111101] = &STD;					//Set Direction
	op_code_table[0b11111010] = &CLI;					//Clear Interrupt
	op_code_table[0b11111011] = &STI;					//Set Interrupt
	op_code_table[0b11110100] = &HLT;					//Halt
	op_code_table[0b10011011] = &Wait;					//Wait
	op_code_table[0b11110000] = &Lock;					//Bus lock prefix
	op_code_table[0b11011000] = &Esc_8087;				//Call 8087
	op_code_table[0b11011001] = &Esc_8087;				//Call 8087
	op_code_table[0b11011010] = &Esc_8087;				//Call 8087
	op_code_table[0b11011011] = &Esc_8087;				//Call 8087
	op_code_table[0b11011100] = &Esc_8087;				//Call 8087
	op_code_table[0b11011101] = &Esc_8087;				//Call 8087
	op_code_table[0b11011110] = &Esc_8087;				//Call 8087
	op_code_table[0b11011111] = &Esc_8087;				//Call 8087

	//Недокументированные команды

	op_code_table[0b11010110] = &CALC;

	//указатели на операнды
	
	ptr_r8[0] = ptr_AL;
	ptr_r8[1] = ptr_CL;
	ptr_r8[2] = ptr_DL;
	ptr_r8[3] = ptr_BL;
	ptr_r8[4] = ptr_AH;
	ptr_r8[5] = ptr_CH;
	ptr_r8[6] = ptr_DH;
	ptr_r8[7] = ptr_BH;

	ptr_r16[0] = ptr_AX;
	ptr_r16[1] = ptr_CX;
	ptr_r16[2] = ptr_DX;
	ptr_r16[3] = ptr_BX;
	ptr_r16[4] = ptr_SP;
	ptr_r16[5] = ptr_BP;
	ptr_r16[6] = ptr_SI;
	ptr_r16[7] = ptr_DI;

	ptr_segreg[0] = ES;
	ptr_segreg[1] = CS;
	ptr_segreg[2] = SS;
	ptr_segreg[3] = DS;

	segreg_name[0] = "ES";
	segreg_name[1] = "CS";
	segreg_name[2] = "SS";
	segreg_name[3] = "DS";

	reg8_name[0] = "AL";		//8 бит
	reg8_name[1] = "CL";		//8 бит
	reg8_name[2] = "DL";		//8 бит
	reg8_name[3] = "BL";		//8 бит
	reg8_name[4] = "AH";		//8 бит
	reg8_name[5] = "CH";		//8 бит
	reg8_name[6] = "DH";		//8 бит
	reg8_name[7] = "BH";		//8 бит

	reg16_name[0] = "AX";		//16 бит
	reg16_name[1] = "CX";		//16 бит
	reg16_name[2] = "DX";		//16 бит
	reg16_name[3] = "BX";		//16 бит
	reg16_name[4] = "SP";		//16 бит
	reg16_name[5] = "BP";		//16 бит
	reg16_name[6] = "SI";		//16 бит
	reg16_name[7] = "DI";		//16 бит

	//таблица для сверки
	for (int i = 0; i < 256; ++i) backup_table[i] = op_code_table[i];


}
void op_code_unknown()		// Unknown operation
{
	if (log_to_console) cout << "Unknown operation IP = " << Instruction_Pointer << " OPCODE = " << (bitset<8>)memory.read_2(Instruction_Pointer + *CS * 16);
	step_mode = 1;
	log_to_console = 1;
	Instruction_Pointer++;
}
void op_code_NOP()  // NOP
{
	Instruction_Pointer++;
#ifdef DEBUG
	if (log_to_console) cout << "NOP";
#endif
}
void segment_override_prefix()
{
	uint8 Seg = (memory_2[(Instruction_Pointer + *CS * 16) & 0xFFFFF] >> 3) & 3;

#ifdef DEBUG
	if (log_to_console) cout << "segment_override_prefix ";
#endif
	
	/*
	Замена сегментов
	ES — 0x00
	CS — 0x01
	SS — 0x10
	DS — 0x11
	*/
	
	switch (Seg)
	{
	case 0:
#ifdef DEBUG
		if (log_to_console) cout << "ES(" << *ES << ")";
#endif
		DS = &ES_data;
		SS = &ES_data;
		break;
	case 1:
#ifdef DEBUG
		if (log_to_console) cout << "CS(" << *CS << ")";
#endif
		DS = &CS_data;
		SS = &CS_data;
		break;
	case 2:
#ifdef DEBUG
		if (log_to_console) cout << "SS(" << *SS << ")";
#endif
		DS = &SS_data;
		break;
	case 3:
#ifdef DEBUG
		if (log_to_console) cout << "DS(" << *DS << ")";
#endif
		SS = &DS_data;
		break;
	}
#ifdef DEBUG
	if (log_to_console) cout << endl;
#endif
	Instruction_Pointer++;

cmd_rep:
	//выполняем следующую команду
#ifdef DEBUG
	if (log_to_console)
	{
		cout << hex;
		//cout << int_to_hex(memory.read_2(0x441, 0), 2) << "  " << int_to_hex(memory.read_2(0x442, 0), 2) << " ";
		cout << *CS << ":" << std::setfill('0') << std::setw(4) << Instruction_Pointer << "  " <<
			std::setfill('0') << std::setw(2) << (int)memory.read_2(Instruction_Pointer + *CS * 16) << "  " <<
			std::setfill('0') << std::setw(2) << (int)memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] << "  " <<
			std::setfill('0') << std::setw(2) << (int)memory.read_2(Instruction_Pointer + 2 + *CS * 16) << "  " <<
			std::setfill('0') << std::setw(2) << (int)memory.read_2(Instruction_Pointer + 3 + *CS * 16) << "  " <<
			std::setfill('0') << std::setw(2) << (int)memory.read_2(Instruction_Pointer + 4 + *CS * 16) << "  " <<
			std::setfill('0') << std::setw(2) << (int)memory.read_2(Instruction_Pointer + 5 + *CS * 16) << "\t";
	}
#endif
	op_code_table[memory.read_2(Instruction_Pointer + *CS * 16)]();
	//если установлен флаг negate_IDIV - выполняем еще одну команду
	if (negate_IDIV)
	{
		if (log_to_console) cout << endl;
		goto cmd_rep;
	}
	DS = &DS_data; //возвращаем назад сегмент
	SS = &SS_data;
}
uint16 mod_RM_Old(uint8 byte2)		//расчет адреса операнда по биту 2
{
	
	additional_IPs = 0;
	
	if ((byte2 >> 6) == 0) //no displacement
	{
		switch (byte2 & 7)
		{
		case 0:
			return BX + Source_Index;
		case 1:
			return BX + Destination_Index;
		case 2:
			return Base_Pointer + Source_Index;
		case 3:
			return Base_Pointer + Destination_Index;
		case 4:
			return Source_Index;
		case 5:
			return Destination_Index;
		case 6:
			additional_IPs = 2;
			return memory.read_2(uint16(Instruction_Pointer + 2) + *CS * 16) + memory.read_2(uint16(Instruction_Pointer + 3) + *CS * 16) * 256;
		case 7:
			return BX;
		}

	}
	if ((byte2 >> 6) == 1) // 8-bit displacement
	{
		//грузим смещение
		__int8 d8 = memory.read_2(uint16(Instruction_Pointer + 2) + *CS * 16);
		
		additional_IPs = 1;
		
		switch (byte2 & 7)
		{
		case 0:
			return BX + Source_Index + d8;
		case 1:
			return BX + Destination_Index + d8;
		case 2:
			return Base_Pointer + Source_Index + d8;
		case 3:
			return Base_Pointer + Destination_Index + d8;
		case 4:
			return Source_Index + d8;
		case 5:
			return Destination_Index + d8;
		case 6:
			return Base_Pointer + d8;
		case 7:
			return BX + d8;
		}

	}
	if ((byte2 >> 6) == 2) // 16-bit displacement
	{
		//грузим два байта смещения
		__int16 d16 = memory.read_2(uint16(Instruction_Pointer + 2) + *CS * 16) + memory.read_2(uint16(Instruction_Pointer + 3) + *CS * 16) * 256;
		
		additional_IPs = 2;

		switch (byte2 & 7)
		{
		case 0:
			
			return BX + Source_Index + d16;
		case 1:
			return BX + Destination_Index + d16;
		case 2:
			return Base_Pointer + Source_Index + d16;
		case 3:
			return Base_Pointer + Destination_Index + d16;
		case 4:
			return Source_Index + d16;
		case 5:
			return Destination_Index + d16;
		case 6:
			return Base_Pointer + d16;
		case 7:
			return BX + d16;
		}
	}


	return 0;
	
}
uint32 mod_RM_2(uint8 byte2)		//расчет адреса операнда по биту 2
{
	if ((byte2 >> 6) == 0) //no displacement
	{
		switch (byte2 & 7)
		{
		case 0:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((BX + Source_Index) & 0xFFFF, 4) + "]";
			return ((BX + Source_Index) & 0xFFFF) + *DS * 16;
		case 1:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((BX + Destination_Index) & 0xFFFF, 4) + "]";
			return ((BX + Destination_Index) & 0xFFFF) + *DS * 16;
		case 2:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*SS, 4) + ":" + int_to_hex((Base_Pointer + Source_Index) & 0xFFFF, 4) + "]";
			return ((Base_Pointer + Source_Index) & 0xFFFF) + *SS * 16;
		case 3:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*SS, 4) + ":" + int_to_hex((Base_Pointer + Destination_Index) & 0xFFFF, 4) + "]";
			return ((Base_Pointer + Destination_Index) & 0xFFFF) + *SS * 16;
		case 4:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((Source_Index) & 0xFFFF, 4) + "]";
			return ((Source_Index) & 0xFFFF) + *DS * 16;
		case 5:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((Destination_Index) & 0xFFFF, 4) + "]";
			return ((Destination_Index) & 0xFFFF) + *DS * 16;
		case 6:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex(memory.read_2(Instruction_Pointer + 2 + *CS * 16) + memory.read_2(Instruction_Pointer + 3 + *CS * 16) * 256, 4) + "]";
			additional_IPs = 2;
			return memory.read_2(Instruction_Pointer + 2 + *CS * 16) + memory.read_2(Instruction_Pointer + 3 + *CS * 16) * 256 + *DS * 16;
		case 7:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex(BX, 4) + "]";
			return BX + *DS * 16;
		}

	}
	if ((byte2 >> 6) == 1) // 8-bit displacement
	{
		additional_IPs = 1;

		//грузим смещение
		__int8 d8 = memory.read_2(Instruction_Pointer + 2 + *CS * 16);

		switch (byte2 & 7)
		{
		case 0:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((BX + Source_Index + d8) & 0xFFFF, 4) + "]";
			return ((BX + Source_Index + d8) & 0xFFFF) + *DS * 16;
		case 1:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((BX + Destination_Index + d8) & 0xFFFF, 4) + "]";
			return ((BX + Destination_Index + d8) & 0xFFFF) + *DS * 16;
		case 2:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*SS, 4) + ":" + int_to_hex((Base_Pointer + Source_Index + d8) & 0xFFFF, 4) + "]";
			return ((Base_Pointer + Source_Index + d8) & 0xFFFF) + *SS * 16;
		case 3:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*SS, 4) + ":" + int_to_hex((Base_Pointer + Destination_Index + d8) & 0xFFFF, 4) + "]";
			return ((Base_Pointer + Destination_Index + d8) & 0xFFFF) + *SS * 16;
		case 4:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((Source_Index + d8) & 0xFFFF, 4) + "]";
			return ((Source_Index + d8) & 0xFFFF) + *DS * 16;
		case 5:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((Destination_Index + d8) & 0xFFFF, 4) + "]";
			return ((Destination_Index + d8) & 0xFFFF) + *DS * 16;
		case 6:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*SS, 4) + ":" + int_to_hex((Base_Pointer + d8) & 0xFFFF, 4) + "]";
			return ((Base_Pointer + d8) & 0xFFFF) + *SS * 16;
		case 7:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((BX + d8) & 0xFFFF, 4) + "]";
			return ((BX + d8) & 0xFFFF) + *DS * 16;
		}
	}
	if ((byte2 >> 6) == 2) // 16-bit displacement
	{
		additional_IPs = 2;

		//грузим два байта смещения
		__int16 d16 = memory.read_2(Instruction_Pointer + 2 + *CS * 16) + memory.read_2(Instruction_Pointer + 3 + *CS * 16) * 256;

		switch (byte2 & 7)
		{
		case 0:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((BX + Source_Index + d16) & 0xFFFF, 4) + "]";
			return ((BX + Source_Index + d16) & 0xFFFF) + *DS * 16;
		case 1:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((BX + Destination_Index + d16) & 0xFFFF, 4) + "]";
			return ((BX + Destination_Index + d16) & 0xFFFF) + *DS * 16;
		case 2:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*SS, 4) + ":" + int_to_hex((Base_Pointer + Source_Index + d16) & 0xFFFF, 4) + "]";
			return ((Base_Pointer + Source_Index + d16) & 0xFFFF) + *SS * 16;
		case 3:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*SS, 4) + ":" + int_to_hex((Base_Pointer + Destination_Index + d16) & 0xFFFF, 4) + "]";
			return ((Base_Pointer + Destination_Index + d16) & 0xFFFF) + *SS * 16;
		case 4:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((Source_Index + d16) & 0xFFFF, 4) + "]";
			return ((Source_Index + d16) & 0xFFFF) + *DS * 16;
		case 5:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((Destination_Index + d16) & 0xFFFF, 4) + "]";
			return ((Destination_Index + d16) & 0xFFFF) + *DS * 16;
		case 6:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*SS, 4) + ":" + int_to_hex((Base_Pointer + d16) & 0xFFFF, 4) + "]";
			return ((Base_Pointer + d16) & 0xFFFF) + *SS * 16;
		case 7:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((BX + d16) & 0xFFFF, 4) + "]";
			return ((BX + d16) & 0xFFFF) + *DS * 16;
		}
	}
	return 0;
}
void mod_RM_3(uint8 byte2)		//расчет адреса операнда по биту 2
{
	if ((byte2 >> 6) == 0) //no displacement
	{
		additional_IPs = 0;
		switch (byte2 & 7)
		{
		case 0:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS,4) + ":" + int_to_hex((BX + Source_Index) & 0xFFFF, 4) + "]";
			operand_RM_seg = *DS;
			operand_RM_offset = BX + Source_Index;
			break;
		case 1:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((BX + Destination_Index) & 0xFFFF, 4) + "]";
			operand_RM_seg = *DS;
			operand_RM_offset = BX + Destination_Index;
			break;
		case 2:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*SS, 4) + ":" + int_to_hex((Base_Pointer + Source_Index) & 0xFFFF, 4) + "]";
			operand_RM_seg = *SS;
			operand_RM_offset = Base_Pointer + Source_Index;
			break;
		case 3:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*SS, 4) + ":" + int_to_hex((Base_Pointer + Destination_Index) & 0xFFFF, 4) + "]";
			operand_RM_seg = *SS;
			operand_RM_offset = Base_Pointer + Destination_Index;
			break;
		case 4:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((Source_Index) & 0xFFFF, 4) + "]";
			operand_RM_seg = *DS;
			operand_RM_offset = Source_Index;
			break;
		case 5:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((Destination_Index) & 0xFFFF, 4) + "]";
			operand_RM_seg = *DS;
			operand_RM_offset = Destination_Index;
			break;
		case 6:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex(memory.read_2(Instruction_Pointer + 2 + *CS * 16) + memory.read_2(Instruction_Pointer + 3 + *CS * 16) * 256, 4) + "]";
			additional_IPs = 2;
			operand_RM_seg = *DS;
			operand_RM_offset = memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF] + memory_2[(Instruction_Pointer + 3 + *CS * 16) & 0xFFFFF] * 256;
			break;
		case 7:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex(BX, 4) + "]";
			operand_RM_seg = *DS;
			operand_RM_offset = BX;
			break;
		}
		return;
	}
	if ((byte2 >> 6) == 1) // 8-bit displacement
	{
		additional_IPs = 1;
		
		//грузим смещение
		__int8 d8 = memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF];

		switch (byte2 & 7)
		{
		case 0:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((BX + Source_Index + d8) & 0xFFFF, 4) + "]";
			operand_RM_seg = *DS;
			operand_RM_offset = BX + Source_Index + d8;
			break;
		case 1:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((BX + Destination_Index + d8) & 0xFFFF, 4) + "]";
			operand_RM_seg = *DS;
			operand_RM_offset = BX + Destination_Index + d8;
			break;
		case 2:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*SS, 4) + ":" + int_to_hex((Base_Pointer + Source_Index + d8) & 0xFFFF, 4) + "]";
			operand_RM_seg = *SS;
			operand_RM_offset = Base_Pointer + Source_Index + d8;
			break;
		case 3:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*SS, 4) + ":" + int_to_hex((Base_Pointer + Destination_Index + d8) & 0xFFFF, 4) + "]";
			operand_RM_seg = *SS;
			operand_RM_offset = Base_Pointer + Destination_Index + d8;
			break;
		case 4:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((Source_Index + d8) & 0xFFFF, 4) + "]";
			operand_RM_seg = *DS;
			operand_RM_offset = Source_Index + d8;
			break;
		case 5:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((Destination_Index + d8) & 0xFFFF, 4) + "]";
			operand_RM_seg = *DS;
			operand_RM_offset = Destination_Index + d8;
			break;
		case 6:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*SS, 4) + ":" + int_to_hex((Base_Pointer + d8) & 0xFFFF, 4) + "]";
			operand_RM_seg = *SS;
			operand_RM_offset = Base_Pointer + d8;
			break;
		case 7:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((BX + d8) & 0xFFFF, 4) + "]";
			operand_RM_seg = *DS;
			operand_RM_offset = BX + d8;
			break;
		}
		return;
	}
	if ((byte2 >> 6) == 2) // 16-bit displacement
	{
		additional_IPs = 2;
		
		//грузим два байта смещения
		__int16 d16 = memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF] + memory_2[(Instruction_Pointer + 3 + *CS * 16) & 0xFFFFF] * 256;

		switch (byte2 & 7)
		{
		case 0:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((BX + Source_Index + d16) & 0xFFFF, 4) + "]";
			operand_RM_seg = *DS;
			operand_RM_offset = BX + Source_Index + d16;
			break;
		case 1:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((BX + Destination_Index + d16) & 0xFFFF, 4) + "]";
			operand_RM_seg = *DS;
			operand_RM_offset = BX + Destination_Index + d16;
			break;
		case 2:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*SS, 4) + ":" + int_to_hex((Base_Pointer + Source_Index + d16) & 0xFFFF, 4) + "]";
			operand_RM_seg = *SS;
			operand_RM_offset = Base_Pointer + Source_Index + d16;
			break;
		case 3:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*SS, 4) + ":" + int_to_hex((Base_Pointer + Destination_Index + d16) & 0xFFFF, 4) + "]";
			operand_RM_seg = *SS;
			operand_RM_offset = Base_Pointer + Destination_Index + d16;
			break;
		case 4:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((Source_Index + d16) & 0xFFFF, 4) + "]";
			operand_RM_seg = *DS;
			operand_RM_offset = Source_Index + d16;
			break;
		case 5:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((Destination_Index + d16) & 0xFFFF, 4) + "]";
			operand_RM_seg = *DS;
			operand_RM_offset = Destination_Index + d16;
			break;
		case 6:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*SS, 4) + ":" + int_to_hex((Base_Pointer + d16) & 0xFFFF, 4) + "] d16=" + to_string(d16);
			operand_RM_seg = *SS;
			operand_RM_offset = Base_Pointer + d16;
			break;
		case 7:
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*DS, 4) + ":" + int_to_hex((BX + d16) & 0xFFFF, 4) + "]";
			operand_RM_seg = *DS;
			operand_RM_offset = BX + d16;
			break;
		}
	}
}

__int8 DispCalc8(uint8 data)
{
	__int8 disp;
	//превращаем беззнаковое смещение в число со знаком
	if (data >> 7) disp = -127 + ((data - 1) & 127);
	else disp = data;
	if (data == 0x80) disp = -128;
	//if (log_to_console) cout << " disp = " << (int)data << "	";
	return disp;
}
__int16 DispCalc16(uint16 data)
{
	__int16 disp;
	//превращаем беззнаковое смещение в число со знаком
	if (data >> 15) disp = -0x7FFF + ((data - 1) & 0x7FFF);
	else disp = data;
	if (data == 0x8000) disp = -32768;
	//if (log_to_console) cout << " disp = " << (int)data << "	";
	return disp;
}
void syscall(uint8 INT_number)
{
	if (log_to_console) cout << "INT " << (int)INT_number << "(syscall)" << endl;
	if (INT_number == 0x10)
	{
		//INT 10 - усправление режимами работы монитора
		if ((AX >> 8) == 0)
		{
			//устанавливаем видеорежим
			monitor.set_CGA_mode(AX & 255);
			return;
		}
		if ((AX >> 8) == 1)
		{
			//установка типа курсора
			monitor.set_cursor_type(CX);
			return;
		}
		if ((AX >> 8) == 2)
		{
			//установка позиции курсора
			monitor.set_cursor_position(DX & 255, DX >> 8, BX >> 8);
			return;
		}
		if ((AX >> 8) == 3)
		{
			//чтение позиции курсора
			//регистры обновятся в самой функции
			monitor.read_cursor_position();
			return;
		}
		if ((AX >> 8) == 0xE)
		{
			//вывод символа в позицию курсора
			monitor.teletype(AX & 255);
			monitor.sync(1);
			//if (log_to_console || 1) cout << "sym_out(" << (int)(AX & 255) << ")" << endl;
			return;
		}

	}
	if (INT_number == 0x16)
	{
		//функции клавиатуры
		if ((AX >> 8) == 0)
		{
			//ждем нажатия клавиши
			uint8 key = 0;
			while (!key)
			{
			start:
				key = _getch();
				if (!key) 
				{ 
					key = _getch(); 
					goto start;
				}
				if (key == 13) { key = 13; break; }
				if (key == 8) { key = 8; break; }
				if (key == 9) { key = 9; break; }
				if (key < 32 || key > 126) key = 0;
				
			};

			//while (_kbhit()) _getch(); //очищаем буфер
			//cout << (int)key << " = " << (int)key << endl;
			AX = (AX & 0xFF00) | key;
			//if (log_to_console || 1) cout << "get key_code (" << (int)key << ")" << endl;
		}
		if ((AX >> 8) == 1)
		{
			//проверка наличия символа в буфере ввода



		}
	}
	if (INT_number == 0x21)
	{
		//cout << "(DOS " << (int)(AX >> 8) << ")";
		//системные вызовы DOS
		if ((AX >> 8) == 2)
		{
			//вывод символа
			monitor.teletype(DX & 255);
		}

		if ((AX >> 8) == 1)
		{
			//ввод символа + вывод на экран
			//ждем нажатия клавиши
			uint8 key = 0;
			while (!key)
			{
			start_2:
				key = _getch();
				if (!key)
				{
					key = _getch();
					goto start_2;
				}
				if (key == 13) { key = 13; break; }
				if (key == 8) { key = 8; break; }
				if (key == 9) { key = 9; break; }
				if (key < 32 || key > 126) key = 0;
				
			};
			//cout << "key = " << (int)key << endl;
			AX = (AX & 0xFF00) | key;
			monitor.teletype(key);
			monitor.sync(1);
		}
		if ((AX >> 8) == 9)
		{
			//вывод строки ($)
			
			while (memory.read_2(DX + *DS * 16) != 36)
			{
				monitor.teletype(memory.read_2(DX + *DS * 16));
				DX++;
			}
			monitor.sync(1);
		}

	}
}
std::string get_int21_data()
{
	string out;
	switch (*ptr_AH)
	{
	case 0:
		return "Program terminate";
	case 1:
		return "Keyboard input with echo";
	case 2:
		return "Display output";
	case 3:
		return "Wait for auxiliary device input";
	case 4:
		return "Auxiliary output";
	case 5:
		return "Printer output";
	case 6:
		if (*ptr_DL == 0xFF) "Direct console IO (INPUT)";
		return "Direct console IO (" + to_string(*ptr_DL) + " char(s) OUT)";
	case 7:
		return "Wait for direct console input without echo";
	case 8:
		return "Wait for console input without echo";
	case 9:
		out = "Print string: ";
		for (int i = 0; i < 80; ++i)
		{
			if (memory_2[(DS_data * 16 + DX + i) & 0xFFFFF] == 36) return out;
			if (memory_2[(DS_data * 16 + DX + i) & 0xFFFFF]) out+=(char)memory_2[(DS_data * 16 + DX + i) & 0xFFFFF];
		}
		return out;
	case 10:
		return "Buffered keyboard input";
	case 11:
		return "Check standard input status";
	case 12:
		return "Clear keyboard buffer, invoke keyboard function";
	case 13:
		return "Disk reset";
	case 14:
		return "Select disk";
	case 15:
		return "Open file using FCB";
	case 16:
		return "Close file using FCB";
	case 17:
		return "Search for first entry using FCB";
	case 18:
		return "Search for next entry using FCB";
	case 19:
		return "Delete file using FCB";
	case 20:
		return "Sequential read using FCB";
	case 21:
		return "Sequential write using FCB";
	case 22:
		return "Create a file using FCB";
	case 23:
		return "Rename file using FCB";
	case 24:
		return "DOS dummy function(CP / M) (not used / listed)";
	case 25:
		return "Get current default drive";
	case 26:
		return "Set disk transfer address";
	case 27:
		return "Get allocation table information";
	case 28:
		return "Get allocation table info for specific device";
	case 29:
		return "DOS dummy function(CP / M) (not used / listed)";
	case 30:
		return "DOS dummy function(CP / M) (not used / listed)";
	case 31:
		return "Get pointer to default drive parameter table(undocumented)";
	case 32:
		return "DOS dummy function(CP / M) (not used / listed)";
	case 33:
		return "Random read using FCB";
	case 34:
		return "Random write using FCB";
	case 35:
		return "Get file size using FCB";
	case 36:
		return "Set relative record field for FCB";
	case 37:
		out = "Set interrupt vector (" + int_to_hex(AX & 255, 2) + ") to " + int_to_hex(DS_data, 4) + ":" + int_to_hex(DX, 4);
		return out;
	case 38:
		return "Create new program segment";
	case 39:
		return "Random block read using FCB";
	case 41:
		return "Parse filename for FCB";
	case 42:
		return "Get date";
	case 43:
		return "Set date";
	case 44:
		return "Get time";
	case 45:
		return "Set time";
	case 46:
		return "Set / reset verify switch";
	case 47:
		return "Get disk transfer address";
	case 48:
		return "Get DOS version number";
	case 49:
		return "Terminate process and remain resident";
	case 50:
		return "Get pointer to drive parameter table(undocumented)";
	case 51:
		return "Get / set Ctrl - Break check state & get boot drive";
	case 52:
		return "Get address to DOS critical flag(undocumented)";
	case 53:
		return "Get vector";
	case 54:
		return "Get disk free space";
	case 55:
		out = "Get/set switch character -> ";
		switch (*ptr_AL)
		{
		case 0:
			out += "get switch character into DL";
			break;
		case 1:
			out += "set switch character to ";
			switch (*ptr_DL)
			{
			case 0:
				out += "\\DEV\\ is neccesary";
				break;
			case 1:
				out += "\\DEV\\ is NOT neccesary";
				break;
			}
			break;
		case 2:
			out += "read device prefix flag into DL";
			break;
		case 3:
			out += "set device names must begin with \\DEV\\";
			break;
		}
		return out;
	case 56:
		return "Get / set country dependent information";
	case 57:
		return "Create subdirectory(mkdir)";
	case 58:
		return "Remove subdirectory(rmdir)";
	case 59:
		return "Change current subdirectory(chdir)";
	case 60:
		return "Create file using handle";
	case 61:
		out = "Open file using handle ";
		switch (*ptr_AL)
		{
		case 0:
			out += "(read only) ";
			break;
		case 1:
			out += "(write only) ";
			break;
		case 2:
			out += "(read/write) ";
			break;
		}
		out += "name: ";
		for (int i = 0; i < 12; ++i)
		{
			if (memory_2[(DS_data * 16 + DX + i) & 0xFFFFF]) out += memory_2[(DS_data * 16 + DX + i) & 0xFFFFF];
		}
		return out;
	case 62:
		return "Close file using handle";
	case 63:
		return "Read file or device using handle";
	case 64:
		out = "Write file or device using handle (" + int_to_hex(BX,4) + ") " + to_string(CX) + " bytes\n";
		out += "Output buffer: ";
		for (int i = 0; i < CX; ++i)
		{
			if (memory_2[(DS_data * 16 + DX + i) & 0xFFFFF]) out += memory_2[(DS_data * 16 + DX + i) & 0xFFFFF];
			monitor.teletype(memory_2[(DS_data * 16 + DX + i) & 0xFFFFF]);
		}
		return out;
	case 65:
		return "Delete file";
	case 66:
		return "Move file pointer using handle";
	case 67:
		return "Change file mode";
	case 68:
		out = "I/O CTRL: ";
		switch (*ptr_AL)
		{
		case 0:
			out = out + "Get Device Information #" + int_to_hex(BX, 4);
			break;
		case 1:
			out = out + "Set Device Information #" + int_to_hex(BX, 4);
			break;
		case 2:
			out = out + "Read From Character Device #" + int_to_hex(BX, 4);
			break;
		case 3:
			out = out + "Write to Character Device #" + int_to_hex(BX, 4);
			break;
		case 4:
			out = out + "Read From Block Device #" + int_to_hex(BX, 4);
			break;
		case 5:
			out = out + "Write to Block Device #" + int_to_hex(BX, 4);
			break;
		case 6:
			out = out + "Get Input Status #" + int_to_hex(BX, 4);
			break;
		case 7:
			out = out + "Get Output Status #" + int_to_hex(BX, 4);
			break;
		case 8:
			out = out + "Device Removable Query #" + int_to_hex(BX, 4) + " drive #" + int_to_hex((BX & 255), 2);
			break;
		case 9:
			out = out + "Device Local or Remote Query #" + int_to_hex(BX, 4);
			break;
		case 10:
			out = out + "Handle Local or Remote Query #" + int_to_hex(BX, 4);
			break;
		case 11:
			out = out + "Set Sharing Retry Count #" + int_to_hex(BX, 4);
			break;
		case 12:
			out = out + "Generic I/O for Handles #" + int_to_hex(BX, 4);
			break;
		case 13:
			out = out + "Generic I/O for Block Devices (3.2+) #" + int_to_hex(BX, 4);
			break;
		case 14:
			out = out + "Get Logical Drive (3.2+) #" + int_to_hex(BX, 4);
			break;
		case 15:
			out = out + "Set Logical Drive (3.2+) #" + int_to_hex(BX, 4);
			break;
		}
		
		return out;
	case 69:
		return "Duplicate file handle";
	case 70:
		return "Force duplicate file handle";
	case 71:
		return "Get current directory";
	case 72:
		return "Allocate memory blocks";
	case 73:
		return "Free allocated memory blocks";
	case 74:
		return "Modify allocated memory blocks";
	case 75:
		switch (*ptr_AL)
		{
		case 0:
			out = "EXEC load and execute program ";
			for (int i = 0; i < 12; ++i)
			{
				if (memory_2[(DS_data * 16 + DX + i) & 0xFFFFF]) out += memory_2[(DS_data * 16 + DX + i) & 0xFFFFF];
			}
			return out;
		case 1:
			return "EXEC load but dont execute (UNDOC) ";
		case 3:
			return "EXEC load program only ";
		case 4:
			return "EXEC called by MSC spawn() ";
		}
		return "EXEC load and execute program (unknown subfunction)";
	case 76:
		return "Terminate process with return code";
	case 77:
		return "Get return code of a sub - process";
	case 78:
		return "Find first matching file";
	case 79:
		return "Find next matching file";
	case 80:
		return "Set current process id(undocumented)";
	case 81:
		return "Get current process id(undocumented)";
	case 82:
		return "Get pointer to DOS INVARS (undocumented)";
	case 83:
		return "Generate drive parameter table(undocumented)";
	case 84:
		return "Get verify setting";
	case 85:
		return "Create PSP(undocumented)";
	case 86:
		return "Rename file";
	case 87:
		return "Get / set file date and time using handle";
	case 88:
		return "Get / set memory allocation strategy(3.x + , undocumented)";
	case 89:
		return "Get extended error information(3.x + )";
	case 90:
		return "Create temporary file(3.x + )";
	case 91:
		return "Create new file(3.x + )";
	case 92:
		return "Lock / unlock file access(3.x + )";
	case 93:
		return "Critical error information(undocumented 3.x + )";
	case 94:
		return "Network services(3.1 + )";
	case 95:
		return "Network redirection(3.1 + )";
	case 96:
		return "Get fully qualified file name(undocumented 3.x + )";
	case 97:
		return "Get address of program segment prefix(3.x + )";
	case 98:
		return "Get system lead byte table(MSDOS 2.25 only)";
	case 99:
		return "Set device driver look ahead(undocumented 3.3 + )";
	case 100:
		return "Get extended country information(3.3 + )";
	case 101:
		return "Get / set global code page(3.3 + )";
	case 102:
		return "Set handle count(3.3 + )";
	case 103:
		return "Flush buffer(3.3 + )";
	case 104:
		return "Get / set disk serial number(undocumented DOS 4.0 + )";
	case 105:
		return "DOS reserved(DOS 4.0 + )";
	case 106:
		return "DOS reserved";
	case 107:
		return "Extended open / create(4.x + )";
	case 108:
		return "Set OEM INT 21 handler(functions F9 - FF) (undocumented)";
	}
	return "Unknown function";
}

//============== Data Transfer Group ===========

void mov_R_to_RM_8() //Move 8 bit R->R/M
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if (log_to_console) cout << "MOV " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ")  to ";

	//определяем получателя
	if ((byte2 >> 6) == 3)
	{
		// mod 11 получатель - регистр
		additional_IPs = 0; //обязательно обнулить
		*ptr_r8[byte2 & 7] = *ptr_r8[(byte2 >> 3) & 7];
		if (log_to_console) cout << reg8_name[byte2 & 7];
	}
	else
	{
		//получатель - память
		mod_RM_3(byte2);
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = *ptr_r8[(byte2 >> 3) & 7];
		if (log_to_console) cout << "M" << OPCODE_comment;
	}

	//if (log_to_console) cout << " ADDR = (" << (int)operand_RM_seg << " * 16 + " << (int)operand_RM_offset << ") & 0xFFFFF = " << (int)((operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF) << "  SS("<<*SS<<") DS(" << *DS << ")";
	Instruction_Pointer += 2 + additional_IPs;
}
void mov_R_to_RM_16() //Move 16 bit R->R/M
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if (log_to_console) cout << "MOV " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ")  to ";
	
	//определяем получателя
	if((byte2 >> 6) == 3)
	{
		// mod 11 получатель - регистр
		additional_IPs = 0; //обязательно обнулить
		*ptr_r16[byte2 & 7] = *ptr_r16[(byte2 >> 3) & 7];
		if (log_to_console) cout << reg16_name[(byte2 >> 3) & 7];
	}
	else
	{
		//получатель - память
		mod_RM_3(byte2);
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = *ptr_r16[(byte2 >> 3) & 7] & 255;
		operand_RM_offset++;
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = *ptr_r16[(byte2 >> 3) & 7] >> 8;
		if (log_to_console) cout << "M" << OPCODE_comment;
	}

	Instruction_Pointer += 2 + additional_IPs;
}
void mov_RM_to_R_8() //Move 8 bit R/M->R
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
		
	//определяем источник
	if((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		additional_IPs = 0;
		*ptr_r8[(byte2 >> 3) & 7] = *ptr_r8[byte2 & 7];
		if (log_to_console) cout << "MOV " << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") to " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ")";
	}
	else
	{
		//источник - память
		mod_RM_3(byte2);
		*ptr_r8[(byte2 >> 3) & 7] = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		if (log_to_console) cout << "MOV M" << OPCODE_comment << " to " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ")";
	}

	Instruction_Pointer += 2 + additional_IPs;
}
void mov_RM_to_R_16() //Move 16 bit R/M->R
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	//определяем источник
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		additional_IPs = 0; //обязательно обнулять
		*ptr_r16[(byte2 >> 3) & 7] = *ptr_r16[byte2 & 7];
		if (log_to_console) cout << "MOV " << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") to " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ")";
	}
	else
	{
		//источник - память
		mod_RM_3(byte2);
		*ptr_r16[(byte2 >> 3) & 7] = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		if (log_to_console) cout << "M1 = " << (int)memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		if (log_to_console) cout << " M2 = " << (int)memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		*ptr_r16[(byte2 >> 3) & 7] = *ptr_r16[(byte2 >> 3) & 7]  + memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] * 256;
		if (log_to_console) cout << " MOV M" << OPCODE_comment << " to " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ")";
	}

	Instruction_Pointer += 2 + additional_IPs;
}
void IMM_8_to_RM()		//IMM_8 to R/M
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
																			//определяем получателя
	if ((byte2 >> 6) == 3)
	{
		// mod 11 получатель - регистр
		additional_IPs = 0; //обязательно устанавливать
		if (log_to_console) cout << "MOV IMM(" << (int)memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF] << ") to " << reg8_name[byte2 & 7];
		*ptr_r8[byte2 & 7] = memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF];
	}
	else
	{
		mod_RM_3(byte2);
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = memory_2[(Instruction_Pointer + 2 + additional_IPs  + *CS * 16) & 0xFFFFF];
		if (log_to_console) cout << "IMM (" << (int)memory_2[(Instruction_Pointer + 2 + additional_IPs + *CS * 16) & 0xFFFFF] << ") to M" << OPCODE_comment;
	}

	Instruction_Pointer += 3 + additional_IPs;
}
void IMM_16_to_RM()	//IMM_16 to R/M
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	
	//определяем получателя
	if((byte2 >> 6) == 3)
	{
		// mod 11 получатель - регистр
		additional_IPs = 0; //ставить обязательно
		*ptr_r16[byte2 & 7] = memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF] + memory_2[(Instruction_Pointer + 3 + *CS * 16) & 0xFFFFF] * 256;
		if (log_to_console) cout << "MOV IMM(" << (int)(memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF] + memory_2[(Instruction_Pointer + 3 + *CS * 16) & 0xFFFFF] * 256) << ") to " << reg16_name[byte2 & 7];
	}
	else
	{
		//получатель память
		mod_RM_3(byte2);
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = memory_2[(Instruction_Pointer + 2 + additional_IPs + *CS * 16) & 0xFFFFF];
		operand_RM_offset++;
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = memory_2[(Instruction_Pointer + 3 + additional_IPs + *CS * 16) & 0xFFFFF];
		if (log_to_console) cout << "IMM (" << (int)(memory_2[(Instruction_Pointer + 2 + additional_IPs + *CS * 16) & 0xFFFFF] + memory_2[(Instruction_Pointer + 3 + additional_IPs + *CS * 16) & 0xFFFFF] * 256) << ") to M" << OPCODE_comment;
	}
	Instruction_Pointer += 4 + additional_IPs;
}

void IMM_8_to_R()		//IMM_8 to Register
{
	*ptr_r8[memory_2[(Instruction_Pointer + *CS * 16) & 0xFFFFF] & 7] = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];
	if (log_to_console) cout << "IMM(" << (int)memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] << ") to " << reg8_name[memory_2[(Instruction_Pointer + *CS * 16) & 0xFFFFF] & 7];
	Instruction_Pointer += 2;
}
void IMM_16_to_R()		//IMM_16 to Register
{
	*ptr_r16[memory_2[(Instruction_Pointer + *CS * 16) & 0xFFFFF] & 7] = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF] * 256;
	if (log_to_console) cout << "IMM(" << (int)(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory.read_2(Instruction_Pointer + 2 + *CS * 16) * 256) << ") to " << reg16_name[memory_2[(Instruction_Pointer + *CS * 16) & 0xFFFFF] & 7];
	Instruction_Pointer += 3;
}

void M_8_to_ACC()		//Memory to Accumulator 8
{
	New_Addr = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF] * 256;
	*ptr_AL = memory_2[(New_Addr + *DS * 16) & 0xFFFFF];
	Instruction_Pointer += 3;
	if (log_to_console) cout << "M[" << (int)*DS  << ":" << (int)New_Addr << "] to AL(" << (int)(AX & 255) << ")";
}
void M_16_to_ACC()		//Memory to Accumulator 16
{
	New_Addr = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF] * 256;
	*ptr_AL = memory_2[(New_Addr + *DS * 16) & 0xFFFFF];
	*ptr_AH = memory_2[(New_Addr + 1 + *DS * 16) & 0xFFFFF];
	Instruction_Pointer += 3;
	if (log_to_console) cout << "M[" << (int)*DS << ":" << (int)New_Addr << "] to AX(" << (int)(AX) << ")";
}
void ACC_8_to_M()		//Accumulator to Memory 8
{
	New_Addr = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF] * 256;
	memory_2[(New_Addr + *DS * 16) & 0xFFFFF] = *ptr_AL;
	Instruction_Pointer += 3;
	if (log_to_console) cout << "AL(" << (int)(AX & 255) << ") to M[" << (int)*DS << ":" << (int)New_Addr << "]";
}
void ACC_16_to_M()		//Accumulator to Memory 16
{
	New_Addr = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF] * 256;
	memory_2[(New_Addr + *DS * 16) & 0xFFFFF] = *ptr_AL;
	memory_2[(New_Addr + 1 + *DS * 16) & 0xFFFFF] = *ptr_AH;
	Instruction_Pointer += 3;
	if (log_to_console) cout << "AX(" << (int)AX << ") to M[" << (int)*DS << ":" << (int)New_Addr << "]";
}

void RM_to_Segment_Reg()	//Register/Memory to Segment Register
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	//определяем источник
	if((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		additional_IPs = 0;
		*ptr_segreg[(byte2 >> 3) & 3] = *ptr_r16[byte2 & 7];
		if (log_to_console) cout << "MOV " << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") to " << segreg_name[(byte2 >> 3) & 3];
	}
	else
	{
		mod_RM_3(byte2);
		*ptr_segreg[(byte2 >> 3) & 3] = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		*ptr_segreg[(byte2 >> 3) & 3] = *ptr_segreg[(byte2 >> 3) & 3] + memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] * 256;
		if (log_to_console) cout << "M" << OPCODE_comment << " to " << segreg_name[(byte2 >> 3) & 3] << "(" << (int)*ptr_segreg[(byte2 >> 3) & 3] << ")";
	}

	Instruction_Pointer += 2 + additional_IPs;
}
void Segment_Reg_to_RM()	//Segment Register to Register/Memory
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	//выбираем приёмник данных
	if((byte2 >> 6) == 3)
	{
		// mod 11 приемник - регистр
		additional_IPs = 0;
		*ptr_r16[byte2 & 7] = *ptr_segreg[(byte2 >> 3) & 3];
		if (log_to_console) cout << "MOV " << segreg_name[(byte2 >> 3) & 3] << "(" << *ptr_segreg[(byte2 >> 3) & 3] << ") to " << reg16_name[byte2 & 7];
	}
	else
	{
		mod_RM_3(byte2);
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = *ptr_segreg[(byte2 >> 3) & 3] & 255;
		operand_RM_offset++;
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = *ptr_segreg[(byte2 >> 3) & 3] >> 8;
		if (log_to_console) cout << "MOV M" << OPCODE_comment << " to " << segreg_name[(byte2 >> 3) & 3] << "(" << *ptr_segreg[(byte2 >> 3) & 3] << ")";
	}
	
	Instruction_Pointer += 2 + additional_IPs;
}

void Push_R()		//PUSH Register
{
	uint8 reg = memory_2[(Instruction_Pointer + *CS * 16) & 0xFFFFF] & 7;
	if (log_to_console) cout << "PUSH " << reg16_name[reg] << "(" << (int)*ptr_r16[reg] << ")";
		
	//пушим число
	Stack_Pointer--;
	memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFf] = *ptr_r16[reg] >> 8;
	Stack_Pointer--;
	memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] = *ptr_r16[reg] & 255;
	Instruction_Pointer++;
}
void Push_SegReg()	//PUSH Segment Register
{
	uint8 reg = (memory_2[(Instruction_Pointer + *CS * 16) & 0xFFFFF] >> 3) & 3;
	if (log_to_console) cout << "PUSH " << segreg_name[reg] <<  "(" << (int)*ptr_segreg[reg] << ")";

	//пушим число
	Stack_Pointer--;
	memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] = *ptr_segreg[reg] >> 8;
	Stack_Pointer--;
	memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] = *ptr_segreg[reg] & 255;
	Instruction_Pointer++;
}

void Pop_RM()			//POP Register/Memory
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];//второй байт
	uint8 OP = (byte2 >> 3) & 7;
	uint16 Src = 0;

	if (OP == 0)
	{
		//делаем POP
		Src = memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF];
		Stack_Pointer++;
		Src += memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] * 256;
		Stack_Pointer++;

		if ((byte2 >> 6) == 3)
		{
			// mod 11 - регистр
			additional_IPs = 0;
			*ptr_r16[byte2 & 7] = Src;
			if (log_to_console) cout << "POP " << reg16_name[byte2 & 7] << "(" << (int)Src << ")";
		}
		else
		{
			// грузим в память
			mod_RM_3(byte2);
			memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = Src & 255;
			operand_RM_offset++;
			memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = Src >> 8;
			if (log_to_console) cout << "POP M" << OPCODE_comment;
		}
		Instruction_Pointer += 2 + additional_IPs;
	}
	else
	{
		cout << "Error: POP RM with REG!=0. IP= " << (int)Instruction_Pointer;
		log_to_console = 1;
		step_mode = 1;
		Instruction_Pointer += 2 + additional_IPs;
	}
}
void Pop_R()			//POP Register
{
	uint8 reg = memory_2[(Instruction_Pointer + *CS * 16) & 0xFFFFF] & 7;
	uint16 Src = 0;

	//извлекаем число
	Src = memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF];
	Stack_Pointer++;
	Src += memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] * 256;
	Stack_Pointer ++;

	*ptr_r16[reg] = Src;
	if (log_to_console) cout << "POP " << reg16_name[reg] <<"(" << (int)Src << ")";

	Instruction_Pointer++;
}
void Pop_SegReg()		//POP Segment Register
{
	uint8 reg = (memory_2[(Instruction_Pointer + *CS * 16) & 0xFFFFF] >> 3) & 3;
	uint16 Src = 0;

	//извлекаем число
	Src = memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF];
	Stack_Pointer++;
	Src += memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] * 256;
	Stack_Pointer++;
	
	*ptr_segreg[reg] = Src;
	if (log_to_console) cout << "POP " << segreg_name[reg] << "(" << (int)Src << ")";
		
	Instruction_Pointer++;
}

void XCHG_8()			//Exchange Register/Memory with Register 8bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 Src = 0;
	uint8 Dest = 0;
	additional_IPs = 0;

	//выбираем источник данных в регистре
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Src = AX & 255;
		if (log_to_console) cout << "Exchange AL(" << (int)Src << ") with";
		break;
	case 1:
		Src = CX & 255;
		if (log_to_console) cout << "Exchange CL(" << (int)Src << ") with";
		break;
	case 2:
		Src = DX & 255;
		if (log_to_console) cout << "Exchange DL(" << (int)Src << ") with";
		break;
	case 3:
		Src = BX & 255;
		if (log_to_console) cout << "Exchange BL(" << (int)Src << ") with";
		break;
	case 4:
		Src = AX >> 8;
		if (log_to_console) cout << "Exchange AH(" << (int)Src << ") with";
		break;
	case 5:
		Src = CX >> 8;
		if (log_to_console) cout << "Exchange CH(" << (int)Src << ") with";
		break;
	case 6:
		Src = DX >> 8;
		if (log_to_console) cout << "Exchange DH(" << (int)Src << ") with";
		break;
	case 7:
		Src = BX >> 8;
		if (log_to_console) cout << "Exchange BH(" << (int)Src << ") with";
	}

	//заменяем данные в другом регистре/памяти
	//интерпретация MOD
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Dest = memory.read_2(New_Addr);
		memory.write_2(New_Addr,  Src);
		if (log_to_console) cout << " M" << OPCODE_comment << "";
		break;
	case 3:
		// mod 11 получатель - регистр
		switch (byte2 & 7)
		{
		case 0:
			Dest = AX & 255;
			AX = (AX & 0xFF00) | Src;
			if (log_to_console) cout << " AL(" << (int)Dest << ")";
			break;
		case 1:
			Dest = CX & 255;
			CX = (CX & 0xFF00) | Src;
			if (log_to_console) cout << " CL(" << (int)Dest << ")";
			break;
		case 2:
			Dest = DX & 255;
			DX = (DX & 0xFF00) | Src;
			if (log_to_console) cout << " DL(" << (int)Dest << ")";
			break;
		case 3:
			Dest = BX & 255;
			BX = (BX & 0xFF00) | Src;
			if (log_to_console) cout << " BL(" << (int)Dest << ")";
			break;
		case 4:
			Dest = AX >> 8;
			AX = (AX & 0x00FF) | (Src * 256);
			if (log_to_console) cout << " AH(" << (int)Dest << ")";
			break;
		case 5:
			Dest = CX >> 8;
			CX = (CX & 0x00FF) | (Src * 256);
			if (log_to_console) cout << " CH(" << (int)Dest << ")";
			break;
		case 6:
			Dest = DX >> 8;
			DX = (DX & 0x00FF) | (Src * 256);
			if (log_to_console) cout << " DH(" << (int)Dest << ")";
			break;
		case 7:
			Dest = BX >> 8;
			BX = (BX & 0x00FF) | (Src * 256);
			if (log_to_console) cout << " BH(" << (int)Dest << ")";
		}
	}

	//заменяем исходный источник данных
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		AX = (AX & 0xFF00) | (Dest);
		break;
	case 1:
		CX = (CX & 0xFF00) | (Dest);
		break;
	case 2:
		DX = (DX & 0xFF00) | (Dest);
		break;
	case 3:
		BX = (BX & 0xFF00) | (Dest);
		break;
	case 4:
		AX = (AX & 0x00FF) | (Dest * 256);
		break;
	case 5:
		CX = (CX & 0x00FF) | (Dest * 256);
		break;
	case 6:
		DX = (DX & 0x00FF) | (Dest * 256);
		break;
	case 7:
		BX = (BX & 0x00FF) | (Dest * 256);
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void XCHG_16()			//Exchange Register/Memory with Register 16bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Src = 0;
	uint16 Dest = 0;
	additional_IPs = 0;

	//выбираем источник данных в регистре
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Src = AX;
		if (log_to_console) cout << "Exchange AX(" << (int)Src << ") with";
		break;
	case 1:
		Src = CX;
		if (log_to_console) cout << "Exchange CX(" << (int)Src << ") with";
		break;
	case 2:
		Src = DX;
		if (log_to_console) cout << "Exchange DX(" << (int)Src << ") with";
		break;
	case 3:
		Src = BX;
		if (log_to_console) cout << "Exchange BX(" << (int)Src << ") with";
		break;
	case 4:
		Src = Stack_Pointer;
		if (log_to_console) cout << "Exchange SP(" << (int)Src << ") with";
		break;
	case 5:
		Src = Base_Pointer;
		if (log_to_console) cout << "Exchange BP(" << (int)Src << ") with";
		break;
	case 6:
		Src = Source_Index;
		if (log_to_console) cout << "Exchange SI(" << (int)Src << ") with";
		break;
	case 7:
		Src = Destination_Index;
		if (log_to_console) cout << "Exchange DI(" << (int)Src << ") with";
	}

	//заменяем данные в другом регистре/памяти
	//интерпретация MOD
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Dest = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
		memory.write_2(New_Addr,  Src & 255);
		memory.write_2(New_Addr + 1,  Src >> 8);
		if (log_to_console) cout << " M" << OPCODE_comment << "";
		break;
	case 3:
		// mod 11 получатель - регистр
		switch (byte2 & 7)
		{
		case 0:
			Dest = AX;
			AX = Src;
			if (log_to_console) cout << " AX(" << (int)Dest << ")";
			break;
		case 1:
			Dest = CX;
			CX = Src;
			if (log_to_console) cout << " CX(" << (int)Dest << ")";
			break;
		case 2:
			Dest = DX;
			DX = Src;
			if (log_to_console) cout << " DX(" << (int)Dest << ")";
			break;
		case 3:
			Dest = BX;
			BX = Src;
			if (log_to_console) cout << " BX(" << (int)Dest << ")";
			break;
		case 4:
			Dest = Stack_Pointer;
			Stack_Pointer = Src;
			if (log_to_console) cout << " SP(" << (int)Dest << ")";
			break;
		case 5:
			Dest = Base_Pointer;
			Base_Pointer = Src;
			if (log_to_console) cout << " BP(" << (int)Dest << ")";
			break;
		case 6:
			Dest = Source_Index;
			Source_Index = Src;
			if (log_to_console) cout << " SI(" << (int)Dest << ")";
			break;
		case 7:
			Dest = Destination_Index;
			Destination_Index = Src;
			if (log_to_console) cout << " DI(" << (int)Dest << ")";
		}
	}

	//заменяем исходный источник данных
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		AX = Dest;
		break;
	case 1:
		CX = Dest;
		break;
	case 2:
		DX = Dest;
		break;
	case 3:
		BX = Dest;
		break;
	case 4:
		Stack_Pointer = Dest;
		break;
	case 5:
		Base_Pointer = Dest;
		break;
	case 6:
		Source_Index = Dest;
		break;
	case 7:
		Destination_Index = Dest;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void XCHG_ACC_R()		//Exchange Register with Accumulator
{
	uint8 reg = memory.read_2(Instruction_Pointer + *CS * 16) & 7; //reg
	uint16 Src = 0;
	uint16 Dest = 0;

	Src = AX;
	switch (reg)
	{
	case 0:
		// AX - AX не нужен
		if (log_to_console) cout << "Exchange AX(" << (int)Src << ") with AX";
		break;
	case 1:
		AX = CX;
		CX = Src;
		if (log_to_console) cout << "Exchange AX(" << (int)Src << ") with CX(" << (int)AX << ")";
		break;
	case 2:
		AX = DX;
		DX = Src;
		if (log_to_console) cout << "Exchange AX(" << (int)Src << ") with DX(" << (int)AX << ")";
		break;
	case 3:
		AX = BX;
		BX = Src;
		if (log_to_console) cout << "Exchange AX(" << (int)Src << ") with BX(" << (int)AX << ")";
		break;
	case 4:
		AX = Stack_Pointer;
		Stack_Pointer = Src;
		if (log_to_console) cout << "Exchange AX(" << (int)Src << ") with SP(" << (int)AX << ")";
		break;
	case 5:
		AX = Base_Pointer;
		Base_Pointer = Src;
		if (log_to_console) cout << "Exchange AX(" << (int)Src << ") with BP(" << (int)AX << ")";
		break;
	case 6:
		AX = Source_Index;
		Source_Index = Src;
		if (log_to_console) cout << "Exchange AX(" << (int)Src << ") with SI(" << (int)AX << ")";
		break;
	case 7:
		AX = Destination_Index;
		Destination_Index = Src;
		if (log_to_console) cout << "Exchange AX(" << (int)Src << ") with DI(" << (int)AX << ")";
	}
	Instruction_Pointer ++;
}

void In_8_to_ACC_from_port()	 //Input 8 to AL/AX AX from fixed PORT
{
	AX = (AX & 0xFF00) | IO_device.input_from_port_8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]); //пишем в AL байт из порта
	SetConsoleTextAttribute(hConsole, 10);
	if (log_to_console) cout << "read (" << (int)(AX & 255) << ") from port " << (int)memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];
	SetConsoleTextAttribute(hConsole, 7);
	Instruction_Pointer += 2;
}
void In_16_to_ACC_from_port()    //Input 16 to AL/AX AX from fixed PORT
{
	AX = IO_device.input_from_port_16(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]); //пишем в AX 2 байта из порта
	SetConsoleTextAttribute(hConsole, 10);
	if (log_to_console) cout << "read (" << (int)AX << ") from port " << (int)memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];
	SetConsoleTextAttribute(hConsole, 7);
	Instruction_Pointer += 2;
}
void Out_8_from_ACC_to_port()    //Output 8 from AL/AX AX from fixed PORT
{
	IO_device.output_to_port_8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF], AX & 255);//выводим в порт байт AL
	SetConsoleTextAttribute(hConsole, 10);
	if (log_to_console) cout << "write AL(" << (int)(AX & 255) << ") to port " << (int)memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];
	SetConsoleTextAttribute(hConsole, 7);
	Instruction_Pointer += 2;
}
void Out_16_from_ACC_to_port()   //Output 16 from AL/AX AX from fixed PORT
{
	IO_device.output_to_port_16(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF], AX);//выводим в порт 2 байта AX
	SetConsoleTextAttribute(hConsole, 10);
	if (log_to_console) cout << "write AX(" << (int)(AX) << ") to port " << (int)memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];
	SetConsoleTextAttribute(hConsole, 7);
	Instruction_Pointer += 2;
}
void In_8_from_DX()		//Input 8 from variable PORT
{
	AX = (AX & 0xFF00) | IO_device.input_from_port_8(DX); //пишем в AL байт из порта DX
	SetConsoleTextAttribute(hConsole, 10);
	if (log_to_console) cout << "read (" << (int)(AX & 255) << ") from port " << (int)DX;
	SetConsoleTextAttribute(hConsole, 7);
	Instruction_Pointer += 1;
}
void In_16_from_DX()	//Input 16 from variable PORT
{
	AX = IO_device.input_from_port_16(DX); //пишем в AX байт из порта DX
	SetConsoleTextAttribute(hConsole, 10);
	if (log_to_console) cout << "read (" << (int)AX << ") from port " << (int)DX;
	SetConsoleTextAttribute(hConsole, 7);
	Instruction_Pointer += 1;
}
void Out_8_to_DX()		//Output 8 to variable PORT
{
	IO_device.output_to_port_8(DX, AX & 255);//выводим в порт DX байт AL
	SetConsoleTextAttribute(hConsole, 10);
	if (log_to_console) cout << "write AL(" << (int)(AX & 255) << ") to port " << (int)DX;
	SetConsoleTextAttribute(hConsole, 7);
	Instruction_Pointer += 1;
}
void Out_16_to_DX()		//Output 16 to variable PORT
{
	IO_device.output_to_port_16(DX, AX);//выводим в порт DX байт AL
	SetConsoleTextAttribute(hConsole, 10);
	if (log_to_console) cout << "write AX(" << (int)(AX) << ") to port " << (int)DX;
	SetConsoleTextAttribute(hConsole, 7);
	Instruction_Pointer += 1;
}

void XLAT()			//Translate Byte to AL
{
	if (log_to_console) cout << "XLAT AL(" << (int)(AX & 255) << ") = ";
	uint16 Addr = BX + (AX & 255);
	AX = (AX & 0xFF00) | memory.read_2(Addr + *DS * 16);
	if (log_to_console) cout << (int)(AX & 255);
	Instruction_Pointer++;
}
void LEA()			//Load EA to Register
{
	uint8 byte2 = memory.read_2(uint16(Instruction_Pointer + 1) + *CS * 16); //mod / reg / rm
	uint16 Src = 0;
	additional_IPs = 0;
	uint16 EA = 0;

	//определяем смещение
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		EA = mod_RM_Old(byte2);
		if (log_to_console) cout << "LEA (" << (int)EA << ")" ;
		break;
	case 3:
		EA = 0; //заглушка
		cout << "LEA ERROR";
		break;
	}

	//выбираем приёмник данных
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		AX = EA;
		if (log_to_console) cout << " to AX";
		break;
	case 1:
		CX = EA;
		if (log_to_console) cout << " to CX";
		break;
	case 2:
		DX = EA;
		if (log_to_console) cout << " to DX";
		break;
	case 3:
		BX = EA;
		if (log_to_console) cout << " to BX";
		break;
	case 4:
		Stack_Pointer = EA;
		if (log_to_console) cout << " to SP";
		break;
	case 5:
		Base_Pointer = EA;
		if (log_to_console) cout << " to BP";
		break;
	case 6:
		Source_Index = EA;
		if (log_to_console) cout << " to SI";
		break;
	case 7:
		Destination_Index = EA;
		if (log_to_console) cout << " to DI";
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void LDS()			//Load Pointer to DS
{
	uint8 byte2 = memory.read_2(uint16(Instruction_Pointer + 1) + *CS * 16); //mod / reg / rm
	uint16 Src = 0;
	uint16 Src_2 = 0;
	additional_IPs = 0;
#ifdef DEBUG
	if (log_to_console) cout << "LDS ";
#endif

	//определяем источник
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
		Src_2 = memory.read_2(New_Addr + 2) + memory.read_2(New_Addr + 3) * 256;
		if (log_to_console) cout << "(" << (int)Src << ") to ";
		break;
	case 3:
		// mod 11 источник - регистр
		// неприменимо
		cout << "ERROR LDS";
		step_mode = 1;
		break;
	}

	//выбираем приёмник данных
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		AX = Src;
		if (log_to_console) cout << "AX(" << (int)Src << ")";
		break;
	case 1:
		CX = Src;
		if (log_to_console) cout << "CX(" << (int)Src << ")";
		break;
	case 2:
		DX = Src;
		if (log_to_console) cout << "DX(" << (int)Src << ")";
		break;
	case 3:
		BX = Src;
		if (log_to_console) cout << "BX(" << (int)Src << ")";
		break;
	case 4:
		Stack_Pointer = Src;
		if (log_to_console) cout << "SP(" << (int)Src << ")";
		break;
	case 5:
		Base_Pointer = Src;
		if (log_to_console) cout << "BP(" << (int)Src << ")";
		break;
	case 6:
		Source_Index = Src;
		if (log_to_console) cout << "SI(" << (int)Src << ")";
		break;
	case 7:
		Destination_Index = Src;
		if (log_to_console) cout << "DI(" << (int)Src << ")";
		break;
	}

	DS_data = Src_2;
	if (log_to_console) cout << " + DS(" << (int)DS_data << ")";

	Instruction_Pointer += 2 + additional_IPs;
}
void LES()			//Load Pointer to ES
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Src = 0;
	uint16 Src_2 = 0;
	additional_IPs = 0;
#ifdef DEBUG
	if (log_to_console) cout << "LES ";
#endif

	//определяем источник
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
		Src_2 = memory.read_2(New_Addr + 2) + memory.read_2(New_Addr + 3) * 256;
		if (log_to_console) cout << "M(" << (int)Src << ") to ";
		break;
	case 3:
		// mod 11 источник - регистр
		// неприменимо
		cout << "ERROR LDS";
		break;
	}

	//выбираем приёмник данных
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		AX = Src;
		if (log_to_console) cout << "AX(" << (int)Src << ")";
		break;
	case 1:
		CX = Src;
		if (log_to_console) cout << "CX(" << (int)Src << ")";
		break;
	case 2:
		DX = Src;
		if (log_to_console) cout << "DX(" << (int)Src << ")";
		break;
	case 3:
		BX = Src;
		if (log_to_console) cout << "BX(" << (int)Src << ")";
		break;
	case 4:
		Stack_Pointer = Src;
		if (log_to_console) cout << "SP(" << (int)Src << ")";
		break;
	case 5:
		Base_Pointer = Src;
		if (log_to_console) cout << "BP(" << (int)Src << ")";
		break;
	case 6:
		Source_Index = Src;
		if (log_to_console) cout << "SI(" << (int)Src << ")";
		break;
	case 7:
		Destination_Index = Src;
		if (log_to_console) cout << "DI(" << (int)Src << ")";
		break;
	}

	ES_data = Src_2;
	if (log_to_console) cout << " + ES(" << (int)ES_data << ")";

	Instruction_Pointer += 2 + additional_IPs;
}

void LAHF()			// Load AH with Flags
{
	AX = (AX & 255) | (Flag_SF << 15) | (Flag_ZF << 14) | (Flag_AF << 12) | (Flag_PF << 10) | (Flag_CF << 8) | (2 << 8);
	if (log_to_console) cout << "Load AH with Flags";
	Instruction_Pointer++;
}
void SAHF()			// Store AH with Flags
{
	Flag_SF = (AX >> 15) & 1;
	Flag_ZF = (AX >> 14) & 1;
	Flag_AF = (AX >> 12) & 1;
	Flag_PF = (AX >> 10) & 1;
	Flag_CF = (AX >> 8) & 1;
	if (log_to_console) cout << "Store AH to Flags";
	Instruction_Pointer++;
}
void PUSHF()		// Push Flags
{
	Stack_Pointer--;
	memory.write_2(SS_data * 16 + Stack_Pointer, 0xF0 | (Flag_OF * 8) | (Flag_DF * 4) | (Flag_IF * 2) | Flag_TF);
	Stack_Pointer--;
	memory.write_2(SS_data * 16 + Stack_Pointer, 0x2 | (Flag_SF * 128) | (Flag_ZF * 64) | (Flag_AF * 16) | (Flag_PF * 4) | (Flag_CF));
	if (log_to_console) cout << "Push Flags";
	Instruction_Pointer++;
}
void POPF()			// Pop Flags
{
	uint32 stack_addr = SS_data * 16 + Stack_Pointer;
	int Flags = memory.read_2(SS_data * 16 + Stack_Pointer);
	Stack_Pointer++;
	Flags += memory.read_2(SS_data * 16 + Stack_Pointer) * 256;
	Stack_Pointer++;

	Flag_OF = (Flags >> 11) & 1;
	Flag_DF = (Flags >> 10) & 1;
	Flag_IF = (Flags >> 9) & 1;
	Flag_TF = (Flags >> 8) & 1;
	Flag_SF = (Flags >> 7) & 1;
	Flag_ZF = (Flags >> 6) & 1;
	Flag_AF = (Flags >> 4) & 1;
	Flag_PF = (Flags >> 2) & 1;
	Flag_CF = (Flags) & 1;

	if (log_to_console) cout << "Pop Flags";
	Instruction_Pointer++;
}

//============Arithmetic===================================

//ADD

void ADD_R_to_RM_8()		// INC R/M -> R/M 8bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 Src = 0;
	bool OF_Carry = false;
	uint16 Result = 0;
	additional_IPs = 0;

	if (log_to_console)
	{
		switch ((byte2 >> 3) & 7)
		{
		case 0:
			//Src = AX & 255;
			if (log_to_console) cout << "ADD AL(" << (int)(Src) << ") ";
			break;
		case 1:
			//Src = CX & 255;
			if (log_to_console) cout << "ADD CL(" << (int)(Src) << ") ";
			break;
		case 2:
			//Src = DX & 255;
			if (log_to_console) cout << "ADD DL(" << (int)(Src) << ") ";
			break;
		case 3:
			//Src = BX & 255;
			if (log_to_console) cout << "ADD BL(" << (int)(Src) << ") ";
			break;
		case 4:
			//Src = AX >> 8;
			if (log_to_console) cout << "ADD AH(" << (int)(Src) << ") ";
			break;
		case 5:
			//Src = CX >> 8;
			if (log_to_console) cout << "ADD CH(" << (int)(Src) << ") ";
			break;
		case 6:
			//Src = DX >> 8;
			if (log_to_console) cout << "ADD DH(" << (int)(Src) << ") ";
			break;
		case 7:
			//Src = BX >> 8;
			if (log_to_console) cout << "ADD BH(" << (int)(Src) << ") ";
			break;
		}
	}

	//определяем объект назначения и результат операции ADD
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		
		if (log_to_console)
		{
			switch (byte2 & 7)
			{
			case 0:
				if (log_to_console) cout << "+ AL(" << (int)(AX & 255) << ") = ";
				break;
			case 1:
				if (log_to_console) cout << "+ CL(" << (int)(CX & 255) << ") = ";
				break;
			case 2:
				if (log_to_console) cout << "+ DL(" << (int)(DX & 255) << ") = ";
				break;
			case 3:
				if (log_to_console) cout << "+ BL(" << (int)(BX & 255) << ") = ";
				break;
			case 4:
				if (log_to_console) cout << "+ AH(" << (int)(AX >> 8) << ") = ";
				break;
			case 5:
				if (log_to_console) cout << "+ CH(" << (int)(CX >> 8) << ") = ";
				break;
			case 6:
				if (log_to_console) cout << "+ DH(" << (int)(DX >> 8) << ") = ";
				break;
			case 7:
				if (log_to_console) cout << "+ BH(" << (int)(BX >> 8) << ") = ";
				break;
			}
		}
		
		//складываем два регистра
		Result = *ptr_r8[(byte2 >> 3) & 7] + *ptr_r8[byte2 & 7];
		if (log_to_console) cout << (int)(Result & 255);
		Flag_AF = (((*ptr_r8[(byte2 >> 3) & 7] & 15) + (*ptr_r8[byte2 & 7] & 15)) >> 4) & 1;
		OF_Carry = ((*ptr_r8[(byte2 >> 3) & 7] & 0x7F) + (*ptr_r8[byte2 & 7] & 0x7F)) >> 7;
		*ptr_r8[byte2 & 7] = Result;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];

	}
	else
	{
		mod_RM_3(byte2);
		New_Addr = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
		Result = memory_2[New_Addr] + *ptr_r8[(byte2 >> 3) & 7];
		Flag_AF = (((memory_2[New_Addr] & 15) + (*ptr_r8[(byte2 >> 3) & 7] & 15)) >> 4) & 1;
		Flag_CF = Result >> 8;
		OF_Carry = ((memory_2[New_Addr] & 0x7F) + (*ptr_r8[(byte2 >> 3) & 7] & 0x7F)) >> 7;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		
		memory_2[New_Addr] = Result;
		if (log_to_console) cout << " Add M" << OPCODE_comment << " = " << (int)(Result & 255);
	}
	
	Instruction_Pointer += 2 + additional_IPs;
}
void ADD_R_to_RM_16()		// INC R/M -> R/M 16bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Src = 0;
	bool OF_Carry = false;
	uint32 Result = 0;
	additional_IPs = 0;

	//определяем 1-й операнд
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Src = AX;
		if (log_to_console) cout << "AX("<<(int)AX<<") ";
		break;
	case 1:
		Src = CX;
		if (log_to_console) cout << "CX(" << (int)CX << ") ";
		break;
	case 2:
		Src = DX;
		if (log_to_console) cout << "DX(" << (int)DX << ") ";
		break;
	case 3:
		Src = BX;
		if (log_to_console) cout << "BX(" << (int)BX << ") ";
		break;
	case 4:
		Src = Stack_Pointer;
		if (log_to_console) cout << "SP(" << (int)Stack_Pointer << ") ";
		break;
	case 5:
		Src = Base_Pointer;
		if (log_to_console) cout << "BP(" << (int)Base_Pointer << ") ";
		break;
	case 6:
		Src = Source_Index;
		if (log_to_console) cout << "SI(" << (int)Source_Index << ") ";
		break;
	case 7:
		Src = Destination_Index;
		if (log_to_console) cout << "DI(" << (int)Destination_Index << ") ";
		break;
	}

	//определяем объект назначения и результат операции ADD
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Result = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256 + Src;
		Flag_AF = (((memory.read_2(New_Addr) & 15) + (Src & 15)) >> 4) & 1;
		OF_Carry = (((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & 0x7FFF) + (Src & 0x7FFF)) >> 15;
		memory.write_2(New_Addr, Result & 255);
		memory.write_2(New_Addr + 1, (Result >> 8) & 255);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " Add M" << OPCODE_comment << " = " << (int)(Result & 0xFFFF);
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Result = AX + Src;
			if (log_to_console) cout << "ADD AX(" << (int)AX << ") = " << (int)(Result & 0xFFFF);
			Flag_AF = (((AX & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = ((AX & 0x7FFF) + (Src & 0x7FFF)) >> 15;
			AX = Result & 0xFFFF;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 1:
			Result = CX + Src;
			if (log_to_console) cout << "ADD CX(" << (int)CX << ") = " << (int)(Result & 0xFFFF);
			Flag_AF = (((CX & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = ((CX & 0x7FFF) + (Src & 0x7FFF)) >> 15;
			CX = Result & 0xFFFF;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 2:
			Result = DX + Src;
			if (log_to_console) cout << "ADD DX(" << (int)DX << ") = " << (int)(Result & 0xFFFF);
			Flag_AF = (((DX & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = ((DX & 0x7FFF) + (Src & 0x7FFF)) >> 15;
			DX = Result & 0xFFFF;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 3:
			Result = BX + Src;
			if (log_to_console) cout << "ADD BX(" << (int)BX << ") = " << (int)(Result & 0xFFFF);
			Flag_AF = (((BX & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = ((BX & 0x7FFF) + (Src & 0x7FFF)) >> 15;
			BX = Result & 0xFFFF;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 4:
			Result = Stack_Pointer + Src;
			if (log_to_console) cout << "ADD SP(" << (int)Stack_Pointer << ") = " << (int)(Result & 0xFFFF);
			Flag_AF = (((Stack_Pointer & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = ((Stack_Pointer & 0x7FFF) + (Src & 0x7FFF)) >> 15;
			Stack_Pointer = Result & 0xFFFF;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 5:
			Result = Base_Pointer + Src;
			if (log_to_console) cout << "ADD BP(" << (int)Base_Pointer << ") = " << (int)(Result & 0xFFFF);
			Flag_AF = (((Base_Pointer & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = ((Base_Pointer & 0x7FFF) + (Src & 0x7FFF)) >> 15;
			Base_Pointer = Result & 0xFFFF;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 6:
			Result = Source_Index + Src;
			if (log_to_console) cout << "ADD SI(" << (int)Source_Index << ") = " << (int)(Result & 0xFFFF);
			Flag_AF = (((Source_Index & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = ((Source_Index & 0x7FFF) + (Src & 0x7FFF)) >> 15;
			Source_Index = Result & 0xFFFF;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 7:
			Result = Destination_Index + Src;
			if (log_to_console) cout << "ADD DI(" << (int)Destination_Index << ") = " << (int)(Result & 0xFFFF);
			Flag_AF = (((Destination_Index & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = ((Destination_Index & 0x7FFF) + (Src & 0x7FFF)) >> 15;
			Destination_Index = Result & 0xFFFF;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		}
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void ADD_RM_to_R_8()		// INC R/M -> R 8bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 Src = 0;
	uint16 Result = 0;
	additional_IPs = 0;
	bool OF_Carry = false;
	
	if (log_to_console) cout << "ADD ";
	//определяем 1-й операнд
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Src = memory.read_2(New_Addr);
		if (log_to_console) cout << "M" << OPCODE_comment << " + ";
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Src = AX & 255;
			if (log_to_console) cout << "AL(" << (int)Src << ") + ";
			break;
		case 1:
			Src = CX & 255;
			if (log_to_console) cout << "CL(" << (int)Src << ") + ";
			break;
		case 2:
			Src = DX & 255;
			if (log_to_console) cout << "DL(" << (int)Src << ") + ";
			break;
		case 3:
			Src = BX & 255;
			if (log_to_console) cout << "BL(" << (int)Src << ") + ";
			break;
		case 4:
			Src = AX >> 8;
			if (log_to_console) cout << "AH(" << (int)Src << ") + ";
			break;
		case 5:
			Src = CX >> 8;
			if (log_to_console) cout << "CH(" << (int)Src << ") + ";
			break;
		case 6:
			Src = DX >> 8;
			if (log_to_console) cout << "DH(" << (int)Src << ") + ";
			break;
		case 7:
			Src = BX >> 8;
			if (log_to_console) cout << "BH(" << (int)Src << ") + ";
			break;
		}
		break;
	}

	//определяем объект назначения и результат операции ADD
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Result = Src + (AX & 255);
		if (log_to_console) cout << "AL("<<(int)(AX & 255)<<") = " << (int)(Result & 255);
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = ((AX & 0x7F) + (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((AX & 15) + (Src & 15)) >> 4) & 1;
		AX = (AX & 0xFF00) | (Result & 255);
		break;
	case 1:
		Result = Src + (CX & 255);
		if (log_to_console) cout << "CL(" << (int)(CX & 255) << ") = " << (int)(Result & 255);
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = ((CX & 0x7F) + (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((CX & 15) + (Src & 15)) >> 4) & 1;
		CX = (CX & 0xFF00) | (Result & 255);
		break;
		break;
	case 2:
		Result = Src + (DX & 255);
		if (log_to_console) cout << "DL(" << (int)(DX & 255) << ") = " << (int)(Result & 255);
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = ((DX & 0x7F) + (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((DX & 15) + (Src & 15)) >> 4) & 1;
		DX = (DX & 0xFF00) | (Result & 255);
		break;
	case 3:
		Result = Src + (BX & 255);
		if (log_to_console) cout << "BL(" << (int)(BX & 255) << ") = " << (int)(Result & 255);
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = ((BX & 0x7F) + (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((BX & 15) + (Src & 15)) >> 4) & 1;
		BX = (BX & 0xFF00) | (Result & 255);
		break;
	case 4:
		Result = Src + ((AX >> 8) & 255);
		if (log_to_console) cout << "AH(" << (int)((AX >> 8) & 255) << ") = " << (int)(Result & 255);
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = (((AX >> 8) & 0x7F) + (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((((AX >> 8) & 15) + (Src & 15)) >> 4) & 1;
		AX = (AX & 0x00FF) | (Result << 8);
		break;
	case 5:
		Result = Src + (CX >> 8);
		if (log_to_console) cout << "CH(" << (int)((CX >> 8) & 255) << ") = " << (int)(Result & 255);
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = (((CX >> 8) & 0x7F) + (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((((CX >> 8) & 15) + (Src & 15)) >> 4) & 1;
		CX = (CX & 0x00FF) | (Result << 8);
		break;
	case 6:
		Result = Src + (DX >> 8);
		if (log_to_console) cout << "DH(" << (int)((DX >> 8) & 255) << ") = " << (int)(Result & 255);
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = (((DX >> 8) & 0x7F) + (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((((DX >> 8) & 15) + (Src & 15)) >> 4) & 1;
		DX = (DX & 0x00FF) | (Result << 8);
		break;
	case 7:
		Result = Src + (BX >> 8);
		if (log_to_console) cout << "BH(" << (int)((BX >> 8) & 255) << ") = " << (int)(Result & 255);
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = (((BX >> 8) & 0x7F) + (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((((BX >> 8) & 15) + (Src & 15)) >> 4) & 1;
		BX = (BX & 0x00FF) | (Result << 8);
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void ADD_RM_to_R_16()		// INC R/M -> R 16bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Src = 0;
	uint32 Result = 0;
	additional_IPs = 0;
	bool OF_Carry = false;

	if (log_to_console) cout << "ADD ";
	//определяем 1-й операнд
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
		if (log_to_console) cout << "M" << OPCODE_comment << " + ";
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Src = AX;
			if (log_to_console) cout << "AX + ";
			break;
		case 1:
			Src = CX;
			if (log_to_console) cout << "CX + ";
			break;
		case 2:
			Src = DX;
			if (log_to_console) cout << "DX + ";
			break;
		case 3:
			Src = BX;
			if (log_to_console) cout << "BX + ";
			break;
		case 4:
			Src = Stack_Pointer;
			if (log_to_console) cout << "SP + ";
			break;
		case 5:
			Src = Base_Pointer;
			if (log_to_console) cout << "BP + ";
			break;
		case 6:
			Src = Source_Index;
			if (log_to_console) cout << "SI + ";
			break;
		case 7:
			Src = Destination_Index;
			if (log_to_console) cout << "DI + ";
			break;
		}
		break;
	}

	//определяем объект назначения и результат операции ADD
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Result = Src + AX;
		if (log_to_console) cout << "AX(" << (int)AX << ") = " << (int)(Result & 0xFFFF);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = ((AX & 0x7FFF) + (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((AX & 15) + (Src & 15)) >> 4) & 1;
		AX = (Result & 0xFFFF);
		break;
	case 1:
		Result = Src + CX;
		if (log_to_console) cout << "CX(" << (int)CX << ") = " << (int)(Result & 0xFFFF);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = ((CX & 0x7FFF) + (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((CX & 15) + (Src & 15)) >> 4) & 1;
		CX = (Result & 0xFFFF);
		break;
	case 2:
		Result = Src + DX;
		if (log_to_console) cout << "DX(" << (int)DX << ") = " << (int)(Result & 0xFFFF);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = ((DX & 0x7FFF) + (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((DX & 15) + (Src & 15)) >> 4) & 1;
		DX = (Result & 0xFFFF);
		break;
	case 3:
		Result = Src + BX;
		if (log_to_console) cout << "BX(" << (int)BX << ") = " << (int)(Result & 0xFFFF);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = ((BX & 0x7FFF) + (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((BX & 15) + (Src & 15)) >> 4) & 1;
		BX = (Result & 0xFFFF);
		break;
	case 4:
		Result = Src + Stack_Pointer;
		if (log_to_console) cout << "SP(" << (int)Stack_Pointer << ") = " << (int)(Result & 0xFFFF);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = ((Stack_Pointer & 0x7FFF) + (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((Stack_Pointer & 15) + (Src & 15)) >> 4) & 1;
		Stack_Pointer = (Result & 0xFFFF);
		break;
	case 5:
		Result = Src + Base_Pointer;
		if (log_to_console) cout << "BP(" << (int)Base_Pointer << ") = " << (int)(Result & 0xFFFF);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = ((Base_Pointer & 0x7FFF) + (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((Base_Pointer & 15) + (Src & 15)) >> 4) & 1;
		Base_Pointer = (Result & 0xFFFF);
		break;
	case 6:
		Result = Src + Source_Index;
		if (log_to_console) cout << "SI(" << (int)Source_Index << ") = " << (int)(Result & 0xFFFF);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = ((Source_Index & 0x7FFF) + (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((Source_Index & 15) + (Src & 15)) >> 4) & 1;
		Source_Index = (Result & 0xFFFF);
		break;
	case 7:
		Result = Src + Destination_Index;
		if (log_to_console) cout << "DI(" << (int)Destination_Index << ") = " << (int)(Result & 0xFFFF);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = ((Destination_Index & 0x7FFF) + (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((Destination_Index & 15) + (Src & 15)) >> 4) & 1;
		Destination_Index = (Result & 0xFFFF);
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void ADD_IMM_RM_16s()		// ADD/ADC IMM -> R/M 16 bit sign ext.
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 OP = (byte2 >> 3) & 7;
	uint16 Src = 0;
	uint32 Result_32 = 0;
	uint16 Result = 0;
	additional_IPs = 0;
	bool OF_Carry = false;

	switch (OP)
	{
	case 0: //  ADD  mod 000 r/m
			//определяем объект назначения и результат операции
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "ADD IMMs(" << (int)Src << ") + ";
			Result_32 = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) + Src;
			Flag_AF = (((memory.read_2(New_Addr) & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = (((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & 0x7FFF) + (Src & 0x7FFF)) >> 15;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			memory.write_2(New_Addr, Result_32 & 255);
			memory.write_2(New_Addr + 1, (Result_32 >> 8) & 255);
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0b1111111100000000; //продолжаем знак на старший байт
			if (log_to_console) cout << "ADD IMMs(" << (int)Src << ") + ";
			switch (byte2 & 7)
			{
			case 0:
				Result_32 = AX + Src;
				Flag_AF = (((AX & 15) + (Src & 15)) >> 4) & 1;
				OF_Carry = ((AX & 0x7FFF) + (Src & 0x7FFF)) >> 15;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				AX = Result_32 & 0xFFFF;
				if (log_to_console) cout << " AX = " << (int)(Result_32 & 0xFFFF);
				break;
			case 1:
				Result_32 = CX + Src;
				Flag_AF = (((CX & 15) + (Src & 15)) >> 4) & 1;
				OF_Carry = ((CX & 0x7FFF) + (Src & 0x7FFF)) >> 15;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				CX = Result_32 & 0xFFFF;
				if (log_to_console) cout << " CX = " << (int)(Result_32 & 0xFFFF);
				break;
			case 2:
				Result_32 = DX + Src;
				Flag_AF = (((DX & 15) + (Src & 15)) >> 4) & 1;
				OF_Carry = ((DX & 0x7FFF) + (Src & 0x7FFF)) >> 15;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				DX = Result_32 & 0xFFFF;
				if (log_to_console) cout << " DX = " << (int)(Result_32 & 0xFFFF);
				break;
			case 3:
				Result_32 = BX + Src;
				Flag_AF = (((BX & 15) + (Src & 15)) >> 4) & 1;
				OF_Carry = ((BX & 0x7FFF) + (Src & 0x7FFF)) >> 15;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				BX = Result_32 & 0xFFFF;
				if (log_to_console) cout << " BX = " << (int)(Result_32 & 0xFFFF);
				break;
			case 4:
				Result_32 = Stack_Pointer + Src;
				Flag_AF = (((Stack_Pointer & 15) + (Src & 15)) >> 4) & 1;
				OF_Carry = ((Stack_Pointer & 0x7FFF) + (Src & 0x7FFF)) >> 15;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				Stack_Pointer = Result_32 & 0xFFFF;
				if (log_to_console) cout << " SP = " << (int)(Result_32 & 0xFFFF);
				break;
			case 5:
				Result_32 = Base_Pointer + Src;
				Flag_AF = (((Base_Pointer & 15) + (Src & 15)) >> 4) & 1;
				OF_Carry = ((Base_Pointer & 0x7FFF) + (Src & 0x7FFF)) >> 15;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				Base_Pointer = Result_32 & 0xFFFF;
				if (log_to_console) cout << " BP = " << (int)(Result_32 & 0xFFFF);
				break;
			case 6:
				Result_32 = Source_Index + Src;
				Flag_AF = (((Source_Index & 15) + (Src & 15)) >> 4) & 1;
				OF_Carry = ((Source_Index & 0x7FFF) + (Src & 0x7FFF)) >> 15;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				Source_Index = Result_32 & 0xFFFF;
				if (log_to_console) cout << " SI = " << (int)(Result_32 & 0xFFFF);
				break;
			case 7:
				Result_32 = Destination_Index + Src;
				Flag_AF = (((Destination_Index & 15) + (Src & 15)) >> 4) & 1;
				OF_Carry = ((Destination_Index & 0x7FFF) + (Src & 0x7FFF)) >> 15;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				Destination_Index = Result_32 & 0xFFFF;
				if (log_to_console) cout << " DI = " << (int)(Result_32 & 0xFFFF);
				break;
			}
			break;
		}
		Instruction_Pointer += 3 + additional_IPs;
		break;
	
	case 1:  //OR mod 001 r/m
		//определяем объект назначения и результат операции
	
		switch (byte2 >> 6)
		{
		case 0:
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)Src << ") OR ";
			Result = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) | Src;
			memory.write_2(New_Addr,  Result & 255);
			memory.write_2(New_Addr + 1,  Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)Result;
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)Src << ") OR ";
			Result = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) | Src;
			memory.write_2(New_Addr,  Result & 255);
			memory.write_2(New_Addr + 1,  Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)Result;
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)Src << ") OR ";
			Result = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) | Src;
			memory.write_2(New_Addr,  Result & 255);
			memory.write_2(New_Addr + 1,  Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)Result;
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)Src << ") OR ";
			switch (byte2 & 7)
			{
			case 0:
				AX = AX | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((AX >> 15) & 1);
				if (AX) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[AX & 255];
				if (log_to_console) cout << " AX = " << (int)AX;
				break;
			case 1:
				CX = CX | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((CX >> 15) & 1);
				if (CX) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[CX & 255];
				if (log_to_console) cout << " CX = " << (int)CX;
				break;
			case 2:
				DX = DX | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((DX >> 15) & 1);
				if (DX) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[DX & 255];
				if (log_to_console) cout << " DX = " << (int)DX;
				break;
			case 3:
				BX = BX | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((BX >> 15) & 1);
				if (BX) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[BX & 255];
				if (log_to_console) cout << " BX = " << (int)BX;
				break;
			case 4:
				Stack_Pointer = Stack_Pointer | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Stack_Pointer >> 15) & 1);
				if (Stack_Pointer) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Stack_Pointer & 255];
				if (log_to_console) cout << " SP = " << (int)Stack_Pointer;
				break;
			case 5:
				Base_Pointer = Base_Pointer | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Base_Pointer >> 15) & 1);
				if (Base_Pointer) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Base_Pointer & 255];
				if (log_to_console) cout << " BP = " << (int)Base_Pointer;
				break;
			case 6:
				Source_Index = Source_Index | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Source_Index >> 15) & 1);
				if (Source_Index) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Source_Index & 255];
				if (log_to_console) cout << " SI = " << (int)Source_Index;
				break;
			case 7:
				Destination_Index = Destination_Index | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Destination_Index >> 15) & 1);
				if (Destination_Index) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Destination_Index & 255];
				if (log_to_console) cout << " DI = " << (int)Destination_Index;
				break;
			}
			break;
		}
		Instruction_Pointer += 3 + additional_IPs;
		break;

	case 2:	//ADC  mod 010 r/m
		//определяем объект назначения и результат операции
		switch (byte2 >> 6)
		{
		case 0:
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "ADC IMMs(" << (int)Src << ") + CF(" << Flag_CF << ") + ";
			Result_32 = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) + Src + Flag_CF;
			Flag_AF = (((memory.read_2(New_Addr) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = (((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & 0x7FFF) + (Src & 0x7FFF)) >> 15;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			memory.write_2(New_Addr,  Result_32 & 255);
			memory.write_2(New_Addr + 1,  (Result_32 >> 8) & 255);
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "ADC IMMs(" << (int)Src << ") + CF(" << Flag_CF << ") + ";
			Result_32 = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) + Src + Flag_CF;
			Flag_AF = (((memory.read_2(New_Addr) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = (((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & 0x7FFF) + (Src & 0x7FFF)) >> 15;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			memory.write_2(New_Addr,  Result_32 & 255);
			memory.write_2(New_Addr + 1,  (Result_32 >> 8) & 255);
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "ADC IMMs(" << (int)Src << ") + CF(" << Flag_CF << ") + ";
			Result_32 = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) + Src + Flag_CF;
			Flag_AF = (((memory.read_2(New_Addr) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = (((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & 0x7FFF) + (Src & 0x7FFF)) >> 15;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			memory.write_2(New_Addr,  Result_32 & 255);
			memory.write_2(New_Addr + 1,  (Result_32 >> 8) & 255);
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0b1111111100000000; //продолжаем знак на старший байт
			if (log_to_console) cout << "ADC IMMs(" << (int)Src << ") + CF(" << Flag_CF << ") + ";
			switch (byte2 & 7)
			{
			case 0:
				Result_32 = AX + Src + Flag_CF;
				Flag_AF = (((AX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
				OF_Carry = ((AX & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				AX = Result_32 & 0xFFFF;
				if (log_to_console) cout << " AX = " << (int)(Result_32 & 0xFFFF);
				break;
			case 1:
				Result_32 = CX + Src + Flag_CF;
				Flag_AF = (((CX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
				OF_Carry = ((CX & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				CX = Result_32 & 0xFFFF;
				if (log_to_console) cout << " CX = " << (int)(Result_32 & 0xFFFF);
				break;
			case 2:
				Result_32 = DX + Src + Flag_CF;
				Flag_AF = (((DX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
				OF_Carry = ((DX & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				DX = Result_32 & 0xFFFF;
				if (log_to_console) cout << " DX = " << (int)(Result_32 & 0xFFFF);
				break;
			case 3:
				Result_32 = BX + Src + Flag_CF;
				Flag_AF = (((BX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
				OF_Carry = ((BX & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				BX = Result_32 & 0xFFFF;
				if (log_to_console) cout << " BX = " << (int)(Result_32 & 0xFFFF);
				break;
			case 4:
				Result_32 = Stack_Pointer + Src + Flag_CF;
				Flag_AF = (((Stack_Pointer & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
				OF_Carry = ((Stack_Pointer & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				Stack_Pointer = Result_32 & 0xFFFF;
				if (log_to_console) cout << " SP = " << (int)(Result_32 & 0xFFFF);
				break;
			case 5:
				Result_32 = Base_Pointer + Src + Flag_CF;
				Flag_AF = (((Base_Pointer & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
				OF_Carry = ((Base_Pointer & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				Base_Pointer = Result_32 & 0xFFFF;
				if (log_to_console) cout << " BP = " << (int)(Result_32 & 0xFFFF);
				break;
			case 6:
				Result_32 = Source_Index + Src + Flag_CF;
				Flag_AF = (((Source_Index & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
				OF_Carry = ((Source_Index & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				Source_Index = Result_32 & 0xFFFF;
				if (log_to_console) cout << " SI = " << (int)(Result_32 & 0xFFFF);
				break;
			case 7:
				Result_32 = Destination_Index + Src + Flag_CF;
				Flag_AF = (((Destination_Index & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
				OF_Carry = ((Destination_Index & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				Destination_Index = Result_32 & 0xFFFF;
				if (log_to_console) cout << " DI = " << (int)(Result_32 & 0xFFFF);
				break;
			}
			break;
		}
		Instruction_Pointer += 3 + additional_IPs;
		break;

	case 3:	//   SBB  mod 011 r/m

		switch (byte2 >> 6)
		{
		case 0:
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0b1111111100000000; //продолжаем знак на старший байт
			if (log_to_console) cout << "SBB IMMs(" << (int)Src << ") + ";
			Result_32 = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) - Src - Flag_CF;
			Flag_AF = (((memory.read_2(New_Addr) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
			OF_Carry = (((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			memory.write_2(New_Addr,  Result_32 & 255);
			memory.write_2(New_Addr + 1,  (Result_32 >> 8) & 255);
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0b1111111100000000; //продолжаем знак на старший байт
			if (log_to_console) cout << "SBB IMMs(" << (int)Src << ") + ";
			Result_32 = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) - Src - Flag_CF;
			Flag_AF = (((memory.read_2(New_Addr) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
			OF_Carry = (((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			memory.write_2(New_Addr,  Result_32 & 255);
			memory.write_2(New_Addr + 1,  (Result_32 >> 8) & 255);
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0b1111111100000000; //продолжаем знак на старший байт
			if (log_to_console) cout << "SBB IMMs(" << (int)Src << ") + ";
			Result_32 = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) - Src - Flag_CF;
			Flag_AF = (((memory.read_2(New_Addr) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
			OF_Carry = (((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			memory.write_2(New_Addr,  Result_32 & 255);
			memory.write_2(New_Addr + 1,  (Result_32 >> 8) & 255);
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0b1111111100000000; //продолжаем знак на старший байт
			if (log_to_console) cout << "SBB IMMs(" << (int)Src << ") + ";
			switch (byte2 & 7)
			{
			case 0:
				Result_32 = AX - Src - Flag_CF;
				Flag_AF = (((AX & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
				OF_Carry = ((AX & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;				
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				AX = Result_32 & 0xFFFF;
				if (log_to_console) cout << " AX = " << (int)(Result_32 & 0xFFFF);
				break;
			case 1:
				Result_32 = CX - Src - Flag_CF;
				OF_Carry = ((CX & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
				Flag_AF = (((CX & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				CX = Result_32 & 0xFFFF;
				if (log_to_console) cout << " CX = " << (int)(Result_32 & 0xFFFF);
				break;
			case 2:
				Result_32 = DX - Src - Flag_CF;
				OF_Carry = ((DX & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
				Flag_AF = (((DX & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				DX = Result_32 & 0xFFFF;
				if (log_to_console) cout << " DX = " << (int)(Result_32 & 0xFFFF);
				break;
			case 3:
				Result_32 = BX - Src - Flag_CF;
				OF_Carry = ((BX & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
				Flag_AF = (((BX & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				BX = Result_32 & 0xFFFF;
				if (log_to_console) cout << " BX = " << (int)(Result_32 & 0xFFFF);
				break;
			case 4:
				Result_32 = Stack_Pointer - Src - Flag_CF;
				OF_Carry = ((Stack_Pointer & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
				Flag_AF = (((Stack_Pointer & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				Stack_Pointer = Result_32 & 0xFFFF;
				if (log_to_console) cout << " SP = " << (int)(Result_32 & 0xFFFF);
				break;
			case 5:
				Result_32 = Base_Pointer - Src - Flag_CF;
				OF_Carry = ((Base_Pointer & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
				Flag_AF = (((Base_Pointer & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				Base_Pointer = Result_32 & 0xFFFF;
				if (log_to_console) cout << " BP = " << (int)(Result_32 & 0xFFFF);
				break;
			case 6:
				Result_32 = Source_Index - Src - Flag_CF;
				OF_Carry = ((Source_Index & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
				Flag_AF = (((Source_Index & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				Source_Index = Result_32 & 0xFFFF;
				if (log_to_console) cout << " SI = " << (int)(Result_32 & 0xFFFF);
				break;
			case 7:
				Result_32 = Destination_Index - Src - Flag_CF;
				OF_Carry = ((Destination_Index & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
				Flag_AF = (((Destination_Index & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				Destination_Index = Result_32 & 0xFFFF;
				if (log_to_console) cout << " DI = " << (int)(Result_32 & 0xFFFF);
				break;
			}
			break;
		}
		Instruction_Pointer += 3 + additional_IPs;
		break;

	case 4:  //AND mod 001 r/m
		//определяем объект назначения и результат операции

		switch (byte2 >> 6)
		{
		case 0:
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)Src << ") AND ";
			Result = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & Src;
			memory.write_2(New_Addr,  Result & 255);
			memory.write_2(New_Addr + 1,  Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)Result;
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)Src << ") AND ";
			Result = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & Src;
			memory.write_2(New_Addr,  Result & 255);
			memory.write_2(New_Addr + 1,  Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)Result;
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)Src << ") AND ";
			Result = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & Src;
			memory.write_2(New_Addr,  Result & 255);
			memory.write_2(New_Addr + 1,  Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)Result;
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)Src << ") AND ";
			switch (byte2 & 7)
			{
			case 0:
				AX = AX & Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((AX >> 15) & 1);
				if (AX) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[AX & 255];
				if (log_to_console) cout << " AX = " << (int)AX;
				break;
			case 1:
				CX = CX & Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((CX >> 15) & 1);
				if (CX) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[CX & 255];
				if (log_to_console) cout << " CX = " << (int)CX;
				break;
			case 2:
				DX = DX & Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((DX >> 15) & 1);
				if (DX) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[DX & 255];
				if (log_to_console) cout << " DX = " << (int)DX;
				break;
			case 3:
				BX = BX & Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((BX >> 15) & 1);
				if (BX) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[BX & 255];
				if (log_to_console) cout << " BX = " << (int)BX;
				break;
			case 4:
				Stack_Pointer = Stack_Pointer & Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Stack_Pointer >> 15) & 1);
				if (Stack_Pointer) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Stack_Pointer & 255];
				if (log_to_console) cout << " SP = " << (int)Stack_Pointer;
				break;
			case 5:
				Base_Pointer = Base_Pointer & Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Base_Pointer >> 15) & 1);
				if (Base_Pointer) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Base_Pointer & 255];
				if (log_to_console) cout << " BP = " << (int)Base_Pointer;
				break;
			case 6:
				Source_Index = Source_Index & Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Source_Index >> 15) & 1);
				if (Source_Index) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Source_Index & 255];
				if (log_to_console) cout << " SI = " << (int)Source_Index;
				break;
			case 7:
				Destination_Index = Destination_Index & Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Destination_Index >> 15) & 1);
				if (Destination_Index) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Destination_Index & 255];
				if (log_to_console) cout << " DI = " << (int)Destination_Index;
				break;
			}
			break;
		}
		Instruction_Pointer += 3 + additional_IPs;
		break;

	case 5:  //   SUB  mod 101 r/m

		switch (byte2 >> 6)
		{
		case 0:
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0b1111111100000000; //продолжаем знак на старший байт
			if (log_to_console) cout << "SUB IMMs(" << (int)Src << ") + ";
			Result_32 = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) - Src;
			Flag_AF = (((memory.read_2(New_Addr) & 15) - (Src & 15)) >> 4) & 1;
			OF_Carry = (((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			memory.write_2(New_Addr,  Result_32 & 255);
			memory.write_2(New_Addr + 1,  (Result_32 >> 8) & 255);
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0b1111111100000000; //продолжаем знак на старший байт
			if (log_to_console) cout << "SUB IMMs(" << (int)Src << ") + ";
			Result_32 = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) - Src;
			Flag_AF = (((memory.read_2(New_Addr) & 15) - (Src & 15)) >> 4) & 1;
			OF_Carry = (((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			memory.write_2(New_Addr,  Result_32 & 255);
			memory.write_2(New_Addr + 1,  (Result_32 >> 8) & 255);
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0b1111111100000000; //продолжаем знак на старший байт
			if (log_to_console) cout << "SUB IMMs(" << (int)Src << ") + ";
			Result_32 = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) - Src;
			Flag_AF = (((memory.read_2(New_Addr) & 15) - (Src & 15)) >> 4) & 1;
			OF_Carry = (((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			memory.write_2(New_Addr,  Result_32 & 255);
			memory.write_2(New_Addr + 1,  (Result_32 >> 8) & 255);
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0b1111111100000000; //продолжаем знак на старший байт
			if (log_to_console) cout << "SUB IMMs(" << (int)Src << ") + ";
			switch (byte2 & 7)
			{
			case 0:
				Result_32 = AX - Src;
				Flag_AF = (((AX & 15) - (Src & 15)) >> 4) & 1;
				OF_Carry = (((AX) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				AX = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << " AX = " << (int)(Result_32 & 0xFFFF);
				break;
			case 1:
				Result_32 = CX - Src;
				Flag_AF = (((CX & 15) - (Src & 15)) >> 4) & 1;
				OF_Carry = (((CX) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				CX = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << " CX = " << (int)(Result_32 & 0xFFFF);
				break;
			case 2:
				Result_32 = DX - Src;
				Flag_AF = (((DX & 15) - (Src & 15)) >> 4) & 1;
				OF_Carry = (((DX) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				DX = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << " DX = " << (int)(Result_32 & 0xFFFF);
				break;
			case 3:
				Result_32 = BX - Src;
				Flag_AF = (((BX & 15) - (Src & 15)) >> 4) & 1;
				OF_Carry = (((BX) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				BX = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << " BX = " << (int)(Result_32 & 0xFFFF);
				break;
			case 4:
				Result_32 = Stack_Pointer - Src;
				Flag_AF = (((Stack_Pointer & 15) - (Src & 15)) >> 4) & 1;
				OF_Carry = (((Stack_Pointer) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Stack_Pointer = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << " SP = " << (int)(Result_32 & 0xFFFF);
				break;
			case 5:
				Result_32 = Base_Pointer - Src;
				Flag_AF = (((Base_Pointer & 15) - (Src & 15)) >> 4) & 1;
				OF_Carry = (((Base_Pointer) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Base_Pointer = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << " BP = " << (int)(Result_32 & 0xFFFF);
				break;
			case 6:
				Result_32 = Source_Index - Src;
				Flag_AF = (((Source_Index & 15) - (Src & 15)) >> 4) & 1;
				OF_Carry = (((Source_Index) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Source_Index = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << " SI = " << (int)(Result_32 & 0xFFFF);
				break;
			case 7:
				Result_32 = Destination_Index - Src;
				Flag_AF = (((Destination_Index & 15) - (Src & 15)) >> 4) & 1;
				OF_Carry = (((Destination_Index) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Destination_Index = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << " DI = " << (int)(Result_32 & 0xFFFF);
				break;
			}
			break;
		}
		Instruction_Pointer += 3 + additional_IPs;
		break;

	case 6:  //XOR mod 110 r/m
		//определяем объект назначения и результат операции

		switch (byte2 >> 6)
		{
		case 0:
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)Src << ") AND ";
			Result = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) ^ Src;
			memory.write_2(New_Addr,  Result & 255);
			memory.write_2(New_Addr + 1,  Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)Result;
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)Src << ") AND ";
			Result = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) ^ Src;
			memory.write_2(New_Addr,  Result & 255);
			memory.write_2(New_Addr + 1,  Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)Result;
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)Src << ") AND ";
			Result = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) ^ Src;
			memory.write_2(New_Addr,  Result & 255);
			memory.write_2(New_Addr + 1,  Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)Result;
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)Src << ") AND ";
			switch (byte2 & 7)
			{
			case 0:
				AX = AX ^ Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((AX >> 15) & 1);
				if (AX) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[AX & 255];
				if (log_to_console) cout << " AX = " << (int)AX;
				break;
			case 1:
				CX = CX ^ Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((CX >> 15) & 1);
				if (CX) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[CX & 255];
				if (log_to_console) cout << " CX = " << (int)CX;
				break;
			case 2:
				DX = DX ^ Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((DX >> 15) & 1);
				if (DX) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[DX & 255];
				if (log_to_console) cout << " DX = " << (int)DX;
				break;
			case 3:
				BX = BX ^ Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((BX >> 15) & 1);
				if (BX) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[BX & 255];
				if (log_to_console) cout << " BX = " << (int)BX;
				break;
			case 4:
				Stack_Pointer = Stack_Pointer ^ Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Stack_Pointer >> 15) & 1);
				if (Stack_Pointer) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Stack_Pointer & 255];
				if (log_to_console) cout << " SP = " << (int)Stack_Pointer;
				break;
			case 5:
				Base_Pointer = Base_Pointer ^ Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Base_Pointer >> 15) & 1);
				if (Base_Pointer) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Base_Pointer & 255];
				if (log_to_console) cout << " BP = " << (int)Base_Pointer;
				break;
			case 6:
				Source_Index = Source_Index ^ Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Source_Index >> 15) & 1);
				if (Source_Index) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Source_Index & 255];
				if (log_to_console) cout << " SI = " << (int)Source_Index;
				break;
			case 7:
				Destination_Index = Destination_Index ^ Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Destination_Index >> 15) & 1);
				if (Destination_Index) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Destination_Index & 255];
				if (log_to_console) cout << " DI = " << (int)Destination_Index;
				break;
			}
			break;
		}
		Instruction_Pointer += 3 + additional_IPs;
		break;

	case 7:  //   CMP mod 111 r/m

		switch (byte2 >> 6)
		{
		case 0:
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "CMP IMMs(" << (int)Src << ") -> ";
			Result_32 = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) - Src;
			OF_Carry = (((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_AF = (((memory.read_2(New_Addr) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			Flag_PF = parity_check[Result_32 & 255];
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "CMP IMMs(" << (int)Src << ") -> ";
			Result_32 = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) - Src;
			OF_Carry = (((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_AF = (((memory.read_2(New_Addr) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			Flag_PF = parity_check[Result_32 & 255];
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "CMP IMMs(" << (int)Src << ") -> ";
			Result_32 = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) - Src;
			OF_Carry = (((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_AF = (((memory.read_2(New_Addr) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			Flag_PF = parity_check[Result_32 & 255];
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "CMP IMMs(" << (__int16)Src << ") -> ";
			switch (byte2 & 7)
			{
			case 0:
				Result_32 = AX - Src;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				OF_Carry = ((AX & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Flag_OF = Flag_CF ^ OF_Carry;
				Flag_AF = (((AX & 0x0F) - (Src & 0x0F)) >> 4) & 1;
				Flag_PF = parity_check[Result_32 & 255];
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				if (log_to_console) cout << " AX("<<(int)AX<<") = " << (int)(Result_32 & 0xFFFF);
				break;
			case 1:
				Result_32 = CX - Src;
				//CX = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				OF_Carry = ((CX & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Flag_OF = Flag_CF ^ OF_Carry;
				Flag_AF = (((CX & 0x0F) - (Src & 0x0F)) >> 4) & 1;
				Flag_PF = parity_check[Result_32 & 255];
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				if (log_to_console) cout << " CX(" << (int)CX << ") = " << (int)(Result_32 & 0xFFFF);
				break;
			case 2:
				Result_32 = DX - Src;
				//DX = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				OF_Carry = ((DX & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Flag_OF = Flag_CF ^ OF_Carry;
				Flag_AF = (((DX & 0x0F) - (Src & 0x0F)) >> 4) & 1;
				Flag_PF = parity_check[Result_32 & 255];
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				if (log_to_console) cout << " DX(" << (int)DX << ") = " << (__int16)(Result_32 & 0xFFFF);
				break;
			case 3:
				Result_32 = BX - Src;
				//BX = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				OF_Carry = ((BX & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Flag_OF = Flag_CF ^ OF_Carry;
				Flag_AF = (((BX & 0x0F) - (Src & 0x0F)) >> 4) & 1;
				Flag_PF = parity_check[Result_32 & 255];
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				if (log_to_console) cout << " BX(" << (int)BX << ") = " << (int)(Result_32 & 0xFFFF);
				break;
			case 4:
				Result_32 = Stack_Pointer - Src;
				//Stack_Pointer = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				OF_Carry = ((Stack_Pointer & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Flag_OF = Flag_CF ^ OF_Carry;
				Flag_AF = (((Stack_Pointer & 0x0F) - (Src & 0x0F)) >> 4) & 1;
				Flag_PF = parity_check[Result_32 & 255];
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				if (log_to_console) cout << " SP(" << (int)Stack_Pointer << ") = " << (__int16)(Result_32 & 0xFFFF);
				break;
			case 5:
				Result_32 = Base_Pointer - Src;
				//Base_Pointer = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				OF_Carry = ((Base_Pointer & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Flag_OF = Flag_CF ^ OF_Carry;
				Flag_AF = (((Base_Pointer & 0x0F) - (Src & 0x0F)) >> 4) & 1;
				Flag_PF = parity_check[Result_32 & 255];
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				if (log_to_console) cout << " BP(" << (int)Base_Pointer << ") = " << (__int16)(Result_32 & 0xFFFF);
				break;
			case 6:
				Result_32 = Source_Index - Src;
				//Source_Index = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				OF_Carry = ((Source_Index & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Flag_OF = Flag_CF ^ OF_Carry;
				Flag_AF = (((Source_Index & 0x0F) - (Src & 0x0F)) >> 4) & 1;
				Flag_PF = parity_check[Result_32 & 255];
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				if (log_to_console) cout << " SI(" << (int)Source_Index << ") = " << (__int16)(Result_32 & 0xFFFF);
				break;
			case 7:
				Result_32 = Destination_Index - Src;
				//Destination_Index = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				OF_Carry = ((Destination_Index & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Flag_OF = Flag_CF ^ OF_Carry;
				Flag_AF = (((Destination_Index & 0x0F) - (Src & 0x0F)) >> 4) & 1;
				Flag_PF = parity_check[Result_32 & 255];
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				if (log_to_console) cout << " DI(" << (int)Destination_Index << ") = " << (__int16)(Result_32 & 0xFFFF);
				break;
			}
			break;
		}
		Instruction_Pointer += 3 + additional_IPs;
		break;
	}
}

void ADD_IMM_to_ACC_8()	// ADD IMM -> ACC 8bit
{
	uint16 Result = 0;
	bool OF_Carry = false;
	uint8 imm = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];
	if (log_to_console) cout << "ADD IMM (" << (int)imm << ") to AL(" << (int)(AX & 255) << ") = ";
	OF_Carry = ((imm & 0x7F) + (AX & 0x7F)) >> 7;
	Result = imm + (AX & 255);
	Flag_AF = (((AX & 15) + (imm & 15)) >> 4) & 1;
	Flag_CF = (Result >> 8) & 1;
	Flag_SF = (Result >> 7) & 1;
	Flag_OF = Flag_CF ^ OF_Carry;
	if (Result & 255) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result & 255];
	AX = (AX & 0xFF00) | (Result & 255);
	if (log_to_console) cout << (int)(Result);
	Instruction_Pointer += 2;
}
void ADD_IMM_to_ACC_16()	// ADD IMM -> ACC 16bit 
{
	uint32 Result = 0;
	bool OF_Carry = false;
	uint16 imm = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory.read_2(Instruction_Pointer + 2 + *CS * 16) * 256;
	if (log_to_console) cout << "ADD IMM (" << (int)imm << ") to AX(" << (int)(AX) << ") = ";
	
	Result = imm + AX;
	Flag_CF = (Result >> 16) & 1;
	Flag_SF = (Result >> 15) & 1;
	OF_Carry = ((imm & 0x7FFF) + (AX & 0x7FFF)) >> 15;
	Flag_OF = Flag_CF ^ OF_Carry;
	if (Result & 0xFFFF) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result & 255];
	Flag_AF = (((AX & 15) + (imm & 15)) >> 4) & 1;
	AX = Result & 0xFFFF;
	if (log_to_console) cout << (int)(AX);

	Instruction_Pointer += 3;
}

//ADC

void ADC_RM_to_RM_8()		// ADC R/M -> R/M 8bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 Src = 0;
	bool OF_Carry = false;
	uint16 Result = 0;
	additional_IPs = 0;

	if (log_to_console) cout << "ADC ";

	//определяем 1-й операнд
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Src = AX & 255;
		if (log_to_console) cout << "AL(" << (int)(Src) << ") + CF(" << (int)Flag_CF << ") ";
		break;
	case 1:
		Src = CX & 255;
		if (log_to_console) cout << "CL(" << (int)(Src) << ") + CF(" << (int)Flag_CF << ") ";
		break;
	case 2:
		Src = DX & 255;
		if (log_to_console) cout << "DL(" << (int)(Src) << ") + CF(" << (int)Flag_CF << ") ";
		break;
	case 3:
		Src = BX & 255;
		if (log_to_console) cout << "BL(" << (int)(Src) << ") + CF(" << (int)Flag_CF << ") ";
		break;
	case 4:
		Src = AX >> 8;
		if (log_to_console) cout << "AH(" << (int)(Src) << ") + CF(" << (int)Flag_CF << ") ";
		break;
	case 5:
		Src = CX >> 8;
		if (log_to_console) cout << "CH(" << (int)(Src) << ") + CF(" << (int)Flag_CF << ") ";
		break;
	case 6:
		Src = DX >> 8;
		if (log_to_console) cout << "DH(" << (int)(Src) << ") + CF(" << (int)Flag_CF << ") ";
		break;
	case 7:
		Src = BX >> 8;
		if (log_to_console) cout << "BH(" << (int)(Src) << ") + CF(" << (int)Flag_CF << ") ";
		break;
	}

	//определяем объект назначения и результат операции ADD
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Result = memory.read_2(New_Addr) + Src + Flag_CF;
		Flag_AF = (((memory.read_2(New_Addr) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
		OF_Carry = ((memory.read_2(New_Addr) & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
		Flag_CF = Result >> 8;
		Flag_SF = (Result >> 7) & 1;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		memory.write_2(New_Addr, Result & 255);
		if (log_to_console) cout << " + M" << OPCODE_comment << " = " << (int)(Result & 255);
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Result = (AX & 255) + Src + Flag_CF;
			if (log_to_console) cout << " + AL(" << (int)(AX & 255) << ") = " << (int)(Result & 255);
			Flag_AF = (((AX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = ((AX & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
			AX = (AX & 0xFF00) | (Result & 255);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 1:
			Result = (CX & 255) + Src + Flag_CF;
			if (log_to_console) cout << " + CL(" << (int)(CX & 255) << ") = " << (int)(Result & 255);
			Flag_AF = (((CX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = ((CX & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
			CX = (CX & 0xFF00) | (Result & 255);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 2:
			Result = (DX & 255) + Src + Flag_CF;
			if (log_to_console) cout << " + DL(" << (int)(DX & 255) << ") = " << (int)(Result & 255);
			Flag_AF = (((DX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = ((DX & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
			DX = (DX & 0xFF00) | (Result & 255);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 3:
			Result = (BX & 255) + Src + Flag_CF;
			if (log_to_console) cout << " + BL(" << (int)(BX & 255) << ") = " << (int)(Result & 255);
			Flag_AF = (((BX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = ((BX & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
			BX = (BX & 0xFF00) | (Result & 255);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 4:
			Result = (AX >> 8) + Src + Flag_CF;
			if (log_to_console) cout << " + AH(" << (int)(AX >> 8) << ") = " << (int)(Result & 255);
			Flag_AF = ((((AX >> 8) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = (((AX >> 8) & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
			AX = (AX & 0x00FF) | ((Result & 255) << 8);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 5:
			Result = (CX >> 8) + Src + Flag_CF;
			if (log_to_console) cout << " + CH(" << (int)(CX >> 8) << ") = " << (int)(Result & 255);
			Flag_AF = ((((CX >> 8) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = (((CX >> 8) & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
			CX = (CX & 0x00FF) | ((Result & 255) << 8);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 6:
			Result = (DX >> 8) + Src + Flag_CF;
			if (log_to_console) cout << " + DH(" << (int)(DX >> 8) << ") = " << (int)(Result & 255);
			Flag_AF = ((((DX >> 8) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = (((DX >> 8) & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
			DX = (DX & 0x00FF) | ((Result & 255) << 8);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 7:
			Result = (BX >> 8) + Src + Flag_CF;
			if (log_to_console) cout << " + BH(" << (int)(BX >> 8) << ") = " << (int)(Result & 255);
			Flag_AF = ((((BX >> 8) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = (((BX >> 8) & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
			BX = (BX & 0x00FF) | ((Result & 255) << 8);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		}
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void ADC_RM_to_RM_16()		// ADC R/M -> R/M 16bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Src = 0;
	bool OF_Carry = false;
	uint32 Result = 0;
	additional_IPs = 0;

	if (log_to_console) cout << "ADC ";

	//определяем 1-й операнд
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Src = AX;
		if (log_to_console) cout << "AX(" << (int)AX << ") + CF(" << (int)Flag_CF << ") ";
		break;
	case 1:
		Src = CX;
		if (log_to_console) cout << "CX(" << (int)CX << ") + CF(" << (int)Flag_CF << ") ";
		break;
	case 2:
		Src = DX;
		if (log_to_console) cout << "DX(" << (int)DX << ") + CF(" << (int)Flag_CF << ") ";
		break;
	case 3:
		Src = BX;
		if (log_to_console) cout << "BX(" << (int)BX << ") + CF(" << (int)Flag_CF << ") ";
		break;
	case 4:
		Src = Stack_Pointer;
		if (log_to_console) cout << "SP(" << (int)Stack_Pointer << ") + CF(" << (int)Flag_CF << ") ";
		break;
	case 5:
		Src = Base_Pointer;
		if (log_to_console) cout << "BP(" << (int)Base_Pointer << ") + CF(" << (int)Flag_CF << ") ";
		break;
	case 6:
		Src = Source_Index;
		if (log_to_console) cout << "SI(" << (int)Source_Index << ") + CF(" << (int)Flag_CF << ") ";
		break;
	case 7:
		Src = Destination_Index;
		if (log_to_console) cout << "DI(" << (int)Destination_Index << ") + CF(" << (int)Flag_CF << ") ";
		break;
	}

	//определяем объект назначения и результат операции ADD
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Result = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256 + Src + Flag_CF;
		Flag_AF = (((memory.read_2(New_Addr) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
		OF_Carry = (((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
		memory.write_2(New_Addr, Result & 255);
		memory.write_2(New_Addr + 1, (Result >> 8) & 255);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " Add M" << OPCODE_comment << " = " << (int)(Result & 0xFFFF);
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Result = AX + Src + Flag_CF;
			if (log_to_console) cout << "ADD AX(" << (int)AX << ") = " << (int)(Result & 0xFFFF);
			Flag_AF = (((AX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = ((AX & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
			AX = Result & 0xFFFF;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 1:
			Result = CX + Src + Flag_CF;
			if (log_to_console) cout << "ADD CX(" << (int)CX << ") = " << (int)(Result & 0xFFFF);
			Flag_AF = (((CX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = ((CX & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
			CX = Result & 0xFFFF;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 2:
			Result = DX + Src + Flag_CF;
			if (log_to_console) cout << "ADD DX(" << (int)DX << ") = " << (int)(Result & 0xFFFF);
			Flag_AF = (((DX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = ((DX & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
			DX = Result & 0xFFFF;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 3:
			Result = BX + Src + Flag_CF;
			if (log_to_console) cout << "ADD BX(" << (int)BX << ") = " << (int)(Result & 0xFFFF);
			Flag_AF = (((BX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = ((BX & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
			BX = Result & 0xFFFF;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 4:
			Result = Stack_Pointer + Src + Flag_CF;
			if (log_to_console) cout << "ADD SP(" << (int)Stack_Pointer << ") = " << (int)(Result & 0xFFFF);
			Flag_AF = (((Stack_Pointer & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = ((Stack_Pointer & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
			Stack_Pointer = Result & 0xFFFF;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 5:
			Result = Base_Pointer + Src + Flag_CF;
			if (log_to_console) cout << "ADD BP(" << (int)Base_Pointer << ") = " << (int)(Result & 0xFFFF);
			Flag_AF = (((Base_Pointer & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = ((Base_Pointer & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
			Base_Pointer = Result & 0xFFFF;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 6:
			Result = Source_Index + Src + Flag_CF;
			if (log_to_console) cout << "ADD SI(" << (int)Source_Index << ") = " << (int)(Result & 0xFFFF);
			Flag_AF = (((Source_Index & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = ((Source_Index & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
			Source_Index = Result & 0xFFFF;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 7:
			Result = Destination_Index + Src + Flag_CF;
			if (log_to_console) cout << "ADD DI(" << (int)Destination_Index << ") = " << (int)(Result & 0xFFFF);
			Flag_AF = (((Destination_Index & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = ((Destination_Index & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
			Destination_Index = Result & 0xFFFF;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		}
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void ADC_RM_to_R_8()		// ADC R/M -> R 8bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 Src = 0;
	uint16 Result = 0;
	additional_IPs = 0;
	bool OF_Carry = false;

	if (log_to_console) cout << "ADC ";
	//определяем 1-й операнд
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Src = memory.read_2(New_Addr);
		if (log_to_console) cout << "M" << OPCODE_comment << " + ";
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Src = AX & 255;
			if (log_to_console) cout << "AL(" << (int)Src << ") + ";
			break;
		case 1:
			Src = CX & 255;
			if (log_to_console) cout << "CL(" << (int)Src << ") + ";
			break;
		case 2:
			Src = DX & 255;
			if (log_to_console) cout << "DL(" << (int)Src << ") + ";
			break;
		case 3:
			Src = BX & 255;
			if (log_to_console) cout << "BL(" << (int)Src << ") + ";
			break;
		case 4:
			Src = AX >> 8;
			if (log_to_console) cout << "AH(" << (int)Src << ") + ";
			break;
		case 5:
			Src = CX >> 8;
			if (log_to_console) cout << "CH(" << (int)Src << ") + ";
			break;
		case 6:
			Src = DX >> 8;
			if (log_to_console) cout << "DH(" << (int)Src << ") + ";
			break;
		case 7:
			Src = BX >> 8;
			if (log_to_console) cout << "BH(" << (int)Src << ") + ";
			break;
		}
		break;
	}

	//определяем объект назначения и результат операции ADD
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Result = Src + (AX & 255) + Flag_CF;
		if (log_to_console) cout << "CF("<<(int)Flag_CF<<") + AL(" << (int)(AX & 255) << ") = " << (int)(Result & 255);
		Flag_AF = (((AX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
		OF_Carry = ((AX & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		AX = (AX & 0xFF00) | (Result & 255);
		break;
	case 1:
		Result = Src + (CX & 255) + Flag_CF;
		if (log_to_console) cout << "CF(" << (int)Flag_CF << ") + CL(" << (int)(CX & 255) << ") = " << (int)(Result & 255);
		OF_Carry = ((CX & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
		Flag_AF = (((CX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		CX = (CX & 0xFF00) | (Result & 255);
		break;
	case 2:
		Result = Src + (DX & 255) + Flag_CF;
		if (log_to_console) cout << "CF(" << (int)Flag_CF << ") + DL(" << (int)(DX & 255) << ") = " << (int)(Result & 255);
		Flag_AF = (((DX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
		OF_Carry = ((DX & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		DX = (DX & 0xFF00) | (Result & 255);
		break;
	case 3:
		Result = Src + (BX & 255) + Flag_CF;
		if (log_to_console) cout << "CF(" << (int)Flag_CF << ") + BL(" << (int)(BX & 255) << ") = " << (int)(Result & 255);
		OF_Carry = ((BX & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
		Flag_AF = (((BX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		BX = (BX & 0xFF00) | (Result & 255);
		break;
	case 4:
		Result = Src + ((AX >> 8) & 255) + Flag_CF;
		if (log_to_console) cout << "CF(" << (int)Flag_CF << ") + AH(" << (int)((AX >> 8) & 255) << ") = " << (int)(Result & 255);
		OF_Carry = (((AX >> 8) & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
		Flag_AF = ((((AX >> 8) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		AX = (AX & 0x00FF) | (Result << 8);
		break;
	case 5:
		Result = Src + (CX >> 8) + Flag_CF;
		if (log_to_console) cout << "CF(" << (int)Flag_CF << ") + CH(" << (int)((CX >> 8) & 255) << ") = " << (int)(Result & 255);
		OF_Carry = (((CX >> 8) & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
		Flag_AF = ((((CX >> 8) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		CX = (CX & 0x00FF) | (Result << 8);
		break;
	case 6:
		Result = Src + (DX >> 8) + Flag_CF;
		if (log_to_console) cout << "CF(" << (int)Flag_CF << ") + DH(" << (int)((DX >> 8) & 255) << ") = " << (int)(Result & 255);
		OF_Carry = (((DX >> 8) & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
		Flag_AF = ((((DX >> 8) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		DX = (DX & 0x00FF) | (Result << 8);
		break;
	case 7:
		Result = Src + (BX >> 8) + Flag_CF;
		if (log_to_console) cout << "CF(" << (int)Flag_CF << ") + BH(" << (int)((BX >> 8) & 255) << ") = " << (int)(Result & 255);
		Flag_AF = ((((BX >> 8) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
		OF_Carry = (((BX >> 8) & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = (Result >> 7) & 1;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		BX = (BX & 0x00FF) | (Result << 8);
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void ADC_RM_to_R_16()		// ADC R/M -> R 16bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Src = 0;
	uint32 Result = 0;
	additional_IPs = 0;
	bool OF_Carry = false;

	if (log_to_console) cout << "ADC ";
	//определяем 1-й операнд
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
		if (log_to_console) cout << "M" << OPCODE_comment << " + CF(" << (int)Flag_CF << ")";
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Src = AX;
			if (log_to_console) cout << "AX + CF(" << (int)Flag_CF << ")";
			break;
		case 1:
			Src = CX;
			if (log_to_console) cout << "CX + CF(" << (int)Flag_CF << ")";
			break;
		case 2:
			Src = DX;
			if (log_to_console) cout << "DX + CF(" << (int)Flag_CF << ")";
			break;
		case 3:
			Src = BX;
			if (log_to_console) cout << "BX + CF(" << (int)Flag_CF << ")";
			break;
		case 4:
			Src = Stack_Pointer;
			if (log_to_console) cout << "SP + CF(" << (int)Flag_CF << ")";
			break;
		case 5:
			Src = Base_Pointer;
			if (log_to_console) cout << "BP + CF(" << (int)Flag_CF << ")";
			break;
		case 6:
			Src = Source_Index;
			if (log_to_console) cout << "SI + CF(" << (int)Flag_CF << ")";
			break;
		case 7:
			Src = Destination_Index;
			if (log_to_console) cout << "DI + CF(" << (int)Flag_CF << ")";
			break;
		}
		break;
	}

	//определяем объект назначения и результат операции ADD
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Result = Src + AX + Flag_CF;
		if (log_to_console) cout << "AX(" << (int)AX << ") = " << (int)(Result & 0xFFFF);
		OF_Carry = ((AX & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
		Flag_AF = (((AX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		AX = (Result & 0xFFFF);
		break;
	case 1:
		Result = Src + CX + Flag_CF;
		if (log_to_console) cout << "CX(" << (int)CX << ") = " << (int)(Result & 0xFFFF);
		OF_Carry = ((CX & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
		Flag_AF = (((CX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		CX = (Result & 0xFFFF);
		break;
	case 2:
		Result = Src + DX + Flag_CF;
		if (log_to_console) cout << "DX(" << (int)DX << ") = " << (int)(Result & 0xFFFF);
		OF_Carry = ((DX & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
		Flag_AF = (((DX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		DX = (Result & 0xFFFF);
		break;
	case 3:
		Result = Src + BX + Flag_CF;
		if (log_to_console) cout << "BX(" << (int)BX << ") = " << (int)(Result & 0xFFFF);
		OF_Carry = ((BX & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
		Flag_AF = (((BX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		BX = (Result & 0xFFFF);
		break;
	case 4:
		Result = Src + Stack_Pointer + Flag_CF;
		OF_Carry = ((Stack_Pointer & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
		Flag_AF = (((Stack_Pointer & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
		if (log_to_console) cout << "SP(" << (int)Stack_Pointer << ") = " << (int)(Result & 0xFFFF);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Stack_Pointer = (Result & 0xFFFF);
		break;
	case 5:
		Result = Src + Base_Pointer + Flag_CF;
		if (log_to_console) cout << "BP(" << (int)Base_Pointer << ") = " << (int)(Result & 0xFFFF);
		OF_Carry = ((Base_Pointer & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
		Flag_AF = (((Base_Pointer & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Base_Pointer = (Result & 0xFFFF);
		break;
	case 6:
		Result = Src + Source_Index + Flag_CF;
		if (log_to_console) cout << "SI(" << (int)Source_Index << ") = " << (int)(Result & 0xFFFF);
		OF_Carry = ((Source_Index & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
		Flag_AF = (((Source_Index & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Source_Index = (Result & 0xFFFF);
		break;
	case 7:
		Result = Src + Destination_Index + Flag_CF;
		if (log_to_console) cout << "DI(" << (int)Destination_Index << ") = " << (int)(Result & 0xFFFF);
		OF_Carry = ((Destination_Index & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
		Flag_AF = (((Destination_Index & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Destination_Index = (Result & 0xFFFF);
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}

void ADC_IMM_to_ACC_8()		// ADC IMM->ACC 8bit
{
	uint16 Result = 0;
	bool OF_Carry = false;
	uint8 imm = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];
	if (log_to_console) cout << "ADC IMM (" << (int)imm << ") to AL(" << (int)(AX & 255) << ") + CF("<<(int)Flag_CF<<") = ";
	OF_Carry = ((imm & 0x7F) + (AX & 0x7F) + Flag_CF) >> 7;
	Flag_AF = (((AX & 15) + (imm & 15) + Flag_CF) >> 4) & 1;
	Result = imm + (AX & 255) + Flag_CF;
	Flag_CF = (Result >> 8) & 1;
	Flag_SF = (Result >> 7) & 1;
	Flag_OF = Flag_CF ^ OF_Carry;
	if (Result & 255) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result & 255];
	AX = (AX & 0xFF00) | (Result & 255);
	if (log_to_console) cout << (int)(Result & 255);

	Instruction_Pointer += 2;
}
void ADC_IMM_to_ACC_16()	// ADC IMM->ACC 16bit 
{
	uint32 Result = 0;
	bool OF_Carry = false;
	uint16 imm = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory.read_2(Instruction_Pointer + 2 + *CS * 16) * 256;
	if (log_to_console) cout << "ADC IMM (" << (int)imm << ") to AX(" << (int)(AX) << ") + CF(" << (int)Flag_CF << ") = ";
	OF_Carry = ((imm & 0x7FFF) + (AX & 0x7FFF) + Flag_CF) >> 15;
	Flag_AF = (((AX & 15) + (imm & 15) + Flag_CF) >> 4) & 1;
	Result = imm + AX + Flag_CF;
	Flag_CF = (Result >> 16) & 1;
	Flag_SF = (Result >> 15) & 1;
	Flag_OF = Flag_CF ^ OF_Carry;
	if (Result & 0xFFFF) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result & 255];
	AX = Result & 0xFFFF;
	if (log_to_console) cout << (int)(AX);
	Instruction_Pointer += 3;
}

//INC
//DEC 8
void INC_RM_8()		// INC R/M 8bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];//второй байт
	uint8 Src = 0;		//источник данных
	additional_IPs = 0;

	switch ((byte2 >> 3) & 7)
	{
	case 0: //INC RM
		//находим цель инкремента
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(New_Addr);
			Flag_AF = (((Src & 0x0F) + 1) >> 4) & 1;
			Flag_OF = ((Src & 0x7F) + 1) >> 7;
			Src++;
			memory.write_2(New_Addr, Src);
			if (Src) Flag_ZF = 0;
			else 
			{
				Flag_ZF = 1;
				Flag_OF = 0;
			}
			Flag_SF = (Src >> 7) & 1;
			Flag_PF = parity_check[Src];
			if (log_to_console) cout << "INC M" << OPCODE_comment << " = " << (int)Src;
			break;
		case 3:
			// mod 11 - адрес в регистре
			switch (byte2 & 7)
			{
			case 0:
				Src = (AX & 255) + 1;
				Flag_AF = (((AX & 0x0F) + 1) >> 4) & 1;
				Flag_OF = ((AX & 0x7F) + 1) >> 7;
				AX = (AX & 0xFF00) | Src;
				if (Src) Flag_ZF = 0;
				else {
					Flag_ZF = 1;
					Flag_OF = 0;
				}
				Flag_SF = (Src >> 7) & 1;
				Flag_PF = parity_check[Src];
				if (log_to_console) cout << "INC AL = " << (int)Src;
				break;
			case 1:
				Src = (CX & 255) + 1;
				Flag_AF = (((CX & 0x0F) + 1) >> 4) & 1;
				Flag_OF = ((CX & 0x7F) + 1) >> 7;
				CX = (CX & 0xFF00) | Src;
				if (Src) Flag_ZF = 0;
				else {
					Flag_ZF = 1;
					Flag_OF = 0;
				}
				Flag_SF = (Src >> 7) & 1;
				Flag_PF = parity_check[Src];
				if (log_to_console) cout << "INC CL = " << (int)Src;
				break;
			case 2:
				Src = (DX & 255) + 1;
				Flag_AF = (((DX & 0x0F) + 1) >> 4) & 1;
				Flag_OF = ((DX & 0x7F) + 1) >> 7;
				DX = (DX & 0xFF00) | Src;
				if (Src) Flag_ZF = 0;
				else {
					Flag_ZF = 1;
					Flag_OF = 0;
				}
				Flag_SF = (Src >> 7) & 1;
				Flag_PF = parity_check[Src];
				if (log_to_console) cout << "INC DL = " << (int)Src;
				break;
			case 3:
				Src = (BX & 255) + 1;
				Flag_AF = (((BX & 0x0F) + 1) >> 4) & 1;
				Flag_OF = ((BX & 0x7F) + 1) >> 7;
				BX = (BX & 0xFF00) | Src;
				if (Src) Flag_ZF = 0;
				else {
					Flag_ZF = 1;
					Flag_OF = 0;
				}
				Flag_SF = (Src >> 7) & 1;
				Flag_PF = parity_check[Src];
				if (log_to_console) cout << "INC BL = " << (int)Src;
				break;
			case 4:
				Src = (AX >> 8) + 1;
				Flag_AF = ((((AX >> 8) & 0x0F) + 1) >> 4) & 1;
				Flag_OF = (((AX >> 8) & 0x7F) + 1) >> 7;
				AX = (AX & 0x00FF) | Src * 256;
				if (Src) Flag_ZF = 0;
				else {
					Flag_ZF = 1;
					Flag_OF = 0;
				}
				Flag_SF = (Src >> 7) & 1;
				Flag_PF = parity_check[Src];
				if (log_to_console) cout << "INC AH = " << (int)Src;
				break;
			case 5:
				Src = (CX >> 8) + 1;
				Flag_AF = ((((CX >> 8) & 0x0F) + 1) >> 4) & 1;
				Flag_OF = (((CX >> 8) & 0x7F) + 1) >> 7;
				CX = (CX & 0x00FF) | Src * 256;
				if (Src) Flag_ZF = 0;
				else {
					Flag_ZF = 1;
					Flag_OF = 0;
				}
				Flag_SF = (Src >> 7) & 1;
				Flag_PF = parity_check[Src];
				if (log_to_console) cout << "INC CH = " << (int)Src;
				break;
			case 6:
				Src = (DX >> 8) + 1;
				Flag_AF = ((((DX >> 8) & 0x0F) + 1) >> 4) & 1;
				Flag_OF = (((DX >> 8) & 0x7F) + 1) >> 7;
				DX = (DX & 0x00FF) | Src * 256;
				if (Src) Flag_ZF = 0;
				else {
					Flag_ZF = 1;
					Flag_OF = 0;
				}
				Flag_SF = (Src >> 7) & 1;
				Flag_PF = parity_check[Src];
				if (log_to_console) cout << "INC DH = " << (int)Src;
				break;
			case 7:
				Src = (BX >> 8) + 1;
				Flag_AF = ((((BX >> 8) & 0x0F) + 1) >> 4) & 1;
				Flag_OF = (((BX >> 8) & 0x7F) + 1) >> 7;
				BX = (BX & 0x00FF) | Src * 256;
				if (Src) Flag_ZF = 0;
				else {
					Flag_ZF = 1;
					Flag_OF = 0;
				}
				Flag_SF = (Src >> 7) & 1;
				Flag_PF = parity_check[Src];
				if (log_to_console) cout << "INC BH = " << (int)Src;
				break;
			}
		}
		break;

	case 1: //DEC RM
		//находим цель декремента
		switch (byte2 >> 6)
		{
		case 0:
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(New_Addr);
			Flag_AF = (((Src & 0x0F) - 1) >> 4) & 1;
			Flag_OF = (((Src & 0x7F) - 1) >> 7) & !(Src == 0);
			Src--;
			memory.write_2(New_Addr,  Src);
			if (Src) Flag_ZF = 0;
			else Flag_ZF = 1;
			Flag_SF = (Src >> 7) & 1;
			Flag_PF = parity_check[Src];
			if (log_to_console) cout << "DEC M" << OPCODE_comment << " = " << (int)Src;
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(New_Addr);
			Flag_AF = (((Src & 0x0F) - 1) >> 4) & 1;
			Flag_OF = (((Src & 0x7F) - 1) >> 7) & !(Src == 0);
			Src--;
			memory.write_2(New_Addr,  Src);
			if (Src) Flag_ZF = 0;
			else Flag_ZF = 1;
			Flag_SF = (Src >> 7) & 1;
			Flag_PF = parity_check[Src];
			if (log_to_console) cout << "DEC M" << OPCODE_comment << " = " << (int)Src;
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(New_Addr);
			Flag_AF = (((Src & 0x0F) - 1) >> 4) & 1;
			Flag_OF = (((Src & 0x7F) - 1) >> 7) & !(Src == 0);
			Src--;
			memory.write_2(New_Addr,  Src);
			if (Src) Flag_ZF = 0;
			else Flag_ZF = 1;
			Flag_SF = (Src >> 7) & 1;
			Flag_PF = parity_check[Src];
			if (log_to_console) cout << "DEC M" << OPCODE_comment << " = " << (int)Src;
			break;
		case 3:
			// mod 11 - адрес в регистре
			switch (byte2 & 7)
			{
			case 0:
				Src = (AX & 255) - 1;
				Flag_AF = (((AX & 0x0F) - 1) >> 4) & 1;
				Flag_OF = (((AX & 0x7F) - 1) >> 7) & !((AX & 255) == 0);
				AX = (AX & 0xFF00) | Src;
				if (Src) Flag_ZF = 0;
				else Flag_ZF = 1;
				Flag_SF = (Src >> 7) & 1;
				Flag_PF = parity_check[Src];
				if (log_to_console) cout << "DEC AL = " << (int)Src;
				break;
			case 1:
				Src = (CX & 255) - 1;
				Flag_AF = (((CX & 0x0F) - 1) >> 4) & 1;
				Flag_OF = (((CX & 0x7F) - 1) >> 7) & !((CX & 255) == 0);
				CX = (CX & 0xFF00) | Src;
				if (Src) Flag_ZF = 0;
				else Flag_ZF = 1;
				Flag_SF = (Src >> 7) & 1;
				Flag_PF = parity_check[Src];
				if (log_to_console) cout << "DEC CL = " << (int)Src;
				break;
			case 2:
				Src = (DX & 255) - 1;
				Flag_AF = (((DX & 0x0F) - 1) >> 4) & 1;
				Flag_OF = (((DX & 0x7F) - 1) >> 7) & !((DX & 255) == 0);
				DX = (DX & 0xFF00) | Src;
				if (Src) Flag_ZF = 0;
				else Flag_ZF = 1;
				Flag_SF = (Src >> 7) & 1;
				Flag_PF = parity_check[Src];
				if (log_to_console) cout << "DEC DL = " << (int)Src;
				break;
			case 3:
				Src = (BX & 255) - 1;
				Flag_AF = (((BX & 0x0F) - 1) >> 4) & 1;
				Flag_OF = (((BX & 0x7F) - 1) >> 7) & !((BX & 255) == 0);
				BX = (BX & 0xFF00) | Src;
				if (Src) Flag_ZF = 0;
				else Flag_ZF = 1;
				Flag_SF = (Src >> 7) & 1;
				Flag_PF = parity_check[Src];
				if (log_to_console) cout << "DEC BL = " << (int)Src;
				break;
			case 4:
				Src = (AX >> 8) - 1;
				Flag_AF = ((((AX >> 8) & 0x0F) - 1) >> 4) & 1;
				Flag_OF = ((((AX >>8) & 0x7F) - 1) >> 7) & !((AX >> 8) == 0);
				AX = (AX & 0x00FF) + Src * 256;
				if (Src) Flag_ZF = 0;
				else Flag_ZF = 1;
				Flag_SF = (Src >> 7) & 1;
				Flag_PF = parity_check[Src];
				if (log_to_console) cout << "DEC AH = " << (int)Src;
				break;
			case 5:
				Src = (CX >> 8) - 1;
				Flag_AF = ((((CX >> 8) & 0x0F) - 1) >> 4) & 1;
				Flag_OF = ((((CX >> 8) & 0x7F) - 1) >> 7) & !((CX >> 8) == 0);
				CX = (CX & 0x00FF) + Src * 256;
				if (Src) Flag_ZF = 0;
				else Flag_ZF = 1;
				Flag_SF = (Src >> 7) & 1;
				Flag_PF = parity_check[Src];
				if (log_to_console) cout << "DEC CH = " << (int)Src;
				break;
			case 6:
				Src = (DX >> 8) - 1;
				Flag_AF = ((((DX >> 8) & 0x0F) - 1) >> 4) & 1;
				Flag_OF = ((((DX >> 8) & 0x7F) - 1) >> 7) & !((DX >> 8) == 0);
				DX = (DX & 0x00FF) + Src * 256;
				if (Src) Flag_ZF = 0;
				else Flag_ZF = 1;
				Flag_SF = (Src >> 7) & 1;
				Flag_PF = parity_check[Src];
				if (log_to_console) cout << "DEC DH = " << (int)Src;
				break;
			case 7:
				Src = (BX >> 8) - 1;
				Flag_AF = ((((BX >> 8) & 0x0F) - 1) >> 4) & 1;
				Flag_OF = ((((BX >> 8) & 0x7F) - 1) >> 7) & !((BX >> 8) == 0);
				BX = (BX & 0x00FF) + Src * 256;
				if (Src) Flag_ZF = 0;
				else Flag_ZF = 1;
				Flag_SF = (Src >> 7) & 1;
				Flag_PF = parity_check[Src];
				if (log_to_console) cout << "DEC BH = " << (int)Src;
				break;
			}
		}
		break;
}
	Instruction_Pointer += 2 + additional_IPs;
}
void INC_Reg()			//  INC reg 16 bit
{
	uint8 reg = memory_2[(Instruction_Pointer + *CS * 16) & 0xFFFFF] & 7;//второй байт
	//uint16 Src = 0;		//источник данных
	Flag_AF = (((*ptr_r16[reg] & 0x0F) + 1) >> 4) & 1;
	Flag_OF = (((*ptr_r16[reg] & 0x7FFF) + 1) >> 15);
	(*ptr_r16[reg])++;
	if (*ptr_r16[reg]) Flag_ZF = 0;
	else
	{
		Flag_ZF = 1;
		Flag_OF = 0;
	}

	Flag_SF = (*ptr_r16[reg] >> 15) & 1;
	Flag_PF = parity_check[*ptr_r16[reg] & 255];

	if (log_to_console)
	{
		switch (reg & 7)
		{
		case 0:
			if (log_to_console) cout << "INC AX = " << (int)*ptr_r16[reg];
			break;
		case 1:
			if (log_to_console) cout << "INC CX = " << (int)*ptr_r16[reg];
			break;
		case 2:
			if (log_to_console) cout << "INC DX = " << (int)*ptr_r16[reg];
			break;
		case 3:
			if (log_to_console) cout << "INC BX = " << (int)*ptr_r16[reg];
			break;
		case 4:
			if (log_to_console) cout << "INC SP = " << (int)*ptr_r16[reg];
			break;
		case 5:
			if (log_to_console) cout << "INC BP = " << (int)*ptr_r16[reg];
			break;
		case 6:
			if (log_to_console) cout << "INC SI = " << (int)*ptr_r16[reg];
			break;
		case 7:
			if (log_to_console) cout << "INC DI = " << (int)*ptr_r16[reg];
			break;
		}
	}

Instruction_Pointer += 1;
}

void AAA()				//  AAA = ASCII Adjust for Add
{
	if (log_to_console) cout << "AAA AL(" << (int)(AX & 255) << ") -> ";
	if (((AX & 0x0F) > 9) || Flag_AF )
	{
		AX = (AX & 0xFF00)|(((AX & 255) + 6) & 255);
		AX = AX + 256;
		Flag_AF = 1;
		Flag_CF = 1;
	}
	else
	{
		Flag_AF = 0;
		Flag_CF = 0;
	}
	AX = AX & 0xFF0F;
	if (log_to_console) cout << "(" << Flag_CF << " + " << (int)(AX & 255) << ")";
	Instruction_Pointer += 1;
}
void DAA()				//  DAA = Decimal Adjust for Add
{
	temp_ACC_8 = AX & 15;
	temp_ACC_16 = AX & 255;
	if (temp_ACC_8 > 9 || Flag_AF)
	{
		temp_ACC_16 = temp_ACC_16 + 6;
		Flag_AF = true;
	}
	temp_ACC_8 = (temp_ACC_16 >> 4) & 31; //старшие 4 бита

	if (temp_ACC_8 > 9 || Flag_CF)
	{
		temp_ACC_8 += 6; // +6 к старшим битам
		if (temp_ACC_8 > 15) { Flag_CF = true;}
		temp_ACC_8 = temp_ACC_8 & 15;
	}
	AX = (AX & 0xFF00)|(temp_ACC_16 & 15)|(temp_ACC_8 << 4);

	if (AX & 255) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_SF = (AX >> 7) & 1;
	Flag_PF = parity_check[AX & 255];
	if (log_to_console) cout << "DAA AL = " << (int)((AX >> 4) & 15) << " + " << (int)(AX & 15);
	Instruction_Pointer += 1;
}

//SUB

void SUB_RM_from_RM_8()			// SUB R/M -> R/M 8bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 Src = 0;
	uint8 Dest = 0;
	uint16 Result = 0;
	additional_IPs = 0;
	bool OF_Carry = false;

	//определяем 1-й операнд
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Src = AX & 255;
		if (log_to_console) cout << "SUB AL("<< setw(2)<<(int)Src<<") ";
		break;
	case 1:
		Src = CX & 255;
		if (log_to_console) cout << "SUB CL(" << setw(2) << (int)Src << ") ";
		break;
	case 2:
		Src = DX & 255;
		if (log_to_console) cout << "SUB DL(" << setw(2) << (int)Src << ") ";
		break;
	case 3:
		Src = BX & 255;
		if (log_to_console) cout << "SUB BL(" << setw(2) << (int)Src << ") ";
		break;
	case 4:
		Src = AX >> 8;
		if (log_to_console) cout << "SUB AH(" << setw(2) << (int)Src << ") ";
		break;
	case 5:
		Src = CX >> 8;
		if (log_to_console) cout << "SUB CH(" << setw(2) << (int)Src << ") ";
		break;
	case 6:
		Src = DX >> 8;
		if (log_to_console) cout << "SUB DH(" << setw(2) << (int)Src << ") ";
		break;
	case 7:
		Src = BX >> 8;
		if (log_to_console) cout << "SUB BH(" << setw(2) << (int)Src << ") ";
		break;
	}

	//определяем объект назначения и результат операции SUB
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Dest = memory.read_2(New_Addr);
		Result = Dest - Src;
		memory.write_2(New_Addr, Result & 255);
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = (Result >> 7) & 1;
		OF_Carry = ((Dest & 0x7F) - (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((Dest & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << "from M" << OPCODE_comment << " = " << (int)(Result & 255);
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Dest = AX & 255;
			Result = Dest - Src;
			AX = (AX & 0xFF00) | (Result & 255);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = (Result >> 7) & 1;
			OF_Carry = ((Dest & 0x7F) - (Src & 0x7F)) >> 7;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = (((Dest & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "from AL = " << (int)(Result & 255);
			break;
		case 1:
			Dest = CX & 255;
			Result = Dest - Src;
			CX = (CX & 0xFF00) | (Result & 255);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = (Result >> 7) & 1;
			OF_Carry = ((Dest & 0x7F) - (Src & 0x7F)) >> 7;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = (((Dest & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "from CL = " << (int)(Result & 255);
			break;
		case 2:
			Dest = DX & 255;
			Result = Dest - Src;
			DX = (DX & 0xFF00) | (Result & 255);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = (Result >> 7) & 1;
			OF_Carry = ((Dest & 0x7F) - (Src & 0x7F)) >> 7;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = (((Dest & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "from DL = " << (int)(Result & 255);
			break;
		case 3:
			Dest = BX & 255;
			Result = Dest - Src;
			BX = (BX & 0xFF00) | (Result & 255);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = (Result >> 7) & 1;
			OF_Carry = ((Dest & 0x7F) - (Src & 0x7F)) >> 7;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = (((Dest & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "from BL = " << (int)(Result & 255);
			break;
		case 4:
			Dest = AX >> 8;
			Result = Dest - Src;
			AX = (AX & 0x00FF) | (Result << 8);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = (Result >> 7) & 1;
			OF_Carry = ((Dest & 0x7F) - (Src & 0x7F)) >> 7;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = (((Dest & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "from AH = " << (int)(Result & 255);
			break;
		case 5:
			Dest = CX >> 8;
			Result = Dest - Src;
			CX = (CX & 0x00FF) | (Result << 8);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = (Result >> 7) & 1;
			OF_Carry = ((Dest & 0x7F) - (Src & 0x7F)) >> 7;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = (((Dest & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "from CH = " << (int)(Result & 255);
			break;
		case 6:
			Dest = DX >> 8;
			Result = Dest - Src;
			DX = (DX & 0x00FF) | (Result << 8);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = (Result >> 7) & 1;
			OF_Carry = ((Dest & 0x7F) - (Src & 0x7F)) >> 7;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = (((Dest & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "from DH = " << (int)(Result & 255);
			break;
		case 7:
			Dest = BX >> 8;
			Result = Dest - Src;
			BX = (BX & 0x00FF) | (Result << 8);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = (Result >> 7) & 1;
			OF_Carry = ((Dest & 0x7F) - (Src & 0x7F)) >> 7;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = (((Dest & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "from BH = " << (int)(Result & 255);
			break;
		}
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void SUB_RM_from_RM_16()		// SUB R/M -> R/M 16bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Src = 0;
	uint32 Result = 0;
	additional_IPs = 0;
	bool OF_Carry = false;

	if (log_to_console) cout << "SUB ";

	//определяем 1-й операнд
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Src = AX;
		if (log_to_console) cout << "AX("<<setw(4)<<(int)Src<<") ";
		break;
	case 1:
		Src = CX;
		if (log_to_console) cout << "CX(" << setw(4) << (int)Src << ") ";
		break;
	case 2:
		Src = DX;
		if (log_to_console) cout << "DX(" << setw(4) << (int)Src << ") ";
		break;
	case 3:
		Src = BX;
		if (log_to_console) cout << "BX(" << setw(4) << (int)Src << ") ";
		break;
	case 4:
		Src = Stack_Pointer;
		if (log_to_console) cout << "SP(" << setw(4) << (int)Src << ") ";
		break;
	case 5:
		Src = Base_Pointer;
		if (log_to_console) cout << "BP(" << setw(4) << (int)Src << ") ";
		break;
	case 6:
		Src = Source_Index;
		if (log_to_console) cout << "SI(" << setw(4) << (int)Src << ") ";
		break;
	case 7:
		Src = Destination_Index;
		if (log_to_console) cout << "DI(" << setw(4) << (int)Src << ") ";
		break;
	}

	//определяем объект назначения и результат операции SUB
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Result = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256 - Src;
		OF_Carry = (((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
		Flag_AF = ((((memory.read_2(New_Addr)) & 15) - (Src & 15)) >> 4) & 1;
		memory.write_2(New_Addr, Result & 255);
		memory.write_2(New_Addr + 1, (Result >> 8) & 255);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " from M" << OPCODE_comment << " = " << (int)(Result & 0xFFFF);
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Result = AX - Src;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			OF_Carry = (((AX) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = (((AX & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "from AX("<<(int)AX<<") = " << (int)(Result & 0xFFFF);
			AX = Result;
			break;
		case 1:
			Result = CX - Src;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			OF_Carry = (((CX) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = (((CX & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "from CX(" << (int)CX << ") = " << (int)(Result & 0xFFFF);
			CX = Result;
			break;
		case 2:
			Result = DX - Src;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			OF_Carry = (((DX) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = (((DX & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "from DX(" << (int)DX << ") = " << (int)(Result & 0xFFFF);
			DX = Result;
			break;
		case 3:
			Result = BX - Src;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			OF_Carry = (((BX) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = (((BX & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "from BX(" << (int)BX << ") = " << (int)(Result & 0xFFFF);
			BX = Result;
			break;
		case 4:
			Result = Stack_Pointer - Src;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			OF_Carry = (((Stack_Pointer) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = (((Stack_Pointer & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "from SP(" << (int)Stack_Pointer << ") = " << (int)(Result & 0xFFFF);
			Stack_Pointer = Result;
			break;
		case 5:
			Result = Base_Pointer - Src;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			OF_Carry = (((Base_Pointer) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = (((Base_Pointer & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "from BP(" << (int)Base_Pointer << ") = " << (int)(Result & 0xFFFF);
			Base_Pointer = Result;
			break;
		case 6:
			Result = Source_Index - Src;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			OF_Carry = (((Source_Index) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = (((Source_Index & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "from SI(" << (int)Source_Index << ") = " << (int)(Result & 0xFFFF);
			Source_Index = Result;
			break;
		case 7:
			Result = Destination_Index - Src;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			OF_Carry = (((Destination_Index) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = (((Destination_Index & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "from DI(" << (int)Destination_Index << ") = " << (int)(Result & 0xFFFF);
			Destination_Index = Result;
			break;
		}
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void SUB_RM_from_R_8()			// SUB R/M -> R 8bit
{
	uint8 byte2 = memory.read_2(Instruction_Pointer + 1 + * CS * 16); //mod / reg / rm
	uint8 Src = 0;
	uint8 Dest = 0;
	uint16 Result = 0;
	additional_IPs = 0;
	bool OF_Carry = false;

	if (log_to_console) cout << "SUB ";
	//определяем 1-й операнд
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Src = memory.read_2(New_Addr);
		if (log_to_console) cout << "M" << OPCODE_comment << " + ";
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Src = AX & 255;
			if (log_to_console) cout << "AL("<<(int)Src<<") + ";
			break;
		case 1:
			Src = CX & 255;
			if (log_to_console) cout << "CL(" << (int)Src << ") + ";
			break;
		case 2:
			Src = DX & 255;
			if (log_to_console) cout << "DL(" << (int)Src << ") + ";
			break;
		case 3:
			Src = BX & 255;
			if (log_to_console) cout << "BL(" << (int)Src << ") + ";
			break;
		case 4:
			Src = AX >> 8;
			if (log_to_console) cout << "AH(" << (int)Src << ") + ";
			break;
		case 5:
			Src = CX >> 8;
			if (log_to_console) cout << "CH(" << (int)Src << ") + ";
			break;
		case 6:
			Src = DX >> 8;
			if (log_to_console) cout << "DH(" << (int)Src << ") + ";
			break;
		case 7:
			Src = BX >> 8;
			if (log_to_console) cout << "BH(" << (int)Src << ") + ";
			break;
		}
		break;
	}

	//определяем объект назначения и результат операции SUB
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Dest = AX & 255;
		Result = Dest - Src;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = ((Dest & 0x7F) - (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((Dest & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		AX = (AX & 0xFF00) | (Result & 255);
		if (log_to_console) cout << " from AL = " << (int)(Result & 255);
		break;
	case 1:
		Dest = CX & 255;
		Result = Dest - Src;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = ((Dest & 0x7F) - (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((Dest & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		CX = (CX & 0xFF00) | (Result & 255);
		if (log_to_console) cout << " from CL = " << (int)(Result & 255);
		break;
	case 2:
		Dest = DX & 255;
		Result = Dest - Src;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = ((Dest & 0x7F) - (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((Dest & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		DX = (DX & 0xFF00) | (Result & 255);
		if (log_to_console) cout << " from DL = " << (int)(Result & 255);
		break;
	case 3:
		Dest = BX & 255;
		Result = Dest - Src;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = ((Dest & 0x7F) - (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((Dest & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		BX = (BX & 0xFF00) | (Result & 255);
		if (log_to_console) cout << " from BL = " << (int)(Result & 255);
		break;
	case 4:
		Dest = AX >> 8;
		Result = Dest - Src;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = ((Dest & 0x7F) - (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((Dest & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		AX = (AX & 0x00FF) | (Result << 8);
		if (log_to_console) cout << " from AH = " << (int)(Result & 255);
		break;
	case 5:
		Dest = CX >> 8;
		Result = Dest - Src;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = ((Dest & 0x7F) - (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((Dest & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		CX = (CX & 0x00FF) | (Result << 8);
		if (log_to_console) cout << " from CH = " << (int)(Result & 255);
		break;
	case 6:
		Dest = DX >> 8;
		Result = Dest - Src;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = ((Dest & 0x7F) - (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((Dest & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		DX = (DX & 0x00FF) | (Result << 8);
		if (log_to_console) cout << " from DH = " << (int)(Result & 255);
		break;
	case 7:
		Dest = BX >> 8;
		Result = Dest - Src;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = ((Dest & 0x7F) - (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((Dest & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		BX = (BX & 0x00FF) | (Result << 8);
		if (log_to_console) cout << " from BH = " << (int)(Result & 255);
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void SUB_RM_from_R_16()			// SUB R/M -> R 16bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Src = 0;
	uint32 Result = 0;
	additional_IPs = 0;
	bool OF_Carry = false;

	if (log_to_console) cout << "SUB ";
	//определяем 1-й операнд
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
		if (log_to_console) cout << "M" << OPCODE_comment << " ";
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Src = AX;
			if (log_to_console) cout << "AX(" << (int)AX << ") ";
			break;
		case 1:
			Src = CX;
			if (log_to_console) cout << "CX(" << (int)CX << ") ";
			break;
		case 2:
			Src = DX;
			if (log_to_console) cout << "DX(" << (int)DX << ") ";
			break;
		case 3:
			Src = BX;
			if (log_to_console) cout << "BX(" << (int)BX << ") ";
			break;
		case 4:
			Src = Stack_Pointer;
			if (log_to_console) cout << "SP(" << (int)Stack_Pointer << ") ";
			break;
		case 5:
			Src = Base_Pointer;
			if (log_to_console) cout << "BP(" << (int)Base_Pointer << ") ";
			break;
		case 6:
			Src = Source_Index;
			if (log_to_console) cout << "SI(" << (int)Source_Index << ") ";
			break;
		case 7:
			Src = Destination_Index;
			if (log_to_console) cout << "DI(" << (int)Destination_Index << ") ";
			break;
		}
		break;
	}

	//определяем объект назначения и результат операции SUB
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Result = AX - Src;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = (((AX) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		//Result = Result & 0xFFFF;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((((AX) & 15) - (Src & 15)) >> 4) & 1;
		if (log_to_console) cout << "from AX("<< (int)(AX) <<") = " << (int)(Result & 0xFFFF);
		AX = Result;
		break;
	case 1:
		Result = CX - Src;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = (((CX) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		//Result = Result & 0xFFFF;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((((CX) & 15) - (Src & 15)) >> 4) & 1;
		if (log_to_console) cout << "from CX(" << (int)(CX) << ") = " << (int)(Result & 0xFFFF);
		CX = Result;
		break;
	case 2:
		Result = DX - Src;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = (((DX) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		//Result = Result & 0xFFFF;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((((DX) & 15) - (Src & 15)) >> 4) & 1;
		if (log_to_console) cout << "from DX(" << (int)(DX) << ") = " << (int)(Result & 0xFFFF);
		DX = Result;
		break;
	case 3:
		Result = BX - Src;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = (((BX) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		//Result = Result & 0xFFFF;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((((BX) & 15) - (Src & 15)) >> 4) & 1;
		if (log_to_console) cout << "from BX(" << (int)(BX) << ") = " << (int)(Result & 0xFFFF);
		BX = Result;
		break;
	case 4:
		Result = Stack_Pointer - Src;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = (((Stack_Pointer) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		//Result = Result & 0xFFFF;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((((Stack_Pointer) & 15) - (Src & 15)) >> 4) & 1;
		if (log_to_console) cout << "from SP(" << (int)(Stack_Pointer) << ") = " << (int)(Result & 0xFFFF);
		Stack_Pointer = Result;
		break;
	case 5:
		Result = Base_Pointer - Src;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = (((Base_Pointer) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		//Result = Result & 0xFFFF;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((((Base_Pointer) & 15) - (Src & 15)) >> 4) & 1;
		if (log_to_console) cout << "from BP(" << (int)(Base_Pointer) << ") = " << (int)(Result & 0xFFFF);
		Base_Pointer = Result;
		break;
	case 6:
		Result = Source_Index - Src;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = (((Source_Index) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		//Result = Result & 0xFFFF;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((((Source_Index) & 15) - (Src & 15)) >> 4) & 1;
		if (log_to_console) cout << "from SI(" << (int)(Source_Index) << ") = " << (int)(Result & 0xFFFF);
		Source_Index = Result;
		break;
	case 7:
		Result = Destination_Index - Src;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = (((Destination_Index) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		//Result = Result & 0xFFFF;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((((Destination_Index) & 15) - (Src & 15)) >> 4) & 1;
		if (log_to_console) cout << "from DI(" << (int)(Destination_Index) << ") = " << (int)(Result & 0xFFFF);
		Destination_Index = Result;
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}

void SUB_IMM_from_ACC_8()		// SUB ACC  8bit - IMM -> ACC
{
	uint16 Result = 0;
	uint8 imm = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];
	bool OF_Carry = 0;

	if (log_to_console) cout << "SUB IMM (" << (int)imm << ") from AL(" << setw(2) << (int)(AX & 255) << ") = ";
	OF_Carry = (((AX & 255) & 0x7F) - (imm & 0x7F)) >> 7;
	Result = (AX & 255) - imm;
	Flag_CF = (Result >> 8) & 1;
	Flag_SF = (Result >> 7) & 1;
	Flag_OF = Flag_CF ^ OF_Carry;
	if (Result & 255) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result & 255];
	Flag_AF = ((((AX) & 15) - (imm & 15)) >> 4) & 1;
	AX = (AX & 0xFF00) | (Result & 255);
	if (log_to_console) cout << (int)(Result & 255);

	Instruction_Pointer += 2;
}
void SUB_IMM_from_ACC_16()		// SUB ACC 16bit - IMM -> ACC
{
	uint32 Result = 0;
	bool OF_Carry = 0;
	uint16 imm = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory.read_2(Instruction_Pointer + 2 + *CS * 16) * 256;
	if (log_to_console) cout << "SUB IMM (" << (int)imm << ") from AX(" << (int)AX << ") = ";
	Result = AX - imm;
	OF_Carry = ((AX & 0x7FFF) - (imm & 0x7FFF)) >> 15;
	Flag_AF = (((AX & 15) - (imm & 15)) >> 4) & 1;
	Flag_CF = (Result >> 16) & 1;
	Flag_SF = (Result >> 15) & 1;
	Flag_OF = Flag_CF ^ OF_Carry;
	if (Result & 0xFFFF) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result & 255];
	AX = Result;
	if (log_to_console) cout << (int)(AX);
	Instruction_Pointer += 3;
}

//SBB

void SBB_RM_from_RM_8()			// SBB R/M -> R/M 8bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 Src = 0;
	uint8 Dest = 0;
	uint16 Result = 0;
	additional_IPs = 0;
	bool OF_Carry = false;

	//определяем 1-й операнд
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Src = AX & 255;
		if (log_to_console) cout << "SBB AL(" << setw(2) << (int)Src << ") ";
		break;
	case 1:
		Src = CX & 255;
		if (log_to_console) cout << "SBB CL(" << setw(2) << (int)Src << ") ";
		break;
	case 2:
		Src = DX & 255;
		if (log_to_console) cout << "SBB DL(" << setw(2) << (int)Src << ") ";
		break;
	case 3:
		Src = BX & 255;
		if (log_to_console) cout << "SBB BL(" << setw(2) << (int)Src << ") ";
		break;
	case 4:
		Src = AX >> 8;
		if (log_to_console) cout << "SBB AH(" << setw(2) << (int)Src << ") ";
		break;
	case 5:
		Src = CX >> 8;
		if (log_to_console) cout << "SBB CH(" << setw(2) << (int)Src << ") ";
		break;
	case 6:
		Src = DX >> 8;
		if (log_to_console) cout << "SBB DH(" << setw(2) << (int)Src << ") ";
		break;
	case 7:
		Src = BX >> 8;
		if (log_to_console) cout << "SBB BH(" << setw(2) << (int)Src << ") ";
		break;
	}

	//определяем объект назначения и результат операции SUB
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Dest = memory.read_2(New_Addr);
		Result = Dest - Src - Flag_CF;
		OF_Carry = ((Dest & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
		Flag_AF = (((Dest & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
		memory.write_2(New_Addr,  Result & 255);
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = (Result >> 7) & 1;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "from M" << OPCODE_comment << "=" << (int)(Result & 255);
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Dest = AX & 255;
			Result = Dest - Src - Flag_CF;
			OF_Carry = ((Dest & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
			Flag_AF = (((Dest & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
			Flag_CF = (Result >> 8) & 1;
			AX = (AX & 0xFF00) | (Result & 255);
			Flag_SF = (Result >> 7) & 1;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "from AL = " << (int)(Result & 255);
			break;
		case 1:
			Dest = CX & 255;
			Result = Dest - Src - Flag_CF;
			OF_Carry = ((Dest & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
			Flag_AF = (((Dest & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
			CX = (CX & 0xFF00) | (Result & 255);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = (Result >> 7) & 1;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "from CL = " << (int)(Result & 255);
			break;
		case 2:
			Dest = DX & 255;
			Result = Dest - Src - Flag_CF;
			OF_Carry = ((Dest & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
			Flag_AF = (((Dest & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
			DX = (DX & 0xFF00) | (Result & 255);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = (Result >> 7) & 1;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "from DL = " << (int)(Result & 255);
			break;
		case 3:
			Dest = BX & 255;
			Result = Dest - Src - Flag_CF;
			OF_Carry = ((Dest & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
			Flag_AF = (((Dest & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
			BX = (BX & 0xFF00) | (Result & 255);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = (Result >> 7) & 1;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "from BL = " << (int)(Result & 255);
			break;
		case 4:
			Dest = AX >> 8;
			Result = Dest - Src - Flag_CF;
			OF_Carry = ((Dest & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
			Flag_AF = (((Dest & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
			AX = (AX & 0x00FF) | (Result << 8);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = (Result >> 7) & 1;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "from AH = " << (int)(Result & 255);
			break;
		case 5:
			Dest = CX >> 8;
			Result = Dest - Src - Flag_CF;
			OF_Carry = ((Dest & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
			Flag_AF = (((Dest & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
			CX = (CX & 0x00FF) | (Result << 8);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = (Result >> 7) & 1;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "from CH = " << (int)(Result & 255);
			break;
		case 6:
			Dest = DX >> 8;
			Result = Dest - Src - Flag_CF;
			OF_Carry = ((Dest & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
			Flag_AF = (((Dest & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
			DX = (DX & 0x00FF) | (Result << 8);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = (Result >> 7) & 1;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "from DH = " << (int)(Result & 255);
			break;
		case 7:
			Dest = BX >> 8;
			Result = Dest - Src - Flag_CF;
			OF_Carry = ((Dest & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
			Flag_AF = (((Dest & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
			BX = (BX & 0x00FF) | (Result << 8);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = (Result >> 7) & 1;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "from BH = " << (int)(Result & 255);
			break;
		}
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void SBB_RM_from_RM_16()		// SBB R/M -> R/M 16bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Src = 0;
	uint32 Result = 0;
	additional_IPs = 0;
	bool OF_Carry = false;

	if (log_to_console) cout << "SBB ";

	//определяем 1-й операнд
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Src = AX;
		if (log_to_console) cout << "AX(" << setw(4) << (int)Src << ") ";
		break;
	case 1:
		Src = CX;
		if (log_to_console) cout << "CX(" << setw(4) << (int)Src << ") ";
		break;
	case 2:
		Src = DX;
		if (log_to_console) cout << "DX(" << setw(4) << (int)Src << ") ";
		break;
	case 3:
		Src = BX;
		if (log_to_console) cout << "BX(" << setw(4) << (int)Src << ") ";
		break;
	case 4:
		Src = Stack_Pointer;
		if (log_to_console) cout << "SP(" << setw(4) << (int)Src << ") ";
		break;
	case 5:
		Src = Base_Pointer;
		if (log_to_console) cout << "BP(" << setw(4) << (int)Src << ") ";
		break;
	case 6:
		Src = Source_Index;
		if (log_to_console) cout << "SI(" << setw(4) << (int)Src << ") ";
		break;
	case 7:
		Src = Destination_Index;
		if (log_to_console) cout << "DI(" << setw(4) << (int)Src << ") ";
		break;
	}

	//определяем объект назначения и результат операции SUB
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Result = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256 - Src - Flag_CF;
		OF_Carry = (((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
		Flag_AF = ((((memory.read_2(New_Addr)) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
		memory.write_2(New_Addr,  Result & 255);
		memory.write_2(New_Addr + 1,  (Result >> 8) & 255);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " from M" << OPCODE_comment << "=" << (int)(Result & 0xFFFF);
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Result = AX - Src - Flag_CF;
			OF_Carry = (((AX) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
			Flag_AF = (((AX & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "from AX(" << (int)AX << ") = " << (int)(Result & 0xFFFF);
			AX = Result;
			break;
		case 1:
			Result = CX - Src - Flag_CF;
			OF_Carry = (((CX) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
			Flag_AF = (((CX & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "from CX(" << (int)CX << ") = " << (int)(Result & 0xFFFF);
			CX = Result;
			break;
		case 2:
			Result = DX - Src - Flag_CF;
			OF_Carry = (((DX) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
			Flag_AF = (((DX & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "from DX(" << (int)DX << ") = " << (int)(Result & 0xFFFF);
			DX = Result;
			break;
		case 3:
			Result = BX - Src - Flag_CF;
			OF_Carry = (((BX) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
			Flag_AF = (((BX & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "from BX(" << (int)BX << ") = " << (int)(Result & 0xFFFF);
			BX = Result;
			break;
		case 4:
			Result = Stack_Pointer - Src - Flag_CF;
			OF_Carry = (((Stack_Pointer) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
			Flag_AF = (((Stack_Pointer & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "from SP(" << (int)Stack_Pointer << ") = " << (int)(Result & 0xFFFF);
			Stack_Pointer = Result;
			break;
		case 5:
			Result = Base_Pointer - Src - Flag_CF;
			OF_Carry = (((Base_Pointer) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
			Flag_AF = (((Base_Pointer & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "from BP(" << (int)Base_Pointer << ") = " << (int)(Result & 0xFFFF);
			Base_Pointer = Result;
			break;
		case 6:
			Result = Source_Index - Src - Flag_CF;
			OF_Carry = (((Source_Index) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
			Flag_AF = (((Source_Index & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "from SI(" << (int)Source_Index << ") = " << (int)(Result & 0xFFFF);
			Source_Index = Result;
			break;
		case 7:
			Result = Destination_Index - Src - Flag_CF;
			OF_Carry = (((Destination_Index) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
			Flag_AF = (((Destination_Index & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "from DI(" << (int)Destination_Index << ") = " << (int)(Result & 0xFFFF);
			Destination_Index = Result;
			break;
		}
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void SBB_RM_from_R_8()			// SBB R/M -> R 8bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 Src = 0;
	uint8 Dest = 0;
	uint16 Result = 0;
	additional_IPs = 0;
	bool OF_Carry = false;

	if (log_to_console) cout << "SBB ";
	//определяем 1-й операнд
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Src = memory.read_2(New_Addr);
		if (log_to_console) cout << "M" << OPCODE_comment << " - ";
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Src = AX & 255;
			if (log_to_console) cout << "AL(" << (int)Src << ") - ";
			break;
		case 1:
			Src = CX & 255;
			if (log_to_console) cout << "CL(" << (int)Src << ") - ";
			break;
		case 2:
			Src = DX & 255;
			if (log_to_console) cout << "DL(" << (int)Src << ") - ";
			break;
		case 3:
			Src = BX & 255;
			if (log_to_console) cout << "BL(" << (int)Src << ") - ";
			break;
		case 4:
			Src = AX >> 8;
			if (log_to_console) cout << "AH(" << (int)Src << ") - ";
			break;
		case 5:
			Src = CX >> 8;
			if (log_to_console) cout << "CH(" << (int)Src << ") - ";
			break;
		case 6:
			Src = DX >> 8;
			if (log_to_console) cout << "DH(" << (int)Src << ") - ";
			break;
		case 7:
			Src = BX >> 8;
			if (log_to_console) cout << "BH(" << (int)Src << ") - ";
			break;
		}
		break;
	}

	//определяем объект назначения и результат операции SUB
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Dest = AX & 255;
		Result = Dest - Src - Flag_CF;
		OF_Carry = ((Dest & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
		Flag_AF = (((Dest & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		AX = (AX & 0xFF00) | (Result & 255);
		if (log_to_console) cout << " from AL = " << (int)(Result & 255);
		break;
	case 1:
		Dest = CX & 255;
		Result = Dest - Src - Flag_CF;
		OF_Carry = ((Dest & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
		Flag_AF = (((Dest & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		CX = (CX & 0xFF00) | (Result & 255);
		if (log_to_console) cout << " from CL = " << (int)(Result & 255);
		break;
	case 2:
		Dest = DX & 255;
		Result = Dest - Src - Flag_CF;
		OF_Carry = ((Dest & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
		Flag_AF = (((Dest & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		DX = (DX & 0xFF00) | (Result & 255);
		if (log_to_console) cout << " from DL = " << (int)(Result & 255);
		break;
	case 3:
		Dest = BX & 255;
		Result = Dest - Src - Flag_CF;
		OF_Carry = ((Dest & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
		Flag_AF = (((Dest & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		BX = (BX & 0xFF00) | (Result & 255);
		if (log_to_console) cout << " from BL = " << (int)(Result & 255);
		break;
	case 4:
		Dest = AX >> 8;
		Result = Dest - Src - Flag_CF;
		OF_Carry = ((Dest & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
		Flag_AF = (((Dest & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		AX = (AX & 0x00FF) | (Result << 8);
		if (log_to_console) cout << " from AH = " << (int)(Result & 255);
		break;
	case 5:
		Dest = CX >> 8;
		Result = Dest - Src - Flag_CF;
		OF_Carry = ((Dest & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
		Flag_AF = (((Dest & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		CX = (CX & 0x00FF) | (Result << 8);
		if (log_to_console) cout << " from CH = " << (int)(Result & 255);
		break;
	case 6:
		Dest = DX >> 8;
		Result = Dest - Src - Flag_CF;
		OF_Carry = ((Dest & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
		Flag_AF = (((Dest & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		DX = (DX & 0x00FF) | (Result << 8);
		if (log_to_console) cout << " from DH = " << (int)(Result & 255);
		break;
	case 7:
		Dest = BX >> 8;
		Result = Dest - Src - Flag_CF;
		OF_Carry = ((Dest & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
		Flag_AF = (((Dest & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		BX = (BX & 0x00FF) | (Result << 8);
		if (log_to_console) cout << " from BH = " << (int)(Result & 255);
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void SBB_RM_from_R_16()			// SBB R/M -> R 16bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Src = 0;
	uint32 Result = 0;
	additional_IPs = 0;
	bool OF_Carry = false;

	if (log_to_console) cout << "SBB ";
	//определяем 1-й операнд
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
		if (log_to_console) cout << "M" << OPCODE_comment << " ";
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Src = AX;
			if (log_to_console) cout << "AX(" << (int)AX << ") ";
			break;
		case 1:
			Src = CX;
			if (log_to_console) cout << "CX(" << (int)CX << ") ";
			break;
		case 2:
			Src = DX;
			if (log_to_console) cout << "DX(" << (int)DX << ") ";
			break;
		case 3:
			Src = BX;
			if (log_to_console) cout << "BX(" << (int)BX << ") ";
			break;
		case 4:
			Src = Stack_Pointer;
			if (log_to_console) cout << "SP(" << (int)Stack_Pointer << ") ";
			break;
		case 5:
			Src = Base_Pointer;
			if (log_to_console) cout << "BP(" << (int)Base_Pointer << ") ";
			break;
		case 6:
			Src = Source_Index;
			if (log_to_console) cout << "SI(" << (int)Source_Index << ") ";
			break;
		case 7:
			Src = Destination_Index;
			if (log_to_console) cout << "DI(" << (int)Destination_Index << ") ";
			break;
		}
		break;
	}

	//определяем объект назначения и результат операции SUB
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Result = AX - Src - Flag_CF;
		OF_Carry = (((AX) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
		Flag_AF = ((((AX) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		//Result = Result & 0xFFFF;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "from AX(" << (int)(AX) << ") = " << (int)(Result & 0xFFFF);
		AX = Result;
		break;
	case 1:
		Result = CX - Src - Flag_CF;
		OF_Carry = (((CX) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
		Flag_AF = ((((CX) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		//Result = Result & 0xFFFF;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "from CX(" << (int)(CX) << ") = " << (int)(Result & 0xFFFF);
		CX = Result;
		break;
	case 2:
		Result = DX - Src - Flag_CF;
		OF_Carry = (((DX) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
		Flag_AF = ((((DX) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		//Result = Result & 0xFFFF;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "from DX(" << (int)(DX) << ") = " << (int)(Result & 0xFFFF);
		DX = Result;
		break;
	case 3:
		Result = BX - Src - Flag_CF;
		OF_Carry = (((BX) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
		Flag_AF = ((((BX) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		//Result = Result & 0xFFFF;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "from BX(" << (int)(BX) << ") = " << (int)(Result & 0xFFFF);
		BX = Result;
		break;
	case 4:
		Result = Stack_Pointer - Src - Flag_CF;
		OF_Carry = (((Stack_Pointer) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
		Flag_AF = ((((Stack_Pointer) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		//Result = Result & 0xFFFF;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "from SP(" << (int)(Stack_Pointer) << ") = " << (int)(Result & 0xFFFF);
		Stack_Pointer = Result;
		break;
	case 5:
		Result = Base_Pointer - Src - Flag_CF;
		OF_Carry = (((Base_Pointer) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
		Flag_AF = ((((Base_Pointer) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		//Result = Result & 0xFFFF;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "from BP(" << (int)(Base_Pointer) << ") = " << (int)(Result & 0xFFFF);
		Base_Pointer = Result;
		break;
	case 6:
		Result = Source_Index - Src - Flag_CF;
		OF_Carry = (((Source_Index) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
		Flag_AF = ((((Source_Index) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		//Result = Result & 0xFFFF;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "from SI(" << (int)(Source_Index) << ") = " << (int)(Result & 0xFFFF);
		Source_Index = Result;
		break;
	case 7:
		Result = Destination_Index - Src - Flag_CF;
		OF_Carry = (((Destination_Index) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
		Flag_AF = ((((Destination_Index) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		//Result = Result & 0xFFFF;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "from DI(" << (int)(Destination_Index) << ") = " << (int)(Result & 0xFFFF);
		Destination_Index = Result;
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}

void SBB_IMM_from_ACC_8()		// SBB ACC  8bit - IMM -> ACC
{
	uint16 Result = 0;
	uint8 imm = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];
	bool OF_Carry = 0;
	if (log_to_console) cout << "SBB IMM (" << (int)imm << ") from AL(" << setw(2) << (int)(AX & 255) << ") = ";
	OF_Carry = ((AX & 0x7F) - (imm & 0x7F) - Flag_CF) >> 7;
	Flag_AF = (((AX & 15) - (imm & 15) - Flag_CF) >> 4) & 1;
	Result = (AX & 255) - imm - Flag_CF;
	Flag_CF = (Result >> 8) & 1;
	Flag_SF = (Result >> 7) & 1;
	Flag_OF = Flag_CF ^ OF_Carry;
	if (Result & 255) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result & 255];
	AX = (AX & 0xFF00) | (Result & 255);
	if (log_to_console) cout << (int)(Result & 255);
	Instruction_Pointer += 2;
}
void SBB_IMM_from_ACC_16()		// SBB ACC 16bit - IMM -> ACC
{
	uint32 Result = 0;
	bool OF_Carry = 0;
	uint16 imm = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory.read_2(Instruction_Pointer + 2 + *CS * 16) * 256;
	if (log_to_console) cout << "SBB IMM (" << (int)imm << ") from AX(" << (int)AX << ") = ";
	OF_Carry = ((AX & 0x7FFF) - (imm & 0x7FFF) - Flag_CF) >> 15;
	Flag_AF = (((AX & 15) - (imm & 15) - Flag_CF) >> 4) & 1;
	Result = AX - imm - Flag_CF;
	Flag_CF = (Result >> 16) & 1;
	Flag_SF = (Result >> 15) & 1;
	Flag_OF = Flag_CF ^ OF_Carry;
	if (Result & 0xFFFF) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result & 255];
	AX = Result;
	if (log_to_console) cout << (int)(AX);

	Instruction_Pointer += 3;
}

void DEC_Reg()			//  DEC reg 16 bit
{
	uint8 byte1 = memory.read_2(Instruction_Pointer + *CS * 16);//второй байт
	uint16 Src = 0;		//источник данных

	switch (byte1 & 7)
	{
	case 0:
		Flag_AF = (((AX & 0x0F) - 1) >> 4) & 1;
		Flag_OF = (((AX & 0x7FFF) - 1) >> 15);
		AX--;
		Src = AX;
		if (Src) Flag_ZF = 0;
		else Flag_ZF = 1;
		Flag_SF = (Src >> 15) & 1;
		Flag_PF = parity_check[Src & 255];
		if (log_to_console) cout << "DEC AX = " << (int)Src;
		break;
	case 1:
		Flag_AF = (((CX & 0x0F) - 1) >> 4) & 1;
		Flag_OF = (((CX & 0x7FFF) - 1) >> 15);
		CX--;
		Src = CX;
		if (Src) Flag_ZF = 0;
		else Flag_ZF = 1;
		Flag_SF = (Src >> 15) & 1;
		Flag_PF = parity_check[Src & 255];
		if (log_to_console) cout << "DEC CX = " << (int)Src;
		break;
	case 2:
		Flag_AF = (((DX & 0x0F) - 1) >> 4) & 1;
		Flag_OF = (((DX & 0x7FFF) - 1) >> 15);
		DX--;
		Src = DX;
		if (Src) Flag_ZF = 0;
		else Flag_ZF = 1;
		Flag_SF = (Src >> 15) & 1;
		Flag_PF = parity_check[Src & 255];
		if (log_to_console) cout << "DEC DX = " << (int)Src;
		break;
	case 3:
		Flag_AF = (((BX & 0x0F) - 1) >> 4) & 1;
		Flag_OF = (((BX & 0x7FFF) - 1) >> 15);
		BX--;
		Src = BX;
		if (Src) Flag_ZF = 0;
		else Flag_ZF = 1;
		Flag_SF = (Src >> 15) & 1;
		Flag_PF = parity_check[Src & 255];
		if (log_to_console) cout << "DEC BX = " << (int)Src;
		break;
	case 4:
		Flag_AF = (((Stack_Pointer & 0x0F) - 1) >> 4) & 1;
		Flag_OF = (((Stack_Pointer & 0x7FFF) - 1) >> 15);
		Stack_Pointer--;
		Src = Stack_Pointer;
		if (Src) Flag_ZF = 0;
		else Flag_ZF = 1;
		Flag_SF = (Src >> 15) & 1;
		Flag_PF = parity_check[Src & 255];
		if (log_to_console) cout << "DEC SP = " << (int)Src;
		break;
	case 5:
		Flag_AF = (((Base_Pointer & 0x0F) - 1) >> 4) & 1;
		Flag_OF = (((Base_Pointer & 0x7FFF) - 1) >> 15);
		Base_Pointer--;
		Src = Base_Pointer;
		if (Src) Flag_ZF = 0;
		else Flag_ZF = 1;
		Flag_SF = (Src >> 15) & 1;
		Flag_PF = parity_check[Src & 255];
		if (log_to_console) cout << "DEC BP = " << (int)Src;
		break;
	case 6:
		Flag_AF = (((Source_Index & 0x0F) - 1) >> 4) & 1;
		Flag_OF = (((Source_Index & 0x7FFF) - 1) >> 15);
		Source_Index--;
		Src = Source_Index;
		if (Src) Flag_ZF = 0;
		else Flag_ZF = 1;
		Flag_SF = (Src >> 15) & 1;
		Flag_PF = parity_check[Src & 255];
		if (log_to_console) cout << "DEC SI = " << (int)Src;
		break;
	case 7:
		Flag_AF = (((Destination_Index & 0x0F) - 1) >> 4) & 1;
		Flag_OF = (((Destination_Index & 0x7FFF) - 1) >> 15);
		Destination_Index--;
		Src = Destination_Index;
		if (Src) Flag_ZF = 0;
		else Flag_ZF = 1;
		Flag_SF = (Src >> 15) & 1;
		Flag_PF = parity_check[Src & 255];
		if (log_to_console) cout << "DEC DI = " << (int)Src;
		break;
	}
	Instruction_Pointer += 1;
}

void CMP_Reg_RM_8()		//  CMP Reg with R/M 8 bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 Dst = 0;
	uint8 Src = 0;
	uint16 Result = 0;
	bool OF_Carry = false;
	
	//в данном случае Src(вычитаемое) - регистр, Destination - память
	//определяем 1-й операнд
	
	Src = *ptr_r8[(byte2 >> 3) & 7];
	if (log_to_console) cout << "CMP " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)Src << ") ";
		
	//определяем объект назначения и результат операции SUB
	if((byte2 >> 6) == 3)
	{
		// mod 11 приемник - регистр (byte2 & 7)
		additional_IPs = 0;
		Dst = *ptr_r8[byte2 & 7];
		if (log_to_console) cout << "with " << reg8_name[byte2 & 7] << "(" << (int)Dst << ") = ";
	}
	else
	{
		//приемник - память
		mod_RM_3(byte2);
		Dst = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		if (log_to_console) cout << "with M" << OPCODE_comment << " = ";
	}

	Result = Dst - Src;
	Flag_CF = (Result >> 8) & 1;
	Flag_SF = ((Result >> 7) & 1);
	OF_Carry = ((Dst & 0x7F) - (Src & 0x7F)) >> 7;
	Flag_OF = Flag_CF ^ OF_Carry;
	if (Result & 255) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result & 255];
	Flag_AF = (((Dst & 0x0F) - (Src & 0x0F)) >> 4) & 1;
	if (log_to_console) cout << "with M" << (int)(Result & 255);
	Instruction_Pointer += 2 + additional_IPs;
}
void CMP_Reg_RM_16()	//  CMP Reg with R/M 16 bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Src = 0;
	uint32 Result = 0;
	bool OF_Carry = false;

	//определяем SRC, это вычитаемое
	Src = *ptr_r16[(byte2 >> 3) & 7];
	if (log_to_console) cout << "CMP " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)Src << ") ";
	
	//определяем объект назначения и результат операции SUB
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		additional_IPs = 0;
		Dst = *ptr_r16[byte2 & 7];
		if (log_to_console) cout << "with " << reg16_name[byte2 & 7] << "(" << (int)Dst << ") = ";
	}
	else
	{
		mod_RM_3(byte2);
		*ptr_Dst_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		*ptr_Dst_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		if (log_to_console) cout << "with M" << OPCODE_comment << " = ";
	}

	Result = Dst - Src;
	Flag_CF = (Result >> 16) & 1;
	Flag_SF = ((Result >> 15) & 1);
	OF_Carry = ((Dst & 0x7FFF) - (Src & 0x7FFF)) >> 15;
	Flag_OF = Flag_CF ^ OF_Carry;
	if (Result & 0xFFFF) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = (parity_check[Result & 255]);
	Flag_AF = (((Dst & 0x0F) - (Src & 0x0F)) >> 4) & 1;
	if (log_to_console) cout << (int)(Result);

	Instruction_Pointer += 2 + additional_IPs;
}
//доработать
void CMP_RM_Reg_8()		//  CMP R/M with Reg 8 bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 Src = 0;
	uint16 Result = 0;
	additional_IPs = 0;
	bool OF_Carry = false;

	if (log_to_console) cout << "CMP ";
	//определяем операнд Src - вычитаемое
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Src = memory.read_2(New_Addr);
		if (log_to_console) cout << "M" << OPCODE_comment << " with ";
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Src = AX & 255;
			if (log_to_console) cout << "AL("<<(int)Src<<") with ";
			break;
		case 1:
			Src = CX & 255;
			if (log_to_console) cout << "CL(" << (int)Src << ") with ";
			break;
		case 2:
			Src = DX & 255;
			if (log_to_console) cout << "DL(" << (int)Src << ") with ";
			break;
		case 3:
			Src = BX & 255;
			if (log_to_console) cout << "BL(" << (int)Src << ") with ";
			break;
		case 4:
			Src = AX >> 8;
			if (log_to_console) cout << "AH(" << (int)Src << ") with ";
			break;
		case 5:
			Src = CX >> 8;
			if (log_to_console) cout << "CH(" << (int)Src << ") with ";
			break;
		case 6:
			Src = DX >> 8;
			if (log_to_console) cout << "DH(" << (int)Src << ") with ";
			break;
		case 7:
			Src = BX >> 8;
			if (log_to_console) cout << "BH(" << (int)Src << ") with ";
			break;
		}
		break;
	}

	//определяем объект назначения и результат операции CMP
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Result = (AX & 255) - Src;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = ((AX & 0x7F) - (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((AX & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << "AL(" << (int)(AX & 255) << ") = " << (int)(Result);
		break;
	case 1:
		Result = (CX & 255) - Src;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = ((CX & 0x7F) - (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((CX & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << "CL(" << (int)(CX & 255) << ") = " << (int)(Result);
		break;
	case 2:
		Result = (DX & 255) - Src;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = ((DX & 0x7F) - (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((DX & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << "DL(" << (int)(DX & 255) << ") = " << (int)(Result);
		break;
	case 3:
		Result = (BX & 255) - Src;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = ((BX & 0x7F) - (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((BX & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << "BL(" << (int)(BX & 255) << ") = " << (int)(Result);
		break;
	case 4:
		Result = (AX >> 8) - Src;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = (((AX >> 8) & 0x7F) - (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((((AX >> 8) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << "AH(" << (int)((AX >> 8) & 255) << ") = " << (int)(Result);
		break;
	case 5:
		Result = (CX >> 8) - Src;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = (((CX >> 8) & 0x7F) - (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((((CX >> 8) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << "CH(" << (int)((CX >> 8) & 255) << ") = " << (int)(Result);
		break;
	case 6:
		Result = (DX >> 8) - Src;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = (((DX >> 8) & 0x7F) - (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((((DX >> 8) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << "DH(" << (int)((DX >> 8) & 255) << ") = " << (int)(Result);
		break;
	case 7:
		Result = (BX >> 8) - Src;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = (((BX >> 8) & 0x7F) - (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((((BX >> 8) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << "BH(" << (int)((BX >> 8) & 255) << ") = " << (int)(Result);
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void CMP_RM_Reg_16()	//  CMP R/M with Reg 16 bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Src = 0;
	uint32 Result = 0;
	additional_IPs = 0;
	bool OF_Carry = false;

	if (log_to_console) cout << "CMP ";
	//определяем операнд Src - вычитаемое
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
		if (log_to_console) cout << "M" << OPCODE_comment << "(" << (int)Src << ") ";
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Src = AX;
			if (log_to_console) cout << "AX(" << (int)Src << ") ";
			break;
		case 1:
			Src = CX;
			if (log_to_console) cout << "CX(" << (int)Src << ") ";
			break;
		case 2:
			Src = DX;
			if (log_to_console) cout << "DX(" << (int)Src << ") ";
			break;
		case 3:
			Src = BX;
			if (log_to_console) cout << "BX(" << (int)Src << ") ";
			break;
		case 4:
			Src = Stack_Pointer;
			if (log_to_console) cout << "SP(" << (int)Src << ") ";
			break;
		case 5:
			Src = Base_Pointer;
			if (log_to_console) cout << "BP(" << (int)Src << ") ";
			break;
		case 6:
			Src = Source_Index;
			if (log_to_console) cout << "SI(" << (int)Src << ") ";
			break;
		case 7:
			Src = Destination_Index;
			if (log_to_console) cout << "DI(" << (int)Src << ") ";
			break;
		}
		break;
	}
	if (log_to_console) cout << "with ";
	//определяем объект назначения и результат операции SUB
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Result = AX - Src;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = ((AX & 0x7FFF) - (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((AX & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << " AX(" << (int) AX<< ") = " << (int)(Result);
		break;
	case 1:
		Result = CX - Src;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = ((CX & 0x7FFF) - (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((CX & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << " CX(" << (int)CX << ") = " << (int)(Result);
		break;
	case 2:
		Result = DX - Src;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = ((DX & 0x7FFF) - (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((DX & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << " DX(" << (int)DX << ") = " << (int)(Result);
		break;
	case 3:
		Result = BX - Src;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = ((BX & 0x7FFF) - (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((BX & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << " BX(" << (int)BX << ") = " << (int)(Result);
		break;
	case 4:
		Result = Stack_Pointer - Src;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = ((Stack_Pointer & 0x7FFF) - (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((Stack_Pointer & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << " SP(" << (int)Stack_Pointer << ") = " << (int)(Result);
		break;
	case 5:
		Result = Base_Pointer - Src;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = ((Base_Pointer & 0x7FFF) - (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((Base_Pointer & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << " BP(" << (int)Base_Pointer << ") = " << (int)(Result);
		break;
	case 6:
		Result = Source_Index - Src;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = ((Source_Index & 0x7FFF) - (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((Source_Index & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << " SI(" << (int)Source_Index << ") = " << (int)(Result);
		break;
	case 7:
		Result = Destination_Index - Src;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = ((Destination_Index & 0x7FFF) - (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((Destination_Index & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << " DI(" << (int)Destination_Index << ") = " << (int)(Result);
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void CMP_IMM_with_ACC_8()		// CMP IMM  8bit - ACC
{
	uint16 Result = 0;
	uint8 imm = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];
	bool OF_Carry = false;

	if (log_to_console) cout << "CMP IMM (" << (int)(imm) << ") with AL(" << (int)(AX & 255) << ") = ";

	Result = (uint16)(AX & 255) - (uint16)imm;
	Flag_CF = (Result >> 8) & 1;
	Flag_SF = (Result >> 7) & 1;
	OF_Carry = ((AX & 0x7F) - (imm & 0x7F)) >> 7;
	Flag_OF = Flag_CF ^ OF_Carry;
	if (Result & 255) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result & 255];
	Flag_AF = (((AX & 15) - (imm & 15)) >> 4) & 1;

	if (log_to_console) cout << (int)(Result & 255);

	Instruction_Pointer += 2;
}
void CMP_IMM_with_ACC_16()		// CMP IMM 16bit - ACC
{
	uint32 Result = 0;
	bool OF_Carry = false;
	uint16 imm = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory.read_2(Instruction_Pointer + 2 + *CS * 16) * 256;
	
	if (log_to_console) cout << "CMP IMM (" << (int)(imm) << ") with AX(" << (int)AX << ") = ";

	Result = (uint32)AX - (uint32)imm;
	Flag_CF = (Result >> 16) & 1;
	Flag_SF = (Result >> 15) & 1;
	OF_Carry = ((AX & 0x7FFF) - (imm & 0x7FFF)) >> 15;
	Flag_OF = Flag_CF ^ OF_Carry;
	if (Result & 0xFFFF) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result & 255];
	Flag_AF = (((AX & 15) - (imm & 15)) >> 4) & 1;
	if (log_to_console) cout << (int)(Result);

	Instruction_Pointer += 3;
}

void AAS() //AAS = ASCII Adjust for Subtract
{
	if (log_to_console) cout << "AAS " << int((AX & 0x00F0) >> 4) << "+" << int(AX & 0x000F) << " -> ";
	if (Flag_AF == 1 || (AX & 15) > 9)
	{
		AX = (AX & 0x00FF) | (((AX >> 8) - 1) << 8);
		AX = (AX & 0xFF00) | (((AX & 255) - 6) & 255);
		Flag_CF = 1;
		Flag_AF = 1;
	}
	else
	{
		Flag_CF = 0;
		Flag_AF = 0;
	}
	AX = (AX & 0xFF0F);
	if (AX & 255) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_SF = ((AX >> 7) & 1);
	if (log_to_console) cout << int((AX & 0x00F0) >> 4) << "+" << int(AX & 0x000F);
	Instruction_Pointer += 1;

}
void DAS() //DAS = Decimal Adjust for Subtract
{
	uint16 AL = AX & 255;
	uint8 low_AL = AL & 15;
	
	if (low_AL > 9 || Flag_AF)
	{
		AL = AL - 6;
		Flag_AF = true;
		if ((AL >> 8) & 1) Flag_CF = true;
	}
	
	uint8 high_AL = (AL >> 4) & 15;

	if (high_AL > 9 || Flag_CF)
	{
		AL = AL - 0x60; // -6 к старшим битам
		Flag_CF = true;
	}
	AX = (AX & 0xFF00) | (AL & 255);

	if (AX & 255) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_SF = (AL >> 7) & 1;
	Flag_PF = parity_check[AX & 255];
	Flag_OF = 0;
	if (log_to_console) cout << "DAS AL = " << (int)((AX >> 4) & 15) << " + " << (int)(AX & 15);
	Instruction_Pointer += 1;
}
void AAM() //AAM = ASCII Adjust for Multiply
{
	uint8 base = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];

	if (log_to_console) cout << "AAM AX =  ";
	if (base == 0)
	{
		//DIV 0
		exeption = 0x10;
		Instruction_Pointer += 2;
		if (log_to_console) cout << " [DIV0] ";
		return;
	}
	
	uint8 AH = (AX & 255) / base;
	uint8 AL = (AX & 255) % base;
	AX = (AH << 8) | AL;

	if (AX & 255) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_SF = ((AX >> 7) & 1);
	Flag_PF = parity_check[AX & 255];
	Flag_AF = 0;
	Flag_CF = 0;
	Flag_OF = 0;
	if (log_to_console) cout << (int)(AX >> 8) << " + " << (int)(AX & 255);
	Instruction_Pointer += 2;
}

void AAD() //AAD = ASCII Adjust for Divide
{
	uint8 base = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];
	if (log_to_console) cout << "AAD[" << dec << (int)base << hex << "] AX =  ";
	/*
	if (base == 0)
	{
		//DIV 0
		exeption = 0x10;
		Instruction_Pointer += 2;
		if (log_to_console) cout << " [DIV0] ";
		return;
	}
	*/
	
	uint8 AL = (AX & 255) + (AX >> 8) * base;
	AX = AL;

	if (AL) Flag_ZF = false;
	else Flag_ZF = true;

	Flag_SF = ((AX >> 7) & 1);
	Flag_PF = parity_check[AL];
	
	if (log_to_console) cout << (int)(AX >> 8) << " + " << (int)(AX & 255);
	Instruction_Pointer += 2;
}
void CBW() //CBW = Convert Byte to Word
{
	if (log_to_console) cout << "CBW " << (int)(AX & 255) << " -> ";
	if ((AX >> 7) & 1) AX = (255 << 8) | (AX & 255);
	else AX = AX & 255;
	if (log_to_console) cout << (int)AX;
	Instruction_Pointer += 1;
}
void CWD() //CWD = Convert Word to Double Word
{
	if (log_to_console) cout << "CWD " << (int)AX << " -> ";
	if ((AX >> 15) & 1) DX = 0xFFFF;
	else DX = 0;
	if (log_to_console) cout << (int)(AX + DX *256 *256);
	Instruction_Pointer += 1;
}

//============Logic========================================

// TEST/NOT/NEG/MUL/IMUL/DIV/IDIV  R/M 8 bit
void Invert_RM_8()			
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];//второй байт
	uint8 OP = (byte2 >> 3) & 7;
	uint16 disp = 0;		//смещение
	uint16 Src = 0;		//источник данных
	uint8 Result = 0;
	uint16 Result_16 = 0;
	additional_IPs = 0;
	uint8 rem; //остаток от деления
	bool OF_Carry = 0;
	bool old_SF = 0;

	switch (OP)
	{
	case 0:  //TEST_IMM_8
	case 1:  //недокументированный алиас
		//определяем объект назначения и результат операции
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);//IMM
			if (log_to_console) cout << "TEST IMM(" << (int)Src << ") AND ";
			Result = memory.read_2(New_Addr) & Src;
			if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)Result;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 3:
			// mod 11 источник - регистр
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "TEST IMM(" << (int)Src << ") AND ";
			switch (byte2 & 7)
			{
			case 0:
				Result = (AX & 255) & Src;
				//AX = (AX & 0xFF00) | Result;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result];
				if (log_to_console) cout << "AL(" << (int)(AX & 255) << ") = " << (int)(Result & 255);
				break;
			case 1:
				Result = (CX & 255) & Src;
				//CX = (CX & 0xFF00) | Result;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result];
				if (log_to_console) cout << "CL(" << (int)(CX & 255) << ") = " << (int)(Result & 255);
				break;
			case 2:
				Result = (DX & 255) & Src;
				//DX = (DX & 0xFF00) | Result;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result];
				if (log_to_console) cout << "DL(" << (int)(DX & 255) << ") = " << (int)(Result & 255);
				break;
			case 3:
				Result = (BX & 255) & Src;
				//BX = (BX & 0xFF00) | Result;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result];
				if (log_to_console) cout << "BL(" << (int)(BX & 255) << ") = " << (int)(Result & 255);
				break;
			case 4:
				Result = ((AX >> 8)) & Src;
				//AX = (AX & 0x00FF) | (Result << 8);
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result];
				if (log_to_console) cout << "AH(" << (int)((AX >> 8) & 255) << ") = " << (int)(Result & 255);
				break;
			case 5:
				Result = ((CX >> 8)) & Src;
				//CX = (CX & 0x00FF) | (Result << 8);
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result];
				if (log_to_console) cout << "CH(" << (int)((CX >> 8) & 255) << ") = " << (int)(Result & 255);
				break;
			case 6:
				Result = ((DX >> 8)) & Src;
				//DX = (DX & 0x00FF) | (Result << 8);
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result];
				if (log_to_console) cout << "DH(" << (int)((DX >> 8) & 255) << ") = " << (int)(Result & 255);
				break;
			case 7:
				Result = ((BX >> 8)) & Src;
				//BX = (BX & 0x00FF) | (Result << 8);
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result];
				if (log_to_console) cout << "BH(" << (int)((BX >> 8) & 255) << ") = " << (int)(Result & 255);
				break;
			}
			break;
		}
		Instruction_Pointer += 3 + additional_IPs;
		break;

	case 2:  //NOT(Invert) RM_8
		//определяем адрес или регистр
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = ~memory.read_2(New_Addr);
			memory.write_2(New_Addr,  Src & 255);
			break;
		case 3:
			// mod 11 - регистр 8bit
			switch (byte2 & 7)
			{
			case 0: //AL
				AX = (AX & 0xFF00) | (~AX & 255);
				break;
			case 1: //CL
				CX = (CX & 0xFF00) | (~CX & 255);
				break;
			case 2: //DL
				DX = (DX & 0xFF00) | (~DX & 255);
				break;
			case 3: //BL
				BX = (BX & 0xFF00) | (~BX & 255);
				break;
			case 4: //AH
				AX = (AX & 0x00FF) | (~AX & 0xFF00);
				break;
			case 5: //CH
				CX = (CX & 0x00FF) | (~CX & 0xFF00);
				break;
			case 6: //DH
				DX = (DX & 0x00FF) | (~DX & 0xFF00);
				break;
			case 7: //BH
				BX = (BX & 0x00FF) | (~BX & 0xFF00);
				break;
			}
			break;
		}
		if (log_to_console) cout << "Invert 8 bit R/M";
		Instruction_Pointer += 2 + additional_IPs;
		break;

	case 3: //NEG_8
		//определяем адрес или регистр
		if (log_to_console) cout << "NEG_8 ";
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			if (memory.read_2(New_Addr)) Flag_CF = 1;
			else Flag_CF = 0;
			if (memory.read_2(New_Addr) == 0x80) Flag_OF = 1;
			else Flag_OF = 0;
			//Flag_AF = (((~memory.read_2(New_Addr) & 15) + 1) >> 4) & 1;
			//OF_Carry = ((~memory.read_2(New_Addr) & 0x7F) + 1) >> 7;
			Src = ~memory.read_2(New_Addr) + 1;
			//Flag_CF = (Src >> 8) & 1;
			Flag_SF = ((Src >> 7) & 1);
			//Flag_OF = Flag_CF ^ OF_Carry;
			if (Src & 0xFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (Src & 15) Flag_AF = 1;
			else Flag_AF = 0;
			memory.write_2(New_Addr,  Src & 255);
			if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)(Src & 255);
			break;
		case 3:
			// mod 11 - регистр 8bit
			switch (byte2 & 7)
			{
			case 0: //AL
				if (AX & 255) Flag_CF = 1;
				else Flag_CF = 0;
				if ((AX & 255) == 0x80) Flag_OF = 1;
				else Flag_OF = 0;
				//Flag_AF = (((~AX & 15) + 1) >> 4) & 1;
				//OF_Carry = ((~AX & 0x7F) + 1) >> 7;
				Src = (~AX & 255) + 1;
				//Flag_CF = (Src >> 8) & 1;
				Flag_SF = ((Src >> 7) & 1);
				//Flag_OF = Flag_CF ^ OF_Carry;
				if (Src & 0xFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (Src & 15) Flag_AF = 1;
				else Flag_AF = 0;
				AX = (AX & 0xFF00) | (Src & 255);
				if (log_to_console) cout << " AL = " << (int)(AX & 255);
				break;
			case 1: //CL
				if (CX & 255) Flag_CF = 1;
				else Flag_CF = 0;
				if ((CX & 255) == 0x80) Flag_OF = 1;
				else Flag_OF = 0;
				//Flag_AF = (((~CX & 15) + 1) >> 4) & 1;
				//OF_Carry = ((~CX & 0x7F) + 1) >> 7;
				Src = (~CX & 255) + 1;
				//Flag_CF = (Src >> 8) & 1;
				Flag_SF = ((Src >> 7) & 1);
				//Flag_OF = Flag_CF ^ OF_Carry;
				if (Src & 0xFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (Src & 15) Flag_AF = 1;
				else Flag_AF = 0;
				CX = (CX & 0xFF00) | (Src & 255);
				if (log_to_console) cout << " CL = " << (int)(CX & 255);
				break;
			case 2: //DL
				if (DX & 255) Flag_CF = 1;
				else Flag_CF = 0;
				if ((DX & 255) == 0x80) Flag_OF = 1;
				else Flag_OF = 0;
				//Flag_AF = (((~DX & 15) + 1) >> 4) & 1;
				//OF_Carry = ((~DX & 0x7F) + 1) >> 7;
				Src = (~DX & 255) + 1;
				//Flag_CF = (Src >> 8) & 1;
				Flag_SF = ((Src >> 7) & 1);
				//Flag_OF = Flag_CF ^ OF_Carry;
				if (Src & 0xFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (Src & 15) Flag_AF = 1;
				else Flag_AF = 0;
				DX = (DX & 0xFF00) | (Src & 255);
				if (log_to_console) cout << " DL = " << (int)(DX & 255);
				break;
			case 3: //BL
				if (BX & 255) Flag_CF = 1;
				else Flag_CF = 0;
				if ((BX & 255) == 0x80) Flag_OF = 1;
				else Flag_OF = 0;
				//Flag_AF = (((~BX & 15) + 1) >> 4) & 1;
				//OF_Carry = ((~BX & 0x7F) + 1) >> 7;
				Src = (~BX & 255) + 1;
				//Flag_CF = (Src >> 8) & 1;
				Flag_SF = ((Src >> 7) & 1);
				//Flag_OF = Flag_CF ^ OF_Carry;
				if (Src & 0xFF) Flag_ZF = false;
				else Flag_ZF = true;
				BX = (BX & 0xFF00) | (Src & 255);
				Flag_PF = parity_check[Src & 255];
				if (Src & 15) Flag_AF = 1;
				else Flag_AF = 0;
				if (log_to_console) cout << " BL = " << (int)(BX & 255);
				break;
			case 4: //AH
				if ((AX >> 8) & 255) Flag_CF = 1;
				else Flag_CF = 0;
				if (((AX >> 8) & 255) == 0x80) Flag_OF = 1;
				else Flag_OF = 0;
				//Flag_AF = (((~(AX >> 8) & 15) + 1) >> 4) & 1;
				//OF_Carry = ((~(AX >> 8) & 0x7F) + 1) >> 7;
				Src = (~AX >> 8) + 1;
				//Flag_CF = (Src >> 8) & 1;
				Flag_SF = ((Src >> 7) & 1);
				//Flag_OF = Flag_CF ^ OF_Carry;
				if (Src & 0xFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (Src & 15) Flag_AF = 1;
				else Flag_AF = 0;
				AX = (AX & 0x00FF) | (Src << 8);
				if (log_to_console) cout << " AH = " << (int)(AX >> 8);
				break;
			case 5: //CH
				if ((CX >> 8) & 255) Flag_CF = 1;
				else Flag_CF = 0;
				if (((CX >> 8) & 255) == 0x80) Flag_OF = 1;
				else Flag_OF = 0;
				//Flag_AF = (((~(CX >> 8) & 15) + 1) >> 4) & 1;
				//OF_Carry = ((~(CX >> 8) & 0x7F) + 1) >> 7;
				Src = (~CX >> 8) + 1;
				//Flag_CF = (Src >> 8) & 1;
				Flag_SF = ((Src >> 7) & 1);
				//Flag_OF = Flag_CF ^ OF_Carry;
				if (Src & 0xFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (Src & 15) Flag_AF = 1;
				else Flag_AF = 0;
				CX = (CX & 0x00FF) | (Src << 8);
				if (log_to_console) cout << " CH = " << (int)(CX >> 8);
				break;
			case 6: //DH
				if ((DX >> 8) & 255) Flag_CF = 1;
				else Flag_CF = 0;
				if (((DX >> 8) & 255) == 0x80) Flag_OF = 1;
				else Flag_OF = 0;
				//Flag_AF = (((~(DX >> 8) & 15) + 1) >> 4) & 1;
				//OF_Carry = ((~(DX >> 8) & 0x7F) + 1) >> 7;
				Src = (~DX >> 8) + 1;
				//Flag_CF = (Src >> 8) & 1;
				Flag_SF = ((Src >> 7) & 1);
				//Flag_OF = Flag_CF ^ OF_Carry;
				if (Src & 0xFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (Src & 15) Flag_AF = 1;
				else Flag_AF = 0;
				DX = (DX & 0x00FF) | (Src << 8);
				if (log_to_console) cout << " DH = " << (int)(DX >> 8);
				break;
			case 7: //BH
				if ((BX >> 8) & 255) Flag_CF = 1;
				else Flag_CF = 0;
				if (((BX >> 8) & 255) == 0x80) Flag_OF = 1;
				else Flag_OF = 0;
				//Flag_AF = (((~(BX >> 8) & 15) + 1) >> 4) & 1;
				//OF_Carry = ((~(BX >> 8) & 0x7F) + 1) >> 7;
				Src = (~BX >> 8) + 1;
				//Flag_CF = (Src >> 8) & 1;
				Flag_SF = ((Src >> 7) & 1);
				//Flag_OF = Flag_CF ^ OF_Carry;
				if (Src & 0xFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (Src & 15) Flag_AF = 1;
				else Flag_AF = 0;
				BX = (BX & 0x00FF) | (Src << 8);
				if (log_to_console) cout << " BH = " << (int)(BX >> 8);
				break;
			}
			break;
		}
		//if (log_to_console) cout << "NEG 8 bit R/M";
		Instruction_Pointer += 2 + additional_IPs;
		break;
		
	case 4: //MUL = Multiply (Unsigned)

		if (log_to_console) cout << "MUL AL(" << int(AX & 255) << ") x ";
		//определяем множитель
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(New_Addr);
			if (log_to_console) cout << "M" << OPCODE_comment << "=";
			break;
		case 3:
			// mod 11 - регистр 8bit
			switch (byte2 & 7)
			{
			case 0: //AL
				Src = (AX & 255);
				if (log_to_console) cout << "AL(" << int(Src) << ")=";
				break;
			case 1: //CL
				Src = (CX & 255);
				if (log_to_console) cout << "CL(" << int(Src) << ")=";
				break;
			case 2: //DL
				Src = (DX & 255);
				if (log_to_console) cout << "DL(" << int(Src) << ")=";
				break;
			case 3: //BL
				Src = (BX & 255);
				if (log_to_console) cout << "BL(" << int(Src) << ")=";
				break;
			case 4: //AH
				Src = (AX >> 8) & 255;
				if (log_to_console) cout << "AH(" << int(Src) << ")=";
				break;
			case 5: //CH
				Src = (CX >> 8) & 255;
				if (log_to_console) cout << "CH(" << int(Src) << ")=";
				break;
			case 6: //DH
				Src = (DX >> 8) & 255;
				if (log_to_console) cout << "DH(" << int(Src) << ")=";
				break;
			case 7: //BH
				Src = (BX >> 8) & 255;
				if (log_to_console) cout << "BH(" << int(Src) << ")=";
				break;
			}
			break;
		}

		AX = (AX & 255) * Src;
		if (AX >> 8)
		{
			Flag_CF = true; Flag_OF = true; Flag_ZF = 0;
		}
		else 
		{
			Flag_CF = false; Flag_OF = false; Flag_ZF = 1;
		}

		Flag_SF = ((AX >> 8) >> 7) & 1;
		Flag_PF = parity_check[(AX >> 8)];
		if (log_to_console) cout << (int)(AX);
		Instruction_Pointer += 2 + additional_IPs;
		break;

	case 5: //IMUL = Integer Multiply(Signed)
		if (log_to_console) cout << "IMUL AL(" << int(AX & 255) << ") * ";
		//определяем множитель
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(New_Addr);
			if (log_to_console) cout << "M" << OPCODE_comment << " = ";
			break;
		case 3:
			// mod 11 - регистр 8bit
			switch (byte2 & 7)
			{
			case 0: //AL
				Src = (AX & 255);
				if (log_to_console) cout << "AL(" << int(Src) << ") = ";
				break;
			case 1: //CL
				Src = (CX & 255);
				if (log_to_console) cout << "CL(" << int(Src) << ") = ";
				break;
			case 2: //DL
				Src = (DX & 255);
				if (log_to_console) cout << "DL(" << int(Src) << ") = ";
				break;
			case 3: //BL
				Src = (BX & 255);
				if (log_to_console) cout << "BL(" << int(Src) << ") = ";
				break;
			case 4: //AH
				Src = (AX >> 8) & 255;
				if (log_to_console) cout << "AH(" << int(Src) << ") = ";
				break;
			case 5: //CH
				Src = (CX >> 8) & 255;
				if (log_to_console) cout << "CH(" << int(Src) << ") = ";
				break;
			case 6: //DH
				Src = (DX >> 8) & 255;
				if (log_to_console) cout << "DH(" << int(Src) << ") = ";
				break;
			case 7: //BH
				Src = (BX >> 8) & 255;
				if (log_to_console) cout << "BH(" << int(Src) << ") = ";
				break;
			}
			break;
		}

		AX = ((__int8)(AX & 0xFF) * (__int8)Src);
		if ((AX >> 7) == 0 || (AX >> 7) == 0b111111111)
		{
			Flag_CF = false; Flag_OF = false;
		}
		else
		{
			Flag_CF = true; Flag_OF = true; 
		}
		if (AX >> 8) Flag_ZF = 0;
		else Flag_ZF = 1;
		Flag_SF = (AX >> 15) & 1;
		Flag_PF = parity_check[AX >> 8];
		if (log_to_console) cout << (__int16)(AX);
		Instruction_Pointer += 2 + additional_IPs;
		break;
	
	case 6: //DIV = Divide (Unsigned)

		if (log_to_console) cout << "DIV AX(" << int(AX) << ") : ";
		//определяем делитель
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(New_Addr);
			if (log_to_console) cout << "M[" << (int)(New_Addr) << "](" << (int)Src << ") = ";
			break;
		case 3:
			// mod 11 - регистр 8bit
			switch (byte2 & 7)
			{
			case 0: //AL
				Src = (AX & 255);
				if (log_to_console) cout << "AL(" << int(Src) << ") = ";
				break;
			case 1: //CL
				Src = (CX & 255);
				if (log_to_console) cout << "CL(" << int(Src) << ") = ";
				break;
			case 2: //DL
				Src = (DX & 255);
				if (log_to_console) cout << "DL(" << int(Src) << ") = ";
				break;
			case 3: //BL
				Src = (BX & 255);
				if (log_to_console) cout << "BL(" << int(Src) << ") = ";
				break;
			case 4: //AH
				Src = (AX >> 8);
				if (log_to_console) cout << "AH(" << int(Src) << ") = ";
				break;
			case 5: //CH
				Src = (CX >> 8);
				if (log_to_console) cout << "CH(" << int(Src) << ") = ";
				break;
			case 6: //DH
				Src = (DX >> 8);
				if (log_to_console) cout << "DH(" << int(Src) << ") = ";
				break;
			case 7: //BH
				Src = (BX >> 8);
				if (log_to_console) cout << "BH(" << int(Src) << ") = ";
				break;
			}
			break;
		}

		//бросаем исключение при делении на 0
		if (Src == 0) {
			exeption = 0x10;
			Instruction_Pointer += (2 + additional_IPs);
			if (log_to_console) cout << " [DIV0] ";
			return;
		}

		rem = AX % Src;
		Result_16 = AX / Src;

		//бросаем исключение при переполнении
		if (Result_16 > 0xFF) {
			exeption = 0x10;
			Instruction_Pointer += (2 + additional_IPs);
			if (log_to_console) cout << " [OVERFLOW] ";
			return;
		}

		AX = (rem << 8) | (Result_16);

		if (log_to_console) cout << (int)(Result_16) << " rem " << (int)rem;
		Instruction_Pointer += 2 + additional_IPs;
		break;

	case 7: //IDIV = Integer Divide (Signed)
		
		if (log_to_console) cout << "IDIV AX(" << setw(4) << (int)AX << ") : ";
		//определяем делитель
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(New_Addr);
			if (log_to_console) cout << "M" << OPCODE_comment << "(" << (int)Src << ") = ";
			break;
		case 3:
			// mod 11 - регистр 8bit
			switch (byte2 & 7)
			{
			case 0: //AL
				Src = (AX & 255);
				if (log_to_console) cout << "AL(" << setw(2) << (int)Src << ")=";
				break;
			case 1: //CL
				Src = (CX & 255);
				if (log_to_console) cout << "CL(" << setw(2) << (int)Src << ")=";
				break;
			case 2: //DL
				Src = (DX & 255);
				if (log_to_console) cout << "DL(" << setw(2) << (int)Src << ")=";
				break;
			case 3: //BL
				Src = (BX & 255);
				if (log_to_console) cout << "BL(" << setw(2) << (int)Src << ")=";
				break;
			case 4: //AH
				Src = (AX >> 8) & 255;
				if (log_to_console) cout << "AH(" << setw(2) << (int)Src << ")=";
				break;
			case 5: //CH
				Src = (CX >> 8) & 255;
				if (log_to_console) cout << "CH(" << setw(2) << (int)Src << ")=";
				break;
			case 6: //DH
				Src = (DX >> 8) & 255;
				if (log_to_console) cout << "DH(" << setw(2) << (int)Src << ")=";
				break;
			case 7: //BH
				Src = (BX >> 8) & 255;
				if (log_to_console) cout << "BH(" << setw(2) << (int)Src << ")=";
				break;
			}
			break;
		}

		//бросаем исключение при делении на 0
		if (Src == 0) {
			exeption = 0x10;
			Instruction_Pointer += (2 + additional_IPs);
			if (log_to_console) cout << " [DIV0] ";
			negate_IDIV = 0; //убираем флаг инверсии знака
			return;
		}

		int quotient = ((__int16)AX / (__int8)Src);
		//меняем знак частного если перед командой стоит REP
		if (negate_IDIV)
		{
			quotient = -quotient;
			negate_IDIV = 0;
			if (log_to_console) cout << " [NEG] ";
		}
		
		//бросаем исключение при переполнении
		if (abs(quotient) > 0x7F) {
			exeption = 0x10;
			Instruction_Pointer += (2 + additional_IPs);
			if (log_to_console) cout << " [OVERFLOW] ";
			return;
		}

		rem = ((__int16)AX % (__int8)Src);
		AX = (quotient & 255) | (rem << 8);
		if (log_to_console) cout << setw(2) << (int)(AX & 255) << " rem " << (int)rem;
		Instruction_Pointer += 2 + additional_IPs;
		break;
	}
	
}
// TEST/NOT/NEG/MUL/IMUL/DIV/IDIV   16 bit
void Invert_RM_16()		
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];//второй байт
	uint8 OP = (byte2 >> 3) & 7;
	uint16 disp = 0;		//смещение
	uint16 Src = 0;			//источник данных
	uint16 Result = 0;
	uint32 Result_32 = 0;
	additional_IPs = 0;
	uint16 rem = 0;   //остаток деления
	bool OF_Carry;

	switch (OP)
	{
	case 0:  //TEST_IMM_16
	case 1:  //Alias
		//определяем объект назначения и результат операции
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16) + memory.read_2(Instruction_Pointer + 3 + additional_IPs + *CS * 16) * 256;
			if (log_to_console) cout << "TEST IMM[" << (int)Src << "] AND ";
			Result = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & Src;
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)Result;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 3:
			// mod 11 источник - регистр
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16) + memory.read_2(Instruction_Pointer + 3 + additional_IPs + *CS * 16) * 256;
			if (log_to_console) cout << "TEST IMM[" << (int)Src << "] AND ";
			switch (byte2 & 7)
			{
			case 0:
				Result = AX & Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 15) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				if (log_to_console) cout << " AX = " << (int)Result;
				Flag_PF = parity_check[Result & 255];
				break;
			case 1:
				Result = CX & Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 15) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				if (log_to_console) cout << " CX = " << (int)Result;
				Flag_PF = parity_check[Result & 255];
				break;
			case 2:
				Result = DX & Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 15) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				if (log_to_console) cout << " DX = " << (int)Result;
				Flag_PF = parity_check[Result & 255];
				break;
			case 3:
				Result = BX & Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 15) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				if (log_to_console) cout << " BX = " << (int)Result;
				Flag_PF = parity_check[Result & 255];
				break;
			case 4:
				Result = Stack_Pointer & Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 15) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				if (log_to_console) cout << " SP = " << (int)Result;
				Flag_PF = parity_check[Result & 255];
				break;
			case 5:
				Result = Base_Pointer & Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 15) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				if (log_to_console) cout << " BP = " << (int)Result;
				Flag_PF = parity_check[Result & 255];
				break;
			case 6:
				Result = Source_Index & Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 15) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				if (log_to_console) cout << " SI = " << (int)Result;
				Flag_PF = parity_check[Result & 255];
				break;
			case 7:
				Result = Destination_Index & Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 15) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				if (log_to_console) cout << " DI = " << (int)Result;
				Flag_PF = parity_check[Result & 255];
				break;
			}
			break;
		}
		Instruction_Pointer += 4 + additional_IPs;
		break;

	case 2: //NOT (Invert) 16 R/M

		//определяем адрес или регистр
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = ~memory.read_2(New_Addr);
			memory.write_2(New_Addr,  Src & 255);
			Src = ~memory.read_2(New_Addr + 1);
			memory.write_2(New_Addr + 1,  Src & 255);
			break;
		case 3:
			// mod 11 - регистр 16bit
			switch (byte2 & 7)
			{
			case 0:
				AX = ~AX;
				break;
			case 1:
				CX = ~CX;
				break;
			case 2:
				DX = ~DX;
				break;
			case 3:
				BX = ~BX;
				break;
			case 4:
				Stack_Pointer = ~Stack_Pointer;
				break;
			case 5:
				Base_Pointer = ~Base_Pointer;
				break;
			case 6:
				Source_Index = ~Source_Index;
				break;
			case 7:
				Destination_Index = ~Destination_Index;
				break;
			}
			break;
		}
		if (log_to_console) cout << "Invert 16 bit R/M";
		Instruction_Pointer += 2 + additional_IPs;
		break;

	case 3:  //NEG 16 R/M
		if (log_to_console) cout << "NEG_16 ";
		//определяем адрес или регистр
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
			if (Src) Flag_CF = 1;
			else Flag_CF = 0;
			if (Src == 0x8000) Flag_OF = 1;
			else Flag_OF = 0;
			Result_32 = ~Src + 1;
			Flag_SF = ((Result_32 >> 15) & 1);
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (Result_32 & 15) Flag_AF = 1;
			else Flag_AF = 0;
			memory.write_2(New_Addr,  Result_32 & 255);
			memory.write_2(New_Addr + 1,  (Result_32 >> 8) & 255);
			if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)(Src & 255);
			break;
		case 3:
			// mod 11 - регистр 16bit
			switch (byte2 & 7)
			{
			case 0:
				if (AX) Flag_CF = 1;
				else Flag_CF = 0;
				if (AX == 0x8000) Flag_OF = 1;
				else Flag_OF = 0;
				Result_32 = ~AX + 1;
				Flag_SF = (Result_32 >> 15) & 1;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (Result_32 & 15) Flag_AF = 1;
				else Flag_AF = 0;
				AX = Result_32;
				if (log_to_console) cout << " AX = " << (int)AX;
				break;
			case 1:
				if (CX) Flag_CF = 1;
				else Flag_CF = 0;
				if (CX == 0x8000) Flag_OF = 1;
				else Flag_OF = 0;
				Result_32 = ~CX + 1;
				Flag_SF = (Result_32 >> 15) & 1;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (Result_32 & 15) Flag_AF = 1;
				else Flag_AF = 0;
				CX = Result_32;
				if (log_to_console) cout << " CX = " << (int)CX;
				break;
			case 2:
				if (DX) Flag_CF = 1;
				else Flag_CF = 0;
				if (DX == 0x8000) Flag_OF = 1;
				else Flag_OF = 0;
				Result_32 = ~DX + 1;
				Flag_SF = (Result_32 >> 15) & 1;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (Result_32 & 15) Flag_AF = 1;
				else Flag_AF = 0;
				DX = Result_32;
				if (log_to_console) cout << " DX = " << (int)DX;
				break;
			case 3:
				if (BX) Flag_CF = 1;
				else Flag_CF = 0;
				if (BX == 0x8000) Flag_OF = 1;
				else Flag_OF = 0;
				Result_32 = ~BX + 1;
				Flag_SF = (Result_32 >> 15) & 1;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (Result_32 & 15) Flag_AF = 1;
				else Flag_AF = 0;
				BX = Result_32;
				if (log_to_console) cout << " BX = " << (int)BX;
				break;
			case 4:
				if (Stack_Pointer) Flag_CF = 1;
				else Flag_CF = 0;
				if (Stack_Pointer == 0x8000) Flag_OF = 1;
				else Flag_OF = 0;
				Result_32 = ~Stack_Pointer + 1;
				Flag_SF = (Result_32 >> 15) & 1;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (Result_32 & 15) Flag_AF = 1;
				else Flag_AF = 0;
				Stack_Pointer = Result_32;
				if (log_to_console) cout << " SP = " << (int)Stack_Pointer;
				break;
			case 5:
				if (Base_Pointer) Flag_CF = 1;
				else Flag_CF = 0;
				if (Base_Pointer == 0x8000) Flag_OF = 1;
				else Flag_OF = 0;
				Result_32 = ~Base_Pointer + 1;
				Flag_SF = (Result_32 >> 15) & 1;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (Result_32 & 15) Flag_AF = 1;
				else Flag_AF = 0;
				Base_Pointer = Result_32;
				if (log_to_console) cout << " BP = " << (int)Base_Pointer;
				break;
			case 6:
				if (Source_Index) Flag_CF = 1;
				else Flag_CF = 0;
				if (Source_Index == 0x8000) Flag_OF = 1;
				else Flag_OF = 0;
				Result_32 = ~Source_Index + 1;
				Flag_SF = (Result_32 >> 15) & 1;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (Result_32 & 15) Flag_AF = 1;
				else Flag_AF = 0;
				Source_Index = Result_32;
				if (log_to_console) cout << " SI = " << (int)Source_Index;
				break;
			case 7:
				if (Destination_Index) Flag_CF = 1;
				else Flag_CF = 0;
				if (Destination_Index == 0x8000) Flag_OF = 1;
				else Flag_OF = 0;
				Result_32 = ~Destination_Index + 1;
				Flag_SF = (Result_32 >> 15) & 1;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (Result_32 & 15) Flag_AF = 1;
				else Flag_AF = 0;
				Destination_Index = Result_32;
				if (log_to_console) cout << " DI = " << (int)Destination_Index;
				break;
			}
			break;
		}
		
		Instruction_Pointer += 2 + additional_IPs;
		break;

	case 4: //MUL = Multiply (Unsigned)

		if (log_to_console) cout << "MUL AX(" << int(AX) << ")*";
		//определяем множитель
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
			if (log_to_console) cout << "M" << OPCODE_comment << " = ";
			break;
		case 3:
			// mod 11 - регистр 16bit
			switch (byte2 & 7)
			{
			case 0: //AX
				Src = AX;
				if (log_to_console) cout << "AX(" << int(AX) << ")=";
				break;
			case 1: //CX
				Src = CX;
				if (log_to_console) cout << "CX(" << int(CX) << ")=";
				break;
			case 2: //DX
				Src = DX;
				if (log_to_console) cout << "DX(" << int(DX) << ")=";
				break;
			case 3: //BX
				Src = BX;
				if (log_to_console) cout << "BX(" << int(BX) << ")=";
				break;
			case 4: //SP
				Src = Stack_Pointer;
				if (log_to_console) cout << "SP(" << int(Stack_Pointer) << ")=";
				break;
			case 5: //BP
				Src = Base_Pointer;
				if (log_to_console) cout << "BP(" << int(Base_Pointer) << ")=";
				break;
			case 6: //SI
				Src = Source_Index;
				if (log_to_console) cout << "SI(" << int(Source_Index) << ")=";
				break;
			case 7: //DI
				Src = Destination_Index;
				if (log_to_console) cout << "DI(" << int(Destination_Index) << ")=";
				break;
			}
			break;
		}

		Result_32 = AX * Src;
		DX = Result_32 >> 16;
		AX = Result_32 & 0xFFFF;
		if (DX)
		{
			Flag_CF = true; Flag_OF = true; Flag_ZF = false;
		}
		else
		{
			Flag_CF = false; Flag_OF = false; Flag_ZF = true;
		}
		Flag_SF = ((DX >> 8) >> 7) & 1;
		Flag_PF = parity_check[(DX & 255)];
		if (log_to_console) cout << (uint32)(Result_32);
		Instruction_Pointer += 2 + additional_IPs;
		break;

	case 5: //IMUL = Integer Multiply(Signed)
		if (log_to_console) cout << "IMUL AX(" << __int16(AX) << ") * ";
		//определяем множитель
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
			if (log_to_console) cout << "M" << OPCODE_comment << "=";
			break;
		case 3:
			// mod 11 - регистр 16bit
			switch (byte2 & 7)
			{
			case 0: //AX
				Src = AX;
				if (log_to_console) cout << "AX(" << __int16(Src) << ")=";
				break;
			case 1: //CX
				Src = CX;
				if (log_to_console) cout << "CX(" << __int16(Src) << ")=";
				break;
			case 2: //DX
				Src = DX;
				if (log_to_console) cout << "DX(" << __int16(Src) << ")=";
				break;
			case 3: //BX
				Src = BX;
				if (log_to_console) cout << "BX(" << __int16(Src) << ")=";
				break;
			case 4: //SP
				Src = Stack_Pointer;
				if (log_to_console) cout << "SP(" << __int16(Src) << ")=";
				break;
			case 5: //BP
				Src = Base_Pointer;
				if (log_to_console) cout << "BP(" << __int16(Src) << ")=";
				break;
			case 6: //SI
				Src = Source_Index;
				if (log_to_console) cout << "SI(" << __int16(Src) << ")=";
				break;
			case 7: //DI
				Src = Destination_Index;
				if (log_to_console) cout << "DI(" << __int16(Src) << ")=";
				break;
			}
			break;
		}
		
		Result_32 = ((__int16)AX * (__int16)Src);

		DX = Result_32 >> 16;
		AX = Result_32 & 0xFFFF;
				
		if ((Result_32 >> 15) == 0 || (Result_32 >> 15) == 0x1FFFF)
		{
			Flag_CF = 0; Flag_OF = 0;
		}
		else
		{
			Flag_CF = 1; Flag_OF = 1;
		}
		if (DX) Flag_ZF = 0;
		else Flag_ZF = 1;
		Flag_SF = (DX >> 15) & 1;
		Flag_PF = parity_check[DX & 255];
		if (log_to_console) cout << (__int32)(Result_32);
		Instruction_Pointer += 2 + additional_IPs;
		break;

	case 6: //DIV = Divide (Unsigned)

		if (log_to_console) cout << "DIV DXAX(" << int(DX * 256 * 256 + AX) << ") : ";
		//определяем делитель
		switch (byte2 >> 6)
		{
		case 0:
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
			if (log_to_console) cout << "M" << OPCODE_comment << " = ";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;;
			if (log_to_console) cout << "M" << OPCODE_comment << " = ";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
			if (log_to_console) cout << "M" << OPCODE_comment << " = ";
			break;
		case 3:
			// mod 11 - регистр 16bit
			switch (byte2 & 7)
			{
			case 0: //AX
				Src = AX;
				if (log_to_console) cout << "AX(" << int(Src) << ") = ";
				break;
			case 1: //CX
				Src = CX;
				if (log_to_console) cout << "CX(" << int(Src) << ") = ";
				break;
			case 2: //DX
				Src = DX;
				if (log_to_console) cout << "DX(" << int(Src) << ") = ";
				break;
			case 3: //BX
				Src = BX;
				if (log_to_console) cout << "BX(" << int(Src) << ") = ";
				break;
			case 4: //SP
				Src = Stack_Pointer;
				if (log_to_console) cout << "SP(" << int(Src) << ") = ";
				break;
			case 5: //BP
				Src = Base_Pointer;
				if (log_to_console) cout << "BP(" << int(Src) << ") = ";
				break;
			case 6: //SI
				Src = Source_Index;
				if (log_to_console) cout << "SI(" << int(Src) << ") = ";
				break;
			case 7: //DI
				Src = Destination_Index;
				if (log_to_console) cout << "DI(" << int(Src) << ") = ";
				break;
			}
			break;
		}

		//бросаем исключение при делении на 0
		if (Src == 0) {
			exeption = 0x10;
			if (log_to_console) cout << " [DIV 0] ";
			Instruction_Pointer += 2 + additional_IPs;
			return;
		}

		Result_32 = ((uint32)(DX * 256 * 256 + AX) / Src);
		//бросаем исключение при переполнении
		if (Result_32 > 0xFFFF) {
			exeption = 0x10;
			if (log_to_console) cout << " [OVERFLOW] ";
			Instruction_Pointer += 2 + additional_IPs;
			return;
		}

		rem = (uint32)(DX * 256 * 256 + AX) % Src; //остаток от деления
		AX = Result_32;
		DX = rem;
		if (log_to_console) cout << (int)(AX) << " rem " << (int)rem;
		Instruction_Pointer += 2 + additional_IPs;
		break;

	case 7: //IDIV = Integer Divide (Signed)

		if (log_to_console) cout << "IDIV DXAX(" << setw(8) << (int)(DX * 256 * 256 + AX) << ") : ";
		//определяем делитель
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
			if (log_to_console) cout << "M" << OPCODE_comment << "(" << (int)Src << ") = ";
			break;
		case 3:
			// mod 11 - регистр 16bit
			switch (byte2 & 7)
			{
			case 0: //AX
				Src = AX;
				if (log_to_console) cout << "AX(" << int(Src) << ") = ";
				break;
			case 1: //CX
				Src = CX;
				if (log_to_console) cout << "CX(" << int(Src) << ") = ";
				break;
			case 2: //DX
				Src = DX;
				if (log_to_console) cout << "DX(" << int(Src) << ") = ";
				break;
			case 3: //BX
				Src = BX;
				if (log_to_console) cout << "BX(" << int(Src) << ") = ";
				break;
			case 4: //SP
				Src = Stack_Pointer;
				if (log_to_console) cout << "SP(" << int(Src) << ") = ";
				break;
			case 5: //BP
				Src = Base_Pointer;
				if (log_to_console) cout << "BP(" << int(Src) << ") = ";
				break;
			case 6: //SI
				Src = Source_Index;
				if (log_to_console) cout << "SI(" << int(Src) << ") = ";
				break;
			case 7: //DI
				Src = Destination_Index;
				if (log_to_console) cout << "DI(" << int(Src) << ") = ";
				break;
			}
			break;
		}

		//бросаем исключение при делении на 0
		if (Src == 0) {
			exeption = 0x10;
			Instruction_Pointer += (2 + additional_IPs);
			if (log_to_console) cout << " [DIV0] ";
			negate_IDIV = 0; //убираем флаг инверсии знака
			return;
		}
		
		__int32 quotient = (__int32)(AX + DX * 256 * 256) / (__int16)Src;
		//меняем знак частного если перед командой стоит REP
		if (negate_IDIV)
		{
			quotient = -quotient;
			negate_IDIV = 0;
			if (log_to_console) cout << " [NEG] ";
		}
		
		//бросаем исключение при переполнении
		if (abs(quotient) > 0x7FFF) {
			exeption = 0x10;
			Instruction_Pointer += (2 + additional_IPs);
			if (log_to_console) cout << " [OVERFLOW] ";
			return;
		}

		rem = (__int32)(AX + DX * 256 * 256) % (__int16)Src; //остаток от деления
		AX = quotient;
		DX = rem;
		if (log_to_console) cout << (uint16)(AX) << " rem " << (uint16)rem;
		Instruction_Pointer += 2 + additional_IPs;
		break;
	}
	
}
void SHL_ROT_8()			// Shift/ROL	8bit / once
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 Result = 0;
	uint16 Src = 0;
	uint8 MSB = 0;
	additional_IPs = 0;

	switch ((byte2 >> 3) & 7)
	{
	case 0:  //ROL Rotate Left

		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(New_Addr);
			Flag_CF = (Src >> 7) & 1;
			Src = (Src << 1) | Flag_CF;
			Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
			memory.write_2(New_Addr,  Src & 255);
			if (log_to_console) cout << "ROL M" << OPCODE_comment << "";
			break;
		case 3:
			// mod 11 источник - регистр
			switch (byte2 & 7)
			{
			case 0:
				Src = AX & 255;
				Flag_CF = (Src >> 7) & 1;
				Src = (Src << 1) | Flag_CF;
				AX = (AX & 0xFF00) | (Src & 255);
				Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "ROL AL";
				break;
			case 1:
				Src = CX & 255;
				Flag_CF = (Src >> 7) & 1;
				Src = (Src << 1) | Flag_CF;
				CX = (CX & 0xFF00) | (Src & 255);
				Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "ROL CL";
				break;
			case 2:
				Src = DX & 255;
				Flag_CF = (Src >> 7) & 1;
				Src = (Src << 1) | Flag_CF;
				DX = (DX & 0xFF00) | (Src & 255);
				Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "ROL DL";
				break;
			case 3:
				Src = BX & 255;
				Flag_CF = (Src >> 7) & 1;
				Src = (Src << 1) | Flag_CF;
				BX = (BX & 0xFF00) | (Src & 255);
				Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "ROL BL";
				break;
			case 4:
				Src = (AX >> 8) & 255;
				Flag_CF = (Src >> 7) & 1;
				Src = (Src << 1) | Flag_CF;
				AX = (AX & 0x00FF) | ((Src & 255) << 8);
				Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "ROL AH";
				break;
			case 5:
				Src = (CX >> 8) & 255;
				Flag_CF = (Src >> 7) & 1;
				Src = (Src << 1) | Flag_CF;
				CX = (CX & 0x00FF) | ((Src & 255) << 8);
				Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "ROL CH";
				break;
			case 6:
				Src = (DX >> 8) & 255;
				Flag_CF = (Src >> 7) & 1;
				Src = (Src << 1) | Flag_CF;
				DX = (DX & 0x00FF) | ((Src & 255) << 8);
				Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "ROL DH";
				break;
			case 7:
				Src = (BX >> 8) & 255;
				Flag_CF = (Src >> 7) & 1;
				Src = (Src << 1) | Flag_CF;
				BX = (BX & 0x00FF) | ((Src & 255) << 8);
				Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "ROL BH";
				break;
			}
			break;
		}
		break;

	case 1:  //ROR = Rotate Right

		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Flag_CF = (memory.read_2(New_Addr)) & 1;
			Src = (Flag_CF * 0x80) | (memory.read_2(New_Addr) >> 1);
			memory.write_2(New_Addr,  Src & 255);
			Flag_OF = !parity_check[Src & 0b11000000];
			if (log_to_console) cout << "ROR M" << OPCODE_comment << "";
			break;
		case 3:
			// mod 11 источник - регистр
			switch (byte2 & 7)
			{
			case 0:
				Flag_CF = (AX) & 1;
				Src = (Flag_CF * 0x80) | ((AX & 255) >> 1);
				AX = (AX & 0xFF00) | (Src & 255);
				Flag_OF = !parity_check[Src & 0b11000000];
				if (log_to_console) cout << "ROR AL = " << (int)Src;
				break;
			case 1:
				Flag_CF = (CX) & 1;
				Src = (Flag_CF * 0x80) | ((CX & 255) >> 1);
				CX = (CX & 0xFF00) | (Src & 255);
				Flag_OF = !parity_check[Src & 0b11000000];
				if (log_to_console) cout << "ROR CL = " << (int)Src;
				break;
			case 2:
				Flag_CF = (DX) & 1;
				Src = (Flag_CF * 0x80) | ((DX & 255) >> 1);
				DX = (DX & 0xFF00) | (Src & 255);
				Flag_OF = !parity_check[Src & 0b11000000];
				if (log_to_console) cout << "ROR DL = " << (int)Src;
				break;
			case 3:
				Flag_CF = (BX) & 1;
				Src = (Flag_CF * 0x80) | ((BX & 255) >> 1);
				BX = (BX & 0xFF00) | (Src & 255);
				Flag_OF = !parity_check[Src & 0b11000000];
				if (log_to_console) cout << "ROR BL = " << (int)Src;
				break;
			case 4:
				Flag_CF = (AX >> 8) & 1;
				Src = (Flag_CF * 0x80) | (AX >> 9);
				AX = (AX & 0x00FF) | (Src << 8);
				Flag_OF = !parity_check[Src & 0b11000000];
				if (log_to_console) cout << "ROR AH = " << (int)Src;
				break;
			case 5:
				Flag_CF = (CX >> 8) & 1;
				Src = (Flag_CF * 0x80) | (CX >> 9);
				CX = (CX & 0x00FF) | (Src << 8);
				Flag_OF = !parity_check[Src & 0b11000000];
				if (log_to_console) cout << "ROR CH = " << (int)Src;
				break;
			case 6:
				Flag_CF = (DX >> 8) & 1;
				Src = (Flag_CF * 0x80) | (DX >> 9);
				DX = (DX & 0x00FF) | (Src << 8);
				Flag_OF = !parity_check[Src & 0b11000000];
				if (log_to_console) cout << "ROR DH = " << (int)Src;
				break;
			case 7:
				Flag_CF = (BX >> 8) & 1;
				Src = (Flag_CF * 0x80) | (BX >> 9);
				BX = (BX & 0x00FF) | (Src << 8);
				Flag_OF = !parity_check[Src & 0b11000000];
				if (log_to_console) cout << "ROR BH = " << (int)Src;
				break;
			}
			break;
		}
		break;
	
	case 2:  //RCL Rotate Left throught carry

		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = (memory.read_2(New_Addr) << 1) | Flag_CF;
			Flag_CF = (Src >> 8) & 1;
			memory.write_2(New_Addr,  Src & 255);
			Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
			if (log_to_console) cout << "RCL M" << OPCODE_comment << "";
			break;
		case 3:
			// mod 11 источник - регистр
			switch (byte2 & 7)
			{
			case 0:
				Src = ((AX & 255) << 1) + Flag_CF;
				Flag_CF = (Src >> 8) & 1;
				AX = (AX & 0xFF00) | (Src & 255);
				Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "RCL AL";
				break;
			case 1:
				Src = ((CX & 255) << 1) + Flag_CF;
				Flag_CF = (Src >> 8) & 1;
				CX = (CX & 0xFF00) | (Src & 255);
				Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "ROL CL";
				break;
			case 2:
				Src = ((DX & 255) << 1) + Flag_CF;
				Flag_CF = (Src >> 8) & 1;
				DX = (DX & 0xFF00) | (Src & 255);
				Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "RCL DL";
				break;
			case 3:
				Src = ((BX & 255) << 1) + Flag_CF;
				Flag_CF = (Src >> 8) & 1;
				BX = (BX & 0xFF00) | (Src & 255);
				Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "RCL BL";
				break;
			case 4:
				Src = (((AX >> 8) & 255) << 1) + Flag_CF;
				Flag_CF = (Src >> 8) & 1;
				AX = (AX & 0x00FF) | ((Src & 255) << 8);
				Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "RCL AH";
				break;
			case 5:
				Src = (((CX >> 8) & 255) << 1) + Flag_CF;
				Flag_CF = (Src >> 8) & 1;
				CX = (CX & 0x00FF) | ((Src & 255) << 8);
				Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "RCL CH";
				break;
			case 6:
				Src = (((DX >> 8) & 255) << 1) + Flag_CF;
				Flag_CF = (Src >> 8) & 1;
				DX = (DX & 0x00FF) | ((Src & 255) << 8);
				Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "RCL DH";
				break;
			case 7:
				Src = (((BX >> 8) & 255) << 1) + Flag_CF;
				Flag_CF = (Src >> 8) & 1;
				BX = (BX & 0x00FF) | ((Src & 255) << 8);
				Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "RCL BH";
				break;
			}
			break;
		}
		break;

	case 3:  //RCR = Rotate Right throught carry

		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = (Flag_CF << 7) | (memory.read_2(New_Addr) >> 1);
			Flag_CF = (memory.read_2(New_Addr)) & 1;
			memory.write_2(New_Addr,  Src & 255);
			Flag_OF = !parity_check[(Src ) & 0b11000000];
			if (log_to_console) cout << "RCR M" << OPCODE_comment << "";
			break;
		case 3:
			// mod 11 источник - регистр
			switch (byte2 & 7)
			{
			case 0:
				Flag_OF = (AX) & 1;  //временное значение
				Src = (Flag_CF << 7) | ((AX & 255) >> 1);
				Flag_CF = Flag_OF;
				AX = (AX & 0xFF00) | (Src);
				Flag_OF = !parity_check[(Src) & 0b11000000];
				if (log_to_console) cout << "RCR AL = " << (int)(Src & 255);
				break;
			case 1:
				Flag_OF = (CX) & 1;  //временное значение
				Src = (Flag_CF << 7) | ((CX & 255) >> 1);
				Flag_CF = Flag_OF;
				CX = (CX & 0xFF00) | (Src);
				Flag_OF = !parity_check[(Src ) & 0b11000000];
				if (log_to_console) cout << "RCR CL = " << (int)(Src & 255);;
				break;
			case 2:
				Flag_OF = (DX) & 1;  //временное значение
				Src = (Flag_CF << 7) | ((DX & 255) >> 1);
				Flag_CF = Flag_OF;
				DX = (DX & 0xFF00) | (Src);
				Flag_OF = !parity_check[(Src ) & 0b11000000];
				if (log_to_console) cout << "RCR DL = " << (int)(Src & 255);
				break;
			case 3:
				Flag_OF = (BX) & 1;  //временное значение
				Src = (Flag_CF << 7) | ((BX & 255) >> 1);
				Flag_CF = Flag_OF;
				BX = (BX & 0xFF00) | (Src);
				Flag_OF = !parity_check[(Src ) & 0b11000000];
				if (log_to_console) cout << "RCR BL = " << (int)(Src & 255);
				break;
			case 4:
				Flag_OF = (AX >> 8) & 1;  //временное значение
				Src = (Flag_CF << 7) | ((AX >> 8) & 255) >> 1;
				Flag_CF = Flag_OF;
				AX = (AX & 0x00FF) | ((Src ) << 8);
				Flag_OF = !parity_check[(Src ) & 0b11000000];
				if (log_to_console) cout << "RCR AH = " << (int)(Src & 255);
				break;
			case 5:
				Flag_OF = (CX >> 8) & 1;  //временное значение
				Src = (Flag_CF << 7) | ((CX >> 8) & 255) >> 1;
				Flag_CF = Flag_OF;
				CX = (CX & 0x00FF) | ((Src ) << 8);
				Flag_OF = !parity_check[(Src ) & 0b11000000];
				if (log_to_console) cout << "RCR CH = " << (int)(Src & 255);
				break;
			case 6:
				Flag_OF = (DX >> 8) & 1;  //временное значение
				Src = (Flag_CF << 7) | ((DX >> 8) & 255) >> 1;
				Flag_CF = Flag_OF;
				DX = (DX & 0x00FF) | ((Src ) << 8);
				Flag_OF = !parity_check[(Src ) & 0b11000000];
				if (log_to_console) cout << "RCR DH = " << (int)(Src & 255);
				break;
			case 7:
				Flag_OF = (BX >> 8) & 1;  //временное значение
				Src = (Flag_CF << 7) | ((BX >> 8) & 255) >> 1;
				Flag_CF = Flag_OF;
				BX = (BX & 0x00FF) | ((Src) << 8);
				Flag_OF = !parity_check[(Src ) & 0b11000000];
				if (log_to_console) cout << "RCR BH = " << (int)(Src & 255);
				break;
			}
			break;
		}
		break;
	
	case 4:  //Shift Left (SHL/SAL)

		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(New_Addr) << 1;
			memory.write_2(New_Addr,  Src & 255);
			Flag_CF = (Src >> 8) & 1;
			Flag_SF = (Src >> 7) & 1;
			Flag_OF = !parity_check[(Src >> 7) & 3];
			if (Src & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift left M" << OPCODE_comment << "";
			break;
		case 3:
			// mod 11 источник - регистр
			switch (byte2 & 7)
			{
			case 0:
				Src = (AX & 255) << 1;
				AX = (AX & 0xFF00) | (Src & 255);
				Flag_CF = (Src >> 8) & 1;
				Flag_SF = (Src >> 7) & 1;
				Flag_OF = !parity_check[(Src >> 7) & 3];
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left AL";
				break;
			case 1:
				Src = (CX & 255) << 1;
				CX = (CX & 0xFF00) | (Src & 255);
				Flag_CF = (Src >> 8) & 1;
				Flag_SF = (Src >> 7) & 1;
				Flag_OF = !parity_check[(Src >> 7) & 3];
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left CL";
				break;
			case 2:
				Src = (DX & 255) << 1;
				DX = (DX & 0xFF00) | (Src & 255);
				Flag_CF = (Src >> 8) & 1;
				Flag_SF = (Src >> 7) & 1;
				Flag_OF = !parity_check[(Src >> 7) & 3];
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left DL";
				break;
			case 3:
				Src = (BX & 255) << 1;
				BX = (BX & 0xFF00) | (Src & 255);
				Flag_CF = (Src >> 8) & 1;
				Flag_SF = (Src >> 7) & 1;
				Flag_OF = !parity_check[(Src >> 7) & 3];
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left BL";
				break;
			case 4:
				Src = ((AX >> 8) & 255) << 1;
				AX = (AX & 0x00FF) | ((Src & 255) << 8);
				Flag_CF = (Src >> 8) & 1;
				Flag_SF = (Src >> 7) & 1;
				Flag_OF = !parity_check[(Src >> 7) & 3];
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left AH";
				break;
			case 5:
				Src = ((CX >> 8) & 255) << 1;
				CX = (CX & 0x00FF) | ((Src & 255) << 8);
				Flag_CF = (Src >> 8) & 1;
				Flag_SF = (Src >> 7) & 1;
				Flag_OF = !parity_check[(Src >> 7) & 3];
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left CH";
				break;
			case 6:
				Src = ((DX >> 8) & 255) << 1;
				DX = (DX & 0x00FF) | ((Src & 255) << 8);
				Flag_CF = (Src >> 8) & 1;
				Flag_SF = (Src >> 7) & 1;
				Flag_OF = !parity_check[(Src >> 7) & 3];
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left DH";
				break;
			case 7:
				Src = ((BX >> 8) & 255) << 1;
				BX = (BX & 0x00FF) | ((Src & 255) << 8);
				Flag_CF = (Src >> 8) & 1;
				Flag_SF = (Src >> 7) & 1;
				Flag_OF = !parity_check[(Src >> 7) & 3];
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left BH";
				break;
			}
			break;
		}
		break;

	case 5:  //Shift Right (SHR)

		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Flag_CF = (memory.read_2(New_Addr)) & 1;
			Src = memory.read_2(New_Addr) >> 1;
			memory.write_2(New_Addr,  Src & 255);
			Flag_SF = ((Src >> 7) & 1);
			Flag_OF = !parity_check[Src & 0b11000000];
			if (Src & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift(SHR) right M" << OPCODE_comment << "";
			break;
		case 3:
			// mod 11 источник - регистр
			switch (byte2 & 7)
			{
			case 0:
				Flag_CF = (AX) & 1;
				Src = (AX & 255) >> 1;
				AX = (AX & 0xFF00) | (Src & 255);
				Flag_SF = ((Src >> 7) & 1);
				Flag_OF = !parity_check[Src & 0b11000000];
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SHR) right AL";
				break;
			case 1:
				Flag_CF = (CX) & 1;
				Src = (CX & 255) >> 1;
				CX = (CX & 0xFF00) | (Src & 255);
				Flag_SF = ((Src >> 7) & 1);
				Flag_OF = !parity_check[Src & 0b11000000];
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SHR) right CL";
				break;
			case 2:
				Flag_CF = (DX) & 1;
				Src = (DX & 255) >> 1;
				DX = (DX & 0xFF00) | (Src & 255);
				Flag_SF = ((Src >> 7) & 1);
				Flag_OF = !parity_check[Src & 0b11000000];
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SHR) right DL";
				break;
			case 3:
				Flag_CF = (BX) & 1;
				Src = (BX & 255) >> 1;
				BX = (BX & 0xFF00) | (Src & 255);
				Flag_SF = ((Src >> 7) & 1);
				Flag_OF = !parity_check[Src & 0b11000000];
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SHR) right BL";
				break;
			case 4:
				Src = ((AX >> 8) & 255) >> 1;
				Flag_CF = (AX >> 8) & 1;
				AX = (AX & 0x00FF) | ((Src & 255) << 8);
				Flag_SF = ((Src >> 7) & 1);
				Flag_OF = !parity_check[Src & 0b11000000];
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SHR) right AH";
				break;
			case 5:
				Src = ((CX >> 8) & 255) >> 1;
				Flag_CF = (CX >> 8) & 1;
				CX = (CX & 0x00FF) | ((Src & 255) << 8);
				Flag_SF = ((Src >> 7) & 1);
				Flag_OF = !parity_check[Src & 0b11000000];
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SHR) right CH";
				break;
			case 6:
				Src = ((DX >> 8) & 255) >> 1;
				Flag_CF = (DX >> 8) & 1;
				DX = (DX & 0x00FF) | ((Src & 255) << 8);
				Flag_SF = ((Src >> 7) & 1);
				Flag_OF = !parity_check[Src & 0b11000000];
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SHR) right DH";
				break;
			case 7:
				Src = ((BX >> 8) & 255) >> 1;
				Flag_CF = (BX >> 8) & 1;
				BX = (BX & 0x00FF) | ((Src & 255) << 8);
				Flag_SF = ((Src >> 7) & 1);
				Flag_OF = !parity_check[Src & 0b11000000];
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SHR) right BH";
				break;
			}
			break;
		}
		break;

	case 6: //SETMO byte R/M

		Src = 0xFF; 
		if (log_to_console) cout << "SETMO_B ";
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Result = memory.read_2(New_Addr) | Src;
			memory.write_2(New_Addr,  Result);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = true;
			Flag_ZF = false;
			Flag_PF = true;
			if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)Result;
			break;
		case 3:
			// mod 11 источник - регистр
			switch (byte2 & 7)
			{
			case 0:
				Result = (AX & 255) | Src;
				AX = (AX & 0xFF00) | Result;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "AL = " << (int)Result;
				break;
			case 1:
				Result = (CX & 255) | Src;
				CX = (CX & 0xFF00) | Result;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "CL = " << (int)Result;
				break;
			case 2:
				Result = (DX & 255) | Src;
				DX = (DX & 0xFF00) | Result;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "DL = " << (int)Result;
				break;
			case 3:
				Result = (BX & 255) | Src;
				BX = (BX & 0xFF00) | Result;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "BL = " << (int)Result;
				break;
			case 4:
				Result = ((AX >> 8) & 255) | Src;
				AX = (AX & 0x00FF) | (Result << 8);
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "AH = " << (int)Result;
				break;
			case 5:
				Result = ((CX >> 8) & 255) | Src;
				CX = (CX & 0x00FF) | (Result << 8);
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "CH = " << (int)Result;
				break;
			case 6:
				Result = ((DX >> 8) & 255) | Src;
				DX = (DX & 0x00FF) | (Result << 8);
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "DH = " << (int)Result;
				break;
			case 7:
				Result = ((BX >> 8) & 255) | Src;
				BX = (BX & 0x00FF) | (Result << 8);
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "BH = " << (int)Result;
				break;
			}
			break;
		}
		Flag_AF = 0;
		break;

	case 7:  //Shift Arithm Right (SAR)

		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Flag_CF = (memory.read_2(New_Addr)) & 1;
			MSB = (memory.read_2(New_Addr)) & 128;
			Src = (memory.read_2(New_Addr) >> 1) | MSB;
			memory.write_2(New_Addr,  Src & 255);
			Flag_SF = ((Src >> 7) & 1);
			Flag_OF = !parity_check[Src & 0b11000000];
			if (Src & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift(SAR) right M" << OPCODE_comment << "";
			break;
		case 3:
			// mod 11 источник - регистр
			switch (byte2 & 7)
			{
			case 0:
				Flag_CF = (AX) & 1;
				MSB = (AX) & 128;
				Src = ((AX & 255) >> 1) | MSB;
				AX = (AX & 0xFF00) | (Src);
				Flag_SF = ((Src >> 7) & 1);
				Flag_OF = !parity_check[Src & 0b11000000];
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SAR) right AL";
				break;
			case 1:
				Flag_CF = (CX) & 1;
				MSB = (CX) & 128;
				Src = ((CX & 255) >> 1) | MSB;
				CX = (CX & 0xFF00) | (Src);
				Flag_SF = ((Src >> 7) & 1);
				Flag_OF = !parity_check[Src & 0b11000000];
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SAR) right CL";
				break;
			case 2:
				Flag_CF = (DX) & 1;
				MSB = (DX) & 128;
				Src = ((DX & 255) >> 1) | MSB;
				DX = (DX & 0xFF00) | (Src);
				Flag_SF = ((Src >> 7) & 1);
				Flag_OF = !parity_check[Src & 0b11000000];
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SAR) right DL";
				break;
			case 3:
				Flag_CF = (BX) & 1;
				MSB = (BX) & 128;
				Src = ((BX & 255) >> 1) | MSB;
				BX = (BX & 0xFF00) | (Src);
				Flag_SF = ((Src >> 7) & 1);
				Flag_OF = !parity_check[Src & 0b11000000];
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SAR) right BL";
				break;
			case 4:
				MSB = (AX >> 8) & 128;
				Src = ((AX >> 9) & 0x7F)| MSB;
				Flag_CF = (AX >> 8) & 1;
				AX = (AX & 0x00FF) | (Src << 8);
				Flag_SF = ((Src >> 7) & 1);
				Flag_OF = !parity_check[Src & 0b11000000];
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SAR) right AH";
				break;
			case 5:
				MSB = (CX >> 8) & 128;
				Src = ((CX >> 9) & 0x7F) | MSB;
				Flag_CF = (CX >> 8) & 1;
				CX = (CX & 0x00FF) | (Src << 8);
				Flag_SF = ((Src >> 7) & 1);
				Flag_OF = !parity_check[Src & 0b11000000];
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SAR) right CH";
				break;
			case 6:
				MSB = (DX >> 8) & 128;
				Src = ((DX >> 9) & 0x7F) | MSB;
				Flag_CF = (DX >> 8) & 1;
				DX = (DX & 0x00FF) | (Src << 8);
				Flag_SF = ((Src >> 7) & 1);
				Flag_OF = !parity_check[Src & 0b11000000];
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SAR) right DH";
				break;
			case 7:
				MSB = (BX >> 8) & 128;
				Src = ((BX >> 9) & 0x7F) | MSB;
				Flag_CF = (BX >> 8) & 1;
				BX = (BX & 0x00FF) | (Src << 8);
				Flag_SF = ((Src >> 7) & 1);
				Flag_OF = !parity_check[Src & 0b11000000];
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SAR) right BH";
				break;
			}
			break;
		}
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void SHL_ROT_16()			// Shift Logical / Arithmetic Left / 16bit / once
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint32 Src = 0;
	uint16 MSB = 0;
	additional_IPs = 0;
	uint16 Result = 0;

	switch ((byte2 >> 3) & 7)
	{

	case 0:  //ROL Rotate Left
	
			//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
			Flag_CF = (Src >> 15) & 1;
			Flag_OF = !parity_check[(Src >> 8) & 0b11000000];
			Src = (Src << 1) | Flag_CF;
			memory.write_2(New_Addr,  Src & 255);
			memory.write_2(New_Addr + 1,  (Src >> 8) & 255);
			if (log_to_console) cout << "ROL M" << OPCODE_comment << "";
			break;
		case 3:
			// mod 11 источник - регистр
			switch (byte2 & 7)
			{
			case 0:
				Src = AX;
				Flag_CF = (Src >> 15) & 1;
				AX = (Src << 1) | Flag_CF;
				Flag_OF = !parity_check[(Src >> 8) & 0b11000000];
				if (log_to_console) cout << "ROL AX";
				break;
			case 1:
				Src = CX;
				Flag_CF = (Src >> 15) & 1;
				CX = (Src << 1) | Flag_CF;
				Flag_OF = !parity_check[(Src >> 8) & 0b11000000];
				if (log_to_console) cout << "ROL CX";
				break;
			case 2:
				Src = DX;
				Flag_CF = (Src >> 15) & 1;
				DX = (Src << 1) | Flag_CF;
				Flag_OF = !parity_check[(Src >> 8) & 0b11000000];
				if (log_to_console) cout << "ROL DX";
				break;
			case 3:
				Src = BX;
				Flag_CF = (Src >> 15) & 1;
				BX = (Src << 1) | Flag_CF;
				Flag_OF = !parity_check[(Src >> 8) & 0b11000000];
				if (log_to_console) cout << "ROL BX";
				break;
			case 4:
				Src = Stack_Pointer;
				Flag_CF = (Src >> 15) & 1;
				Stack_Pointer = (Src << 1) | Flag_CF;
				Flag_OF = !parity_check[(Src >> 8) & 0b11000000];
				if (log_to_console) cout << "ROL SP";
				break;
			case 5:
				Src = Base_Pointer;
				Flag_CF = (Src >> 15) & 1;
				Base_Pointer = (Src << 1) | Flag_CF;
				Flag_OF = !parity_check[(Src >> 8) & 0b11000000];
				if (log_to_console) cout << "ROL BP";
				break;
			case 6:
				Src = Source_Index;
				Flag_CF = (Src >> 15) & 1;
				Source_Index = (Src << 1) | Flag_CF;
				Flag_OF = !parity_check[(Src >> 8) & 0b11000000];
				if (log_to_console) cout << "ROL SI";
				break;
			case 7:
				Src = Destination_Index;
				Flag_CF = (Src >> 15) & 1;
				Destination_Index = (Src << 1) | Flag_CF;
				Flag_OF = !parity_check[(Src >> 8) & 0b11000000];
				if (log_to_console) cout << "ROL DI";
				break;
			}
			break;
		}
		break;
	
	case 1:  //ROR = Rotate Right
	
			//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Flag_CF = memory.read_2(New_Addr) & 1;
			Src = (Flag_CF * 0x8000) | ((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) >> 1);
			memory.write_2(New_Addr,  Src & 255);
			memory.write_2(New_Addr + 1,  Src >> 8);
			Flag_OF = !parity_check[(Src >> 14) & 3];
			if (log_to_console) cout << "ROR M" << OPCODE_comment << "";
			break;
		case 3:
			// mod 11 источник - регистр
			switch (byte2 & 7)
			{
			case 0:
				Flag_CF = (AX) & 1;
				Result = AX << 15;
				AX = (AX >> 1) | Result;
				Flag_OF = !parity_check[AX >> 14];
				if (log_to_console) cout << "ROR AX";
				break;
			case 1:
				Flag_CF = (CX) & 1;
				Result = CX << 15;
				CX = (CX >> 1) | Result;
				Flag_OF = !parity_check[CX >> 14];
				if (log_to_console) cout << "ROR CX";
				break;
			case 2:
				Flag_CF = (DX) & 1;
				Result = DX << 15;
				DX = (DX >> 1) | Result;
				Flag_OF = !parity_check[DX >> 14];
				if (log_to_console) cout << "ROR DX";
				break;
			case 3:
				Flag_CF = (BX) & 1;
				Result = BX << 15;
				BX = (BX >> 1) | Result;
				Flag_OF = !parity_check[BX >> 14];
				if (log_to_console) cout << "ROR BX";
				break;
			case 4:
				Flag_CF = (Stack_Pointer) & 1;
				Result = Stack_Pointer << 15;
				Stack_Pointer = (Stack_Pointer >> 1) | Result;
				Flag_OF = !parity_check[Stack_Pointer >> 14];
				if (log_to_console) cout << "ROR SP";
				break;
			case 5:
				Flag_CF = (Base_Pointer) & 1;
				Result = Base_Pointer << 15;
				Base_Pointer = (Base_Pointer >> 1) | Result;
				Flag_OF = !parity_check[Base_Pointer >> 14];
				if (log_to_console) cout << "ROR BP";
				break;
			case 6:
				Flag_CF = (Source_Index) & 1;
				Result = Source_Index << 15;
				Source_Index = (Source_Index >> 1) | Result;
				Flag_OF = !parity_check[Source_Index >> 14];
				if (log_to_console) cout << "ROR SI";
				break;
			case 7:
				Flag_CF = (Destination_Index) & 1;
				Result = Destination_Index << 15;
				Destination_Index = (Destination_Index >> 1) | Result;
				Flag_OF = !parity_check[Destination_Index >> 14];
				if (log_to_console) cout << "ROR DI";
				break;
			}
			break;
		}
		break;
	
	case 2:  //RCL Rotate Left throught carry
	
			//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = ((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) << 1) | Flag_CF;
			Flag_CF = (Src >> 16) & 1;
			memory.write_2(New_Addr,  Src & 255);
			memory.write_2(New_Addr + 1,  (Src >> 8) & 255);
			Flag_OF = !parity_check[(Src >> 15) & 3];
			if (log_to_console) cout << "RCL M" << OPCODE_comment << "";
			break;
		case 3:
			// mod 11 источник - регистр
			switch (byte2 & 7)
			{
			case 0:
				Src = (AX << 1) | Flag_CF;
				AX = Src & 0xFFFF;
				Flag_CF = (Src >> 16) & 1;
				Flag_OF = !parity_check[(Src >> 15) & 3];
				if (log_to_console) cout << "RCL AX";
				break;
			case 1:
				Src = (CX << 1) | Flag_CF;
				CX = Src & 0xFFFF;
				Flag_CF = (Src >> 16) & 1;
				Flag_OF = !parity_check[(Src >> 15) & 3];
				if (log_to_console) cout << "RCL CX";
				break;
			case 2:
				Src = (DX << 1) | Flag_CF;
				DX = Src & 0xFFFF;
				Flag_CF = (Src >> 16) & 1;
				Flag_OF = !parity_check[(Src >> 15) & 3];
				if (log_to_console) cout << "RCL DX";
				break;
			case 3:
				Src = (BX << 1) | Flag_CF;
				BX = Src & 0xFFFF;
				Flag_CF = (Src >> 16) & 1;
				Flag_OF = !parity_check[(Src >> 15) & 3];
				if (log_to_console) cout << "RCL BX";
				break;
			case 4:
				Src = (Stack_Pointer << 1) | Flag_CF;
				Stack_Pointer = Src & 0xFFFF;
				Flag_CF = (Src >> 16) & 1;
				Flag_OF = !parity_check[(Src >> 15) & 3];
				if (log_to_console) cout << "RCL SP";
				break;
			case 5:
				Src = (Base_Pointer << 1) | Flag_CF;
				Base_Pointer = Src & 0xFFFF;
				Flag_CF = (Src >> 16) & 1;
				Flag_OF = !parity_check[(Src >> 15) & 3];
				if (log_to_console) cout << "RCL BP";
				break;
			case 6:
				Src = (Source_Index << 1) | Flag_CF;
				Source_Index = Src & 0xFFFF;
				Flag_CF = (Src >> 16) & 1;
				Flag_OF = !parity_check[(Src >> 15) & 3];
				if (log_to_console) cout << "RCL SI";
				break;
			case 7:
				Src = (Destination_Index << 1) | Flag_CF;
				Destination_Index = Src & 0xFFFF;
				Flag_CF = (Src >> 16) & 1;
				Flag_OF = !parity_check[(Src >> 15) & 3];
				if (log_to_console) cout << "RCL DI";
				break;
			}
			break;
		}
		break;
	
	case 3:  //RCR = Rotate Right throught carry
	
			//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Flag_OF = memory.read_2(New_Addr) & 1; //tmp
			Src = (Flag_CF << 15) | ((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) >> 1);
			Flag_CF = Flag_OF;
			memory.write_2(New_Addr,  Src & 255);
			memory.write_2(New_Addr + 1,  (Src >> 8) & 255);
			Flag_OF = !parity_check[(Src >> 14) & 3];
			if (log_to_console) cout << "RCR M" << OPCODE_comment << "";
			break;
		case 3:
			// mod 11 источник - регистр
			switch (byte2 & 7)
			{
			case 0:
				Flag_OF = (AX) & 1;
				Src = (AX >> 1) | (Flag_CF << 15);
				AX = Src & 0xFFFF;
				Flag_CF = Flag_OF;
				Flag_OF = !parity_check[(Src >> 14) & 3];
				if (log_to_console) cout << "RCR AX";
				break;
			case 1:
				Flag_OF = (CX) & 1;
				Src = (CX >> 1) | (Flag_CF << 15);
				CX = Src & 0xFFFF;
				Flag_CF = Flag_OF;
				Flag_OF = !parity_check[(Src >> 14) & 3];
				if (log_to_console) cout << "RCR CX";
				break;
			case 2:
				Flag_OF = (DX) & 1;
				Src = (DX >> 1) | (Flag_CF << 15);
				DX = Src & 0xFFFF;
				Flag_CF = Flag_OF;
				Flag_OF = !parity_check[(Src >> 14) & 3];
				if (log_to_console) cout << "RCR DX";
				break;
			case 3:
				Flag_OF = (BX) & 1;
				Src = (BX >> 1) | (Flag_CF << 15);
				BX = Src & 0xFFFF;
				Flag_CF = Flag_OF;
				Flag_OF = !parity_check[(Src >> 14) & 3];
				if (log_to_console) cout << "RCR BX";
				break;
			case 4:
				Flag_OF = (Stack_Pointer) & 1;
				Src = (Stack_Pointer >> 1) | (Flag_CF << 15);
				Stack_Pointer = Src & 0xFFFF;
				Flag_CF = Flag_OF;
				Flag_OF = !parity_check[(Src >> 14) & 3];
				if (log_to_console) cout << "RCR SP";
				break;
			case 5:
				Flag_OF = (Base_Pointer) & 1;
				Src = (Base_Pointer >> 1) | (Flag_CF << 15);
				Base_Pointer = Src & 0xFFFF;
				Flag_CF = Flag_OF;
				Flag_OF = !parity_check[(Src >> 14) & 3];
				if (log_to_console) cout << "RCR BP";
				break;
			case 6:
				Flag_OF = (Source_Index) & 1;
				Src = (Source_Index >> 1) | (Flag_CF << 15);
				Source_Index = Src & 0xFFFF;
				Flag_CF = Flag_OF;
				Flag_OF = !parity_check[(Src >> 14) & 3];
				if (log_to_console) cout << "RCR SI";
				break;
			case 7:
				Flag_OF = (Destination_Index) & 1;
				Src = (Destination_Index >> 1) | (Flag_CF << 15);
				Destination_Index = Src & 0xFFFF;
				Flag_CF = Flag_OF;
				Flag_OF = !parity_check[(Src >> 14) & 3];
				if (log_to_console) cout << "RCR DI";
				break;
			}
			break;
		}
		break;
	
	case 4:  //Shift Left

		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) << 1;
			memory.write_2(New_Addr,  Src & 255);
			memory.write_2(New_Addr + 1,  (Src >> 8) & 255);
			Flag_CF = (Src >> 16) & 1;
			Flag_SF = (Src >> 15) & 1;
			Flag_OF = !parity_check[(Src >> 15) & 3];
			if (Src & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift left M" << OPCODE_comment << "";
			break;
		case 3:
			// mod 11 источник - регистр
			switch (byte2 & 7)
			{
			case 0:
				Src = AX << 1;
				AX = Src & 0xFFFF;
				Flag_CF = (Src >> 16) & 1;
				Flag_SF = (Src >> 15) & 1;
				Flag_OF = !parity_check[(Src >> 15) & 3];
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left AX";
				break;
			case 1:
				Src = CX << 1;
				CX = Src & 0xFFFF;
				Flag_CF = (Src >> 16) & 1;
				Flag_SF = (Src >> 15) & 1;
				Flag_OF = !parity_check[(Src >> 15) & 3];
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left CX";
				break;
			case 2:
				Src = DX << 1;
				DX = Src & 0xFFFF;
				Flag_CF = (Src >> 16) & 1;
				Flag_SF = (Src >> 15) & 1;
				Flag_OF = !parity_check[(Src >> 15) & 3];
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left DX";
				break;
			case 3:
				Src = BX << 1;
				BX = Src & 0xFFFF;
				Flag_CF = (Src >> 16) & 1;
				Flag_SF = (Src >> 15) & 1;
				Flag_OF = !parity_check[(Src >> 15) & 3];
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left BX";
				break;
			case 4:
				Src = Stack_Pointer << 1;
				Stack_Pointer = Src & 0xFFFF;
				Flag_CF = (Src >> 16) & 1;
				Flag_SF = (Src >> 15) & 1;
				Flag_OF = !parity_check[(Src >> 15) & 3];
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left SP";
				break;
			case 5:
				Src = Base_Pointer << 1;
				Base_Pointer = Src & 0xFFFF;
				Flag_CF = (Src >> 16) & 1;
				Flag_SF = (Src >> 15) & 1;
				Flag_OF = !parity_check[(Src >> 15) & 3];
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left BP";
				break;
			case 6:
				Src = Source_Index << 1;
				Source_Index = Src & 0xFFFF;
				Flag_CF = (Src >> 16) & 1;
				Flag_SF = (Src >> 15) & 1;
				Flag_OF = !parity_check[(Src >> 15) & 3];
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left SI";
				break;
			case 7:
				Src = Destination_Index << 1;
				Destination_Index = Src & 0xFFFF;
				Flag_CF = (Src >> 16) & 1;
				Flag_SF = (Src >> 15) & 1;
				Flag_OF = !parity_check[(Src >> 15) & 3];
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left DI";
				break;
			}
			break;
		}
		break;

	case 5:  //Shift right (SHR)

		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Flag_CF = memory.read_2(New_Addr) & 1;
			Src = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) >> 1;
			memory.write_2(New_Addr,  Src & 255);
			memory.write_2(New_Addr + 1,  (Src >> 8) & 255);
			Flag_SF = false;
			Flag_OF = (Src >> 14) & 1;
			if (Src & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift(SHR) right M" << OPCODE_comment << "";
			break;
		case 3:
			// mod 11 источник - регистр
			switch (byte2 & 7)
			{
			case 0:
				Src = AX >> 1;
				Flag_CF = (AX) & 1;
				AX = Src & 0xFFFF;				
				Flag_SF = false;
				Flag_OF = (Src >> 14) & 1;
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SHR) right AX";
				break;
			case 1:
				Src = CX >> 1;
				Flag_CF = (CX) & 1;
				CX = Src & 0xFFFF;
				Flag_SF = false;
				Flag_OF = (Src >> 14) & 1;
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SHR) right CX";
				break;
			case 2:
				Src = DX >> 1;
				Flag_CF = (DX) & 1;
				DX = Src & 0xFFFF;
				Flag_SF = false;
				Flag_OF = (Src >> 14) & 1;
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SHR) right DX";
				break;
			case 3:
				Src = BX >> 1;
				Flag_CF = (BX) & 1;
				BX = Src & 0xFFFF;
				Flag_SF = false;
				Flag_OF = (Src >> 14) & 1;
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SHR) right BX";
				break;
			case 4:
				Src = Stack_Pointer >> 1;
				Flag_CF = (Stack_Pointer) & 1;
				Stack_Pointer = Src & 0xFFFF;
				Flag_SF = false;
				Flag_OF = (Src >> 14) & 1;
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SHR) right SP";
				break;
			case 5:
				Src = Base_Pointer >> 1;
				Flag_CF = (Base_Pointer) & 1;
				Base_Pointer = Src & 0xFFFF;
				Flag_SF = false;
				Flag_OF = (Src >> 14) & 1;
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SHR) right BP";
				break;
			case 6:
				Src = Source_Index >> 1;
				Flag_CF = (Source_Index) & 1;
				Source_Index = Src & 0xFFFF;
				Flag_SF = false;
				Flag_OF = (Src >> 14) & 1;
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SHR) right SI";
				break;
			case 7:
				Src = Destination_Index >> 1;
				Flag_CF = (Destination_Index) & 1;
				Destination_Index = Src & 0xFFFF;
				Flag_SF = false;
				Flag_OF = (Src >> 14) & 1;
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SHR) right DI";
				break;
			}
			break;
		}
		break;

	case 6:  //SETMO 

		Src = 0xFFFF;
		if (log_to_console) cout << "SETMO WORD ";
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Result = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) | Src;
			memory.write_2(New_Addr,  Result & 255);
			memory.write_2(New_Addr + 1,  (Result >> 8) & 255);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = true;
			Flag_ZF = false;
			Flag_PF = true;
			if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)Result;
			break;
		case 3:
			// mod 11 источник - регистр
			switch (byte2 & 7)
			{
			case 0:
				AX = AX | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "AL = " << (int)Result;
				break;
			case 1:
				CX = CX | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "CL = " << (int)Result;
				break;
			case 2:
				DX = DX | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "DL = " << (int)Result;
				break;
			case 3:
				BX = BX | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "BL = " << (int)Result;
				break;
			case 4:
				Stack_Pointer = Stack_Pointer | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "AH = " << (int)Result;
				break;
			case 5:
				Base_Pointer = Base_Pointer | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "CH = " << (int)Result;
				break;
			case 6:
				Source_Index = Source_Index | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "DH = " << (int)Result;
				break;
			case 7:
				Destination_Index = Destination_Index | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "BH = " << (int)Result;
				break;
			}
			break;
		}
		Flag_AF = 0;
		break;

	case 7:  //Shift arithm right (SAR)

		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Flag_CF = memory.read_2(New_Addr) & 1;
			Src = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256);
			MSB = Src & 0x8000;
			Src = (Src >> 1) | MSB;
			memory.write_2(New_Addr,  Src & 255);
			memory.write_2(New_Addr + 1,  (Src >> 8) & 255);
			Flag_SF = ((Src >> 15) & 1);
			Flag_OF = 0;
			if (Src & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift(SAR) right M" << OPCODE_comment << "";
			break;
		case 3:
			// mod 11 источник - регистр
			switch (byte2 & 7)
			{
			case 0:
				MSB = AX & 0x8000;
				Src = (AX >> 1) | MSB;
				Flag_CF = (AX) & 1;
				AX = Src & 0xFFFF;
				Flag_SF = ((Src >> 15) & 1);
				Flag_OF = 0;
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SAR) right AX";
				break;
			case 1:
				MSB = CX & 0x8000;
				Src = (CX >> 1) | MSB;
				Flag_CF = (CX) & 1;
				CX = Src & 0xFFFF;
				Flag_SF = ((Src >> 15) & 1);
				Flag_OF = 0;
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SAR) right CX";
				break;
			case 2:
				MSB = DX & 0x8000;
				Src = (DX >> 1) | MSB;
				Flag_CF = (DX) & 1;
				DX = Src & 0xFFFF;
				Flag_SF = ((Src >> 15) & 1);
				Flag_OF = 0;
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SAR) right DX";
				break;
			case 3:
				MSB = BX & 0x8000;
				Src = (BX >> 1) | MSB;
				Flag_CF = (BX) & 1;
				BX = Src & 0xFFFF;
				Flag_SF = ((Src >> 15) & 1);
				Flag_OF = 0;
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SAR) right BX";
				break;
			case 4:
				MSB = Stack_Pointer & 0x8000;
				Src = (Stack_Pointer >> 1) | MSB;
				Flag_CF = (Stack_Pointer) & 1;
				Stack_Pointer = Src & 0xFFFF;
				Flag_SF = ((Src >> 15) & 1);
				Flag_OF = 0;
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SAR) right SP";
				break;
			case 5:
				MSB = Base_Pointer & 0x8000;
				Src = (Base_Pointer >> 1) | MSB;
				Flag_CF = (Base_Pointer) & 1;
				Base_Pointer = Src & 0xFFFF;
				Flag_SF = ((Src >> 15) & 1);
				Flag_OF = 0;
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SAR) right BP";
				break;
			case 6:
				MSB = Source_Index & 0x8000;
				Src = (Source_Index >> 1) | MSB;
				Flag_CF = (Source_Index) & 1;
				Source_Index = Src & 0xFFFF;
				Flag_SF = ((Src >> 15) & 1);
				Flag_OF = 0;
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SAR) right SI";
				break;
			case 7:
				MSB = Destination_Index & 0x8000;
				Src = (Destination_Index >> 1) | MSB;
				Flag_CF = (Destination_Index) & 1;
				Destination_Index = Src & 0xFFFF;
				Flag_SF = ((Src >> 15) & 1);
				Flag_OF = 0;
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SAR) right DI";
				break;
			}
			break;
		}
		break;
		
	default:
		cout << "unknown OP = " << memory.read_2(Instruction_Pointer + *CS * 16);
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void SHL_ROT_8_mult()		// Shift Logical / Arithmetic Left / 8bit / CL
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Src = 0;
	uint16 Result = 0;
	uint8 repeats = CX & 255;
	uint8 MSB = 0;
	additional_IPs = 0;
	uint8 oldCF;

	switch ((byte2 >> 3) & 7)
	{
	case 0:  //ROL Rotate Left

		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(New_Addr);
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = (Src >> 7) & 1;
				Src = (Src << 1) | Flag_CF;
			}
			memory.write_2(New_Addr,  Src & 255);
			if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
			if (log_to_console) cout << "ROL M" << OPCODE_comment << " " << (int)repeats << " times";
			break;
		case 3:
			// mod 11 источник - регистр
			switch (byte2 & 7)
			{
			case 0:
				Src = AX & 255;
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = (Src >> 7) & 1;
					Src = (Src << 1) | Flag_CF;
				}
				AX = (AX & 0xFF00) | (Src & 255);
				if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "ROL AL " << (int)repeats << " times";
				break;
			case 1:
				Src = CX & 255;
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = (Src >> 7) & 1;
					Src = (Src << 1) | Flag_CF;
				}
				CX = (CX & 0xFF00) | (Src & 255);
				if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "ROL CL " << (int)repeats << " times";
				break;
			case 2:
				Src = DX & 255;
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = (Src >> 7) & 1;
					Src = (Src << 1) | Flag_CF;
				}
				DX = (DX & 0xFF00) | (Src & 255);
				if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "ROL DL " << (int)repeats << " times";
				break;
			case 3:
				Src = BX & 255;
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = (Src >> 7) & 1;
					Src = (Src << 1) | Flag_CF;
				}
				BX = (BX & 0xFF00) | (Src & 255);
				if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "ROL BL " << (int)repeats << " times";
				break;
			case 4:
				Src = ((AX >> 8) & 255);
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = (Src >> 7) & 1;
					Src = (Src << 1) | Flag_CF;
				}
				AX = (AX & 0x00FF) | ((Src & 255) << 8);
				if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "ROL AH " << (int)repeats << " times";
				break;
			case 5:
				Src = ((CX >> 8) & 255);
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = (Src >> 7) & 1;
					Src = (Src << 1) | Flag_CF;
				}
				CX = (CX & 0x00FF) | ((Src & 255) << 8);
				if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "ROL CH " << (int)repeats << " times";
				break;
			case 6:
				Src = ((DX >> 8) & 255);
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = (Src >> 7) & 1;
					Src = (Src << 1) | Flag_CF;
				}
				DX = (DX & 0x00FF) | ((Src & 255) << 8);
				if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "ROL DH " << (int)repeats << " times";
				break;
			case 7:
				Src = ((BX >> 8) & 255);
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = (Src >> 7) & 1;
					Src = (Src << 1) | Flag_CF;
				}
				BX = (BX & 0x00FF) | ((Src & 255) << 8);
				if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "ROL BH " << (int)repeats << " times";
				break;
			}
			break;
		}
		break;

	case 1:  //ROR = Rotate Right

		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			if (!repeats) break;
			Src = memory.read_2(New_Addr);
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = Src & 1;
				Src = (Src >> 1) | (Flag_CF * 0x80);
			}
			memory.write_2(New_Addr,  Src & 255);
			if (log_to_console) cout << "ROR M" << OPCODE_comment << " " << (int)repeats << " times";
			break;
		case 3:
			// mod 11 источник - регистр
			switch (byte2 & 7)
			{
			case 0:
				Src = AX & 255;
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = Src & 1;
					Src = (Src >> 1) | (Flag_CF * 0x80);
				}
				AX = (AX & 0xFF00) | (Src & 255);
				if (log_to_console) cout << "ROR AL " << (int)repeats << " times = " << (int)Src;
				break;
			case 1:
				Src = CX & 255;
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = Src & 1;
					Src = (Src >> 1) | (Flag_CF * 0x80);
				}
				CX = (CX & 0xFF00) | (Src & 255);
				if (log_to_console) cout << "ROR CL " << (int)repeats << " times = " << (int)Src;
				break;
			case 2:
				Src = DX & 255;
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = Src & 1;
					Src = (Src >> 1) | (Flag_CF * 0x80);
				}
				DX = (DX & 0xFF00) | (Src & 255);
				if (log_to_console) cout << "ROR DL " << (int)repeats << " times = " << (int)Src;
				break;
			case 3:
				Src = BX & 255;
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = Src & 1;
					Src = (Src >> 1) | (Flag_CF * 0x80);
				}
				BX = (BX & 0xFF00) | (Src & 255);
				if (log_to_console) cout << "ROR BL " << (int)repeats << " times = " << (int)Src;
				break;
			case 4:
				Src = ((AX >> 8) & 255);
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = Src & 1;
					Src = (Src >> 1) | (Flag_CF * 0x80);
				}
				AX = (AX & 0x00FF) | ((Src) << 8);
				if (log_to_console) cout << "ROR AH " << (int)repeats << " times = " << (int)Src;
				break;
			case 5:
				Src = ((CX >> 8) & 255);
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = Src & 1;
					Src = (Src >> 1) | (Flag_CF * 0x80);
				}
				CX = (CX & 0x00FF) | ((Src) << 8);
				if (log_to_console) cout << "ROR CH " << (int)repeats << " times = " << (int)Src;
				break;
			case 6:
				Src = ((DX >> 8) & 255);
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = Src & 1;
					Src = (Src >> 1) | (Flag_CF * 0x80);
				}
				DX = (DX & 0x00FF) | ((Src) << 8);
				if (log_to_console) cout << "ROR DH " << (int)repeats << " times = " << (int)Src;
				break;
			case 7:
				Src = ((BX >> 8) & 255);
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = Src & 1;
					Src = (Src >> 1) | (Flag_CF * 0x80);
				}
				BX = (BX & 0x00FF) | ((Src) << 8);
				if (log_to_console) cout << "ROR BH " << (int)repeats << " times = " << (int)Src;
				break;
			}
			break;
		}
		break;

	case 2:  //RCL Rotate Left throught carry

		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			if (!repeats) break;
			Src = memory.read_2(New_Addr);
			for (int i = 0; i < repeats; i++)
			{
				Src = (Src << 1) | Flag_CF;
				Flag_CF = (Src >> 8) & 1;
			}
			memory.write_2(New_Addr,  Src & 255);
			if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
			if (log_to_console) cout << "RCL M" << OPCODE_comment << " " << (int)repeats << " times";
			break;
		case 3:
			// mod 11 источник - регистр
			switch (byte2 & 7)
			{
			case 0:
				Src = AX & 255;
				for (int i = 0; i < repeats; i++)
				{
					Src = (Src << 1) | Flag_CF;
					Flag_CF = (Src >> 8) & 1;
				}
				AX = (AX & 0xFF00) | (Src & 255);
				if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "RCL AL " << (int)repeats << " times";
				break;
			case 1:
				Src = CX & 255;
				for (int i = 0; i < repeats; i++)
				{
					Src = (Src << 1) | Flag_CF;
					Flag_CF = (Src >> 8) & 1;
				}
				CX = (CX & 0xFF00) | (Src & 255);
				if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "RCL CL " << (int)repeats << " times";
				break;
			case 2:
				Src = DX & 255;
				for (int i = 0; i < repeats; i++)
				{
					Src = (Src << 1) | Flag_CF;
					Flag_CF = (Src >> 8) & 1;
				}
				DX = (DX & 0xFF00) | (Src & 255);
				if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "RCL DL " << (int)repeats << " times";
				break;
			case 3:
				Src = BX & 255;
				for (int i = 0; i < repeats; i++)
				{
					Src = (Src << 1) | Flag_CF;
					Flag_CF = (Src >> 8) & 1;
				}
				BX = (BX & 0xFF00) | (Src & 255);
				if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "RCL BL " << (int)repeats << " times";
				break;
			case 4:
				Src = ((AX >> 8) & 255);
				for (int i = 0; i < repeats; i++)
				{
					Src = (Src << 1) | Flag_CF;
					Flag_CF = (Src >> 8) & 1;
				}
				AX = (AX & 0x00FF) | ((Src & 255) << 8);
				if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "RCL AH " << (int)repeats << " times";
				break;
			case 5:
				Src = ((CX >> 8) & 255);
				for (int i = 0; i < repeats; i++)
				{
					Src = (Src << 1) | Flag_CF;
					Flag_CF = (Src >> 8) & 1;
				}
				CX = (CX & 0x00FF) | ((Src & 255) << 8);
				if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "RCL CH " << (int)repeats << " times";
				break;
			case 6:
				Src = ((DX >> 8) & 255);
				for (int i = 0; i < repeats; i++)
				{
					Src = (Src << 1) | Flag_CF;
					Flag_CF = (Src >> 8) & 1;
				}
				DX = (DX & 0x00FF) | ((Src & 255) << 8);
				if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "RCL DH " << (int)repeats << " times";
				break;
			case 7:
				Src = ((BX >> 8) & 255);
				for (int i = 0; i < repeats; i++)
				{
					Src = (Src << 1) | Flag_CF;
					Flag_CF = (Src >> 8) & 1;
				}
				BX = (BX & 0x00FF) | ((Src & 255) << 8);
				if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				if (log_to_console) cout << "RCL BH " << (int)repeats << " times";
				break;
			}
			break;
		}
		break;

	case 3:  //RCR = Rotate Right throught carry

		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			if (!repeats) break;
			Src = memory.read_2(New_Addr);
			for (int i = 0; i < repeats; i++)
			{
				oldCF = Src & 1; // tmp
				Src = (Src >> 1) | (Flag_CF << 7);
				Flag_CF = oldCF;
			}
			memory.write_2(New_Addr,  Src & 255);
			if (repeats == 1) Flag_OF = !parity_check[Src & 0b11000000];
			if (log_to_console) cout << "RCR M" << OPCODE_comment << " " << (int)repeats << " times";
			break;
		case 3:
			// mod 11 источник - регистр
			switch (byte2 & 7)
			{
			case 0:
				Src = AX & 255;
				for (int i = 0; i < repeats; i++)
				{
					oldCF = Src & 1;// tmp
					Src = (Src >> 1) | (Flag_CF << 7);
					Flag_CF = oldCF;
				}
				AX = (AX & 0xFF00) | (Src & 255);
				if (repeats == 1) Flag_OF = !parity_check[Src & 0b11000000];
				if (log_to_console) cout << "RCR AL = " << (int)(Src & 255);
				break;
			case 1:
				Src = CX & 255;
				for (int i = 0; i < repeats; i++)
				{
					oldCF = Src & 1;// tmp
					Src = (Src >> 1) | (Flag_CF << 7);
					Flag_CF = oldCF;
				}
				CX = (CX & 0xFF00) | (Src & 255);
				if (repeats == 1) Flag_OF = !parity_check[Src & 0b11000000];
				if (log_to_console) cout << "RCR CL = " << (int)(Src & 255);
				break;
			case 2:
				Src = DX & 255;
				for (int i = 0; i < repeats; i++)
				{
					oldCF = Src & 1;// tmp
					Src = (Src >> 1) | (Flag_CF << 7);
					Flag_CF = oldCF;
				}
				DX = (DX & 0xFF00) | (Src & 255);
				if (repeats == 1) Flag_OF = !parity_check[Src & 0b11000000];
				if (log_to_console) cout << "RCR DL = " << (int)(Src & 255);
				break;
			case 3:
				Src = BX & 255;
				for (int i = 0; i < repeats; i++)
				{
					oldCF = Src & 1;// tmp
					Src = (Src >> 1) | (Flag_CF << 7);
					Flag_CF = oldCF;
				}
				BX = (BX & 0xFF00) | (Src & 255);
				if (repeats == 1) Flag_OF = !parity_check[Src & 0b11000000];
				if (log_to_console) cout << "RCR BL = " << (int)(Src & 255);
				break;
			case 4:
				Src = ((AX >> 8) & 255);
				for (int i = 0; i < repeats; i++)
				{
					oldCF = Src & 1;// tmp
					Src = (Src >> 1) | (Flag_CF << 7);
					Flag_CF = oldCF;
				}
				AX = (AX & 0x00FF) | ((Src & 255) << 8);
				if (repeats == 1) Flag_OF = !parity_check[Src & 0b11000000];
				if (log_to_console) cout << "RCR AH = " << (int)(Src & 255);
				break;
			case 5:
				Src = ((CX >> 8) & 255);
				for (int i = 0; i < repeats; i++)
				{
					oldCF = Src & 1;// tmp
					Src = (Src >> 1) | (Flag_CF << 7);
					Flag_CF = oldCF;
				}
				CX = (CX & 0x00FF) | ((Src & 255) << 8);
				if (repeats == 1) Flag_OF = !parity_check[Src & 0b11000000];
				if (log_to_console) cout << "RCR CH = " << (int)(Src & 255);
				break;
			case 6:
				Src = ((DX >> 8) & 255);
				for (int i = 0; i < repeats; i++)
				{
					oldCF = Src & 1;// tmp
					Src = (Src >> 1) | (Flag_CF << 7);
					Flag_CF = oldCF;
				}
				DX = (DX & 0x00FF) | ((Src & 255) << 8);
				if (repeats == 1) Flag_OF = !parity_check[Src & 0b11000000];
				if (log_to_console) cout << "RCR DH = " << (int)(Src & 255);
				break;
			case 7:
				Src = ((BX >> 8) & 255);
				for (int i = 0; i < repeats; i++)
				{
					oldCF = Src & 1;// tmp
					Src = (Src >> 1) | (Flag_CF << 7);
					Flag_CF = oldCF;
				}
				BX = (BX & 0x00FF) | ((Src & 255) << 8);
				if (repeats == 1) Flag_OF = !parity_check[Src & 0b11000000];
				if (log_to_console) cout << "RCR BH = " << (int)(Src & 255);
				break;
			}
			break;
		}
		break;

	case 4:  //Shift Left
		
		if (repeats > 9) repeats = 9;
		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			if (!repeats) break;
			Src = memory.read_2(New_Addr) << repeats;
			memory.write_2(New_Addr,  Src & 255);
			Flag_CF = (Src >> 8) & 1;
			Flag_SF = (Src >> 7) & 1;
			if (Src & 255) Flag_ZF = false;
			else Flag_ZF = true;
			if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift left M" << OPCODE_comment << " " << (int)repeats << " times";
			break;
		case 3:
			// mod 11 источник - регистр
			if (!repeats) break;
			switch (byte2 & 7)
			{
			case 0:
				Src = (AX & 255) << repeats;
				AX = (AX & 0xFF00) | (Src & 255);
				Flag_CF = (Src >> 8) & 1;
				Flag_SF = ((Src >> 7) & 1);
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				Flag_PF = parity_check[AX & 255];
				if (log_to_console) cout << "Shift left AL " << (int)repeats << " times";
				break;
			case 1:
				Src = (CX & 255) << repeats;
				CX = (CX & 0xFF00) | (Src & 255);
				Flag_CF = (Src >> 8) & 1;
				Flag_SF = ((Src >> 7) & 1);
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				Flag_PF = parity_check[CX & 255];
				if (log_to_console) cout << "Shift left CL " << (int)repeats << " times";
				break;
			case 2:
				Src = (DX & 255) << repeats;
				DX = (DX & 0xFF00) | (Src & 255);
				Flag_CF = (Src >> 8) & 1;
				Flag_SF = ((Src >> 7) & 1);
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left DL " << (int)repeats << " times";
				break;
			case 3:
				Src = (BX & 255) << repeats;
				BX = (BX & 0xFF00) | (Src & 255);
				Flag_CF = (Src >> 8) & 1;
				Flag_SF = ((Src >> 7) & 1);
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left BL " << (int)repeats << " times";
				break;
			case 4:
				Src = ((AX >> 8) & 255) << repeats;
				AX = (AX & 0x00FF) | ((Src & 255) << 8);
				Flag_CF = (Src >> 8) & 1;
				Flag_SF = ((Src >> 7) & 1);
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left AH " << (int)repeats << " times";
				break;
			case 5:
				Src = ((CX >> 8) & 255) << repeats;
				CX = (CX & 0x00FF) | ((Src & 255) << 8);
				Flag_CF = (Src >> 8) & 1;
				Flag_SF = ((Src >> 7) & 1);
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left CH " << (int)repeats << " times";
				break;
			case 6:
				Src = (DX >> 8) << repeats;
				DX = (DX & 0x00FF) | ((Src & 255) << 8);
				Flag_CF = (Src >> 8) & 1;
				Flag_SF = ((Src >> 7) & 1);
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left DH " << (int)repeats << " times";
				break;
			case 7:
				Src = ((BX >> 8) & 255) << repeats;
				BX = (BX & 0x00FF) | ((Src & 255) << 8);
				Flag_CF = (Src >> 8) & 1;
				Flag_SF = ((Src >> 7) & 1);
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left BH " << (int)repeats << " times";
				break;
			}
			break;
		}
		break;

	case 5:  //Shift Right (SHR)

		if (repeats > 9) repeats = 9;
		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			if (!repeats) break;
			Flag_CF = (memory.read_2(New_Addr) >> (repeats - 1)) & 1;
			Src = memory.read_2(New_Addr) >> repeats;
			memory.write_2(New_Addr,  Src & 255);
			Flag_SF = ((Src >> 7) & 1);
			if (Src & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (repeats == 1) Flag_OF = !parity_check[(Src) & 0b11000000];
			if (log_to_console) cout << "Shift(SHR) right M" << OPCODE_comment << " " << (int)repeats << " times";
			break;
		case 3:
			// mod 11 источник - регистр
			if (!repeats) break;
			switch (byte2 & 7)
			{
			case 0:
				Src = (AX & 255);
				Flag_CF = (Src >> (repeats - 1)) & 1;
				Src = Src >> repeats;
				AX = (AX & 0xFF00) | (Src & 255);
				Flag_SF = ((Src >> 7) & 1);
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (repeats == 1) Flag_OF = !parity_check[(Src) & 0b11000000];
				if (log_to_console) cout << "Shift(SHR) right AL " << (int)repeats << " times";
				break;
			case 1:
				Src = (CX & 255);
				Flag_CF = (Src >> (repeats - 1)) & 1;
				Src = Src >> repeats;
				CX = (CX & 0xFF00) | (Src & 255);
				Flag_SF = ((Src >> 7) & 1);
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (repeats == 1) Flag_OF = !parity_check[(Src) & 0b11000000];
				if (log_to_console) cout << "Shift(SHR) right CL " << (int)repeats << " times";
				break;
			case 2:
				Src = (DX & 255);
				Flag_CF = (Src >> (repeats - 1)) & 1;
				Src = Src >> repeats;
				DX = (DX & 0xFF00) | (Src & 255);
				Flag_SF = ((Src >> 7) & 1);
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (repeats == 1) Flag_OF = !parity_check[(Src) & 0b11000000];
				if (log_to_console) cout << "Shift(SHR) right DL " << (int)repeats << " times";
				break;
			case 3:
				Src = (BX & 255);
				Flag_CF = (Src >> (repeats - 1)) & 1;
				Src = Src >> repeats;
				BX = (BX & 0xFF00) | (Src & 255);
				Flag_SF = ((Src >> 7) & 1);
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (repeats == 1) Flag_OF = !parity_check[(Src) & 0b11000000];
				if (log_to_console) cout << "Shift(SHR) right BL " << (int)repeats << " times";
				break;
			case 4:
				Src = AX >> 8;
				Flag_CF = (Src >> (repeats - 1)) & 1;
				Src = Src >> repeats;
				AX = (AX & 0x00FF) | ((Src & 255) << 8);
				Flag_SF = ((Src >> 7) & 1);
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (repeats == 1) Flag_OF = !parity_check[(Src) & 0b11000000];
				if (log_to_console) cout << "Shift(SHR) right AH " << (int)repeats << " times";
				break;
			case 5:
				Src = CX >> 8;
				Flag_CF = (Src >> (repeats - 1)) & 1;
				Src = Src >> repeats;
				CX = (CX & 0x00FF) | ((Src & 255) << 8);
				Flag_SF = ((Src >> 7) & 1);
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (repeats == 1) Flag_OF = !parity_check[(Src) & 0b11000000];
				if (log_to_console) cout << "Shift(SHR) right CH " << (int)repeats << " times";
				break;
			case 6:
				Src = DX >> 8;
				Flag_CF = (Src >> (repeats - 1)) & 1;
				Src = Src >> repeats;
				DX = (DX & 0x00FF) | ((Src & 255) << 8);
				Flag_SF = ((Src >> 7) & 1);
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (repeats == 1) Flag_OF = !parity_check[(Src) & 0b11000000];
				if (log_to_console) cout << "Shift(SHR) right DH " << (int)repeats << " times";
				break;
			case 7:
				Src = BX >> 8;
				Flag_CF = (Src >> (repeats - 1)) & 1;
				Src = Src >> repeats;
				BX = (BX & 0x00FF) | ((Src & 255) << 8);
				Flag_SF = ((Src >> 7) & 1);
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (repeats == 1) Flag_OF = !parity_check[(Src) & 0b11000000];
				if (log_to_console) cout << "Shift(SHR) right BH " << (int)repeats << " times";
				break;
			}
			break;
		}
		break;

	case 6: //SETMOC byte R/M

		
		Src = 0xFF;
		if (log_to_console) cout << "SETMOC_B ";
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			if (!(CX & 255)) break;
			Result = memory.read_2(New_Addr) | Src;
			memory.write_2(New_Addr,  Result);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = true;
			Flag_ZF = false;
			Flag_PF = true;
			Flag_AF = 0;
			if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)Result;
			break;
		case 3:
			// mod 11 источник - регистр
			if (!(CX & 255)) break;
			Flag_AF = 0;
			switch (byte2 & 7)
			{
			case 0:
				Result = (AX & 255) | Src;
				AX = (AX & 0xFF00) | Result;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "AL = " << (int)Result;
				break;
			case 1:
				Result = (CX & 255) | Src;
				CX = (CX & 0xFF00) | Result;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "CL = " << (int)Result;
				break;
			case 2:
				Result = (DX & 255) | Src;
				DX = (DX & 0xFF00) | Result;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "DL = " << (int)Result;
				break;
			case 3:
				Result = (BX & 255) | Src;
				BX = (BX & 0xFF00) | Result;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "BL = " << (int)Result;
				break;
			case 4:
				Result = ((AX >> 8) & 255) | Src;
				AX = (AX & 0x00FF) | (Result << 8);
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "AH = " << (int)Result;
				break;
			case 5:
				Result = ((CX >> 8) & 255) | Src;
				CX = (CX & 0x00FF) | (Result << 8);
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "CH = " << (int)Result;
				break;
			case 6:
				Result = ((DX >> 8) & 255) | Src;
				DX = (DX & 0x00FF) | (Result << 8);
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "DH = " << (int)Result;
				break;
			case 7:
				Result = ((BX >> 8) & 255) | Src;
				BX = (BX & 0x00FF) | (Result << 8);
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "BH = " << (int)Result;
				break;
			}
			break;
		}

		break;

	case 7:  //Shift Right (SAR)

		if (repeats > 9) repeats = 9;
		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			if (!repeats) break;
			Src = memory.read_2(New_Addr);
			for (int v = 0; v < repeats; v++)
			{
				Flag_CF = Src & 1;
				Flag_SF = MSB = Src & 0b10000000;
				Src = (Src >> 1) | MSB;
			}
			memory.write_2(New_Addr,  Src & 255);
			if (Src & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (repeats == 1) Flag_OF = false;
			if (log_to_console) cout << "Shift(SAR) right M" << OPCODE_comment << " " << (int)repeats << " times";
			break;
		case 3:
			// mod 11 источник - регистр
			if (!repeats) break;
			switch (byte2 & 7)
			{
			case 0:
				Src = AX & 255;
				for (int v = 0; v < repeats; v++)
				{
					Flag_CF = Src & 1;
					Flag_SF = MSB = Src & 0b10000000;
					Src = (Src >> 1) | MSB;
				}
				AX = (AX & 0xFF00) | (Src);
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (repeats == 1) Flag_OF = false;
				if (log_to_console) cout << "Shift(SAR) right AL " << (int)repeats << " times";
				break;
			case 1:
				Src = CX & 255;
				for (int v = 0; v < repeats; v++)
				{
					Flag_CF = Src & 1;
					Flag_SF = MSB = Src & 0b10000000;
					Src = (Src >> 1) | MSB;
				}
				CX = (CX & 0xFF00) | (Src);
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (repeats == 1) Flag_OF = false;
				if (log_to_console) cout << "Shift(SAR) right CL " << (int)repeats << " times";
				break;
			case 2:
				Src = DX & 255;
				for (int v = 0; v < repeats; v++)
				{
					Flag_CF = Src & 1;
					Flag_SF = MSB = Src & 0b10000000;
					Src = (Src >> 1) | MSB;
				}
				DX = (DX & 0xFF00) | (Src);
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (repeats == 1) Flag_OF = false;
				if (log_to_console) cout << "Shift(SAR) right DL " << (int)repeats << " times";
				break;
			case 3:
				Src = BX & 255;
				for (int v = 0; v < repeats; v++)
				{
					Flag_CF = Src & 1;
					Flag_SF = MSB = Src & 0b10000000;
					Src = (Src >> 1) | MSB;
				}
				BX = (BX & 0xFF00) | (Src);
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (repeats == 1) Flag_OF = false;
				if (log_to_console) cout << "Shift(SAR) right BL " << (int)repeats << " times";
				break;
			case 4:
				Src = (AX >> 8) & 255;
				for (int v = 0; v < repeats; v++)
				{
					Flag_CF = Src & 1;
					Flag_SF = MSB = Src & 0b10000000;
					Src = (Src >> 1) | MSB;
				}
				AX = (AX & 0x00FF) | (Src << 8);
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (repeats == 1) Flag_OF = false;
				if (log_to_console) cout << "Shift(SAR) right AH " << (int)repeats << " times";
				break;
			case 5:
				Src = (CX >> 8) & 255;
				for (int v = 0; v < repeats; v++)
				{
					Flag_CF = Src & 1;
					Flag_SF = MSB = Src & 0b10000000;
					Src = (Src >> 1) | MSB;
				}
				CX = (CX & 0x00FF) | (Src << 8);
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (repeats == 1) Flag_OF = false;
				if (log_to_console) cout << "Shift(SAR) right CH " << (int)repeats << " times";
				break;
			case 6:
				Src = (DX >> 8) & 255;
				for (int v = 0; v < repeats; v++)
				{
					Flag_CF = Src & 1;
					Flag_SF = MSB = Src & 0b10000000;
					Src = (Src >> 1) | MSB;
				}
				DX = (DX & 0x00FF) | (Src << 8);
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (repeats == 1) Flag_OF = false;
				if (log_to_console) cout << "Shift(SAR) right DH " << (int)repeats << " times";
				break;
			case 7:
				Src = (BX >> 8) & 255;
				for (int v = 0; v < repeats; v++)
				{
					Flag_CF = Src & 1;
					Flag_SF = MSB = Src & 0b10000000;
					Src = (Src >> 1) | MSB;
				}
				BX = (BX & 0x00FF) | (Src << 8);
				if (Src & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (repeats == 1) Flag_OF = false;
				if (log_to_console) cout << "Shift(SAR) right BH " << (int)repeats << " times";
				break;
			}
			break;
		}
		break;
	
	default:
		cout << "unknown OP = " << memory.read_2(Instruction_Pointer + *CS * 16);
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void SHL_ROT_16_mult()		// Shift Logical / Arithmetic Left / 16bit / CL
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint32 Src = 0;
	uint32 Result = 0;
	uint8 repeats = CX & 255;
	uint16 MSB = 0;
	additional_IPs = 0;
	uint16 oldCF;

	switch ((byte2 >> 3) & 7)
	{
	case 0:  //ROL Rotate Left

		//while (repeats > 16) repeats -= 16;
		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			if (!repeats) break;
			Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
			if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = (Src >> 15) & 1;
				Src = Src << 1;
				Src = Src | Flag_CF;
			}
			memory.write_2(New_Addr,  Src & 255);
			memory.write_2(New_Addr + 1,  (Src >> 8) & 255);
			if (log_to_console) cout << "ROL M" << OPCODE_comment << " " << (int)repeats << " times";
			break;
		case 3:
			// mod 11 источник - регистр
			if (!repeats) break;
			switch (byte2 & 7)
			{
			case 0:
				Src = AX;
				if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = (Src >> 15) & 1;
					Src = Src << 1;
					Src = Src | Flag_CF;
				}
				AX = Src & 0xFFFF;
				if (log_to_console) cout << "ROL AX" << (int)repeats << " times";
				break;
			case 1:
				Src = CX;
				if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = (Src >> 15) & 1;
					Src = Src << 1;
					Src = Src | Flag_CF;
				}
				CX = Src & 0xFFFF;
				if (log_to_console) cout << "ROL CX" << (int)repeats << " times";
				break;
			case 2:
				Src = DX;
				if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = (Src >> 15) & 1;
					Src = Src << 1;
					Src = Src | Flag_CF;
				}
				DX = Src & 0xFFFF;
				if (log_to_console) cout << "ROL DX" << (int)repeats << " times";
				break;
			case 3:
				Src = BX;
				if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = (Src >> 15) & 1;
					Src = Src << 1;
					Src = Src | Flag_CF;
				}
				BX = Src & 0xFFFF;
				if (log_to_console) cout << "ROL BX" << (int)repeats << " times";
				break;
			case 4:
				Src = Stack_Pointer;
				if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = (Src >> 15) & 1;
					Src = Src << 1;
					Src = Src | Flag_CF;
				}
				Stack_Pointer = Src & 0xFFFF;
				if (log_to_console) cout << "ROL SP" << (int)repeats << " times";
				break;
			case 5:
				Src = Base_Pointer;
				if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = (Src >> 15) & 1;
					Src = Src << 1;
					Src = Src | Flag_CF;
				}
				Base_Pointer = Src & 0xFFFF;
				if (log_to_console) cout << "ROL BP" << (int)repeats << " times";
				break;
			case 6:
				Src = Source_Index;
				if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = (Src >> 15) & 1;
					Src = Src << 1;
					Src = Src | Flag_CF;
				}
				Source_Index = Src & 0xFFFF;
				if (log_to_console) cout << "ROL SI" << (int)repeats << " times";
				break;
			case 7:
				Src = Destination_Index;
				if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = (Src >> 15) & 1;
					Src = Src << 1;
					Src = Src | Flag_CF;
				}
				Destination_Index = Src & 0xFFFF;
				if (log_to_console) cout << "ROL DI" << (int)repeats << " times";
				break;
			}
			break;
		}
		break;

	case 1:  //ROR = Rotate Right
		
		//while (repeats > 16) repeats -= 16;
		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			if (!repeats) break;
			Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = Src & 1;
				Src = (Src >> 1) | (Flag_CF * 0x8000);
			}
			if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
			memory.write_2(New_Addr,  Src & 255);
			memory.write_2(New_Addr + 1,  (Src >> 8) & 255);
			if (log_to_console) cout << "ROR M" << OPCODE_comment << " " << (int)repeats << " times";
			break;
		case 3:
			// mod 11 источник - регистр
			if (!repeats) break;
			switch (byte2 & 7)
			{
			case 0:
				Src = AX;
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = Src & 1;
					Src = (Src >> 1) | (Flag_CF * 0x8000);
				}
				if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
				AX = Src;
				if (log_to_console) cout << "ROR AX" << (int)repeats << " times";
				break;
			case 1:
				Src = CX;
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = Src & 1;
					Src = (Src >> 1) | (Flag_CF * 0x8000);
				}
				if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
				CX = Src;
				if (log_to_console) cout << "ROR CX" << (int)repeats << " times";
				break;
			case 2:
				Src = DX;
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = Src & 1;
					Src = (Src >> 1) | (Flag_CF * 0x8000);
				}
				if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
				DX = Src;
				if (log_to_console) cout << "ROR DX" << (int)repeats << " times";
				break;
			case 3:
				Src = BX;
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = Src & 1;
					Src = (Src >> 1) | (Flag_CF * 0x8000);
				}
				if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
				BX = Src;
				if (log_to_console) cout << "ROR BX" << (int)repeats << " times";
				break;
			case 4:
				Src = Stack_Pointer;
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = Src & 1;
					Src = (Src >> 1) | (Flag_CF * 0x8000);
				}
				if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
				Stack_Pointer = Src;
				if (log_to_console) cout << "ROR SP" << (int)repeats << " times";
				break;
			case 5:
				Src = Base_Pointer;
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = Src & 1;
					Src = (Src >> 1) | (Flag_CF * 0x8000);
				}
				if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
				Base_Pointer = Src;
				if (log_to_console) cout << "ROR BP" << (int)repeats << " times";
				break;
			case 6:
				Src = Source_Index;
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = Src & 1;
					Src = (Src >> 1) | (Flag_CF * 0x8000);
				}
				if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
				Source_Index = Src;
				if (log_to_console) cout << "ROR SI" << (int)repeats << " times";
				break;
			case 7:
				Src = Destination_Index;
				for (int i = 0; i < repeats; i++)
				{
					Flag_CF = Src & 1;
					Src = (Src >> 1) | (Flag_CF * 0x8000);
				}
				if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
				Destination_Index = Src;
				if (log_to_console) cout << "ROR DI" << (int)repeats << " times";
				break;
			}
			break;
		}
		break;

	case 2:  //RCL Rotate Left throught carry
		
		//while (repeats > 17) repeats -= 17;
		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			if (!repeats) break;
			Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
			for (int i = 0; i < repeats; i++)
			{
				Src = (Src << 1) | Flag_CF;
				Flag_CF = (Src >> 16) & 1;
			}
			if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
			memory.write_2(New_Addr,  Src & 255);
			memory.write_2(New_Addr + 1,  (Src >> 8) & 255);
			if (log_to_console) cout << "RCL M" << OPCODE_comment << " " << (int)repeats << " times";
			break;
		case 3:
			// mod 11 источник - регистр
			if (!repeats) break;
			switch (byte2 & 7)
			{
			case 0:
				Src = AX;
				for (int i = 0; i < repeats; i++)
				{
					Src = (Src << 1) | Flag_CF;
					Flag_CF = (Src >> 16) & 1;
				}
				if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
				AX = Src & 0xFFFF;
				if (log_to_console) cout << "RCL AX " << (int)repeats << " times";
				break;
			case 1:
				Src = CX;
				for (int i = 0; i < repeats; i++)
				{
					Src = (Src << 1) | Flag_CF;
					Flag_CF = (Src >> 16) & 1;
				}
				if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
				CX = Src & 0xFFFF;
				if (log_to_console) cout << "RCL CX " << (int)repeats << " times";
				break;
			case 2:
				Src = DX;
				for (int i = 0; i < repeats; i++)
				{
					Src = (Src << 1) | Flag_CF;
					Flag_CF = (Src >> 16) & 1;
				}
				if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
				DX = Src & 0xFFFF;
				if (log_to_console) cout << "RCL DX " << (int)repeats << " times";
				break;
			case 3:
				Src = BX;
				for (int i = 0; i < repeats; i++)
				{
					Src = (Src << 1) | Flag_CF;
					Flag_CF = (Src >> 16) & 1;
				}
				if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
				BX = Src & 0xFFFF;
				if (log_to_console) cout << "RCL BX " << (int)repeats << " times";
				break;
			case 4:
				Src = Stack_Pointer;
				for (int i = 0; i < repeats; i++)
				{
					Src = (Src << 1) | Flag_CF;
					Flag_CF = (Src >> 16) & 1;
				}
				if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
				Stack_Pointer = Src & 0xFFFF;
				if (log_to_console) cout << "RCL SP " << (int)repeats << " times";
				break;
			case 5:
				Src = Base_Pointer;
				for (int i = 0; i < repeats; i++)
				{
					Src = (Src << 1) | Flag_CF;
					Flag_CF = (Src >> 16) & 1;
				}
				if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
				Base_Pointer = Src & 0xFFFF;
				if (log_to_console) cout << "RCL BP " << (int)repeats << " times";
				break;
			case 6:
				Src = Source_Index;
				for (int i = 0; i < repeats; i++)
				{
					Src = (Src << 1) | Flag_CF;
					Flag_CF = (Src >> 16) & 1;
				}
				if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
				Source_Index = Src & 0xFFFF;
				if (log_to_console) cout << "RCL SI " << (int)repeats << " times";
				break;
			case 7:
				Src = Destination_Index;
				for (int i = 0; i < repeats; i++)
				{
					Src = (Src << 1) | Flag_CF;
					Flag_CF = (Src >> 16) & 1;
				}
				if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
				Destination_Index = Src & 0xFFFF;
				if (log_to_console) cout << "RCL DI " << (int)repeats << " times";
				break;
			}
			break;
		}
		break;

	case 3:  //RCR = Rotate Right throught carry

		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			if (!repeats) break;
			Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
			for (int i = 0; i < repeats; i++)
			{
				oldCF = Flag_CF;
				Flag_CF = Src & 1;
				Src = (Src >> 1) | (oldCF << 15);
			}
			if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
			memory.write_2(New_Addr,  Src & 255);
			memory.write_2(New_Addr + 1,  (Src >> 8) & 255);
			if (log_to_console) cout << "RCR M" << OPCODE_comment << " " << (int)repeats << " times";
			break;
		case 3:
			// mod 11 источник - регистр
			if (!repeats) break;
			switch (byte2 & 7)
			{
			case 0:
				Src = AX;
				for (int i = 0; i < repeats; i++)
				{
					oldCF = Flag_CF;
					Flag_CF = Src & 1;
					Src = (Src >> 1) | (oldCF << 15);
				}
				if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
				AX = Src;
				if (log_to_console) cout << "RCR AX " << (int)repeats << " times";
				break;
			case 1:
				Src = CX;
				for (int i = 0; i < repeats; i++)
				{
					oldCF = Flag_CF;
					Flag_CF = Src & 1;
					Src = (Src >> 1) | (oldCF << 15);
				}
				if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
				CX = Src;
				if (log_to_console) cout << "RCR CX " << (int)repeats << " times";
				break;
			case 2:
				Src = DX;
				for (int i = 0; i < repeats; i++)
				{
					oldCF = Flag_CF;
					Flag_CF = Src & 1;
					Src = (Src >> 1) | (oldCF << 15);
				}
				if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
				DX = Src;
				if (log_to_console) cout << "RCR DX " << (int)repeats << " times";
				break;
			case 3:
				Src = BX;
				for (int i = 0; i < repeats; i++)
				{
					oldCF = Flag_CF;
					Flag_CF = Src & 1;
					Src = (Src >> 1) | (oldCF << 15);
				}
				if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
				BX = Src;
				if (log_to_console) cout << "RCR BX " << (int)repeats << " times";
				break;
			case 4:
				Src = Stack_Pointer;
				for (int i = 0; i < repeats; i++)
				{
					oldCF = Flag_CF;
					Flag_CF = Src & 1;
					Src = (Src >> 1) | (oldCF << 15);
				}
				if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
				Stack_Pointer = Src;
				if (log_to_console) cout << "RCR SP " << (int)repeats << " times";
				break;
			case 5:
				Src = Base_Pointer;
				for (int i = 0; i < repeats; i++)
				{
					oldCF = Flag_CF;
					Flag_CF = Src & 1;
					Src = (Src >> 1) | (oldCF << 15);
				}
				if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
				Base_Pointer = Src;
				if (log_to_console) cout << "RCR BP " << (int)repeats << " times";
				break;
			case 6:
				Src = Source_Index;
				for (int i = 0; i < repeats; i++)
				{
					oldCF = Flag_CF;
					Flag_CF = Src & 1;
					Src = (Src >> 1) | (oldCF << 15);
				}
				if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
				Source_Index = Src;
				if (log_to_console) cout << "RCR SI " << (int)repeats << " times";
				break;
			case 7:
				Src = Destination_Index;
				for (int i = 0; i < repeats; i++)
				{
					oldCF = Flag_CF;
					Flag_CF = Src & 1;
					Src = (Src >> 1) | (oldCF << 15);
				}
				if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
				Destination_Index = Src;
				if (log_to_console) cout << "RCR DI " << (int)repeats << " times";
				break;
			}
			break;
		}
		break;
	
	case 4:  //Shift Left

		if (repeats > 17) repeats = 17;
		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			if (!repeats) break;
			Src = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) << repeats;
			if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
			memory.write_2(New_Addr,  Src & 255);
			memory.write_2(New_Addr + 1,  (Src >> 8) & 255);
			Flag_CF = (Src >> 16) & 1;
			Flag_SF = ((Src >> 15) & 1);
			if (Src & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift left M" << OPCODE_comment << " " << (int)repeats << " times";
			break;
		case 3:
			// mod 11 источник - регистр
			if (!repeats) break;
			switch (byte2 & 7)
			{
			case 0:
				Src = AX << repeats;
				if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
				AX = Src & 0xFFFF;
				Flag_CF = (Src >> 16) & 1;
				Flag_SF = ((Src >> 15) & 1);
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left AX " << (int)repeats << " times";
				break;
			case 1:
				Src = CX << repeats;
				if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
				CX = Src & 0xFFFF;
				Flag_CF = (Src >> 16) & 1;
				Flag_SF = ((Src >> 15) & 1);
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left CX " << (int)repeats << " times";
				break;
			case 2:
				Src = DX << repeats;
				if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
				DX = Src & 0xFFFF;
				Flag_CF = (Src >> 16) & 1;
				Flag_SF = ((Src >> 15) & 1);
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left DX " << (int)repeats << " times";
				break;
			case 3:
				Src = BX << repeats;
				if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
				BX = Src & 0xFFFF;
				Flag_CF = (Src >> 16) & 1;
				Flag_SF = ((Src >> 15) & 1);
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left BX " << (int)repeats << " times" ;
				break;
			case 4:
				Src = Stack_Pointer << repeats;
				if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
				Stack_Pointer = Src & 0xFFFF;
				Flag_CF = (Src >> 16) & 1;
				Flag_SF = ((Src >> 15) & 1);
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left SP " << (int)repeats << " times";
				break;
			case 5:
				Src = Base_Pointer << repeats;
				if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
				Base_Pointer = Src & 0xFFFF;
				Flag_CF = (Src >> 16) & 1;
				Flag_SF = ((Src >> 15) & 1);
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left BP " << (int)repeats << " times";
				break;
			case 6:
				Src = Source_Index << repeats;
				if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
				Source_Index = Src & 0xFFFF;
				Flag_CF = (Src >> 16) & 1;
				Flag_SF = ((Src >> 15) & 1);
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left SI " << (int)repeats << " times";
				break;
			case 7:
				Src = Destination_Index << repeats;
				if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
				Destination_Index = Src & 0xFFFF;
				Flag_CF = (Src >> 16) & 1;
				Flag_SF = ((Src >> 15) & 1);
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift left DI " << (int)repeats << " times";
				break;
			}
			break;
		}
		break;

	case 5:  //Shift right (SHR)
		if (repeats > 17) repeats = 17;
		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			if (!repeats) break;
			Src = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256);
			Flag_CF = (Src >> (repeats - 1)) & 1;
			Src = Src >> repeats;
			memory.write_2(New_Addr,  Src & 255);
			memory.write_2(New_Addr + 1,  (Src >> 8) & 255);
			Flag_SF = ((Src >> 15) & 1);
			if (Src & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			if (repeats == 1) Flag_OF = Flag_SF;
			else Flag_OF = 0;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift(SHR) right M" << OPCODE_comment << " " << (int)repeats << " times";
			break;
		case 3:
			// mod 11 источник - регистр
			if (!repeats) break;
			switch (byte2 & 7)
			{
			case 0:
				Src = AX >> repeats;
				Flag_CF = (AX >> (repeats - 1)) & 1;
				AX = Src & 0xFFFF;
				Flag_SF = ((Src >> 15) & 1);
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				if (repeats == 1) Flag_OF = Flag_SF;
				else Flag_OF = 0;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SHR) right AX " << (int)repeats << " times";
				break;
			case 1:
				Src = CX >> repeats;
				Flag_CF = (CX >> (repeats - 1)) & 1;
				CX = Src & 0xFFFF;
				Flag_SF = ((Src >> 15) & 1);
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				if (repeats == 1) Flag_OF = Flag_SF;
				else Flag_OF = 0;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SHR) right CX " << (int)repeats << " times";
				break;
			case 2:
				Src = DX >> repeats;
				Flag_CF = (DX >> (repeats - 1)) & 1;
				DX = Src & 0xFFFF;
				Flag_SF = ((Src >> 15) & 1);
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				if (repeats == 1) Flag_OF = Flag_SF;
				else Flag_OF = 0;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SHR) right DX " << (int)repeats << " times";
				break;
			case 3:
				Src = BX >> repeats;
				Flag_CF = (BX >> (repeats - 1)) & 1;
				BX = Src & 0xFFFF;
				Flag_SF = ((Src >> 15) & 1);
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				if (repeats == 1) Flag_OF = Flag_SF;
				else Flag_OF = 0;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SHR) right BX " << (int)repeats << " times";
				break;
			case 4:
				Src = Stack_Pointer >> repeats;
				Flag_CF = (Stack_Pointer >> (repeats - 1)) & 1;
				Stack_Pointer = Src & 0xFFFF;
				Flag_SF = ((Src >> 15) & 1);
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				if (repeats == 1) Flag_OF = Flag_SF;
				else Flag_OF = 0;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SHR) right SP " << (int)repeats << " times";
				break;
			case 5:
				Src = Base_Pointer >> repeats;
				Flag_CF = (Base_Pointer >> (repeats - 1)) & 1;
				Base_Pointer = Src & 0xFFFF;
				Flag_SF = ((Src >> 15) & 1);
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				if (repeats == 1) Flag_OF = Flag_SF;
				else Flag_OF = 0;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SHR) right BP " << (int)repeats << " times";
				break;
			case 6:
				Src = Source_Index >> repeats;
				Flag_CF = (Source_Index >> (repeats - 1)) & 1;
				Source_Index = Src & 0xFFFF;
				Flag_SF = ((Src >> 15) & 1);
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				if (repeats == 1) Flag_OF = Flag_SF;
				else Flag_OF = 0;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SHR) right SI " << (int)repeats << " times";
				break;
			case 7:
				Src = Destination_Index >> repeats;
				Flag_CF = (Destination_Index >> (repeats - 1)) & 1;
				Destination_Index = Src & 0xFFFF;
				Flag_SF = ((Src >> 15) & 1);
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				if (repeats == 1) Flag_OF = Flag_SF;
				else Flag_OF = 0;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SHR) right DI " << (int)repeats << " times";
				break;
			}
			break;
		}
		break;

	case 6:  //SETMOC word R/M
		
		Src = 0xFFFF;
		if (log_to_console) cout << "SETMO WORD ";
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			if (!(CX & 255)) break;
			Result = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) | Src;
			memory.write_2(New_Addr,  Result & 255);
			memory.write_2(New_Addr + 1,  (Result >> 8) & 255);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = true;
			Flag_ZF = false;
			Flag_PF = true;
			Flag_AF = 0;
			if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)Result;
			break;
		case 3:
			// mod 11 источник - регистр
			if (!(CX & 255)) break;
			Flag_AF = 0;
			switch (byte2 & 7)
			{
			case 0:
				AX = AX | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "AL = " << (int)Result;
				break;
			case 1:
				CX = CX | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "CL = " << (int)Result;
				break;
			case 2:
				DX = DX | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "DL = " << (int)Result;
				break;
			case 3:
				BX = BX | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "BL = " << (int)Result;
				break;
			case 4:
				Stack_Pointer = Stack_Pointer | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "AH = " << (int)Result;
				break;
			case 5:
				Base_Pointer = Base_Pointer | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "CH = " << (int)Result;
				break;
			case 6:
				Source_Index = Source_Index | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "DH = " << (int)Result;
				break;
			case 7:
				Destination_Index = Destination_Index | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = true;
				Flag_ZF = false;
				Flag_PF = true;
				if (log_to_console) cout << "BH = " << (int)Result;
				break;
			}
			break;
		}
		break;

	case 7:  //Shift right (SAR)

		if (repeats > 16) repeats = 16;
		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			if (!repeats) break;
			Src = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256);
			if (Src & 0x8000) MSB = 0b1111111111111111 << (16 - repeats);
			Flag_CF = (Src >> (repeats - 1)) & 1;
			Src = (Src >> repeats) | MSB;
			memory.write_2(New_Addr,  Src & 255);
			memory.write_2(New_Addr + 1,  (Src >> 8) & 255);
			Flag_SF = ((Src >> 15) & 1);
			if (Src & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			if (repeats == 1) Flag_OF = 0;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift(SAR) right M" << OPCODE_comment << " " << (int)repeats << " times";
			break;
		case 3:
			// mod 11 источник - регистр
			if (!repeats) break;
			switch (byte2 & 7)
			{
			case 0:
				if (AX & 0x8000) MSB = 0b1111111111111111 << (16 - repeats);
				Src = (AX >> repeats) | MSB;
				Flag_CF = (AX >> (repeats - 1)) & 1;
				AX = Src & 0xFFFF;
				Flag_SF = ((Src >> 15) & 1);
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				if (repeats == 1) Flag_OF = 0;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SAR) right AX " << (int)repeats << " times";
				break;
			case 1:
				if (CX & 0x8000) MSB = 0b1111111111111111 << (16 - repeats);
				Src = (CX >> repeats) | MSB;
				Flag_CF = (CX >> (repeats - 1)) & 1;
				CX = Src & 0xFFFF;
				Flag_SF = ((Src >> 15) & 1);
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				if (repeats == 1) Flag_OF = 0;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SAR) right CX " << (int)repeats << " times";
				break;
			case 2:
				if (DX & 0x8000) MSB = 0b1111111111111111 << (16 - repeats);
				Src = (DX >> repeats) | MSB;
				Flag_CF = (DX >> (repeats - 1)) & 1;
				DX = Src & 0xFFFF;
				Flag_SF = ((Src >> 15) & 1);
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				if (repeats == 1) Flag_OF = 0;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SAR) right DX " << (int)repeats << " times";
				break;
			case 3:
				if (BX & 0x8000) MSB = 0b1111111111111111 << (16 - repeats);
				Src = (BX >> repeats) | MSB;
				Flag_CF = (BX >> (repeats - 1)) & 1;
				BX = Src & 0xFFFF;
				Flag_SF = ((Src >> 15) & 1);
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				if (repeats == 1) Flag_OF = 0;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SAR) right BX " << (int)repeats << " times";
				break;
			case 4:
				if (Stack_Pointer & 0x8000) MSB = 0b1111111111111111 << (16 - repeats);
				Src = (Stack_Pointer >> repeats) | MSB;
				Flag_CF = (Stack_Pointer >> (repeats - 1)) & 1;
				Stack_Pointer = Src & 0xFFFF;
				Flag_SF = ((Src >> 15) & 1);
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				if (repeats == 1) Flag_OF = 0;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SAR) right SP " << (int)repeats << " times";
				break;
			case 5:
				if (Base_Pointer & 0x8000) MSB = 0b1111111111111111 << (16 - repeats);
				Src = (Base_Pointer >> repeats) | MSB;
				Flag_CF = (Base_Pointer >> (repeats - 1)) & 1;
				Base_Pointer = Src & 0xFFFF;
				Flag_SF = ((Src >> 15) & 1);
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				if (repeats == 1) Flag_OF = 0;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SAR) right BP " << (int)repeats << " times";
				break;
			case 6:
				if (Source_Index & 0x8000) MSB = 0b1111111111111111 << (16 - repeats);
				Src = (Source_Index >> repeats) | MSB;
				Flag_CF = (Source_Index >> (repeats - 1)) & 1;
				Source_Index = Src & 0xFFFF;
				Flag_SF = ((Src >> 15) & 1);
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				if (repeats == 1) Flag_OF = 0;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SAR) right SI " << (int)repeats << " times";
				break;
			case 7:
				if (Destination_Index & 0x8000) MSB = 0b1111111111111111 << (16 - repeats);
				Src = (Destination_Index >> repeats) | MSB;
				Flag_CF = (Destination_Index >> (repeats - 1)) & 1;
				Destination_Index = Src & 0xFFFF;
				Flag_SF = ((Src >> 15) & 1);
				if (Src & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				if (repeats == 1) Flag_OF = 0;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "Shift(SAR) right DI " << (int)repeats << " times";
				break;
			}
			break;
		}
		break;
	
	default:
		cout << "unknown OP = " << memory.read_2(Instruction_Pointer + *CS * 16);
	}
	Instruction_Pointer += 2 + additional_IPs;
}

//AND
void AND_RM_8()				// AND R + R/M -> R/M 8bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 Src = 0;
	uint8 Result = 0;
	additional_IPs = 0;

	//определяем регистр - источник
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Src = AX & 255;
		if (log_to_console) cout << "AL(" << (int)Src << ") ";
		break;
	case 1:
		Src = CX & 255;
		if (log_to_console) cout << "CL(" << (int)Src << ") ";
		break;
	case 2:
		Src = DX & 255;
		if (log_to_console) cout << "DL(" << (int)Src << ") ";
		break;
	case 3:
		Src = BX & 255;
		if (log_to_console) cout << "BL(" << (int)Src << ") ";
		break;
	case 4:
		Src = AX >> 8;
		if (log_to_console) cout << "AH(" << (int)Src << ") ";
		break;
	case 5:
		Src = CX >> 8;
		if (log_to_console) cout << "CH(" << (int)Src << ") ";
		break;
	case 6:
		Src = DX >> 8;
		if (log_to_console) cout << "DH(" << (int)Src << ") ";
		break;
	case 7:
		Src = BX >> 8;
		if (log_to_console) cout << "BH(" << (int)Src << ") ";
		break;
	}

	//определяем объект назначения и результат операции
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		if (log_to_console) cout << "AND M" << OPCODE_comment << " = ";
		Result = memory.read_2(New_Addr) & Src;
		memory.write_2(New_Addr,  Result);
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 7) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			if (log_to_console) cout << "AND AL(" << (int)(AX & 255) << ") = ";
			Result = (AX & 255) & Src;
			AX = (AX & 0xFF00) | Result;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << (int)Result;
			break;
		case 1:
			if (log_to_console) cout << "AND CL(" << (int)(CX & 255) << ") = ";
			Result = (CX & 255) & Src;
			CX = (CX & 0xFF00) | Result;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << (int)Result;
			break;
		case 2:
			if (log_to_console) cout << "AND DL(" << (int)(DX & 255) << ") = ";
			Result = (DX & 255) & Src;
			DX = (DX & 0xFF00) | Result;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << (int)Result;
			break;
		case 3:
			if (log_to_console) cout << "AND BL(" << (int)(BX & 255) << ") = ";
			Result = (BX & 255) & Src;
			BX = (BX & 0xFF00) | Result;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << (int)Result;
			break;
		case 4:
			if (log_to_console) cout << "AND AH(" << (int)((AX >> 8) & 255) << ") = ";
			Result = ((AX >> 8) & 255) & Src;
			AX = (AX & 0x00FF) | (Result << 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << (int)Result;
			break;
		case 5:
			if (log_to_console) cout << "AND CH(" << (int)((CX >> 8) & 255) << ") = ";
			Result = ((CX >> 8) & 255) & Src;
			CX = (CX & 0x00FF) | (Result << 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << (int)Result;
			break;
		case 6:
			if (log_to_console) cout << "AND DH(" << (int)((DX >> 8) & 255) << ") = ";
			Result = ((DX >> 8) & 255) & Src;
			DX = (DX & 0x00FF) | (Result << 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << (int)Result;
			break;
		case 7:
			if (log_to_console) cout << "AND BH(" << (int)((BX >> 8) & 255) << ") = ";
			Result = ((BX >> 8) & 255) & Src;
			BX = (BX & 0x00FF) | (Result << 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << (int)Result;
			break;
		}
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void AND_RM_16()			// AND R + R/M -> R/M 16bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Src = 0;
	uint16 Result = 0;
	additional_IPs = 0;

	//определяем регистр - источник
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Src = AX;
		if (log_to_console) cout << "AX(" << (int)Src << ") ";
		break;
	case 1:
		Src = CX;
		if (log_to_console) cout << "CX(" << (int)Src << ") ";
		break;
	case 2:
		Src = DX;
		if (log_to_console) cout << "DX(" << (int)Src << ") ";
		break;
	case 3:
		Src = BX;
		if (log_to_console) cout << "BX(" << (int)Src << ") ";
		break;
	case 4:
		Src = Stack_Pointer;
		if (log_to_console) cout << "SP(" << (int)Src << ") ";
		break;
	case 5:
		Src = Base_Pointer;
		if (log_to_console) cout << "BP(" << (int)Src << ") ";
		break;
	case 6:
		Src = Source_Index;
		if (log_to_console) cout << "SI(" << (int)Src << ") ";
		break;
	case 7:
		Src = Destination_Index;
		if (log_to_console) cout << "DI(" << (int)Src << ") ";
		break;
	}

	//определяем объект назначения и результат операции
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		if (log_to_console) cout << "AND M" << OPCODE_comment << " = ";
		Result = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & Src;
		memory.write_2(New_Addr,  Result & 255);
		memory.write_2(New_Addr + 1,  Result >> 8);
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 15) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			if (log_to_console) cout << "AND AX(" << (int)AX << ") = ";
			AX = AX & Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((AX >> 15) & 1);
			if (AX) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[AX & 255];
			if (log_to_console) cout << (int)AX;
			break;
		case 1:
			if (log_to_console) cout << "AND CX(" << (int)CX << ") = ";
			CX = CX & Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((CX >> 15) & 1);
			if (CX) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[CX & 255];
			if (log_to_console) cout << (int)CX;
			break;
		case 2:
			if (log_to_console) cout << "AND DX(" << (int)DX << ") = ";
			DX = DX & Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((DX >> 15) & 1);
			if (DX) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[DX & 255];
			if (log_to_console) cout << (int)DX;
			break;
		case 3:
			if (log_to_console) cout << "AND BX(" << (int)BX << ") = ";
			BX = BX & Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((BX >> 15) & 1);
			if (BX) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[BX & 255];
			if (log_to_console) cout << (int)BX;
			break;
		case 4:
			if (log_to_console) cout << "AND SP(" << (int)Stack_Pointer << ") = ";
			Stack_Pointer = Stack_Pointer & Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Stack_Pointer >> 15) & 1);
			if (Stack_Pointer) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Stack_Pointer & 255];
			if (log_to_console) cout << (int)Stack_Pointer;
			break;
		case 5:
			if (log_to_console) cout << "AND BP(" << (int)Base_Pointer << ") = ";
			Base_Pointer = Base_Pointer & Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Base_Pointer >> 15) & 1);
			if (Base_Pointer) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Base_Pointer & 255];
			if (log_to_console) cout << (int)Base_Pointer;
			break;
		case 6:
			if (log_to_console) cout << "AND SI(" << (int)Source_Index << ") = ";
			Source_Index = Source_Index & Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Source_Index >> 15) & 1);
			if (Source_Index) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Source_Index & 255];
			if (log_to_console) cout << (int)Source_Index;
			break;
		case 7:
			if (log_to_console) cout << "AND DI(" << (int)Destination_Index << ") = ";
			Destination_Index = Destination_Index & Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Destination_Index >> 15) & 1);
			if (Destination_Index) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Destination_Index & 255];
			if (log_to_console) cout << (int)Destination_Index;
			break;
		}
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void AND_RM_R_8()			// AND R + R/M -> R 8bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	New_Addr = 0;
	uint8 Src = 0;
	uint8 Result = 0;
	additional_IPs = 0;

	//определяем источник
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Src = memory.read_2(New_Addr);
		if (log_to_console) cout << "M" << OPCODE_comment << " AND ";
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Src = AX & 255;
			if (log_to_console) cout << "AL(" << (int)Src << ") AND ";
			break;
		case 1:
			Src = CX & 255;
			if (log_to_console) cout << "CL(" << (int)Src << ") AND ";
			break;
		case 2:
			Src = DX & 255;
			if (log_to_console) cout << "DL(" << (int)Src << ") AND ";
			break;
		case 3:
			Src = BX & 255;
			if (log_to_console) cout << "BL(" << (int)Src << ") AND ";
			break;
		case 4:
			Src = AX >> 8;
			if (log_to_console) cout << "AH(" << (int)Src << ") AND ";
			break;
		case 5:
			Src = CX >> 8;
			if (log_to_console) cout << "CH(" << (int)Src << ") AND ";
			break;
		case 6:
			Src = DX >> 8;
			if (log_to_console) cout << "DH(" << (int)Src << ") AND ";
			break;
		case 7:
			Src = BX >> 8;
			if (log_to_console) cout << "BH(" << (int)Src << ") AND ";
			break;
		}
		break;
	}

	//определяем регистр - получатель
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		if (log_to_console) cout << "AL = ";
		Result = Src & (AX & 255);
		AX = (AX & 0xFF00) | Result;
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 1:
		if (log_to_console) cout << "CL = ";
		Result = Src & (CX & 255);
		CX = (CX & 0xFF00) | Result;
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 2:
		if (log_to_console) cout << "DL = ";
		Result = Src & (DX & 255);
		DX = (DX & 0xFF00) | Result;
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 3:
		if (log_to_console) cout << "BL = ";
		Result = Src & (BX & 255);
		BX = (BX & 0xFF00) | Result;
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 4:
		if (log_to_console) cout << "AH = ";
		Result = Src & (AX >> 8);
		AX = (AX & 0x00FF) | (Result << 8);
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 5:
		if (log_to_console) cout << "CH = ";
		Result = Src & (CX >> 8);
		CX = (CX & 0x00FF) | (Result << 8);
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 6:
		if (log_to_console) cout << "DH = ";
		Result = Src & (DX >> 8);
		DX = (DX & 0x00FF) | (Result << 8);
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 7:
		if (log_to_console) cout << "BH = ";
		Result = Src & (BX >> 8);
		BX = (BX & 0x00FF) | (Result << 8);
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)Result;
		break;
	}

	Instruction_Pointer += 2 + additional_IPs;
}
void AND_RM_R_16()			// AND R + R/M -> R 16bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Src = 0;
	uint16 Result = 0;
	additional_IPs = 0;

	//определяем источник
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
		if (log_to_console) cout << OPCODE_comment;
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Src = AX;
			if (log_to_console) cout << "AX("<< (int)AX << ") ";
			break;
		case 1:
			Src = CX;
			if (log_to_console) cout << "CX(" << (int)CX << ") ";
			break;
		case 2:
			Src = DX;
			if (log_to_console) cout << "DX(" << (int)DX << ") ";
			break;
		case 3:
			Src = BX;
			if (log_to_console) cout << "BX(" << (int)BX << ") ";
			break;
		case 4:
			Src = Stack_Pointer;
			if (log_to_console) cout << "SP(" << (int)Stack_Pointer << ") ";
			break;
		case 5:
			Src = Base_Pointer;
			if (log_to_console) cout << "BP(" << (int)Base_Pointer << ") ";
			break;
		case 6:
			Src = Source_Index;
			if (log_to_console) cout << "SI(" << (int)Source_Index << ") ";
			break;
		case 7:
			Src = Destination_Index;
			if (log_to_console) cout << "DI(" << (int)Destination_Index << ") ";
			break;
		}
		break;
	}

	//определяем регистр - получатель
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		if (log_to_console) cout << "AND AX(" << (int)AX << ") = ";
		AX = Src & AX;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((AX >> 15) & 1);
		if (AX) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[AX & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 1:
		if (log_to_console) cout << "AND CX(" << (int)CX << ") = ";
		CX = Src & CX;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((CX >> 15) & 1);
		if (CX) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[CX & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 2:
		if (log_to_console) cout << "AND DX(" << (int)DX << ") = ";
		DX = Src & DX;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((DX >> 15) & 1);
		if (DX) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[DX & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 3:
		if (log_to_console) cout << "AND BX(" << (int)BX << ") = ";
		BX = Src & BX;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((BX >> 15) & 1);
		if (BX) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[BX & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 4:
		if (log_to_console) cout << "AND SP(" << (int)Stack_Pointer << ") = ";
		Stack_Pointer = Src & Stack_Pointer;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Stack_Pointer >> 15) & 1);
		if (Stack_Pointer) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Stack_Pointer & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 5:
		if (log_to_console) cout << "AND BP(" << (int)Base_Pointer << ") = ";
		Base_Pointer = Src & Base_Pointer;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Base_Pointer >> 15) & 1);
		if (Base_Pointer) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Base_Pointer & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 6:
		if (log_to_console) cout << "AND SI(" << (int)Source_Index << ") = ";
		Source_Index = Src & Source_Index;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Source_Index >> 15) & 1);
		if (Source_Index) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Source_Index & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 7:
		if (log_to_console) cout << "AND DI(" << (int)Destination_Index << ") = ";
		Destination_Index = Src & Destination_Index;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Destination_Index >> 15) & 1);
		if (Destination_Index) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Destination_Index & 255];
		if (log_to_console) cout << (int)Result;
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void AND_IMM_ACC_8()		//AND IMM to ACC 8bit
{
	uint8 Src = 0;
	uint8 Result = 0;

	//непосредственный операнд
	Src = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];

	if (log_to_console) cout << "IMM(" << (int)Src << ") AND AL(" << (int)(AX & 255) << ") ";

	//определяем объект назначения и результат операции
	Result = (AX & 255) & Src;
	AX = (AX & 0xFF00) | Result;
	Flag_CF = 0;
	Flag_OF = 0;
	Flag_SF = ((Result >> 7) & 1);
	if (Result) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result];
	if (log_to_console) cout << " = " << (int)Result;

	Instruction_Pointer += 2;
}
void AND_IMM_ACC_16()		//AND IMM to ACC 16bit
{
	uint16 Src = 0;
	uint16 Result = 0;

	//непосредственный операнд
	Src = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory.read_2(Instruction_Pointer + 2 + *CS * 16) * 256;

	if (log_to_console) cout << "IMM(" << (int)Src << ") AND AX(" << (int)AX << ")";

	//определяем объект назначения и результат операции
	Result = AX & Src;
	AX = Result;
	Flag_CF = 0;
	Flag_OF = 0;
	Flag_SF = ((Result >> 15) & 1);
	if (Result) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result & 255];
	if (log_to_console) cout << " = " << (int)Result;

	Instruction_Pointer += 3;
}

//TEST

void TEST_8()		  //TEST = AND Function to Flags
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 Result = 0;

	//определяем источник
	if((byte2 >> 6 == 3))
	{
		// mod 11 источник - регистр (byte2 & 7)
		additional_IPs = 0;
		if (log_to_console) cout << "TEST " << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") AND " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ")";
		Result = *ptr_r8[byte2 & 7] & *ptr_r8[(byte2 >> 3) & 7];
	}
	else
	{
		//память
		mod_RM_3(byte2);
		Result = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] & *ptr_r8[(byte2 >> 3) & 7];
		if (log_to_console) cout << "TEST M" << OPCODE_comment << " AND " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ")";
	}
	
	Flag_CF = 0;
	Flag_SF = ((Result >> 7) & 1);
	Flag_OF = 0;
	if (Result) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result];
	Instruction_Pointer += 2 + additional_IPs;
}
void TEST_16()       //TEST = AND Function to Flags
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Result = 0;

	//определяем источник
	if ((byte2 >> 6 == 3))
	{
		// mod 11 источник - регистр (byte2 & 7)
		additional_IPs = 0;
		if (log_to_console) cout << "TEST " << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") AND " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ")";
		Result = *ptr_r16[byte2 & 7] & *ptr_r16[(byte2 >> 3) & 7];
	}
	else
	{
		//память
		mod_RM_3(byte2);
		Result = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		Result = Result + memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		Result = Result & *ptr_r16[(byte2 >> 3) & 7];
		if (log_to_console) cout << "TEST M" << OPCODE_comment << " AND " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ")";
	}

	Flag_CF = 0;
	Flag_SF = ((Result >> 15) & 1);
	Flag_OF = 0;
	if (Result) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result];
	Instruction_Pointer += 2 + additional_IPs;
}
void TEST_IMM_ACC_8()		//TEST = AND Function to Flags
{
	uint8 Src = 0;
	uint8 Result = 0;

	if (log_to_console) cout << "TEST IMM[" << (int)memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] << "] AND AL(" << (int)*ptr_AL << ") = ";

	//определяем объект назначения и результат операции
	Result = *ptr_AL & memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];
	Flag_CF = 0;
	Flag_OF = 0;
	Flag_SF = ((Result >> 7) & 1);
	if (Result) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result];
	if (log_to_console) cout << (int)Result;
	Instruction_Pointer += 2;
}
void TEST_IMM_ACC_16()     //TEST = AND Function to Flags
{
	uint16 Result = 0;

	if (log_to_console) cout << "TEST IMM[" << (int)(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory.read_2(Instruction_Pointer + 2 + *CS * 16) * 256) << "] AND AX(" << (int)AX << ") = ";

	//определяем объект назначения и результат операции
	Result = AX & (memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory.read_2(Instruction_Pointer + 2 + *CS * 16) * 256);
	Flag_CF = 0;
	Flag_OF = 0;
	Flag_SF = ((Result >> 15) & 1);
	if (Result) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result & 255];
	if (log_to_console) cout << (int)Result;
	Instruction_Pointer += 3;
}

//OR = Or
void OR_RM_8()		// OR R + R/M -> R/M 8bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 Src = 0;
	uint8 Result = 0;
	additional_IPs = 0;

	//определяем регистр - источник
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Src = AX & 255;
		if (log_to_console) cout << "AL(" << (int)Src << ") ";
		break;
	case 1:
		Src = CX & 255;
		if (log_to_console) cout << "CL(" << (int)Src << ") ";
		break;
	case 2:
		Src = DX & 255;
		if (log_to_console) cout << "DL(" << (int)Src << ") ";
		break;
	case 3:
		Src = BX & 255;
		if (log_to_console) cout << "BL(" << (int)Src << ") ";
		break;
	case 4:
		Src = AX >> 8;
		if (log_to_console) cout << "AH(" << (int)Src << ") ";
		break;
	case 5:
		Src = CX >> 8;
		if (log_to_console) cout << "CH(" << (int)Src << ") ";
		break;
	case 6:
		Src = DX >> 8;
		if (log_to_console) cout << "DH(" << (int)Src << ") ";
		break;
	case 7:
		Src = BX >> 8;
		if (log_to_console) cout << "BH(" << (int)Src << ") ";
		break;
	}
	
	//определяем объект назначения и результат операции
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		if (log_to_console) cout << "OR M" << OPCODE_comment << " = ";
		Result = memory.read_2(New_Addr) | Src;
		memory.write_2(New_Addr,  Result);
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 7) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			if (log_to_console) cout << "OR AL(" << (int)(AX & 255) << ") = ";
			Result = (AX & 255) | Src;
			AX = (AX & 0xFF00) | Result;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << (int)Result;
			break;
		case 1:
			if (log_to_console) cout << "OR CL(" << (int)(CX & 255) << ") = ";
			Result = (CX & 255) | Src;
			CX = (CX & 0xFF00) | Result;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << (int)Result;
			break;
		case 2:
			if (log_to_console) cout << "OR DL(" << (int)(DX & 255) << ") = ";
			Result = (DX & 255) | Src;
			DX = (DX & 0xFF00) | Result;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << (int)Result;
			break;
		case 3:
			if (log_to_console) cout << "OR BL(" << (int)(BX & 255) << ") = ";
			Result = (BX & 255) | Src;
			BX = (BX & 0xFF00) | Result;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << (int)Result;
			break;
		case 4:
			if (log_to_console) cout << "OR AH(" << (int)((AX >> 8) & 255) << ") = ";
			Result = ((AX >> 8) & 255) | Src;
			AX = (AX & 0x00FF) | (Result << 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << (int)Result;
			break;
		case 5:
			if (log_to_console) cout << "OR CH(" << (int)((CX >> 8) & 255) << ") = ";
			Result = ((CX >> 8) & 255) | Src;
			CX = (CX & 0x00FF) | (Result << 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << (int)Result;
			break;
		case 6:
			if (log_to_console) cout << "OR DH(" << (int)((DX >> 8) & 255) << ") = ";
			Result = ((DX >> 8) & 255) | Src;
			DX = (DX & 0x00FF) | (Result << 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << (int)Result;
			break;
		case 7:
			if (log_to_console) cout << "OR BH(" << (int)((BX >> 8) & 255) << ") = ";
			Result = ((BX >> 8) & 255) | Src;
			BX = (BX & 0x00FF) | (Result << 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << (int)Result;
			break;
		}
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void OR_RM_16()		// OR R + R/M -> R/M 16bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Src = 0;
	uint16 Result = 0;
	additional_IPs = 0;

	//определяем регистр - источник
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Src = AX;
		if (log_to_console) cout << "AX(" << (int)Src << ") ";
		break;
	case 1:
		Src = CX;
		if (log_to_console) cout << "CX(" << (int)Src << ") ";
		break;
	case 2:
		Src = DX;
		if (log_to_console) cout << "DX(" << (int)Src << ") ";
		break;
	case 3:
		Src = BX;
		if (log_to_console) cout << "BX(" << (int)Src << ") ";
		break;
	case 4:
		Src = Stack_Pointer;
		if (log_to_console) cout << "SP(" << (int)Src << ") ";
		break;
	case 5:
		Src = Base_Pointer;
		if (log_to_console) cout << "BP(" << (int)Src << ") ";
		break;
	case 6:
		Src = Source_Index;
		if (log_to_console) cout << "SI(" << (int)Src << ") ";
		break;
	case 7:
		Src = Destination_Index;
		if (log_to_console) cout << "DI(" << (int)Src << ") ";
		break;
	}

	//определяем объект назначения и результат операции
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		if (log_to_console) cout << "OR M" << OPCODE_comment << " = ";
		Result = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) | Src;
		memory.write_2(New_Addr,  Result & 255);
		memory.write_2(New_Addr + 1,  Result >> 8);
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 15) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			if (log_to_console) cout << "OR AX(" << (int)AX << ") = ";
			AX = AX | Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((AX >> 15) & 1);
			if (AX) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[AX & 255];
			if (log_to_console) cout << (int)AX;
			break;
		case 1:
			if (log_to_console) cout << "OR CX(" << (int)CX << ") = ";
			CX = CX | Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((CX >> 15) & 1);
			if (CX) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[CX & 255];
			if (log_to_console) cout << (int)CX;
			break;
		case 2:
			if (log_to_console) cout << "OR DX(" << (int)DX << ") = ";
			DX = DX | Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((DX >> 15) & 1);
			if (DX) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[DX & 255];
			if (log_to_console) cout << (int)DX;
			break;
		case 3:
			if (log_to_console) cout << "OR BX(" << (int)BX << ") = ";
			BX = BX | Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((BX >> 15) & 1);
			if (BX) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[BX & 255];
			if (log_to_console) cout << (int)BX;
			break;
		case 4:
			if (log_to_console) cout << "OR SP(" << (int)Stack_Pointer << ") = ";
			Stack_Pointer = Stack_Pointer | Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Stack_Pointer >> 15) & 1);
			if (Stack_Pointer) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Stack_Pointer & 255];
			if (log_to_console) cout << (int)Stack_Pointer;
			break;
		case 5:
			if (log_to_console) cout << "OR BP(" << (int)Base_Pointer << ") = ";
			Base_Pointer = Base_Pointer | Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Base_Pointer >> 15) & 1);
			if (Base_Pointer) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Base_Pointer & 255];
			if (log_to_console) cout << (int)Base_Pointer;
			break;
		case 6:
			if (log_to_console) cout << "OR SI(" << (int)Source_Index << ") = ";
			Source_Index = Source_Index | Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Source_Index >> 15) & 1);
			if (Source_Index) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Source_Index & 255];
			if (log_to_console) cout << (int)Source_Index;
			break;
		case 7:
			if (log_to_console) cout << "OR DI(" << (int)Destination_Index << ") = ";
			Destination_Index = Destination_Index | Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Destination_Index >> 15) & 1);
			if (Destination_Index) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Destination_Index & 255];
			if (log_to_console) cout << (int)Destination_Index;
			break;
		}
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void OR_RM_R_8()		// OR R + R/M -> R 8bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 Src = 0;
	uint8 Result = 0;
	additional_IPs = 0;

	//определяем источник
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Src = memory.read_2(New_Addr);
		if (log_to_console) cout << "M" << OPCODE_comment << " OR ";
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Src = AX & 255;
			if (log_to_console) cout << "AL(" << (int)Src << ") OR ";
			break;
		case 1:
			Src = CX & 255;
			if (log_to_console) cout << "CL(" << (int)Src << ") OR ";
			break;
		case 2:
			Src = DX & 255;
			if (log_to_console) cout << "DL(" << (int)Src << ") OR ";
			break;
		case 3:
			Src = BX & 255;
			if (log_to_console) cout << "BL(" << (int)Src << ") OR ";
			break;
		case 4:
			Src = AX >> 8;
			if (log_to_console) cout << "AH(" << (int)Src << ") OR ";
			break;
		case 5:
			Src = CX >> 8;
			if (log_to_console) cout << "CH(" << (int)Src << ") OR ";
			break;
		case 6:
			Src = DX >> 8;
			if (log_to_console) cout << "DH(" << (int)Src << ") OR ";
			break;
		case 7:
			Src = BX >> 8;
			if (log_to_console) cout << "BH(" << (int)Src << ") OR ";
			break;
		}
		break;
	}

	//определяем регистр - получатель
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		if (log_to_console) cout << "AL(" << (int)(AX & 255) << ") = ";
		Result = Src | (AX & 255);
		AX = (AX & 0xFF00) | Result;
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)(Result);
		break;
	case 1:
		if (log_to_console) cout << "CL(" << (int)(CX & 255) << ") = ";
		Result = Src | (CX & 255);
		CX = (CX & 0xFF00) | Result;
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)(Result);
		break;
	case 2:
		if (log_to_console) cout << "DL(" << (int)(DX & 255) << ") = ";
		Result = Src | (DX & 255);
		DX = (DX & 0xFF00) | Result;
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)(Result);
		break;
	case 3:
		if (log_to_console) cout << "BL(" << (int)(BX & 255) << ") = ";
		Result = Src | (BX & 255);
		BX = (BX & 0xFF00) | Result;
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)(Result);
		break;
	case 4:
		if (log_to_console) cout << "AH(" << (int)((AX >> 8) & 255) << ") = ";
		Result = Src | (AX >> 8);
		AX = (AX & 0x00FF) | (Result << 8);
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)(Result);
		break;
	case 5:
		if (log_to_console) cout << "CH(" << (int)((CX >> 8) & 255) << ") = ";
		Result = Src | (CX >> 8);
		CX = (CX & 0x00FF) | (Result << 8);
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)(Result);
		break;
	case 6:
		if (log_to_console) cout << "DH(" << (int)((DX >> 8) & 255) << ") = ";
		Result = Src | (DX >> 8);
		DX = (DX & 0x00FF) | (Result << 8);
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)(Result);
		break;
	case 7:
		if (log_to_console) cout << "BH(" << (int)((BX >> 8) & 255) << ") = ";
		Result = Src | (BX >> 8);
		BX = (BX & 0x00FF) | (Result << 8);
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)(Result);
		break;
	}

	Instruction_Pointer += 2 + additional_IPs;
}
void OR_RM_R_16()		// OR R + R/M -> R 16bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Src = 0;
	uint16 Result = 0;
	additional_IPs = 0;

	//определяем источник
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
		if (log_to_console) cout << OPCODE_comment << " ";
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Src = AX;
			if (log_to_console) cout << "AX(" << (int)Src << ") ";
			break;
		case 1:
			Src = CX;
			if (log_to_console) cout << "CX(" << (int)Src << ") ";
			break;
		case 2:
			Src = DX;
			if (log_to_console) cout << "DX(" << (int)Src << ") ";
			break;
		case 3:
			Src = BX;
			if (log_to_console) cout << "BX(" << (int)Src << ") ";
			break;
		case 4:
			Src = Stack_Pointer;
			if (log_to_console) cout << "SP(" << (int)Src << ") ";
			break;
		case 5:
			Src = Base_Pointer;
			if (log_to_console) cout << "BP(" << (int)Src << ") ";
			break;
		case 6:
			Src = Source_Index;
			if (log_to_console) cout << "SI(" << (int)Src << ") ";
			break;
		case 7:
			Src = Destination_Index;
			if (log_to_console) cout << "DI(" << (int)Src << ") ";
			break;
		}
		break;
	}

	//определяем регистр - получатель
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		if (log_to_console) cout << "OR AX(" << (int)AX << ") = ";
		AX = Src | AX;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((AX >> 15) & 1);
		if (AX) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[AX & 255];
		if (log_to_console) cout << (int)AX;
		break;
	case 1:
		if (log_to_console) cout << "OR CX(" << (int)CX << ") = ";
		CX = Src | CX;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((CX >> 15) & 1);
		if (CX) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[CX & 255];
		if (log_to_console) cout << (int)CX;
		break;
	case 2:
		if (log_to_console) cout << "OR DX(" << (int)DX << ") = ";
		DX = Src | DX;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((DX >> 15) & 1);
		if (DX) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[DX & 255];
		if (log_to_console) cout << (int)DX;
		break;
	case 3:
		if (log_to_console) cout << "OR BX(" << (int)BX << ") = ";
		BX = Src | BX;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((BX >> 15) & 1);
		if (BX) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[BX & 255];
		if (log_to_console) cout << (int)BX;
		break;
	case 4:
		if (log_to_console) cout << "OR SP(" << (int)Stack_Pointer << ") = ";
		Stack_Pointer = Src | Stack_Pointer;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Stack_Pointer >> 15) & 1);
		if (Stack_Pointer) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Stack_Pointer & 255];
		if (log_to_console) cout << (int)Stack_Pointer;
		break;
	case 5:
		if (log_to_console) cout << "OR BP(" << (int)Base_Pointer << ") = ";
		Base_Pointer = Src | Base_Pointer;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Base_Pointer >> 15) & 1);
		if (Base_Pointer) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Base_Pointer & 255];
		if (log_to_console) cout << (int)Base_Pointer;
		break;
	case 6:
		if (log_to_console) cout << "OR SI(" << (int)Source_Index << ") = ";
		Source_Index = Src | Source_Index;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Source_Index >> 15) & 1);
		if (Source_Index) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Source_Index & 255];
		if (log_to_console) cout << (int)Source_Index;
		break;
	case 7:
		if (log_to_console) cout << "OR DI(" << (int)Destination_Index << ") = ";
		Destination_Index = Src | Destination_Index;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Destination_Index >> 15) & 1);
		if (Destination_Index) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Destination_Index & 255];
		if (log_to_console) cout << (int)Destination_Index;
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void OR_IMM_ACC_8()    //OR IMM to ACC 8bit
{
	uint8 Src = 0;
	uint8 Result = 0;

	//непосредственный операнд
	Src = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];

	if (log_to_console) cout << "IMM(" << (int)Src << ") OR AL(" << (AX & 255) << ") ";

	//определяем объект назначения и результат операции
	Result = (AX & 255) | Src;

	AX = (AX & 0xFF00) | Result;
	Flag_CF = 0;
	Flag_OF = 0;
	Flag_SF = ((Result >> 7) & 1);
	if (Result) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result];
	if (log_to_console) cout << " = " << (int)Result;

	Instruction_Pointer += 2;
}
void OR_IMM_ACC_16()   //OR IMM to ACC 16bit
{
	uint16 Src = 0;
	uint16 Result = 0;

	//непосредственный операнд
	Src = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory.read_2(Instruction_Pointer + 2 + *CS * 16) * 256;

	if (log_to_console) cout << "IMM(" << (int)Src << ") OR AX(" << (int)AX << ")";

	//определяем объект назначения и результат операции
	Result = AX | Src;
	AX = Result;
	Flag_CF = 0;
	Flag_OF = 0;
	Flag_SF = ((Result >> 15) & 1);
	if (Result) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result & 255];
	if (log_to_console) cout << " = " << (int)Result;

	Instruction_Pointer += 3;
}

//XOR
void XOR_RM_8()		// XOR R + R/M -> R/M 8bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 Src = 0;
	uint8 Result = 0;
	additional_IPs = 0;

	//определяем регистр - источник
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Src = AX & 255;
		if (log_to_console) cout << "AL ";
		break;
	case 1:
		Src = CX & 255;
		if (log_to_console) cout << "CL ";
		break;
	case 2:
		Src = DX & 255;
		if (log_to_console) cout << "DL ";
		break;
	case 3:
		Src = BX & 255;
		if (log_to_console) cout << "BL ";
		break;
	case 4:
		Src = AX >> 8;
		if (log_to_console) cout << "AH ";
		break;
	case 5:
		Src = CX >> 8;
		if (log_to_console) cout << "CH ";
		break;
	case 6:
		Src = DX >> 8;
		if (log_to_console) cout << "DH ";
		break;
	case 7:
		Src = BX >> 8;
		if (log_to_console) cout << "BH  ";
		break;
	}
		
	//определяем объект назначения и результат операции
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Result = memory.read_2(New_Addr) ^ Src;
		memory.write_2(New_Addr,  Result);
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 7) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "XOR M" << OPCODE_comment << "=" << (int)Result;
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Result = (AX & 255) ^ Src;
			AX = (AX & 0xFF00) | Result;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "XOR AL = " << (int)Result;
			break;
		case 1:
			Result = (CX & 255) ^ Src;
			CX = (CX & 0xFF00) | Result;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "XOR CL = " << (int)Result;
			break;
		case 2:
			Result = (DX & 255) ^ Src;
			DX = (DX & 0xFF00) | Result;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "XOR DL = " << (int)Result;
			break;
		case 3:
			Result = (BX & 255) ^ Src;
			BX = (BX & 0xFF00) | Result;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "XOR BL = " << (int)Result;
			break;
		case 4:
			Result = ((AX >> 8) & 255) ^ Src;
			AX = (AX & 0x00FF) | (Result << 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "XOR AH = " << (int)Result;
			break;
		case 5:
			Result = ((CX >> 8) & 255) ^ Src;
			CX = (CX & 0x00FF) | (Result << 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "XOR CH = " << (int)Result;
			break;
		case 6:
			Result = ((DX >> 8) & 255) ^ Src;
			DX = (DX & 0x00FF) | (Result << 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "XOR DH = " << (int)Result;
			break;
		case 7:
			Result = ((BX >> 8) & 255) ^ Src;
			BX = (BX & 0x00FF) | (Result << 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "XOR BH = " << (int)Result;
			break;
		}
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void XOR_RM_16()		// XOR R + R/M -> R/M 16bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Src = 0;
	uint16 Result = 0;
	additional_IPs = 0;

	//определяем регистр - источник
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Src = AX;
		if (log_to_console) cout << "XOR AX("<<(int)Src<<") ^ ";
		break;
	case 1:
		Src = CX;
		if (log_to_console) cout << "XOR CX(" << (int)Src << ") ^ ";
		break;
	case 2:
		Src = DX;
		if (log_to_console) cout << "XOR DX(" << (int)Src << ") ^ ";
		break;
	case 3:
		Src = BX;
		if (log_to_console) cout << "XOR BX(" << (int)Src << ") ^ ";
		break;
	case 4:
		Src = Stack_Pointer;
		if (log_to_console) cout << "XOR SP(" << (int)Src << ") ^ ";
		break;
	case 5:
		Src = Base_Pointer;
		if (log_to_console) cout << "XOR BP(" << (int)Src << ") ^ ";
		break;
	case 6:
		Src = Source_Index;
		if (log_to_console) cout << "XOR SI(" << (int)Src << ") ^ ";
		break;
	case 7:
		Src = Destination_Index;
		if (log_to_console) cout << "XOR DI(" << (int)Src << ") ^ ";
		break;
	}

	//определяем объект назначения и результат операции
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Result = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) ^ Src;
		memory.write_2(New_Addr,  Result & 255);
		memory.write_2(New_Addr + 1,  Result >> 8);
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 15) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " M" << OPCODE_comment << "=" << (int)Result;
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			AX = AX ^ Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((AX >> 15) & 1);
			if (AX) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[AX & 255];
			if (log_to_console) cout << " AX = " << (int)AX;
			break;
		case 1:
			CX = CX ^ Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((CX >> 15) & 1);
			if (CX) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[CX & 255];
			if (log_to_console) cout << " CX = " << (int)CX;
			break;
		case 2:
			DX = DX ^ Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((DX >> 15) & 1);
			if (DX) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[DX & 255];
			if (log_to_console) cout << " DX = " << (int)DX;
			break;
		case 3:
			BX = BX ^ Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((BX >> 15) & 1);
			if (BX) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[BX & 255];
			if (log_to_console) cout << " BX = " << (int)BX;
			break;
		case 4:
			Stack_Pointer = Stack_Pointer ^ Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Stack_Pointer >> 15) & 1);
			if (Stack_Pointer) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Stack_Pointer & 255];
			if (log_to_console) cout << " SP = " << (int)Stack_Pointer;
			break;
		case 5:
			Base_Pointer = Base_Pointer ^ Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Base_Pointer >> 15) & 1);
			if (Base_Pointer) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Base_Pointer & 255];
			if (log_to_console) cout << " BP = " << (int)Base_Pointer;
			break;
		case 6:
			Source_Index = Source_Index ^ Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Source_Index >> 15) & 1);
			if (Source_Index) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Source_Index & 255];
			if (log_to_console) cout << " SI = " << (int)Source_Index;
			break;
		case 7:
			Destination_Index = Destination_Index ^ Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Destination_Index >> 15) & 1);
			if (Destination_Index) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Destination_Index & 255];
			if (log_to_console) cout << " DI = " << (int)Destination_Index;
			break;
		}
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void XOR_RM_R_8()		// XOR R + R/M -> R 8bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 Src = 0;
	uint8 Result = 0;
	additional_IPs = 0;

	//определяем источник
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Src = memory.read_2(New_Addr);
		if (log_to_console) cout << "M" << OPCODE_comment << " XOR ";
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Src = AX & 255;
			if (log_to_console) cout << "AL XOR ";
			break;
		case 1:
			Src = CX & 255;
			if (log_to_console) cout << "CL XOR ";
			break;
		case 2:
			Src = DX & 255;
			if (log_to_console) cout << "DL XOR ";
			break;
		case 3:
			Src = BX & 255;
			if (log_to_console) cout << "BL XOR ";
			break;
		case 4:
			Src = AX >> 8;
			if (log_to_console) cout << "AH XOR ";
			break;
		case 5:
			Src = CX >> 8;
			if (log_to_console) cout << "CH XOR ";
			break;
		case 6:
			Src = DX >> 8;
			if (log_to_console) cout << "DH XOR ";
			break;
		case 7:
			Src = BX >> 8;
			if (log_to_console) cout << "BH XOR ";
			break;
		}
		break;
	}

	//определяем регистр - получатель
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Result = Src ^ (AX & 255);
		AX = (AX & 0xFF00) | Result;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 7) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "AL = " << (int)Result;
		break;
	case 1:
		Result = Src ^ (CX & 255);
		CX = (CX & 0xFF00) | Result;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 7) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "CL = " << (int)Result;
		break;
	case 2:
		Result = Src ^ (DX & 255);
		DX = (DX & 0xFF00) | Result;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 7) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "DL = " << (int)Result;
		break;
	case 3:
		Result = Src ^ (BX & 255);
		BX = (BX & 0xFF00) | Result;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 7) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "BL = " << (int)Result;
		break;
	case 4:
		Result = Src ^ (AX >> 8);
		AX = (AX & 0x00FF) | (Result << 8);
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 7) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "AH = " << (int)Result;
		break;
	case 5:
		Result = Src ^ (CX >> 8);
		CX = (CX & 0x00FF) | (Result << 8);
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 7) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "CH = " << (int)Result;
		break;
	case 6:
		Result = Src ^ (DX >> 8);
		DX = (DX & 0x00FF) | (Result << 8);
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 7) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "DH = " << (int)Result;
		break;
	case 7:
		Result = Src ^ (BX >> 8);
		BX = (BX & 0x00FF) | (Result << 8);
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 7) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "BH = " << (int)Result;
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void XOR_RM_R_16()		// XOR R + R/M -> R 16bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Src = 0;
	uint16 Result = 0;
	additional_IPs = 0;

	//определяем источник
	switch (byte2 >> 6)
	{
	case 0:
	case 1:
	case 2:
		New_Addr = mod_RM_2(byte2);
		Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
		if (log_to_console) cout << "XOR M" << OPCODE_comment << " ^ ";
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Src = AX;
			if (log_to_console) cout << "AX("<<(int)Src<<") ^ ";
			break;
		case 1:
			Src = CX;
			if (log_to_console) cout << "CX(" << (int)Src << ") ^ ";
			break;
		case 2:
			Src = DX;
			if (log_to_console) cout << "DX(" << (int)Src << ") ^ ";
			break;
		case 3:
			Src = BX;
			if (log_to_console) cout << "BX(" << (int)Src << ") ^ ";
			break;
		case 4:
			Src = Stack_Pointer;
			if (log_to_console) cout << "SP(" << (int)Src << ") ^ ";
			break;
		case 5:
			Src = Base_Pointer;
			if (log_to_console) cout << "BP(" << (int)Src << ") ^ ";
			break;
		case 6:
			Src = Source_Index;
			if (log_to_console) cout << "SI(" << (int)Src << ") ^ ";
			break;
		case 7:
			Src = Destination_Index;
			if (log_to_console) cout << "DI(" << (int)Src << ") ^ ";
			break;
		}
		break;
	}

	//определяем регистр - получатель
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		AX = Src ^ AX;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((AX >> 15) & 1);
		if (AX) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[AX & 255];
		if (log_to_console) cout << "AX = " << (int)AX;
		break;
	case 1:
		CX = Src ^ CX;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((CX >> 15) & 1);
		if (CX) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[CX & 255];
		if (log_to_console) cout << "CX = " << (int)CX;
		break;
	case 2:
		DX = Src ^ DX;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((DX >> 15) & 1);
		if (DX) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[DX & 255];
		if (log_to_console) cout << "DX = " << (int)DX;
		break;
	case 3:
		BX = Src ^ BX;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((BX >> 15) & 1);
		if (BX) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[BX & 255];
		if (log_to_console) cout << "BX = " << (int)BX;
		break;
	case 4:
		Stack_Pointer = Src ^ Stack_Pointer;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Stack_Pointer >> 15) & 1);
		if (Stack_Pointer) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Stack_Pointer & 255];
		if (log_to_console) cout << "SP = " << (int)Stack_Pointer;
		break;
	case 5:
		Base_Pointer = Src ^ Base_Pointer;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Base_Pointer >> 15) & 1);
		if (Base_Pointer) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Base_Pointer & 255];
		if (log_to_console) cout << "BP = " << (int)Base_Pointer;
		break;
	case 6:
		Source_Index = Src ^ Source_Index;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Source_Index >> 15) & 1);
		if (Source_Index) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Source_Index & 255];
		if (log_to_console) cout << "SI = " << (int)Source_Index;
		break;
	case 7:
		Destination_Index = Src ^ Destination_Index;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Destination_Index >> 15) & 1);
		if (Destination_Index) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Destination_Index & 255];
		if (log_to_console) cout << "DI = " << (int)Destination_Index;
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}

//XOR/OR/AND/ADD/ADC/SBB/SUB IMM to Register/Memory
void XOR_OR_IMM_RM_8()		
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 OP = (byte2 >> 3) & 7;
	uint8 Src = 0;
	uint8 Result = 0;
	uint16 Result_16 = 0;
	additional_IPs = 0;
	bool OF_Carry = false;
	
	switch (OP)
	{
	case 0:   //ADD IMM -> R/M 8 bit

		//определяем объект назначения и результат операции
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "ADD IMM[" << (int)Src << "] + ";
			Flag_AF = (((memory.read_2(New_Addr) & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = ((memory.read_2(New_Addr) & 0x7F) + (Src & 0x7F)) >> 7;
			Result_16 = memory.read_2(New_Addr) + Src;
			memory.write_2(New_Addr,  Result_16 & 255);
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)(Result & 255);
			break;
		case 3:
			// mod 11 источник - регистр
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "ADD IMM[" << (int)Src << "] + ";
			switch (byte2 & 7)
			{
			case 0:
				Flag_AF = (((AX & 15) + (Src & 15)) >> 4) & 1;
				OF_Carry = ((AX & 0x7F) + (Src & 0x7F)) >> 7;
				Result_16 = (AX & 255) + Src;
				AX = (AX & 0xFF00) | (Result_16 & 255);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "AL = " << (int)(Result_16 & 255);
				break;
			case 1:
				Flag_AF = (((CX & 15) + (Src & 15)) >> 4) & 1;
				OF_Carry = ((CX & 0x7F) + (Src & 0x7F)) >> 7;
				Result_16 = (CX & 255) + Src;
				CX = (CX & 0xFF00) | (Result_16 & 255);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "CL = " << (int)(Result_16 & 255);
				break;
			case 2:
				Flag_AF = (((DX & 15) + (Src & 15)) >> 4) & 1;
				OF_Carry = ((DX & 0x7F) + (Src & 0x7F)) >> 7;
				Result_16 = (DX & 255) + Src;
				DX = (DX & 0xFF00) | (Result_16 & 255);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "DL = " << (int)(Result_16 & 255);
				break;
			case 3:
				Flag_AF = (((BX & 15) + (Src & 15)) >> 4) & 1;
				OF_Carry = ((BX & 0x7F) + (Src & 0x7F)) >> 7;
				Result_16 = (BX & 255) + Src;
				BX = (BX & 0xFF00) | (Result_16 & 255);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "BL = " << (int)(Result_16 & 255);
				break;
			case 4:
				Flag_AF = ((((AX >> 8) & 15) + (Src & 15)) >> 4) & 1;
				OF_Carry = (((AX >> 8) & 0x7F) + (Src & 0x7F)) >> 7;
				Result_16 = (AX >> 8) + Src;
				AX = (AX & 0x00FF) | (Result_16 << 8);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "AH = " << (int)(Result_16 & 255);
				break;
			case 5:
				Flag_AF = ((((CX >> 8) & 15) + (Src & 15)) >> 4) & 1;
				OF_Carry = (((CX >> 8) & 0x7F) + (Src & 0x7F)) >> 7;
				Result_16 = (CX >> 8) + Src;
				CX = (CX & 0x00FF) | (Result_16 << 8);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "CH = " << (int)(Result_16 & 255);
				break;
			case 6:
				Flag_AF = ((((DX >> 8) & 15) + (Src & 15)) >> 4) & 1;
				OF_Carry = (((DX >> 8) & 0x7F) + (Src & 0x7F)) >> 7;
				Result_16 = (DX >> 8) + Src;
				DX = (DX & 0x00FF) | (Result_16 << 8);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "DH = " << (int)(Result_16 & 255);
				break;
			case 7:
				Flag_AF = ((((BX >> 8) & 15) + (Src & 15)) >> 4) & 1;
				OF_Carry = (((BX >> 8) & 0x7F) + (Src & 0x7F)) >> 7;
				Result_16 = (BX >> 8) + Src;
				BX = (BX & 0x00FF) | (Result_16 << 8);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "BH = " << (int)(Result_16 & 255);
				break;
			}
			break;
		}
		break;

	case 1:   //OR

		//определяем объект назначения и результат операции
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "IMM(" << (int)Src << ") OR ";
			Result = memory.read_2(New_Addr) | Src;
			memory.write_2(New_Addr,  Result);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result];
			if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)Result;
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "IMM(" << (int)Src << ") OR ";
			switch (byte2 & 7)
			{
			case 0:
				Result = (AX & 255) | Src;
				AX = (AX & 0xFF00) | Result;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result & 255];
				if (log_to_console) cout << "AL = " << (int)Result;
				break;
			case 1:
				Result = (CX & 255) | Src;
				CX = (CX & 0xFF00) | Result;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result & 255];
				if (log_to_console) cout << "CL = " << (int)Result;
				break;
			case 2:
				Result = (DX & 255) | Src;
				DX = (DX & 0xFF00) | Result;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result & 255];
				if (log_to_console) cout << "DL = " << (int)Result;
				break;
			case 3:
				Result = (BX & 255) | Src;
				BX = (BX & 0xFF00) | Result;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result & 255];
				if (log_to_console) cout << "BL = " << (int)Result;
				break;
			case 4:
				Result = ((AX >> 8) & 255) | Src;
				AX = (AX & 0x00FF) | (Result << 8);
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result & 255];
				if (log_to_console) cout << "AH = " << (int)Result;
				break;
			case 5:
				Result = ((CX >> 8) & 255) | Src;
				CX = (CX & 0x00FF) | (Result << 8);
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result & 255];
				if (log_to_console) cout << "CH = " << (int)Result;
				break;
			case 6:
				Result = ((DX >> 8) & 255) | Src;
				DX = (DX & 0x00FF) | (Result << 8);
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result & 255];
				if (log_to_console) cout << "DH = " << (int)Result;
				break;
			case 7:
				Result = ((BX >> 8) & 255) | Src;
				BX = (BX & 0x00FF) | (Result << 8);
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result & 255];
				if (log_to_console) cout << "BH = " << (int)Result;
				break;
			}
			break;
		}
		break;

	case 2:   //ADC IMM -> R/M 8 bit

		//определяем объект назначения и результат операции
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "ADC IMM(" << (int)Src << ") + ";
			Flag_AF = (((memory.read_2(New_Addr) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = ((memory.read_2(New_Addr) & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
			Result_16 = memory.read_2(New_Addr) + Src + Flag_CF;
			memory.write_2(New_Addr,  Result_16 & 255);
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)(Result & 255);
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "ADC IMM(" << (int)Src << ") + ";
			switch (byte2 & 7)
			{
			case 0:
				Flag_AF = (((AX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
				OF_Carry = ((AX & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
				Result_16 = (AX & 255) + Src + Flag_CF;
				AX = (AX & 0xFF00) | (Result_16 & 255);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "AL = " << (int)(Result_16 & 255);
				break;
			case 1:
				Flag_AF = (((CX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
				OF_Carry = ((CX & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
				Result_16 = (CX & 255) + Src + Flag_CF;
				CX = (CX & 0xFF00) | (Result_16 & 255);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "CL = " << (int)(Result_16 & 255);
				break;
			case 2:
				Flag_AF = (((DX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
				OF_Carry = ((DX & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
				Result_16 = (DX & 255) + Src + Flag_CF;
				DX = (DX & 0xFF00) | (Result_16 & 255);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "DL = " << (int)(Result_16 & 255);
				break;
			case 3:
				Flag_AF = (((BX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
				OF_Carry = ((BX & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
				Result_16 = (BX & 255) + Src + Flag_CF;
				BX = (BX & 0xFF00) | (Result_16 & 255);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "BL = " << (int)(Result_16 & 255);
				break;
			case 4:
				Flag_AF = ((((AX >> 8) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
				OF_Carry = (((AX >> 8) & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
				Result_16 = (AX >> 8) + Src + Flag_CF;
				AX = (AX & 0x00FF) | (Result_16 << 8);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "AH = " << (int)(Result_16 & 255);
				break;
			case 5:
				Flag_AF = ((((CX >> 8) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
				OF_Carry = (((CX >> 8) & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
				Result_16 = (CX >> 8) + Src + Flag_CF;
				CX = (CX & 0x00FF) | (Result_16 << 8);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "CH = " << (int)(Result_16 & 255);
				break;
			case 6:
				Flag_AF = ((((DX >> 8) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
				OF_Carry = (((DX >> 8) & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
				Result_16 = (DX >> 8) + Src + Flag_CF;
				DX = (DX & 0x00FF) | (Result_16 << 8);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "DH = " << (int)(Result_16 & 255);
				break;
			case 7:
				Flag_AF = ((((BX >> 8) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
				OF_Carry = (((BX >> 8) & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
				Result_16 = (BX >> 8) + Src + Flag_CF;
				BX = (BX & 0x00FF) | (Result_16 << 8);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "BH = " << (int)(Result_16 & 255);
				break;
			}
			break;
		}
		break;

	case 3:   //SBB IMM -> R/M 8 bit

		//определяем объект назначения и результат операции
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "SBB IMM[" << (int)Src << "] ";
			Flag_AF = (((memory.read_2(New_Addr) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
			OF_Carry = ((memory.read_2(New_Addr) & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
			Result_16 = memory.read_2(New_Addr) - Src - Flag_CF;
			memory.write_2(New_Addr,  Result_16 & 255);
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			if (log_to_console) cout << "from M" << OPCODE_comment << " = " << (int)(Result & 255);
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "SBB IMM[" << (int)Src << "] ";
			switch (byte2 & 7)
			{
			case 0:
				Flag_AF = (((AX & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
				OF_Carry = ((AX & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
				Result_16 = (AX & 255) - Src - Flag_CF;
				AX = (AX & 0xFF00) | (Result_16 & 255);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "from AL = " << (int)(Result_16 & 255);
				break;
			case 1:
				Flag_AF = (((CX & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
				OF_Carry = ((CX & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
				Result_16 = (CX & 255) - Src - Flag_CF;
				CX = (CX & 0xFF00) | (Result_16 & 255);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "from CL = " << (int)(Result_16 & 255);
				break;
			case 2:
				Flag_AF = (((DX & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
				OF_Carry = ((DX & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
				Result_16 = (DX & 255) - Src - Flag_CF;
				DX = (DX & 0xFF00) | (Result_16 & 255);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "from DL = " << (int)(Result_16 & 255);
				break;
			case 3:
				Flag_AF = (((BX & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
				OF_Carry = ((BX & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
				Result_16 = (BX & 255) - Src - Flag_CF;
				BX = (BX & 0xFF00) | (Result_16 & 255);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "from BL = " << (int)(Result_16 & 255);
				break;
			case 4:
				Flag_AF = ((((AX >> 8) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
				OF_Carry = (((AX >> 8) & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
				Result_16 = (AX >> 8) - Src - Flag_CF;
				AX = (AX & 0x00FF) | (Result_16 << 8);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "from AH = " << (int)(Result_16 & 255);
				break;
			case 5:
				Flag_AF = ((((CX >> 8) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
				OF_Carry = (((CX >> 8) & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
				Result_16 = (CX >> 8) - Src - Flag_CF;
				CX = (CX & 0x00FF) | (Result_16 << 8);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "from CH = " << (int)(Result_16 & 255);
				break;
			case 6:
				Flag_AF = ((((DX >> 8) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
				OF_Carry = (((DX >> 8) & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
				Result_16 = (DX >> 8) - Src - Flag_CF;
				DX = (DX & 0x00FF) | (Result_16 << 8);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "from DH = " << (int)(Result_16 & 255);
				break;
			case 7:
				Flag_AF = ((((BX >> 8) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
				OF_Carry = (((BX >> 8) & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
				Result_16 = (BX >> 8) - Src - Flag_CF;
				BX = (BX & 0x00FF) | (Result_16 << 8);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "from BH = " << (int)(Result_16 & 255);
				break;
			}
			break;
		}
		break;

	case 4:   //AND
		//определяем объект назначения и результат операции
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "IMM(" << (int)Src << ") AND ";
			Result = memory.read_2(New_Addr) & Src;
			memory.write_2(New_Addr,  Result);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)Result;
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "IMM(" << (int)Src << ") AND ";
			switch (byte2 & 7)
			{
			case 0:
				Result = (AX & 255) & Src;
				AX = (AX & 0xFF00) | Result;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result & 255];
				if (log_to_console) cout << "AL = " << (int)Result;
				break;
			case 1:
				Result = (CX & 255) & Src;
				CX = (CX & 0xFF00) | Result;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result & 255];
				if (log_to_console) cout << "CL = " << (int)Result;
				break;
			case 2:
				Result = (DX & 255) & Src;
				DX = (DX & 0xFF00) | Result;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result & 255];
				if (log_to_console) cout << "DL = " << (int)Result;
				break;
			case 3:
				Result = (BX & 255) & Src;
				BX = (BX & 0xFF00) | Result;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result & 255];
				if (log_to_console) cout << "BL = " << (int)Result;
				break;
			case 4:
				Result = ((AX >> 8) & 255) & Src;
				AX = (AX & 0x00FF) | (Result << 8);
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result & 255];
				if (log_to_console) cout << "AH = " << (int)Result;
				break;
			case 5:
				Result = ((CX >> 8) & 255) & Src;
				CX = (CX & 0x00FF) | (Result << 8);
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result & 255];
				if (log_to_console) cout << "CH = " << (int)Result;
				break;
			case 6:
				Result = ((DX >> 8) & 255) & Src;
				DX = (DX & 0x00FF) | (Result << 8);
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result & 255];
				if (log_to_console) cout << "DH = " << (int)Result;
				break;
			case 7:
				Result = ((BX >> 8) & 255) & Src;
				BX = (BX & 0x00FF) | (Result << 8);
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result & 255];
				if (log_to_console) cout << "BH = " << (int)Result;
				break;
			}
			break;
		}
		break;

	case 5:   //SUB IMM -> R/M 8 bit
		//определяем объект назначения и результат операции
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "SUB IMM(" << (int)Src << ") ";
			Flag_AF = (((memory.read_2(New_Addr) & 15) - (Src & 15)) >> 4) & 1;
			OF_Carry = ((memory.read_2(New_Addr) & 0x7F) - (Src & 0x7F)) >> 7;
			Result_16 = memory.read_2(New_Addr) - Src;
			memory.write_2(New_Addr,  Result_16 & 255);
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			if (log_to_console) cout << "from M" << OPCODE_comment << " = " << (int)(Result & 255);
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "SUB IMM(" << (int)Src << ") ";
			switch (byte2 & 7)
			{
			case 0:
				Flag_AF = (((AX & 15) - (Src & 15)) >> 4) & 1;
				OF_Carry = ((AX & 0x7F) - (Src & 0x7F)) >> 7;
				Result_16 = (AX & 255) - Src;
				AX = (AX & 0xFF00) | (Result_16 & 255);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "from AL = " << (int)(Result_16 & 255);
				break;
			case 1:
				Flag_AF = (((CX & 15) - (Src & 15)) >> 4) & 1;
				OF_Carry = ((CX & 0x7F) - (Src & 0x7F)) >> 7;
				Result_16 = (CX & 255) - Src;
				CX = (CX & 0xFF00) | (Result_16 & 255);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "from CL = " << (int)(Result_16 & 255);
				break;
			case 2:
				Flag_AF = (((DX & 15) - (Src & 15)) >> 4) & 1;
				OF_Carry = ((DX & 0x7F) - (Src & 0x7F)) >> 7;
				Result_16 = (DX & 255) - Src;
				DX = (DX & 0xFF00) | (Result_16 & 255);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "from DL = " << (int)(Result_16 & 255);
				break;
			case 3:
				Flag_AF = (((BX & 15) - (Src & 15)) >> 4) & 1;
				OF_Carry = ((BX & 0x7F) - (Src & 0x7F)) >> 7;
				Result_16 = (BX & 255) - Src;
				BX = (BX & 0xFF00) | (Result_16 & 255);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "from BL = " << (int)(Result_16 & 255);
				break;
			case 4:
				Flag_AF = ((((AX >> 8) & 15) - (Src & 15)) >> 4) & 1;
				OF_Carry = (((AX >> 8) & 0x7F) - (Src & 0x7F)) >> 7;
				Result_16 = (AX >> 8) - Src;
				AX = (AX & 0x00FF) | (Result_16 << 8);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "from AH = " << (int)(Result_16 & 255);
				break;
			case 5:
				Flag_AF = ((((CX >> 8) & 15) - (Src & 15)) >> 4) & 1;
				OF_Carry = (((CX >> 8) & 0x7F) - (Src & 0x7F)) >> 7;
				Result_16 = (CX >> 8) - Src;
				CX = (CX & 0x00FF) | (Result_16 << 8);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "from CH = " << (int)(Result_16 & 255);
				break;
			case 6:
				Flag_AF = ((((DX >> 8) & 15) - (Src & 15)) >> 4) & 1;
				OF_Carry = (((DX >> 8) & 0x7F) - (Src & 0x7F)) >> 7;
				Result_16 = (DX >> 8) - Src;
				DX = (DX & 0x00FF) | (Result_16 << 8);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "from DH = " << (int)(Result_16 & 255);
				break;
			case 7:
				Flag_AF = ((((BX >> 8) & 15) - (Src & 15)) >> 4) & 1;
				OF_Carry = (((BX >> 8) & 0x7F) - (Src & 0x7F)) >> 7;
				Result_16 = (BX >> 8) - Src;
				BX = (BX & 0x00FF) | (Result_16 << 8);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = ((Result_16 >> 7) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				if (log_to_console) cout << "from BH = " << (int)(Result_16 & 255);
				break;
			}
			break;
		}
		break;

	case 6: //XOR

		//определяем объект назначения и результат операции
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "XOR IMM[" << (int)Src << "] ^ ";
			Result = memory.read_2(New_Addr) ^ Src;
			memory.write_2(New_Addr,  Result);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)Result;
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "XOR IMM(" << (int)Src << ") ^ ";
			switch (byte2 & 7)
			{
			case 0:
				Result = (AX & 255) ^ Src;
				AX = (AX & 0xFF00) | Result;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result & 255];
				if (log_to_console) cout << "AL = " << (int)Result;
				break;
			case 1:
				Result = (CX & 255) ^ Src;
				CX = (CX & 0xFF00) | Result;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result & 255];
				if (log_to_console) cout << "CL = " << (int)Result;
				break;
			case 2:
				Result = (DX & 255) ^ Src;
				DX = (DX & 0xFF00) | Result;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result & 255];
				if (log_to_console) cout << "DL = " << (int)Result;
				break;
			case 3:
				Result = (BX & 255) ^ Src;
				BX = (BX & 0xFF00) | Result;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result & 255];
				if (log_to_console) cout << "BL = " << (int)Result;
				break;
			case 4:
				Result = ((AX >> 8) & 255) ^ Src;
				AX = (AX & 0x00FF) | (Result << 8);
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result & 255];
				if (log_to_console) cout << "AH = " << (int)Result;
				break;
			case 5:
				Result = ((CX >> 8) & 255) ^ Src;
				CX = (CX & 0x00FF) | (Result << 8);
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result & 255];
				if (log_to_console) cout << "CH = " << (int)Result;
				break;
			case 6:
				Result = ((DX >> 8) & 255) ^ Src;
				DX = (DX & 0x00FF) | (Result << 8);
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result & 255];
				if (log_to_console) cout << "DH = " << (int)Result;
				break;
			case 7:
				Result = ((BX >> 8) & 255) ^ Src;
				BX = (BX & 0x00FF) | (Result << 8);
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Result >> 7) & 1);
				if (Result) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result & 255];
				if (log_to_console) cout << "BH = " << (int)Result;
				break;
			}
			break;
		}
		break;

	case 7:   //CMP IMM -> R/M 8 bit

		//определяем объект назначения и результат операции
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд - вычитаемое
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "CMP IMM(" << (int)(Src) << ") ";
			Result_16 = memory.read_2(New_Addr) - Src;
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = (Result_16 >> 7) & 1;
			OF_Carry = ((memory.read_2(New_Addr) & 0x7F) - (Src & 0x7F)) >> 7;
			Flag_OF = Flag_CF ^ OF_Carry;
			Flag_PF = parity_check[Result_16 & 255];
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_AF = (((memory.read_2(New_Addr) & 15) - (Src & 15)) >> 4) & 1;
			if (log_to_console) cout << "with M" << OPCODE_comment << "(" << (int)memory.read_2(New_Addr) << ") ";
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд - вычитаемое
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "CMP IMM(" << (int)(Src) << ") ";
			switch (byte2 & 7)
			{
			case 0:
				Result_16 = (AX & 255) - Src;
				//AX = (AX & 0xFF00) | (Result_16 & 255);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = (Result_16 >> 7) & 1;
				OF_Carry = ((AX & 0x7F) - (Src & 0x7F)) >> 7;
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				Flag_AF = (((AX & 15) - (Src & 15)) >> 4) & 1;
				if (log_to_console) cout << "with AL(" << (int)(AX & 255) << ") ";
				break;
			case 1:
				Result_16 = (CX & 255) - Src;
				//CX = (CX & 0xFF00) | (Result_16 & 255);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = (Result_16 >> 7) & 1;
				OF_Carry = ((CX & 0x7F) - (Src & 0x7F)) >> 7;
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				Flag_AF = (((CX & 15) - (Src & 15)) >> 4) & 1;
				if (log_to_console) cout << "with CL(" << (int)(CX & 255) << ") ";
				break;
			case 2:
				Result_16 = (DX & 255) - Src;
				//DX = (DX & 0xFF00) | (Result_16 & 255);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = (Result_16 >> 7) & 1;
				OF_Carry = ((DX & 0x7F) - (Src & 0x7F)) >> 7;
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				Flag_AF = (((DX & 15) - (Src & 15)) >> 4) & 1;
				if (log_to_console) cout << "with DL(" << (int)(DX & 255) << ") ";
				break;
			case 3:
				Result_16 = (BX & 255) - Src;
				//BX = (BX & 0xFF00) | (Result_16 & 255);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = (Result_16 >> 7) & 1;
				OF_Carry = ((BX & 0x7F) - (Src & 0x7F)) >> 7;
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				Flag_AF = (((BX & 15) - (Src & 15)) >> 4) & 1;
				if (log_to_console) cout << "with BL(" << (int)(BX & 255) << ") ";
				break;
			case 4:
				Result_16 = (AX >> 8) - Src;
				//AX = (AX & 0x00FF) | (Result_16 << 8);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = (Result_16 >> 7) & 1;
				OF_Carry = (((AX >> 8) & 0x7F) - (Src & 0x7F)) >> 7;
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				Flag_AF = ((((AX >> 8) & 15) - (Src & 15)) >> 4) & 1;
				if (log_to_console) cout << "with AH(" << (int)((AX>>8) & 255) << ") ";
				break;
			case 5:
				Result_16 = (CX >> 8) - Src;
				//CX = (CX & 0x00FF) | (Result_16 << 8);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = (Result_16 >> 7) & 1;
				OF_Carry = (((CX >> 8) & 0x7F) - (Src & 0x7F)) >> 7;
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				Flag_AF = ((((CX >> 8) & 15) - (Src & 15)) >> 4) & 1;
				if (log_to_console) cout << "with CH(" << (int)((CX >> 8) & 255) << ") ";
				break;
			case 6:
				Result_16 = (DX >> 8) - Src;
				//DX = (DX & 0x00FF) | (Result_16 << 8);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = (Result_16 >> 7) & 1;
				OF_Carry = (((DX >> 8) & 0x7F) - (Src & 0x7F)) >> 7;
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				Flag_AF = ((((DX >> 8) & 15) - (Src & 15)) >> 4) & 1;
				if (log_to_console) cout << "with DH(" << (int)((DX >> 8) & 255) << ") ";
				break;
			case 7:
				Result_16 = (BX >> 8) - Src;
				//BX = (BX & 0x00FF) | (Result_16 << 8);
				Flag_CF = (Result_16 >> 8) & 1;
				Flag_SF = (Result_16 >> 7) & 1;
				OF_Carry = (((BX >> 8) & 0x7F) - (Src & 0x7F)) >> 7;
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_16 & 255) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_16 & 255];
				Flag_AF = ((((BX >> 8) & 15) - (Src & 15)) >> 4) & 1;
				if (log_to_console) cout << "with BH(" << (int)((BX >> 8) & 255) << ") ";
				break;
			}
			break;
		}
		break;
	}
	Instruction_Pointer += 3 + additional_IPs;
}
void XOR_OR_IMM_RM_16()   //XOR/OR/ADD/ADC IMM to Register/Memory 16bit
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 OP = (byte2 >> 3) & 7;
	uint16 Src = 0;
	uint16 Result = 0;
	uint32 Result_32 = 0;
	additional_IPs = 0;
	bool OF_Carry = false;

	switch (OP)
	{
	case 0:   //ADD IMM -> R/M 16 bit
		//определяем объект назначения и результат операции
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16) + memory.read_2(Instruction_Pointer + 3 + additional_IPs + *CS * 16) * 256;
			if (log_to_console) cout << "IMM(" << (int)Src << ") ADD ";
			Flag_AF = (((memory.read_2(New_Addr) & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = (((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & 0x7FFF) + (Src & 0x7FFF)) >> 15;
			Result_32 = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) + Src;
			memory.write_2(New_Addr,  Result_32 & 255);
			memory.write_2(New_Addr + 1,  (Result_32 >> 8) & 255);
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16) + memory.read_2(Instruction_Pointer + 3 + additional_IPs + *CS * 16) * 256;
			if (log_to_console) cout << "IMM(" << (int)Src << ") ADD ";
			switch (byte2 & 7)
			{
			case 0:
				Flag_AF = (((AX & 15) + (Src & 15)) >> 4) & 1;
				OF_Carry = ((AX & 0x7FFF) + (Src & 0x7FFF)) >> 15;
				Result_32 = AX + Src;
				AX = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << " AX = " << (int)AX;
				break;
			case 1:
				Flag_AF = (((CX & 15) + (Src & 15)) >> 4) & 1;
				OF_Carry = ((CX & 0x7FFF) + (Src & 0x7FFF)) >> 15;
				Result_32 = CX + Src;
				CX = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << " CX = " << (int)CX;
				break;
			case 2:
				Flag_AF = (((DX & 15) + (Src & 15)) >> 4) & 1;
				OF_Carry = ((DX & 0x7FFF) + (Src & 0x7FFF)) >> 15;
				Result_32 = DX + Src;
				DX = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << " DX = " << (int)DX;
				break;
			case 3:
				Flag_AF = (((BX & 15) + (Src & 15)) >> 4) & 1;
				OF_Carry = ((BX & 0x7FFF) + (Src & 0x7FFF)) >> 15;
				Result_32 = BX + Src;
				BX = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << " BX = " << (int)BX;
				break;
			case 4:
				Flag_AF = (((Stack_Pointer & 15) + (Src & 15)) >> 4) & 1;
				OF_Carry = ((Stack_Pointer & 0x7FFF) + (Src & 0x7FFF)) >> 15;
				Result_32 = Stack_Pointer + Src;
				Stack_Pointer = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << " SP = " << (int)Stack_Pointer;
				break;
			case 5:
				Flag_AF = (((Base_Pointer & 15) + (Src & 15)) >> 4) & 1;
				OF_Carry = ((Base_Pointer & 0x7FFF) + (Src & 0x7FFF)) >> 15;
				Result_32 = Base_Pointer + Src;
				Base_Pointer = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << " BP = " << (int)Base_Pointer;
				break;
			case 6:
				Flag_AF = (((Source_Index & 15) + (Src & 15)) >> 4) & 1;
				OF_Carry = ((Source_Index & 0x7FFF) + (Src & 0x7FFF)) >> 15;
				Result_32 = Source_Index + Src;
				Source_Index = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << " SI = " << (int)Source_Index;
				break;
			case 7:
				Flag_AF = (((Destination_Index & 15) + (Src & 15)) >> 4) & 1;
				OF_Carry = ((Destination_Index & 0x7FFF) + (Src & 0x7FFF)) >> 15;
				Result_32 = Destination_Index + Src;
				Destination_Index = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << " DI = " << (int)Destination_Index;
				break;
			}
			break;
		}
		break;

	case 1:   //OR
		//определяем объект назначения и результат операции
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16) + memory.read_2(Instruction_Pointer + 3 + additional_IPs + *CS * 16) * 256;
			if (log_to_console) cout << "IMM(" << (int)Src << ") OR ";
			Result = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) | Src;
			memory.write_2(New_Addr,  Result & 255);
			memory.write_2(New_Addr + 1,  Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)Result;
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16) + memory.read_2(Instruction_Pointer + 3 + additional_IPs + *CS * 16) * 256;
			if (log_to_console) cout << "IMM(" << (int)Src << ") OR ";
			switch (byte2 & 7)
			{
			case 0:
				AX = AX | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((AX >> 15) & 1);
				if (AX) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[AX & 255];
				if (log_to_console) cout << " AX = " << (int)AX;
				break;
			case 1:
				CX = CX | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((CX >> 15) & 1);
				if (CX) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[CX & 255];
				if (log_to_console) cout << " CX = " << (int)CX;
				break;
			case 2:
				DX = DX | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((DX >> 15) & 1);
				if (DX) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[DX & 255];
				if (log_to_console) cout << " DX = " << (int)DX;
				break;
			case 3:
				BX = BX | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((BX >> 15) & 1);
				if (BX) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[BX & 255];
				if (log_to_console) cout << " BX = " << (int)BX;
				break;
			case 4:
				Stack_Pointer = Stack_Pointer | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Stack_Pointer >> 15) & 1);
				if (Stack_Pointer) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Stack_Pointer & 255];
				if (log_to_console) cout << " SP = " << (int)Stack_Pointer;
				break;
			case 5:
				Base_Pointer = Base_Pointer | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Base_Pointer >> 15) & 1);
				if (Base_Pointer) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Base_Pointer & 255];
				if (log_to_console) cout << " BP = " << (int)Base_Pointer;
				break;
			case 6:
				Source_Index = Source_Index | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Source_Index >> 15) & 1);
				if (Source_Index) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Source_Index & 255];
				if (log_to_console) cout << " SI = " << (int)Source_Index;
				break;
			case 7:
				Destination_Index = Destination_Index | Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Destination_Index >> 15) & 1);
				if (Destination_Index) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Destination_Index & 255];
				if (log_to_console) cout << " DI = " << (int)Destination_Index;
				break;
			}
			break;
		}
		break;

	case 2:   //ADC IMM -> R/M 16 bit
		//определяем объект назначения и результат операции
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16) + memory.read_2(Instruction_Pointer + 3 + additional_IPs + *CS * 16) * 256;
			if (log_to_console) cout << "ADC IMM(" << (int)Src << ") + ";
			Flag_AF = (((memory.read_2(New_Addr) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = (((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
			Result_32 = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) + Src + Flag_CF;
			memory.write_2(New_Addr,  Result_32 & 255);
			memory.write_2(New_Addr + 1,  (Result_32 >> 8) & 255);
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16) + memory.read_2(Instruction_Pointer + 3 + additional_IPs + *CS * 16) * 256;
			if (log_to_console) cout << "ADC IMM(" << (int)Src << ") + ";
			switch (byte2 & 7)
			{
			case 0:
				Flag_AF = ((( AX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
				OF_Carry = ((AX & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
				Result_32 = AX + Src + Flag_CF;
				AX = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << " AX = " << (int)AX;
				break;
			case 1:
				Flag_AF = (((CX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
				OF_Carry = ((CX & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
				Result_32 = CX + Src + Flag_CF;
				CX = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << " CX = " << (int)CX;
				break;
			case 2:
				Flag_AF = (((DX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
				OF_Carry = ((DX & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
				Result_32 = DX + Src + Flag_CF;
				DX = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << " DX = " << (int)DX;
				break;
			case 3:
				Flag_AF = (((BX & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
				OF_Carry = ((BX & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
				Result_32 = BX + Src + Flag_CF;
				BX = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << " BX = " << (int)BX;
				break;
			case 4:
				Flag_AF = (((Stack_Pointer & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
				OF_Carry = ((Stack_Pointer & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
				Result_32 = Stack_Pointer + Src + Flag_CF;
				Stack_Pointer = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << " SP = " << (int)Stack_Pointer;
				break;
			case 5:
				Flag_AF = (((Base_Pointer & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
				OF_Carry = ((Base_Pointer & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
				Result_32 = Base_Pointer + Src + Flag_CF;
				Base_Pointer = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << " BP = " << (int)Base_Pointer;
				break;
			case 6:
				Flag_AF = (((Source_Index & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
				OF_Carry = ((Source_Index & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
				Result_32 = Source_Index + Src + Flag_CF;
				Source_Index = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << " SI = " << (int)Source_Index;
				break;
			case 7:
				Flag_AF = (((Destination_Index & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
				OF_Carry = ((Destination_Index & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
				Result_32 = Destination_Index + Src + Flag_CF;
				Destination_Index = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << " DI = " << (int)Destination_Index;
				break;
			}
			break;
		}
		break;

	case 3:   //SBB IMM -> R/M 16 bit
		//определяем объект назначения и результат операции
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16) + memory.read_2(Instruction_Pointer + 3 + additional_IPs + *CS * 16) * 256;
			if (log_to_console) cout << "SBB IMM[" << (int)Src << "] ";
			Flag_AF = (((memory.read_2(New_Addr) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
			OF_Carry = (((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
			Result_32 = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) - Src - Flag_CF;
			memory.write_2(New_Addr,  Result_32 & 255);
			memory.write_2(New_Addr + 1,  (Result_32 >> 8) & 255);
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << "from M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF) - Flag_CF;
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16) + memory.read_2(Instruction_Pointer + 3 + additional_IPs + *CS * 16) * 256;
			if (log_to_console) cout << "SBB IMM[" << (int)Src << "] ";
			switch (byte2 & 7)
			{
			case 0:
				Flag_AF = (((AX & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
				OF_Carry = ((AX & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
				Result_32 = AX - Src - Flag_CF;
				AX = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << "from AX = " << (int)AX;
				break;
			case 1:
				Flag_AF = (((CX & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
				OF_Carry = ((CX & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
				Result_32 = CX - Src - Flag_CF;
				CX = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << "from CX = " << (int)CX;
				break;
			case 2:
				Flag_AF = (((DX & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
				OF_Carry = ((DX & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
				Result_32 = DX - Src - Flag_CF;
				DX = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << "from DX = " << (int)DX;
				break;
			case 3:
				Flag_AF = (((BX & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
				OF_Carry = ((BX & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
				Result_32 = BX - Src - Flag_CF;
				BX = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << "from BX = " << (int)BX;
				break;
			case 4:
				Flag_AF = (((Stack_Pointer & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
				OF_Carry = ((Stack_Pointer & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
				Result_32 = Stack_Pointer - Src - Flag_CF;
				Stack_Pointer = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << "from SP = " << (int)Stack_Pointer;
				break;
			case 5:
				Flag_AF = (((Base_Pointer & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
				OF_Carry = ((Base_Pointer & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
				Result_32 = Base_Pointer - Src - Flag_CF;
				Base_Pointer = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << "from BP = " << (int)Base_Pointer;
				break;
			case 6:
				Flag_AF = (((Source_Index & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
				OF_Carry = ((Source_Index & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
				Result_32 = Source_Index - Src - Flag_CF;
				Source_Index = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << "from SI = " << (int)Source_Index;
				break;
			case 7:
				Flag_AF = (((Destination_Index & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
				OF_Carry = ((Destination_Index & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
				Result_32 = Destination_Index - Src - Flag_CF;
				Destination_Index = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << "from DI = " << (int)Destination_Index;
				break;
			}
			break;
		}
		break;

	case 4:   //AND

		//определяем объект назначения и результат операции
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16) + memory.read_2(Instruction_Pointer + 3 + additional_IPs + *CS * 16) * 256;
			if (log_to_console) cout << "IMM[" << (int)Src << "] AND ";
			Result = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & Src;
			memory.write_2(New_Addr,  Result & 255);
			memory.write_2(New_Addr + 1,  Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)Result;
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16) + memory.read_2(Instruction_Pointer + 3 + additional_IPs + *CS * 16) * 256;
			if (log_to_console) cout << "IMM[" << (int)Src << "] AND ";
			switch (byte2 & 7)
			{
			case 0:
				AX = AX & Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((AX >> 15) & 1);
				if (AX) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[AX & 255];
				if (log_to_console) cout << " AX = " << (int)AX;
				break;
			case 1:
				CX = CX & Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((CX >> 15) & 1);
				if (CX) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[CX & 255];
				if (log_to_console) cout << " CX = " << (int)CX;
				break;
			case 2:
				DX = DX & Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((DX >> 15) & 1);
				if (DX) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[DX & 255];
				if (log_to_console) cout << " DX = " << (int)DX;
				break;
			case 3:
				BX = BX & Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((BX >> 15) & 1);
				if (BX) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[BX & 255];
				if (log_to_console) cout << " BX = " << (int)BX;
				break;
			case 4:
				Stack_Pointer = Stack_Pointer & Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Stack_Pointer >> 15) & 1);
				if (Stack_Pointer) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Stack_Pointer & 255];
				if (log_to_console) cout << " SP = " << (int)Stack_Pointer;
				break;
			case 5:
				Base_Pointer = Base_Pointer & Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Base_Pointer >> 15) & 1);
				if (Base_Pointer) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Base_Pointer & 255];
				if (log_to_console) cout << " BP = " << (int)Base_Pointer;
				break;
			case 6:
				Source_Index = Source_Index & Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Source_Index >> 15) & 1);
				if (Source_Index) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Source_Index & 255];
				if (log_to_console) cout << " SI = " << (int)Source_Index;
				break;
			case 7:
				Destination_Index = Destination_Index & Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Destination_Index >> 15) & 1);
				if (Destination_Index) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Destination_Index & 255];
				if (log_to_console) cout << " DI = " << (int)Destination_Index;
				break;
			}
			break;
		}
		break;

	case 5:   //SUB IMM -> R/M 16 bit
		//определяем объект назначения и результат операции
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16) + memory.read_2(Instruction_Pointer + 3 + additional_IPs + *CS * 16) * 256;
			if (log_to_console) cout << "SUB IMM[" << (int)Src << "] ";
			Flag_AF = (((memory.read_2(New_Addr) & 15) - (Src & 15)) >> 4) & 1;
			OF_Carry = (((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Result_32 = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) - Src;
			memory.write_2(New_Addr,  Result_32 & 255);
			memory.write_2(New_Addr + 1,  (Result_32 >> 8) & 255);
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << "from M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16) + memory.read_2(Instruction_Pointer + 3 + additional_IPs + *CS * 16) * 256;
			if (log_to_console) cout << "SUB IMM[" << (int)Src << "] ";
			switch (byte2 & 7)
			{
			case 0:
				Flag_AF = (((AX & 15) - (Src & 15)) >> 4) & 1;
				OF_Carry = ((AX & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Result_32 = AX - Src;
				AX = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << "from AX = " << (int)AX;
				break;
			case 1:
				Flag_AF = (((CX & 15) - (Src & 15)) >> 4) & 1;
				OF_Carry = ((CX & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Result_32 = CX - Src;
				CX = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << "from CX = " << (int)CX;
				break;
			case 2:
				Flag_AF = (((DX & 15) - (Src & 15)) >> 4) & 1;
				OF_Carry = ((DX & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Result_32 = DX - Src;
				DX = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << "from DX = " << (int)DX;
				break;
			case 3:
				Flag_AF = (((BX & 15) - (Src & 15)) >> 4) & 1;
				OF_Carry = ((BX & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Result_32 = BX - Src;
				BX = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << "from BX = " << (int)BX;
				break;
			case 4:
				Flag_AF = (((Stack_Pointer & 15) - (Src & 15)) >> 4) & 1;
				OF_Carry = ((Stack_Pointer & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Result_32 = Stack_Pointer - Src;
				Stack_Pointer = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << "from SP = " << (int)Stack_Pointer;
				break;
			case 5:
				Flag_AF = (((Base_Pointer & 15) - (Src & 15)) >> 4) & 1;
				OF_Carry = ((Base_Pointer & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Result_32 = Base_Pointer - Src;
				Base_Pointer = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << "from BP = " << (int)Base_Pointer;
				break;
			case 6:
				Flag_AF = (((Source_Index & 15) - (Src & 15)) >> 4) & 1;
				OF_Carry = ((Source_Index & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Result_32 = Source_Index - Src;
				Source_Index = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << "from SI = " << (int)Source_Index;
				break;
			case 7:
				Flag_AF = (((Destination_Index & 15) - (Src & 15)) >> 4) & 1;
				OF_Carry = ((Destination_Index & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Result_32 = Destination_Index - Src;
				Destination_Index = Result_32 & 0xFFFF;
				Flag_CF = ((Result_32 >> 16) & 1);
				Flag_SF = ((Result_32 >> 15) & 1);
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				if (log_to_console) cout << "from DI = " << (int)Destination_Index;
				break;
			}
			break;
		}
		break;

	case 6:  //XOR
		//определяем объект назначения и результат операции
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16) + memory.read_2(Instruction_Pointer + 3 + additional_IPs + *CS * 16) * 256;
			if (log_to_console) cout << "XOR IMM(" << (int)Src << ") ";
			Result = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) ^ Src;
			memory.write_2(New_Addr,  Result & 255);
			memory.write_2(New_Addr + 1,  Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)Result;
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16) + memory.read_2(Instruction_Pointer + 3 + additional_IPs + *CS * 16) * 256;
			if (log_to_console) cout << "XOR IMM(" << (int)Src << ") ";
			switch (byte2 & 7)
			{
			case 0:
				AX = AX ^ Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((AX >> 15) & 1);
				if (AX) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[AX & 255];
				if (log_to_console) cout << " AX = " << (int)AX;
				break;
			case 1:
				CX = CX ^ Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((CX >> 15) & 1);
				if (CX) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[CX & 255];
				if (log_to_console) cout << " CX = " << (int)CX;
				break;
			case 2:
				DX = DX ^ Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((DX >> 15) & 1);
				if (DX) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[DX & 255];
				if (log_to_console) cout << " DX = " << (int)DX;
				break;
			case 3:
				BX = BX ^ Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((BX >> 15) & 1);
				if (BX) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[BX & 255];
				if (log_to_console) cout << " BX = " << (int)BX;
				break;
			case 4:
				Stack_Pointer = Stack_Pointer ^ Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Stack_Pointer >> 15) & 1);
				if (Stack_Pointer) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Stack_Pointer & 255];
				if (log_to_console) cout << " SP = " << (int)Stack_Pointer;
				break;
			case 5:
				Base_Pointer = Base_Pointer ^ Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Base_Pointer >> 15) & 1);
				if (Base_Pointer) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Base_Pointer & 255];
				if (log_to_console) cout << " BP = " << (int)Base_Pointer;
				break;
			case 6:
				Source_Index = Source_Index ^ Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Source_Index >> 15) & 1);
				if (Source_Index) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Source_Index & 255];
				if (log_to_console) cout << " SI = " << (int)Source_Index;
				break;
			case 7:
				Destination_Index = Destination_Index ^ Src;
				Flag_CF = 0;
				Flag_OF = 0;
				Flag_SF = ((Destination_Index >> 15) & 1);
				if (Destination_Index) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Destination_Index & 255];
				if (log_to_console) cout << " DI = " << (int)Destination_Index;
				break;
			}
			break;
		}
		break;

	case 7:   //CMP IMM -> R/M 16 bit
		//определяем объект назначения и результат операции
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16) + memory.read_2(Instruction_Pointer + 3 + additional_IPs + *CS * 16) * 256;
			if (log_to_console) cout << "CMP IMM(" << (int)Src << ") ";
			Result_32 = (memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) - Src;
			Flag_CF = (Result_32 >> 16) & 1;
			Flag_SF = (Result_32 >> 15) & 1;
			OF_Carry = (((memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			Flag_AF = ((((memory.read_2(New_Addr)) & 15) - (Src & 15)) >> 4) & 1;
			if (log_to_console) cout << "with M" << OPCODE_comment << "";
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд - вычитаемое
			Src = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16) + memory.read_2(Instruction_Pointer + 3 + additional_IPs + *CS * 16) * 256;
			if (log_to_console) cout << "CMP IMM(" << (int)Src << ") ";
			switch (byte2 & 7)
			{
			case 0:
				Result_32 = AX - Src;
				Flag_CF = (Result_32 >> 16) & 1;
				Flag_SF = (Result_32 >> 15) & 1;
				OF_Carry = ((AX & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				Flag_AF = (((AX & 15) - (Src & 15)) >> 4) & 1;
				if (log_to_console) cout << "with AX(" << (int)AX << ")";
				break;
			case 1:
				Result_32 = CX - Src;
				Flag_CF = (Result_32 >> 16) & 1;
				Flag_SF = (Result_32 >> 15) & 1;
				OF_Carry = ((CX & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				Flag_AF = (((CX & 15) - (Src & 15)) >> 4) & 1;
				if (log_to_console) cout << "with CX = " << (int)CX;
				break;
			case 2:
				Result_32 = DX - Src;
				Flag_CF = (Result_32 >> 16) & 1;
				Flag_SF = (Result_32 >> 15) & 1;
				OF_Carry = ((DX & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				Flag_AF = (((DX & 15) - (Src & 15)) >> 4) & 1;
				if (log_to_console) cout << "with DX = " << (int)DX;
				break;
			case 3:
				Result_32 = BX - Src;
				Flag_CF = (Result_32 >> 16) & 1;
				Flag_SF = (Result_32 >> 15) & 1;
				OF_Carry = ((BX & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				Flag_AF = (((BX & 15) - (Src & 15)) >> 4) & 1;
				if (log_to_console) cout << "with BX = " << (int)BX;
				break;
			case 4:
				Result_32 = Stack_Pointer - Src;
				Flag_CF = (Result_32 >> 16) & 1;
				Flag_SF = (Result_32 >> 15) & 1;
				OF_Carry = ((Stack_Pointer & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				Flag_AF = (((Stack_Pointer & 15) - (Src & 15)) >> 4) & 1;
				if (log_to_console) cout << "with SP = " << (int)Stack_Pointer;
				break;
			case 5:
				Result_32 = Base_Pointer - Src;
				Flag_CF = (Result_32 >> 16) & 1;
				Flag_SF = (Result_32 >> 15) & 1;
				OF_Carry = ((Base_Pointer & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				Flag_AF = (((Base_Pointer & 15) - (Src & 15)) >> 4) & 1;
				if (log_to_console) cout << "with BP = " << (int)Base_Pointer;
				break;
			case 6:
				Result_32 = Source_Index - Src;
				Flag_CF = (Result_32 >> 16) & 1;
				Flag_SF = (Result_32 >> 15) & 1;
				OF_Carry = ((Source_Index & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				Flag_AF = (((Source_Index & 15) - (Src & 15)) >> 4) & 1;
				if (log_to_console) cout << "with SI = " << (int)Source_Index;
				break;
			case 7:
				Result_32 = Destination_Index - Src;
				Flag_CF = (Result_32 >> 16) & 1;
				Flag_SF = (Result_32 >> 15) & 1;
				OF_Carry = ((Destination_Index & 0x7FFF) - (Src & 0x7FFF)) >> 15;
				Flag_OF = Flag_CF ^ OF_Carry;
				if (Result_32 & 0xFFFF) Flag_ZF = false;
				else Flag_ZF = true;
				Flag_PF = parity_check[Result_32 & 255];
				Flag_AF = (((Destination_Index & 15) - (Src & 15)) >> 4) & 1;
				if (log_to_console) cout << "with DI = " << (int)Destination_Index;
				break;
			}
			break;
		}
		break;
	}
	Instruction_Pointer += 4 + additional_IPs;
}

void XOR_IMM_ACC_8()   //XOR IMM to ACC 8bit
{
	uint8 Src = 0;
	uint8 Result = 0;

	//непосредственный операнд
	Src = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];

	if (log_to_console) cout << "XOR IMM(" << (int)Src << ") ^ ";

	//определяем объект назначения и результат операции
	Result = (AX & 255) ^ Src;
	if (log_to_console) cout << " AL(" << (int)(AX & 255) << ") = " << (int)Result;
	AX = (AX & 0xFF00) | Result;
	Flag_CF = 0;
	Flag_OF = 0;
	Flag_SF = ((Result >> 7) & 1);
	if (Result) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result];
	Instruction_Pointer += 2;
}
void XOR_IMM_ACC_16()  //XOR IMM to ACC 16bit
{
	uint16 Src = 0;
	uint16 Result = 0;

	//непосредственный операнд
	Src = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory.read_2(Instruction_Pointer + 2 + *CS * 16) * 256;

	if (log_to_console) cout << "XOR IMM(" << (int)Src << ") ^ ";

	//определяем объект назначения и результат операции
	Result = AX ^ Src;
	if (log_to_console) cout << " AX(" << (int)AX << ") = " << (int)Result;
	AX = Result;
	Flag_CF = 0;
	Flag_OF = 0;
	Flag_SF = ((Result >> 15) & 1);
	if (Result) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result & 255];
	Instruction_Pointer += 3;
}

//============String Manipulation=========================

void REPNE()		//REP = Repeat while ZF=0 and CX > 0   [F2]  для CMPS, SCAS
{
	uint8 OP = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Result_16 = 0;
	uint32 Result_32 = 0;
	bool OF_Carry = false;
	if (log_to_console) cout << "REPNE[F2] ";
	switch (OP)
	{
	case 0b10100100:  //MOVS 8

		if (log_to_console) cout << "MOVES_B (" << (int)CX << " bytes) from [" << (int)*DS << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
		while (CX)
		{
			memory.write_2(Destination_Index + *ES * 16, memory.read_2(Source_Index + *DS * 16));
			if (Flag_DF)
			{
				Destination_Index--;
				Source_Index--;
			}
			else
			{
				Destination_Index++;
				Source_Index++;
			}
			CX--;
		}
		break;

	case 0b10100101:  //MOVS 16

		if (log_to_console) cout << "MOVES_W (" << (int)CX << " words) from [" << (int)*DS << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
		while (CX)
		{
			memory.write_2(Destination_Index + *ES * 16, memory.read_2(Source_Index + *DS * 16));
			Destination_Index++;
			Source_Index++;
			memory.write_2(Destination_Index + *ES * 16, memory.read_2(Source_Index + *DS * 16));

			if (Flag_DF)
			{
				Destination_Index -= 3;
				Source_Index -= 3;
			}
			else
			{
				Destination_Index ++;
				Source_Index ++;
			}
			CX--;
		}
		break;

	case 0b10100110:  //CMPS 8

		if (log_to_console) cout << "CMPS_B until == 0 (" << (int)CX << " bytes) from [" << (int)*DS << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]  ";
		if (log_to_console)
		{
			cout << "[";
			for (int i = 0; i < CX; i++) cout << memory.read_2(Source_Index + *DS * 16);
			cout << "][";
			for (int i = 0; i < CX; i++) cout << memory.read_2(Destination_Index + *ES * 16);
			cout << "]" << endl;;
		}
		
		do
		{
			if (!CX) break;
			Result_16 = memory.read_2(Source_Index + *DS * 16) - memory.read_2(Destination_Index + *ES * 16);
			OF_Carry = ((memory.read_2(Source_Index + *DS * 16) & 0x7F) - (memory.read_2(Destination_Index + *ES * 16) & 0x7F)) >> 7;
			//if (log_to_console || 1) cout << " (" << memory.read_2(Source_Index + *DS * 16) << "-" << memory.read_2(Destination_Index + *ES * 16) << ") ";
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			//cout << (int)Flag_ZF << endl;
			Flag_PF = parity_check[Result_16 & 255];
			Flag_AF = (((memory.read_2(Source_Index + *DS * 16) & 0x0F) - (memory.read_2(Destination_Index + *ES * 16) & 0x0F)) >> 4) & 1;
			if (Flag_DF)
			{
				Destination_Index--;
				Source_Index--;
			}
			else
			{
				Destination_Index++;
				Source_Index++;
			}
			CX--;
		} while (CX && !Flag_ZF);
		
		break;

	case 0b10100111:  //CMPS 16

		if (log_to_console) cout << "CMPS_W (" << (int)CX << " word) from [" << (int)*DS << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]" << endl;
		do
		{
			if (!CX) break;
			Src = memory.read_2(Source_Index + *DS * 16);
			Dst = memory.read_2(Destination_Index + *ES * 16);
			Destination_Index++;
			Source_Index++;
			Src += memory.read_2(Source_Index + *DS * 16) * 256;
			Dst += memory.read_2(Destination_Index + *ES * 16) * 256;

			Result_32 = Src - Dst;
			if (log_to_console) cout << (int)Src << " - " << (int)Dst << " = " << (int)Result_32;
			OF_Carry = ((Src & 0x7FFF) - (Dst & 0x7FFF)) >> 15;
			Flag_CF = (Result_32 >> 16) & 1;
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_AF = (((Src & 15) - (Dst & 15)) >> 4) & 1;
			Flag_PF = parity_check[Result_32 & 255];
			
			if (log_to_console) cout << " ZF = " << int(Flag_ZF) << endl;

			if (Flag_DF)
			{
				Destination_Index -= 3;
				Source_Index -= 3;
			}
			else
			{
				Destination_Index ++;
				Source_Index ++;
			}
			CX--;
		} while (CX && !Flag_ZF);
		break;

	case 0b10101110:  //SCAS 8
		if (log_to_console) cout << "SCAS_B (" << (int)CX << " bytes) from [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
		do
		{
			if (!CX) break;
			uint16 Result = (AX & 255) - memory.read_2(Destination_Index + *ES * 16);
			OF_Carry = ((AX & 0x7F) - (memory.read_2(Destination_Index + *ES * 16) & 0x7F)) >> 7;
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = (((AX & 15) - (memory.read_2(Destination_Index + *ES * 16) & 15)) >> 4) & 1;
			if (Flag_DF)
			{
				Destination_Index--;
			}
			else
			{
				Destination_Index++;
			}
			CX--;
		} while (CX && !Flag_ZF);
		break;

	case 0b10101111:  //SCAS 16

		if (log_to_console) cout << "SCAS_W (" << (int)CX << " word) from [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
		do
		{
			if (!CX) break;
			*ptr_Dst_L = memory_2[(Destination_Index + *ES * 16) & 0xFFFFF];
			Destination_Index++;
			*ptr_Dst_H = memory_2[(Destination_Index + *ES * 16) & 0xFFFFF];
			Result_32 = AX - Dst;
			OF_Carry = ((AX & 0x7FFF) - (Dst & 0x7FFF)) >> 15;
			Flag_CF = (Result_32 >> 16) & 1;
			Flag_SF = (Result_32 >> 15) & 1;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			Flag_AF = (((AX & 15) - (*ptr_Dst_L & 15)) >> 4) & 1;
			if (Flag_DF)
			{
				Destination_Index -= 3;
			}
			else
			{
				Destination_Index ++;
			}
			CX--;
		} while (CX && !Flag_ZF);
		break;

	case 0b10101100:  //LODS 8
		if (log_to_console) cout << "LODS_B (" << (int)CX << " bytes) from [" << (int)*DS << "]:[" << (int)Source_Index << "]";
		while (CX)
		{
			AX = (AX & 0xFF00) | memory.read_2(Source_Index + *DS * 16);
			if (Flag_DF)
			{
				Source_Index--;
			}
			else
			{
				Source_Index++;
			}
			CX--;
		}
		break;

	case 0b10101101:  //LODS 16
		if (log_to_console) cout << "LODS_B (" << (int)CX << " words) from [" << (int)*DS << "]:[" << (int)Source_Index << "]";
		while (CX)
		{
			AX = memory.read_2(Source_Index + *DS * 16);
			Source_Index++;
			AX += memory.read_2(Source_Index + *DS * 16) * 256;
			if (Flag_DF)
			{
				Source_Index -= 3;
			}
			else
			{
				Source_Index ++;
			}
			CX--;
		}
		break;

	case 0b10101010:  //STOS 8

		if (log_to_console) cout << "STOS_B (" << (int)CX << " bytes) to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
		while (CX)
		{
			memory.write_2(Destination_Index + *ES * 16, AX & 255);
			if (Flag_DF)
			{
				Destination_Index--;
			}
			else
			{
				Destination_Index++;
			}
			CX--;
		}
		break;

	case 0b10101011:  //STOS 16

		if (log_to_console) cout << "STOS_W (" << (int)CX << " words) to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
		while (CX)
		{
			memory.write_2(Destination_Index + *ES * 16, AX & 255);
			Destination_Index++;
			memory.write_2(Destination_Index + *ES * 16, AX >> 8);
			if (Flag_DF)
			{
				Destination_Index -= 3;
			}
			else
			{
				Destination_Index ++;
			}
			CX--;
		}
		break;

	default:
		Instruction_Pointer ++;
		if (test_mode) repeat_test_op = 1;
		//проверяем следующую команду F6 или F7 + mod 111 r/m
		if (memory.read_2(Instruction_Pointer + *CS * 16) == 0xF6 || memory.read_2(Instruction_Pointer + *CS * 16) == 0xF7)
		{
			if ((memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] & 0b00111000) == 0b00111000) 
			{
				negate_IDIV = true;
				if (log_to_console) cout << "negate ON ";
			}
		}
		return;
	}
	Instruction_Pointer += 2;
}
void REP()			//REP = Repea while CX > 0			[F3]
{
	uint8 OP = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Result_16 = 0;
	uint32 Result_32 = 0;
	bool OF_Carry = false;
	if (log_to_console) cout << "REP[F3] ";
	switch (OP)
	{
	case 0b10100100:  //MOVS 8
		
		if (log_to_console) cout << "MOVES_B (" << (int)CX << " bytes) from [" << (int)*DS << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]"; 
		while (CX)
		{
			memory.write_2(Destination_Index + *ES * 16, memory.read_2(Source_Index + *DS * 16));
			if (Flag_DF)
			{
				Destination_Index--;
				Source_Index--;
			}
			else
			{
				Destination_Index++;
				Source_Index++;
			}
			CX--;
		}
		break;

	case 0b10100101:  //MOVS 16

		if (log_to_console) cout << "MOVES_W (" << (int)CX << " words) from [" << (int)*DS << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
		while (CX)
		{
			memory.write_2(Destination_Index + *ES * 16, memory.read_2(Source_Index + *DS * 16));
			Destination_Index ++;
			Source_Index ++;
			memory.write_2(Destination_Index + *ES * 16, memory.read_2(Source_Index + *DS * 16));
			if (Flag_DF)
			{
				Destination_Index -= 3;
				Source_Index -= 3;
			}
			else
			{
				Destination_Index ++;
				Source_Index ++;
			}
			CX--;
		}
		break;

	case 0b10100110:  //CMPS 8
		
		if (log_to_console) cout << "CMPS_B while == 0 (" << (int)CX << " bytes) from [" << (int)*DS << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]  ";
		if (log_to_console)
		{
			cout << "[";
			for (int i = 0; i < CX; i = i + (1 + Flag_DF * -2)) cout << memory.read_2(Source_Index + i + *DS * 16);
			cout << "][";
			for (int i = 0; i < CX; i = i + (1 + Flag_DF * -2)) cout << memory.read_2(Destination_Index + i + *ES * 16);
			cout << "]" << endl;
		}
				
		do
		{
			if (!CX) break;
			Result_16 = memory.read_2(Source_Index + *DS * 16) - memory.read_2(Destination_Index + *ES * 16);
			OF_Carry = ((memory.read_2(Source_Index + *DS * 16) & 0x7F) - (memory.read_2(Destination_Index + *ES * 16) & 0x7F)) >> 7;
			//if (log_to_console || 1) cout << " (" << memory.read_2(Source_Index + *DS * 16) << "-" << memory.read_2(Destination_Index + *ES * 16) << ") ";
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			//cout << (int)Flag_ZF << endl;
			Flag_PF = parity_check[Result_16 & 255];
			Flag_AF = (((memory.read_2(Source_Index + *DS * 16) & 0x0F) - (memory.read_2(Destination_Index + *ES * 16) & 0x0F)) >> 4) & 1;
			if (Flag_DF)
			{
				Destination_Index--;
				Source_Index--;
			}
			else
			{
				Destination_Index++;
				Source_Index++;
			}
			CX--;
		} while (CX && Flag_ZF);
		break;

	case 0b10100111:  //CMPS 16

		if (log_to_console) cout << "CMPS_W (" << (int)CX << " word) from [" << (int)*DS << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
		do
		{
			if (!CX) break;
			*ptr_Src_L = memory_2[(Source_Index + *DS * 16) & 0xFFFFF];
			*ptr_Dst_L = memory_2[(Destination_Index + *ES * 16) & 0xFFFFF];
			Destination_Index++;
			Source_Index++;
			*ptr_Src_H = memory_2[(Source_Index + *DS * 16) & 0xFFFFF];
			*ptr_Dst_H = memory_2[(Destination_Index + *ES * 16) & 0xFFFFF];
			Result_32 = Src - Dst;
			if (log_to_console) cout << (int)Src << " - " << (int)Dst << " = " << (int)Result_32;
			OF_Carry = ((Src & 0x7FFF) - (Dst & 0x7FFF)) >> 15;
			Flag_CF = (Result_32 >> 16) & 1;
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_AF = (((Src & 15) - (Dst & 15)) >> 4) & 1;
			Flag_PF = parity_check[Result_32 & 255];

			//if (log_to_console) cout << " ZF = " << int(Flag_ZF) << endl;

			if (Flag_DF)
			{
				Destination_Index -= 3;
				Source_Index -= 3;
			}
			else
			{
				Destination_Index ++;
				Source_Index ++;
			}
			CX--;
		} while (CX && Flag_ZF);
		break;

	case 0b10101110:  //SCAS 8
		if (log_to_console) cout << "SCAS_B (" << (int)CX << " bytes) from [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
		do
		{
			if (!CX) break;
			Result_16 = (AX & 255) - memory.read_2(Destination_Index + *ES * 16);
			OF_Carry = (((AX & 0x7F) - (memory.read_2(Destination_Index + *ES * 16) & 0x7F))) >> 7;
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = (Result_16 >> 7) & 1;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			Flag_AF = (((AX & 15) - (memory.read_2(Destination_Index + *ES * 16) & 15)) >> 4) & 1;
			if (Flag_DF)
			{
				Destination_Index--;
				//Source_Index--;
			}
			else
			{
				Destination_Index++;
				//Source_Index++;
			}
			CX--;
		} while (CX && Flag_ZF);
		break;

	case 0b10101111:  //SCAS 16

		if (log_to_console) cout << "SCAS_W (" << (int)CX << " word) from [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
		do
		{
			if (!CX) break;
			*ptr_Dst_L = memory_2[(Destination_Index + *ES * 16) & 0xFFFFF];
			Destination_Index++;
			*ptr_Dst_H = memory_2[(Destination_Index + *ES * 16) & 0xFFFFF];
			Result_32 = AX - Dst;
			OF_Carry = ((AX & 0x7FFF) - (Dst & 0x7FFF)) >> 15;
			Flag_CF = (Result_32 >> 16) & 1;
			Flag_SF = (Result_32 >> 15) & 1;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_AF = (((AX & 15) - (Dst & 15)) >> 4) & 1;
			Flag_PF = parity_check[Result_32 & 255];
			if (Flag_DF)
			{
				Destination_Index -= 3;
			}
			else
			{
				Destination_Index ++;
			}
			CX--;
		} while (CX && Flag_ZF);
		break;

	case 0b10101100:  //LODS 8
		if (log_to_console) cout << "LODS_B (" << (int)CX << " bytes) from [" << (int)*DS << "]:[" << (int)Source_Index << "]";
		while (CX)
		{
			AX = (AX & 0xFF00) | memory.read_2(Source_Index + *DS * 16);
			if (Flag_DF)
			{
				Source_Index--;
			}
			else
			{
				Source_Index++;
			}
			CX--;
		}
		break;

	case 0b10101101:  //LODS 16
		if (log_to_console) cout << "LODS_B (" << (int)CX << " words) from [" << (int)*DS << "]:[" << (int)Source_Index << "]";
		while (CX)
		{
			*ptr_AL = memory.read_2(Source_Index + *DS * 16);
			Source_Index++;
			*ptr_AH = memory.read_2(Source_Index + *DS * 16);
			if (Flag_DF)
			{
				Source_Index -= 3;
			}
			else
			{
				Source_Index ++;
			}
			CX--;
		}
		break;
	
	case 0b10101010:  //STOS 8

		if (log_to_console) cout << "STOS_B (" << (int)CX << " bytes) to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
		while (CX)
		{
			memory.write_2(Destination_Index + *ES * 16, AX & 255);
			if (Flag_DF)
			{
				Destination_Index--;
			}
			else
			{
				Destination_Index++;
			}
			CX--;
		}
		break;

	case 0b10101011:  //STOS 16

		if (log_to_console) cout << "STOS_W (" << (int)CX << " words) to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
		while (CX)
		{
			memory.write_2(Destination_Index + *ES * 16, AX & 255);
			Destination_Index++;
			memory.write_2(Destination_Index + *ES * 16, AX >> 8);
			if (Flag_DF)
			{
				Destination_Index -= 3;
			}
			else
			{
				Destination_Index ++;
			}
			CX--;
		}
		break;
	
	default:
		Instruction_Pointer++;
		if (test_mode) repeat_test_op = 1;
		//проверяем следующую команду F6 или F7 + mod 111 r/m
		if (memory.read_2(Instruction_Pointer + *CS * 16) == 0xF6 || memory.read_2(Instruction_Pointer + *CS * 16) == 0xF7)
		{
			if ((memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] & 0b00111000) == 0b00111000)
			{
				negate_IDIV = true;
				if (log_to_console) cout << "negate ON ";
			}
		}
		return;
	}
	Instruction_Pointer += 2;
}
void MOVES_8()    //MOVS = Move String 8bit
{
	if (log_to_console) cout << "MOVES_B("<< (int)memory.read_2(Source_Index + *DS * 16) << ") from [" << (int)*DS << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
	memory.write_2(Destination_Index + *ES * 16, memory.read_2(Source_Index + *DS * 16));
	if (Flag_DF)
	{
		Destination_Index--;
		Source_Index--;
	}
	else
	{
		Destination_Index++;
		Source_Index++;
	}
	Instruction_Pointer ++;
}
void MOVES_16()   //MOVS = Move String 16bit
{
	if (log_to_console) cout << "MOVES_W ("<< (int)memory.read_2(Source_Index + *DS * 16) << (int)memory.read_2(Source_Index + 1 + *DS * 16) << ") from [" << (int)*DS << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
	memory.write_2(Destination_Index + *ES * 16, memory.read_2(Source_Index + *DS * 16));
	Destination_Index++;
	Source_Index++;
	memory.write_2(Destination_Index + *ES * 16, memory.read_2(Source_Index + *DS * 16));
	if (Flag_DF)
	{
		Destination_Index -= 3;
		Source_Index-=3;
	}
	else
	{
		Destination_Index++;
		Source_Index++;
	}
	Instruction_Pointer ++;
}
void CMPS_8()     //CMPS = Compare String
{
	if (log_to_console) cout << "CMPS_8 SOLO (" << memory.read_2(Source_Index + *DS * 16) << ") from [" << (int)*DS << "]:[" << Source_Index << "] to [" << (int)*ES << "]:[" << Destination_Index << "]";
	uint8 Src = memory.read_2(Source_Index + *DS * 16);
	uint8 Dst = memory.read_2(Destination_Index + *ES * 16);
	uint16 Result = Src - Dst;
	bool OF_Carry = ((Src & 0x7F) - (Dst & 0x7F)) >> 7;
	Flag_CF = (Result >> 8) & 1;
	Flag_SF = ((Result >> 7) & 1);
	Flag_OF = Flag_CF ^ OF_Carry;
	if (Result & 255) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result & 255];
	Flag_AF = (((Src & 0x0F) - (Dst & 0x0F)) >> 4) & 1;
	if (Flag_DF)
	{
		Destination_Index--;
		Source_Index--;
	}
	else
	{
		Destination_Index++;
		Source_Index++;
	}
	Instruction_Pointer++;
}
void CMPS_16()    //CMPS = Compare String
{
	if (log_to_console) cout << "CMPS_16 SOLO (" << (int)(memory.read_2(Source_Index + *DS * 16) + memory.read_2(Source_Index + *DS * 16)*256) << ") from [" << (int)*DS << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
	*ptr_Src_L = memory_2[(Source_Index + *DS * 16) & 0xFFFFF];
	Source_Index++;
	*ptr_Src_H = memory_2[(Source_Index + *DS * 16) & 0xFFFFF];
	*ptr_Dst_L = memory_2[(Destination_Index + *ES * 16) & 0xFFFFF];
	Destination_Index++;
	*ptr_Dst_H = memory_2[(Destination_Index + *ES * 16) & 0xFFFFF];
	uint32 Result_32 = Src - Dst;
	if (log_to_console) cout << " Res32 (" <<  (int)Src << " - " << (int)Dst << ") = " << (int)Result_32;
	bool OF_Carry = ((Src & 0x7FFF) - (Dst & 0x7FFF)) >> 15;
	Flag_CF = (Result_32 >> 16) & 1;
	Flag_SF = ((Result_32 >> 15) & 1);
	Flag_OF = Flag_CF ^ OF_Carry;
	if (Result_32 & 0xFFFF) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_AF = (((Src & 15) - (Dst & 15)) >> 4) & 1;
	Flag_PF = parity_check[Result_32 & 255];
	if (Flag_DF)
	{
		Destination_Index-=3;
		Source_Index-=3;
	}
	else
	{
		Destination_Index++;
		Source_Index++;
	}
	Instruction_Pointer++;
}
void SCAS_8()     //SCAS = Scan String
{
	if (log_to_console) cout << "SCAS 8 SOLO AL with string at [" << (int)*ES << "]:[" << Destination_Index << "]";
	uint16 Result = (AX & 255) - memory.read_2(Destination_Index + *ES * 16);
	bool OF_Carry = ((AX & 0x7F) - (memory.read_2(Destination_Index + *ES * 16) & 0x7F)) >> 7;
	Flag_CF = (Result >> 8) & 1;
	Flag_SF = ((Result >> 7) & 1);
	Flag_OF = Flag_CF ^ OF_Carry;
	if (Result & 255) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result & 255];
	Flag_AF = (((AX & 0x0F) - (memory.read_2(Destination_Index + *ES * 16) & 0x0F)) >> 4) & 1;
	if (Flag_DF)
	{
		Destination_Index--;
		//Source_Index--;
	}
	else
	{
		Destination_Index++;
		//Source_Index++;
	}
	Instruction_Pointer++;
}
void SCAS_16()     //SCAS = Scan String
{
	if (log_to_console) cout << "SCAS 16 SOLO AX with string at [" << (int)*ES << "]:[" << Destination_Index << "]";
	uint16 Dst = memory.read_2(Destination_Index + *ES * 16);
	Destination_Index++;
	Dst += memory.read_2(Destination_Index + *ES * 16) * 256;
	uint32 Result_32 = AX - Dst;
	bool OF_Carry = ((AX & 0x7FFF) - (Dst & 0x7FFF)) >> 15;
	Flag_CF = (Result_32 >> 16) & 1;
	Flag_SF = (Result_32 >> 15) & 1;
	Flag_OF = Flag_CF ^ OF_Carry;
	if (Result_32) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_AF = (((AX & 15) - (Dst & 15)) >> 4) & 1;
	Flag_PF = parity_check[Result_32 & 255];
	if (Flag_DF)
	{
		Destination_Index -= 3;
	}
	else
	{
		Destination_Index ++;
	}
	Instruction_Pointer++;
}
void LODS_8()     //LODS = Load String
{
	if (log_to_console) cout << "move string from [" << (int)*DS << "]:[" << Source_Index << "] to AL";
	AX = (AX & 0xFF00)|memory.read_2(Source_Index + *DS * 16);
	if (Flag_DF)
	{
		Source_Index--;
	}
	else
	{
		Source_Index++;
	}
	Instruction_Pointer ++;
}
void LODS_16()    //LODS = Load String
{
	if (log_to_console) cout << "move string_w from [" << (int)*DS << "]:[" << Source_Index << "] to AX";
	*ptr_AL = memory_2[(Source_Index + *DS * 16) & 0xFFFFF];
	Source_Index++;
	*ptr_AH = memory_2[(Source_Index + *DS * 16) & 0xFFFFF];
	if (Flag_DF)
	{
		Source_Index-=3;
	}
	else
	{
		Source_Index++;
	}
	Instruction_Pointer++;
}
void STOS_8()     //STOS = Store String
{
	if (log_to_console && memory.read_2(Instruction_Pointer + *CS * 16) != 0xF2 && memory.read_2(Instruction_Pointer + *CS * 16) != 0xF3) cout << "move string from AL to [" << (int)*ES << "]:[" << Destination_Index << "]";
	memory.write_2(Destination_Index + *ES * 16, AX & 255);
	if (Flag_DF)
	{
		Destination_Index--;
	}
	else
	{
		Destination_Index++;
	}
	Instruction_Pointer++;
}
void STOS_16()    //STOS = Store String
{
	if (log_to_console && memory.read_2(Instruction_Pointer + *CS * 16) != 0xF2 && memory.read_2(Instruction_Pointer + *CS * 16) != 0xF3) cout << "move string_w from AX to [" << (int)*ES << "]:[" << Destination_Index << "]";
	memory.write_2(Destination_Index + *ES * 16, AX & 255);
	Destination_Index++;
	memory.write_2(Destination_Index + *ES * 16, AX >> 8);
	if (Flag_DF)
	{
		Destination_Index -= 3;
	}
	else
	{
		Destination_Index ++;
	}
	Instruction_Pointer++;
}

//============ Control Transfer============================

// Call + Jump
void Call_Near()				//Direct Call within Segment
{
	SetConsoleTextAttribute(hConsole, 13);
	if (log_to_console) cout << "Direct Call within Segment. Ret to " << (int)*CS << ":" << Instruction_Pointer + 3;
	//else cout << "Direct Call within Segment. Ret to " << (int)*CS << ":" << Instruction_Pointer + 3 << endl;
	SetConsoleTextAttribute(hConsole, 7);
	Stack_Pointer--;
	memory.write_2(SS_data * 16 + Stack_Pointer, (Instruction_Pointer + 3) >> 8);
	Stack_Pointer--;
	memory.write_2(SS_data * 16 + Stack_Pointer, (Instruction_Pointer + 3) & 255);

	Instruction_Pointer += DispCalc16(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory.read_2(Instruction_Pointer + 2 + *CS * 16) * 256) + 3;
}
// INC/DEC/CALL/JUMP/PUSH
void Call_Jump_Push()				//Indirect (4 operations)
{
	uint8 byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];//второй байт
	uint8 OP = (byte2 >> 3) & 7;
	uint16 disp = 0;		//смещение
	uint16 Src = 0;			//источник данных
	additional_IPs = 0;
	uint8 tmp;
	uint16 old_IP = 0;
	uint16 new_IP = 0;
	uint32 stack_addr = 0;


	switch (OP)
	{
	case 0:  //INC R/M 16bit
	
		//находим цель инкремента
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
			Flag_AF = (((Src & 0x0F) + 1) >> 4) & 1;
			Flag_OF = (((Src & 0x7FFF) + 1) >> 15) & 1;
			Src++;
			memory.write_2(New_Addr,  Src & 255);
			memory.write_2(New_Addr + 1,  Src >> 8);
			if (Src) Flag_ZF = 0;
			else Flag_ZF = 1;
			Flag_SF = (Src >> 15) & 1;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "INC M" << OPCODE_comment << " = " << (int)Src;
			break;
		case 3:
			// mod 11 - адрес в регистре
			switch (byte2 & 7)
			{
			case 0:
				Flag_AF = (((AX & 0x0F) + 1) >> 4) & 1;
				Flag_OF = (((AX & 0x7FFF) + 1) >> 15) & 1;
				AX++;
				Src = AX;
				if (Src) Flag_ZF = 0;
				else Flag_ZF = 1;
				Flag_SF = (Src >> 15) & 1;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "INC AX = " << (int)Src;
				break;
			case 1:
				Flag_AF = (((CX & 0x0F) + 1) >> 4) & 1;
				Flag_OF = (((CX & 0x7FFF) + 1) >> 15) & 1;
				CX++;
				Src = CX;
				if (Src) Flag_ZF = 0;
				else Flag_ZF = 1;
				Flag_SF = (Src >> 15) & 1;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "INC CX = " << (int)Src;
				break;
			case 2:
				Flag_AF = (((DX & 0x0F) + 1) >> 4) & 1;
				Flag_OF = (((DX & 0x7FFF) + 1) >> 15) & 1;
				DX++;
				Src = DX;
				if (Src) Flag_ZF = 0;
				else Flag_ZF = 1;
				Flag_SF = (Src >> 15) & 1;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "INC DX = " << (int)Src;
				break;
			case 3:
				Flag_AF = (((BX & 0x0F) + 1) >> 4) & 1;
				Flag_OF = (((BX & 0x7FFF) + 1) >> 15) & 1;
				BX++;
				Src = BX;
				if (Src) Flag_ZF = 0;
				else Flag_ZF = 1;
				Flag_SF = (Src >> 15) & 1;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "INC BX = " << (int)Src;
				break;
			case 4:
				Flag_AF = (((Stack_Pointer & 0x0F) + 1) >> 4) & 1;
				Flag_OF = (((Stack_Pointer & 0x7FFF) + 1) >> 15) & 1;
				Stack_Pointer++;
				Src = Stack_Pointer;
				if (Src) Flag_ZF = 0;
				else Flag_ZF = 1;
				Flag_SF = (Src >> 15) & 1;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "INC SP = " << (int)Src;
				break;
			case 5:
				Flag_AF = (((Base_Pointer & 0x0F) + 1) >> 4) & 1;
				Flag_OF = (((Base_Pointer & 0x7FFF) + 1) >> 15) & 1;
				Base_Pointer++;
				Src = Base_Pointer;
				if (Src) Flag_ZF = 0;
				else Flag_ZF = 1;
				Flag_SF = (Src >> 15) & 1;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "INC BP = " << (int)Src;
				break;
			case 6:
				Flag_AF = (((Source_Index & 0x0F) + 1) >> 4) & 1;
				Flag_OF = (((Source_Index & 0x7FFF) + 1) >> 15) & 1;
				Source_Index++;
				Src = Source_Index;
				if (Src) Flag_ZF = 0;
				else Flag_ZF = 1;
				Flag_SF = (Src >> 15) & 1;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "INC SI = " << (int)Src;
				break;
			case 7:
				Flag_AF = (((Destination_Index & 0x0F) + 1) >> 4) & 1;
				Flag_OF = (((Destination_Index & 0x7FFF) + 1) >> 15) & 1;
				Destination_Index++;
				Src = Destination_Index;
				if (Src) Flag_ZF = 0;
				else Flag_ZF = 1;
				Flag_SF = (Src >> 15) & 1;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "INC DI = " << (int)Src;
				break;
			}
		}
		Instruction_Pointer += 2 + additional_IPs;
		break;

	case 1:  //DEC R/M 16bit

		//находим цель инкремента
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
			Flag_AF = (((Src & 0x0F) - 1) >> 4) & 1;
			Flag_OF = (((Src & 0x7FFF) - 1) >> 15) & 1;
			Src--;
			memory.write_2(New_Addr,  Src & 255);
			memory.write_2(New_Addr + 1,  Src >> 8);
			if (Src) Flag_ZF = 0;
			else Flag_ZF = 1;
			Flag_SF = (Src >> 15) & 1;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "DEC M" << OPCODE_comment << " = " << (int)Src;
			break;
		case 3:
			// mod 11 - адрес в регистре
			switch (byte2 & 7)
			{
			case 0:
				Flag_AF = (((AX & 0x0F) - 1) >> 4) & 1;
				Flag_OF = (((AX & 0x7FFF) - 1) >> 15) & 1;
				AX--;
				Src = AX;
				if (Src) Flag_ZF = 0;
				else Flag_ZF = 1;
				Flag_SF = (Src >> 15) & 1;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "DEC AX = " << (int)Src;
				break;
			case 1:
				Flag_AF = (((CX & 0x0F) - 1) >> 4) & 1;
				Flag_OF = (((CX & 0x7FFF) - 1) >> 15) & 1;
				CX--;
				Src = CX;
				if (Src) Flag_ZF = 0;
				else Flag_ZF = 1;
				Flag_SF = (Src >> 15) & 1;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "DEC CX = " << (int)Src;
				break;
			case 2:
				Flag_AF = (((DX & 0x0F) - 1) >> 4) & 1;
				Flag_OF = (((DX & 0x7FFF) - 1) >> 15) & 1;
				DX--;
				Src = DX;
				if (Src) Flag_ZF = 0;
				else Flag_ZF = 1;
				Flag_SF = (Src >> 15) & 1;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "DEC DX = " << (int)Src;
				break;
			case 3:
				Flag_AF = (((BX & 0x0F) - 1) >> 4) & 1;
				Flag_OF = (((BX & 0x7FFF) - 1) >> 15) & 1;
				BX--;
				Src = BX;
				if (Src) Flag_ZF = 0;
				else Flag_ZF = 1;
				Flag_SF = (Src >> 15) & 1;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "DEC BX = " << (int)Src;
				break;
			case 4:
				Flag_AF = (((Stack_Pointer & 0x0F) - 1) >> 4) & 1;
				Flag_OF = (((Stack_Pointer & 0x7FFF) - 1) >> 15) & 1;
				Stack_Pointer--;
				Src = Stack_Pointer;
				if (Src) Flag_ZF = 0;
				else Flag_ZF = 1;
				Flag_SF = (Src >> 15) & 1;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "DEC SP = " << (int)Src;
				break;
			case 5:
				Flag_AF = (((Base_Pointer & 0x0F) - 1) >> 4) & 1;
				Flag_OF = (((Base_Pointer & 0x7FFF) - 1) >> 15) & 1;
				Base_Pointer--;
				Src = Base_Pointer;
				if (Src) Flag_ZF = 0;
				else Flag_ZF = 1;
				Flag_SF = (Src >> 15) & 1;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "DEC BP = " << (int)Src;
				break;
			case 6:
				Flag_AF = (((Source_Index & 0x0F) - 1) >> 4) & 1;
				Flag_OF = (((Source_Index & 0x7FFF) - 1) >> 15) & 1;
				Source_Index--;
				Src = Source_Index;
				if (Src) Flag_ZF = 0;
				else Flag_ZF = 1;
				Flag_SF = (Src >> 15) & 1;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "DEC SI = " << (int)Src;
				break;
			case 7:
				Flag_AF = (((Destination_Index & 0x0F) - 1) >> 4) & 1;
				Flag_OF = (((Destination_Index & 0x7FFF) - 1) >> 15) & 1;
				Destination_Index--;
				Src = Destination_Index;
				if (Src) Flag_ZF = 0;
				else Flag_ZF = 1;
				Flag_SF = (Src >> 15) & 1;
				Flag_PF = parity_check[Src & 255];
				if (log_to_console) cout << "DEC DI = " << (int)Src;
				break;
			}
		}
		Instruction_Pointer += 2 + additional_IPs;
		break;

	case 2:  //Indirect Call within Segment

		//рассчитываем адрес где находится адрес перехода
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			new_IP = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
			break;
		case 3:
			// mod 11 - адрес в регистре
			switch (byte2 & 7)
			{
			case 0:
				new_IP = AX;
				break;
			case 1:
				new_IP = CX;
				break;
			case 2:
				new_IP = DX;
				break;
			case 3:
				new_IP = BX;
				break;
			case 4:
				new_IP = Stack_Pointer;
				break;
			case 5:
				new_IP = Base_Pointer;
				break;
			case 6:
				new_IP = Source_Index;
				break;
			case 7:
				new_IP = Destination_Index;
				break;
			}
		}
		old_IP = Instruction_Pointer;
		stack_addr = SS_data * 16 + Stack_Pointer - 1;
		Stack_Pointer--;
		memory.write_2(SS_data * 16 + Stack_Pointer, (Instruction_Pointer + 2 + additional_IPs) >> 8);
		Stack_Pointer--;
		memory.write_2(SS_data * 16 + Stack_Pointer, (Instruction_Pointer + 2 + additional_IPs) & 255);
		
		Instruction_Pointer = new_IP;
		SetConsoleTextAttribute(hConsole, 13);
		if (log_to_console) cout << "Near Indirect Call to " << (int)*CS << ":" << (int)Instruction_Pointer << " (ret to " << (int)*CS << ":" << (int)(old_IP + 2 + additional_IPs) << ")";
		//else cout << "Near Indirect Call to " << (int)*CS << ":" << (int)Instruction_Pointer << " (ret to " << (int)*CS << ":" << (int)(old_IP + 2 + additional_IPs) << ")";
		SetConsoleTextAttribute(hConsole, 7);
		break;

	case 3:  //Indirect Intersegment Call
			//рассчитываем адрес, где хранится адрес перехода (IP + CS)
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			break;
		case 3:
			// mod 11 - адрес в регистре
			switch (byte2 & 7)
			{
			case 0:
				New_Addr = AX;
				break;
			case 1:
				New_Addr = CX;
				break;
			case 2:
				New_Addr = DX;
				break;
			case 3:
				New_Addr = BX;
				break;
			case 4:
				New_Addr = Stack_Pointer;
				break;
			case 5:
				New_Addr = Base_Pointer;
				break;
			case 6:
				New_Addr = Source_Index;
				break;
			case 7:
				New_Addr = Destination_Index;
				break;
			}
		}
		Stack_Pointer--;
		memory.write_2(SS_data * 16 + Stack_Pointer, CS_data >> 8);
		Stack_Pointer--;
		memory.write_2(SS_data * 16 + Stack_Pointer, CS_data & 255);
		Stack_Pointer--;
		memory.write_2(SS_data * 16 + Stack_Pointer, (Instruction_Pointer + 2 + additional_IPs)  >> 8);
		Stack_Pointer--;
		memory.write_2(SS_data * 16 + Stack_Pointer, (Instruction_Pointer + 2 + additional_IPs) & 255);
		
		Instruction_Pointer = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
		*CS = memory.read_2(New_Addr + 2) + memory.read_2(New_Addr + 3) * 256;
		SetConsoleTextAttribute(hConsole, 13);
		if (log_to_console) cout << "Indirect Intersegment Call to " << (int)*CS << ":" << (int)Instruction_Pointer;
		if (log_to_console) cout << " DEBUG addr = " << (int)New_Addr;
		//else cout << "Indirect Intersegment Call to " << (int)*CS << ":" << (int)Instruction_Pointer << endl;
		SetConsoleTextAttribute(hConsole, 7);
		break;

	case 4:  //Indirect Jump within Segment  (mod 100 rm)
			//рассчитываем новый адрес в текущем сегменте
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Instruction_Pointer = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
			break;
		case 3:
			// mod 11 - адрес в регистре
			switch (byte2 & 7)
			{
			case 0:
				Instruction_Pointer = AX;
				break;
			case 1:
				Instruction_Pointer = CX;
				break;
			case 2:
				Instruction_Pointer = DX;
				break;
			case 3:
				Instruction_Pointer = BX;
				break;
			case 4:
				Instruction_Pointer = Stack_Pointer;
				break;
			case 5:
				Instruction_Pointer = Base_Pointer;
				break;
			case 6:
				Instruction_Pointer = Source_Index;
				break;
			case 7:
				Instruction_Pointer = Destination_Index;
				break;
			}
		}
		
		SetConsoleTextAttribute(hConsole, 13);
		if (log_to_console) cout << "Indirect Jump within Segment to " << (int)*CS << ":" << (int)Instruction_Pointer;
		//if (log_to_console) cout << " DEBUG ADDR = " << (int)New_Addr;
		SetConsoleTextAttribute(hConsole, 7);
		break;

	case 5:  //Indirect Intersegment Jump
			//рассчитываем адрес, где хранится адрес перехода (IP + CS)
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			break;
		case 3:
			// mod 11 - адрес в регистре
			switch (byte2 & 7)
			{
			case 0:
				New_Addr = AX;
				break;
			case 1:
				New_Addr = CX;
				break;
			case 2:
				New_Addr = DX;
				break;
			case 3:
				New_Addr = BX;
				break;
			case 4:
				New_Addr = Stack_Pointer;
				break;
			case 5:
				New_Addr = Base_Pointer;
				break;
			case 6:
				New_Addr = Source_Index;
				break;
			case 7:
				New_Addr = Destination_Index;
				break;
			}
		}

		Instruction_Pointer = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
		*CS = memory.read_2(New_Addr + 2) + memory.read_2(New_Addr + 3) * 256;
		SetConsoleTextAttribute(hConsole, 13);
		if (log_to_console) cout << "Indirect Intersegment Jump to " << (int)*CS << ":" << (int)Instruction_Pointer;
		SetConsoleTextAttribute(hConsole, 7);
		break;

	case 6:  //PUSH Register/Memory 16bit
	case 7:  //ALIAS
			 //определяем источник данных
		switch (byte2 >> 6)
		{
		case 0:
		case 1:
		case 2:
			New_Addr = mod_RM_2(byte2);
			Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
			if (log_to_console) cout << "PUSH M" << OPCODE_comment;
			break;
		case 3:
			// mod 11 - адрес в регистре
			switch (byte2 & 7)
			{
			case 0:
				Src = AX;
				if (log_to_console) cout << "PUSH AX("<<(int)Src<<")";
				break;
			case 1:
				Src = CX;
				if (log_to_console) cout << "PUSH CX(" << (int)Src << ")";
				break;
			case 2:
				Src = DX;
				if (log_to_console) cout << "PUSH DX(" << (int)Src << ")";
				break;
			case 3:
				Src = BX;
				if (log_to_console) cout << "PUSH BX(" << (int)Src << ")";
				break;
			case 4:
				Src = Stack_Pointer - 2;
				if (log_to_console) cout << "PUSH SP(" << (int)Src << ")";
				break;
			case 5:
				Src = Base_Pointer;
				if (log_to_console) cout << "PUSH BP(" << (int)Src << ")";
				break;
			case 6:
				Src = Source_Index;
				if (log_to_console) cout << "PUSH SI(" << (int)Src << ")";
				break;
			case 7:
				Src = Destination_Index;
				if (log_to_console) cout << "PUSH DI(" << (int)Src << ")";
				break;
			}
		}
		//пушим число
		Stack_Pointer--;
		memory.write_2(SS_data * 16 + Stack_Pointer, Src >> 8);
		Stack_Pointer--;
		memory.write_2(SS_data * 16 + Stack_Pointer, Src & 255);
		Instruction_Pointer += 2 + additional_IPs;
		break;
	}
}
void Call_dir_interseg()		//Direct Intersegment Call
{
	Stack_Pointer--;
	memory.write_2(SS_data * 16 + Stack_Pointer, *CS >> 8);
	Stack_Pointer--;
	memory.write_2(SS_data * 16 + Stack_Pointer, (*CS) & 255);
	Stack_Pointer--;
	memory.write_2(SS_data * 16 + Stack_Pointer, (Instruction_Pointer + 5) >> 8);
	Stack_Pointer--;
	memory.write_2(SS_data * 16 + Stack_Pointer, (Instruction_Pointer + 5) & 255);
	Instruction_Pointer++;
	uint16 new_IP = memory.read_2(Instruction_Pointer + *CS * 16);
	Instruction_Pointer++;
	new_IP += memory.read_2(Instruction_Pointer + *CS * 16) * 256;
	Instruction_Pointer++;
	uint16 new_CS = memory.read_2(Instruction_Pointer + *CS * 16);
	Instruction_Pointer++;
	new_CS += memory.read_2(Instruction_Pointer + *CS * 16) * 256;
	Instruction_Pointer = new_IP;
	*CS = new_CS;
	SetConsoleTextAttribute(hConsole, 13);
	if (log_to_console) cout << "Direct Intersegment Call to " << (int)*CS << ":" << (int)Instruction_Pointer;
	//else cout << "Direct Intersegment Call to " << (int)*CS << ":" << (int)Instruction_Pointer << endl;
	SetConsoleTextAttribute(hConsole, 7);
}
void Jump_Near_8()				//Direct jump within Segment-Short
{
	Instruction_Pointer += DispCalc8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]) + 2;
	SetConsoleTextAttribute(hConsole, 15);
	if (log_to_console) cout << "Direct jump(8) within Segment-Short to " << (int)*CS << ":" << (int)Instruction_Pointer;
	SetConsoleTextAttribute(hConsole, 7);
}
void Jump_Near_16()				//Direct jump within Segment-Short
{
	Instruction_Pointer += DispCalc16(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory.read_2(Instruction_Pointer + 2 + *CS * 16) * 256) + 3;
	SetConsoleTextAttribute(hConsole, 15);
	if (log_to_console) cout << "Direct jump(16) within Segment-Short to " << (int)*CS << ":" << (int)Instruction_Pointer;
	SetConsoleTextAttribute(hConsole, 7);
}
void Jump_Far()					//Direct Intersegment Jump
{
	uint16 new_IP = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory.read_2(Instruction_Pointer + 2 + *CS * 16) * 256;
	*CS = memory.read_2(Instruction_Pointer + 3 + *CS * 16) + memory.read_2(Instruction_Pointer + 4 + *CS * 16) * 256;
	Instruction_Pointer = new_IP;
	SetConsoleTextAttribute(hConsole, 15);
	if (log_to_console) cout << "Direct Intersegment Jump to " << (int)*CS << ":" << (int)Instruction_Pointer;
	SetConsoleTextAttribute(hConsole, 7);
}

// Conditional Jumps
void JE_JZ()			// JE/JZ = Jump on Equal/Zero
{
	if (Flag_ZF)
	{
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]) + 2;
		SetConsoleTextAttribute(hConsole, 15);
		if (log_to_console) cout << "Jump on Equal / Zero (JE/JZ) to " << (int)*CS << ":" << (int)Instruction_Pointer;
		SetConsoleTextAttribute(hConsole, 7);
	}
	else
	{
		Instruction_Pointer += 2;
		SetConsoleTextAttribute(hConsole, 15);
		if (log_to_console) cout << "NOT Jump on Equal / Zero (JE/JZ)";
		SetConsoleTextAttribute(hConsole, 7);
	}
}
void JL_JNGE()			// JL/JNGE = Jump on Less/Not Greater, or Equal
{
	if (Flag_SF ^ Flag_OF)
	{
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]) + 2;
		SetConsoleTextAttribute(hConsole, 15);
		if (log_to_console) cout << "Jump on Less/Not Greater, or Equal (JL/JNGE) to " << (int)*CS << ":" << (int)Instruction_Pointer;
		SetConsoleTextAttribute(hConsole, 7);
	}
	else
	{
		Instruction_Pointer += 2;
		SetConsoleTextAttribute(hConsole, 15);
		if (log_to_console) cout << "NOT Jump on Less/Not Greater, or Equal (JL/JNGE)";
		SetConsoleTextAttribute(hConsole, 7);
	}
}
void JLE_JNG()			// JLE/JNG = Jump on Less, or Equal/Not Greater
{
	if ((Flag_SF ^ Flag_OF) | Flag_ZF)
	{
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]) + 2;
		if (log_to_console) cout << "Jump on Less, or Equal/Not Greater (JLE/JNG) to " << (int)*CS << ":" << (int)Instruction_Pointer;
	}
	else
	{
		Instruction_Pointer += 2;
		if (log_to_console) cout << "NOT Jump on Less, or Equal/Not Greater (JLE/JNG)";
	}
}
void JB_JNAE()			// JB/JNAE = Jump on Below/Not Above, or Equal
{
	if (Flag_CF)
	{
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]) + 2;
		if (log_to_console) cout << "Jump on Below/Not Above, or Equal (JB/JNAE) to " << (int)*CS << ":" << (int)Instruction_Pointer;
	}
	else
	{
		Instruction_Pointer += 2;
		if (log_to_console) cout << "NOT Jump on Below/Not Above, or Equal (JB/JNAE)";
	}
}
void JBE_JNA()			// JBE/JNA = Jump on Below, or Equal/Not Above
{
	if ((Flag_ZF | Flag_CF))
	{
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]) + 2;
		if (log_to_console) cout << "Jump on Below, or Equal/Not Above (JBE/JNA) to " << (int)*CS << ":" << (int)Instruction_Pointer;
	}
	else
	{
		Instruction_Pointer += 2;
		if (log_to_console) cout << "NOT Jump on Below, or Equal/Not Above (JBE/JNA)";
	}
}
void JP_JPE()			// JP/JPE = Jump on Parity/Parity Even
{
	if (Flag_PF)
	{
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]) + 2;
		if (log_to_console) cout << "Jump on Parity/Parity Even (JP/JPE) to " << (int)*CS << ":" << (int)Instruction_Pointer;
	}
	else
	{
		Instruction_Pointer += 2;
		if (log_to_console) cout << "NOT Jump on Parity/Parity Even (JP/JPE)";
	}
}
void JO()				// JO = Jump on Overflow
{
	if (Flag_OF)
	{
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]) + 2;
		if (log_to_console) cout << "Jump on Overflow (JO) to " << (int)*CS << ":" << (int)Instruction_Pointer;
	}
	else
	{
		Instruction_Pointer += 2;
		if (log_to_console) cout << "NOT Jump on Overflow (JO)";
	}
}
void JS()				// JS = Jump on Sign
{
	if (Flag_SF)
	{
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]) + 2;
		if (log_to_console) cout << "Jump on Sign (JS) to " << (int)*CS << ":" << (int)Instruction_Pointer;
	}
	else
	{
		Instruction_Pointer += 2;
		if (log_to_console) cout << "NOT Jump on Sign (JS)";
	}
}
void JNE_JNZ()			// JNE/JNZ = Jump on Not Equal/Not Zero
{
	if (!Flag_ZF)
	{
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]) + 2;
		if (log_to_console) cout << "Jump on Not Equal/Not Zero (JNE/JNZ) to " << (int)*CS << ":" << (int)Instruction_Pointer;
	}
	else
	{
		Instruction_Pointer += 2;
		if (log_to_console) cout << "NOT Jump on Not Equal/Not Zero (JNE/JNZ)";
	}
}
void JNL_JGE()			// JNL/JGE = Jump on Not Less/Greater, or Equal
{
	if (!(Flag_SF ^ Flag_OF))
	{
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]) + 2;
		if (log_to_console) cout << "Jump on Not Less/Greater, or Equal (JNL/JGE) to " << (int)*CS << ":" << (int)Instruction_Pointer;
	}
	else
	{
		Instruction_Pointer += 2;
		if (log_to_console) cout << "NOT Jump on Not Less/Greater, or Equal (JNL/JGE)";
	}
}
void JNLE_JG()			// JNLE/JG = Jump on Not Less, or Equal/Greater
{
	if (Flag_ZF || (Flag_SF ^ Flag_OF))
	{
		Instruction_Pointer += 2;
		if (log_to_console) cout << "NOT Jump on Not Less, or Equal/Greater (JNLE/JG)";
	}
	else
	{
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]) + 2;
		if (log_to_console) cout << "Jump on Not Less, or Equal/Greater (JNLE/JG) to " << (int)*CS << ":" << (int)Instruction_Pointer;
	}
}
void JNB_JAE()			// JNB/JAE = Jump on Not Below/Above, or Equal
{
	if (!Flag_CF)
	{
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]) + 2;
		if (log_to_console) cout << "Jump on Not Below/Above, or Equal (JNB/JAE) to " << (int)*CS << ":" << (int)Instruction_Pointer;
	}
	else
	{
		Instruction_Pointer += 2;
		if (log_to_console) cout << "NOT Jump on Not Below/Above, or Equal (JNB/JAE)";
	}
}
void JNBE_JA()			// JNBE/JA = Jump on Not Below, or Equal/Above
{
	if (!(Flag_CF | Flag_ZF))
	{
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]) + 2;
		if (log_to_console) cout << "Jump on Not Below, or Equal/Above (JNBE/JA) to " << (int)*CS << ":" << (int)Instruction_Pointer;
	}
	else
	{
		Instruction_Pointer += 2;
		if (log_to_console) cout << "NOT Jump on Not Below, or Equal/Above (JNBE/JA)";
	}
}
void JNP_JPO()			// JNP/JPO = Jump on Not Parity/Parity Odd
{
	if (!Flag_PF)
	{
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]) + 2;
		if (log_to_console) cout << "Jump on Not Parity/Parity Odd (JNP/JPO) to " << (int)*CS << ":" << (int)Instruction_Pointer;
	}
	else
	{
		Instruction_Pointer += 2;
		if (log_to_console) cout << "NOT Jump on Not Parity/Parity Odd (JNP/JPO)";
	}
}
void JNO()				// JNO = Jump on Not Overflow
{
	if (!Flag_OF)
	{
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]) + 2;
		if (log_to_console) cout << "Jump on Not Overflow (JNO) to " << (int)*CS << ":" << (int)Instruction_Pointer;
	}
	else
	{
		Instruction_Pointer += 2;
		if (log_to_console) cout << "NOT Jump on Not Overflow (JNO)";
	}
}
void JNS()				// JNS = Jump on Not Sign
{
	if (!Flag_SF)
	{
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]) + 2;
		if (log_to_console) cout << "Jump on Not Sign (JNS) to " << (int)*CS << ":" << (int)Instruction_Pointer;
	}
	else
	{
		Instruction_Pointer += 2;
		if (log_to_console) cout << "NOT Jump on Not Sign (JNS)";
	}
}
void LOOP()			// LOOP = Loop CX Times
{
	CX--;
	if (CX)
	{
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]) + 2;
		if (log_to_console) cout << "Continue loop at " << Instruction_Pointer << " CX = " << (int)CX;
	}
	else
	{
		Instruction_Pointer += 2;
		if (log_to_console) cout << "Loop finished";
	}
}
void LOOPZ_LOOPE()		// LOOPZ/LOOPE = Loop while Zero/Equal
{
	CX--;
	if (CX && Flag_ZF)
	{
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]) + 2;
		if (log_to_console) cout << "Continue loop CX = " << (int)CX << " ZF = " << Flag_ZF;
	}
	else
	{
		Instruction_Pointer += 2;
		if (log_to_console) cout << "LoopZ finished";
	}
}
void LOOPNZ_LOOPNE()	// LOOPNZ/LOOPNE = Loop while Not Zero/Not Equal
{
	CX--;
	if (CX && !Flag_ZF)
	{
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]) + 2;
		if (log_to_console) cout << "Continue loop CX = " << (int)CX << " ZF = " << Flag_ZF;
	}
	else
	{
		Instruction_Pointer += 2;
		if (log_to_console) cout << "LoopNZ finished";
	}
}
void JCXZ()			// JCXZ = Jump on CX Zero
{
	if (!CX)
	{
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]) + 2;
		if (log_to_console) cout << "Jump on CX Zero to " << (int)*CS << ":" << (int)Instruction_Pointer;
	}
	else
	{
		Instruction_Pointer += 2;
		if (log_to_console) cout << "NOT Jump on CX Zero";
	}
}

//RET

void RET_Segment()					//Return Within Segment
{
	uint32 stack_addr = SS_data * 16 + Stack_Pointer;
	Instruction_Pointer = memory.read_2(stack_addr) + memory.read_2(stack_addr + 1) * 256;
	Stack_Pointer += 2;
	SetConsoleTextAttribute(hConsole, 13);
	if (log_to_console) cout << "Near RET to " << (int)*CS << ":" << (int)Instruction_Pointer;
	//else cout << "Near RET to " << (int)*CS << ":" << (int)Instruction_Pointer << endl;
	SetConsoleTextAttribute(hConsole, 7);
}
void RET_Segment_IMM_SP()			//Return Within Segment Adding Immediate to SP
{
	uint16 pop_bytes = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory.read_2(Instruction_Pointer + 2 + *CS * 16) * 256;
	uint32 stack_addr = SS_data * 16 + Stack_Pointer;
	Instruction_Pointer = memory.read_2(stack_addr) + memory.read_2(stack_addr + 1) * 256;
	Stack_Pointer += 2 + pop_bytes;
	SetConsoleTextAttribute(hConsole, 13);
	if (log_to_console) cout << "Near RET to " << (int)*CS << ":" << (int)Instruction_Pointer << " + " << pop_bytes << " bytes popped";
	//else  cout << "Near RET to " << (int)*CS << ":" << (int)Instruction_Pointer << " + " << pop_bytes << " bytes popped" << endl;
	SetConsoleTextAttribute(hConsole, 7);
}
void RET_Inter_Segment()			//Return Intersegment
{
	uint32 stack_addr = SS_data * 16 + Stack_Pointer;
	Instruction_Pointer = memory.read_2(stack_addr) + memory.read_2(stack_addr + 1) * 256;
	Stack_Pointer += 2;
	*CS = memory.read_2(Stack_Pointer + SS_data * 16) + memory.read_2(Stack_Pointer + 1 + SS_data * 16) * 256;
	Stack_Pointer += 2;
	SetConsoleTextAttribute(hConsole, 13);
	if (log_to_console) cout << "Far RET to " << (int)*CS << ":" << (int)Instruction_Pointer;
	//else cout << "Far RET to " << (int)*CS << ":" << (int)Instruction_Pointer << endl;
	SetConsoleTextAttribute(hConsole, 7);
}
void RET_Inter_Segment_IMM_SP()		//Return Intersegment Adding Immediate to SP
{
	uint16 pop_bytes = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory.read_2(Instruction_Pointer + 2 + *CS * 16) * 256;
	
	Instruction_Pointer = memory.read_2(Stack_Pointer + SS_data * 16);
	Stack_Pointer ++;
	Instruction_Pointer += memory.read_2(Stack_Pointer + SS_data * 16) * 256;
	Stack_Pointer++;
	
	*CS = memory.read_2(Stack_Pointer + SS_data * 16);
	Stack_Pointer++;
	*CS += memory.read_2(Stack_Pointer + SS_data * 16) * 256;
	Stack_Pointer += 1 + pop_bytes;
	SetConsoleTextAttribute(hConsole, 13);
	if (log_to_console || last_INT == 0x21) cout << "Far RET to " << (int)*CS << ":" << (int)Instruction_Pointer << " + " << pop_bytes << " bytes popped " << " AX(" << (int)AX << ") CF=" << (int)Flag_CF;
	if (!log_to_console && last_INT == 0x21) cout << endl;
	SetConsoleTextAttribute(hConsole, 7);
}

//========Conditional Transfer-===============

void INT_N()			//INT = Interrupt
{
	//перехват системных прерываний
	//syscall(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]);
	//Instruction_Pointer += 2;
	//return;

	uint8 int_type = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];

	if (!Flag_IF && !test_mode)
	{
		if (log_to_console) cout << "INT("<<(int)int_type<<") disabled!";
		Instruction_Pointer += 2;
		return;
	}
	
	last_INT = int_type;

	//определяем новый IP и CS
	uint16 new_IP = memory.read_2(int_type * 4) + memory.read_2(int_type * 4 + 1) * 256;
	uint16 new_CS = memory.read_2(int_type * 4 + 2) + memory.read_2(int_type * 4 + 3) * 256;
	
	//помещаем в стек флаги
	Stack_Pointer--;
	memory.write_2(Stack_Pointer + SS_data * 16, 0xF0 | (Flag_OF * 8) | (Flag_DF * 4) | (Flag_IF * 2) | Flag_TF);
	Stack_Pointer--;
	memory.write_2(Stack_Pointer + SS_data * 16, 0x2 | (Flag_SF * 128) | (Flag_ZF * 64) | (Flag_AF * 16) | (Flag_PF * 4) | (Flag_CF));

	//помещаем в стек сегмент
	Stack_Pointer--;
	memory.write_2(Stack_Pointer + SS_data * 16, *CS >> 8);
	Stack_Pointer--;
	memory.write_2(Stack_Pointer + SS_data * 16, (*CS) & 255);
	
	//помещаем в стек IP
	Stack_Pointer--;
	memory.write_2(Stack_Pointer + SS_data * 16, (Instruction_Pointer + 2) >> 8);
	Stack_Pointer--;
	memory.write_2(Stack_Pointer + SS_data * 16, (Instruction_Pointer + 2) & 255);

	//передаем управление
	Flag_IF = false;//запрет внешних прерываний
	Flag_TF = false;
	*CS = new_CS;
	uint16 old = Instruction_Pointer;
	Instruction_Pointer = new_IP;
	SetConsoleTextAttribute(hConsole, 10);
	//if (int_type != 0x10) cout << "INT " << (int)int_type << " (AX=" << (int)AX << ") -> " << (int)new_CS << ":" << (int)Instruction_Pointer << " call from " << old << endl;
	if (log_to_console) cout << "INT " << (int)int_type << " (AX=" << (int)AX << ") -> " << (int)new_CS << ":" << (int)Instruction_Pointer << " call from " << old;
	if (int_type == 0x13)
	{
		
		//добавляем инфу по вызову
		if (log_to_console_FDD && (AX >> 8) == 2) FDD_monitor.log("INT13 drv=" + int_to_hex(DX & 255,2) + " head=" + int_to_hex(DX >> 8, 2) + " track=" + int_to_hex(CX >> 8, 2) + " sector_beg=" + int_to_hex(CX & 255, 2) + " sector_num=" + int_to_hex(AX & 255,2)
			+ " buff = [" + int_to_hex(*ES,4) + ":" + int_to_hex(BX,4) + "]");
		if (log_to_console && (AX >> 8) == 0) cout << "RESET ";
		if (log_to_console && (AX >> 8) == 1) cout << "STATUS ";
		if (log_to_console && (AX >> 8) == 2) cout << "READ ";
		if (log_to_console && (AX >> 8) == 3) cout << "WRITE ";
		if (log_to_console && (AX >> 8) == 4) cout << "VERIFY ";
		if (log_to_console && (AX >> 8) == 5) cout << "FORMAT ";
		if (log_to_console) cout << "  drv=" << (int)(DX & 255) << " head=" << (int)(DX >> 8) << " track=" << (int)(CX >> 8) << " sector_beg=" << (int)(CX & 255) << " sector_num=" << (int)(AX & 255)
			<< " buff = [" << (int)*ES << ":" << (int)BX << "]";
	}
	else
	{
		if (int_type == 0x21)
		{
			cout << "INT 21H from [" << (int)*CS << ":" << (int)Instruction_Pointer << "]" << " (AX=" << (int)AX << ") ";
			if (*ptr_AH == 0x40) SetConsoleTextAttribute(hConsole, 14);
			cout << get_int21_data();
			if (!log_to_console) cout << endl;

		}
		
		//if (int_type == 0x10) cout << "INT " << (int)int_type << " from [" << (int)*CS << ":" << (int)Instruction_Pointer << "]" << " (AX=" << (int)AX << ")" << endl;

	}
	SetConsoleTextAttribute(hConsole, 7);
}
void INT_3()			//INT = Interrupt Type 3
{
	if (!Flag_IF && !test_mode)
	{
		if (log_to_console) cout << "INT disabled - return ";
		Instruction_Pointer += 1;
		return;
	}
	uint8 int_type = 3;
	//определяем новый IP и CS
	uint16 new_IP = memory.read_2(int_type * 4) + memory.read_2(int_type * 4 + 1) * 256;
	uint16 new_CS = memory.read_2(int_type * 4 + 2) + memory.read_2(int_type * 4 + 3) * 256;

	//помещаем в стек флаги
	Stack_Pointer--;
	memory.write_2(Stack_Pointer + SS_data * 16, 0xF0 | (Flag_OF * 8) | (Flag_DF * 4) | (Flag_IF * 2) | Flag_TF);
	Stack_Pointer--;
	memory.write_2(Stack_Pointer + SS_data * 16, 0x2 | (Flag_SF * 128) | (Flag_ZF * 64) | (Flag_AF * 16) | (Flag_PF * 4) | (Flag_CF));
	
	//помещаем в стек сегмент
	Stack_Pointer--;
	memory.write_2(Stack_Pointer + SS_data * 16, *CS >> 8);
	Stack_Pointer--;
	memory.write_2(Stack_Pointer + SS_data * 16, (*CS) & 255);
	
	//помещаем в стек IP
	Stack_Pointer--;
	memory.write_2(Stack_Pointer + SS_data * 16, (Instruction_Pointer + 1) >> 8);
	Stack_Pointer--;
	memory.write_2(Stack_Pointer + SS_data * 16, (Instruction_Pointer + 1) & 255);
	
	//передаем управление
	Flag_IF = false;//запрет внешних прерываний
	Flag_TF = false;
	*CS = new_CS;
	
	Instruction_Pointer = new_IP;
	if (log_to_console)cout << "INT " << (int)int_type << " -> " << (int)new_CS << ":" << (int)Instruction_Pointer;
}
void INT_O()			//INTO = Interrupt on Overflow
{
	if (!Flag_IF && !test_mode)
	{
		if (log_to_console) cout << "INT disabled - return ";
		Instruction_Pointer += 1;
		return;
	}
	
	if (Flag_OF)
	{
		//определяем новый IP и CS
		uint16 new_IP = memory.read_2(0x10) + memory.read_2(0x11) * 256;
		uint16 new_CS = memory.read_2(0x12) + memory.read_2(0x13) * 256;

		//помещаем в стек флаги
		Stack_Pointer--;
		memory.write_2(Stack_Pointer + SS_data * 16, 0xF0 | (Flag_OF * 8) | (Flag_DF * 4) | (Flag_IF * 2) | Flag_TF);
		Stack_Pointer--; 
		memory.write_2(Stack_Pointer + SS_data * 16, 0x2 | (Flag_SF * 128) | (Flag_ZF * 64) | (Flag_AF * 16) | (Flag_PF * 4) | (Flag_CF));
		
		//помещаем в стек сегмент
		Stack_Pointer--;
		memory.write_2(Stack_Pointer + SS_data * 16, *CS >> 8);
		Stack_Pointer--;
		memory.write_2(Stack_Pointer + SS_data * 16, (*CS) & 255);
		
		//помещаем в стек IP
		Stack_Pointer--;
		memory.write_2(Stack_Pointer + SS_data * 16, (Instruction_Pointer + 1) >> 8);
		Stack_Pointer--;
		memory.write_2(Stack_Pointer + SS_data * 16, (Instruction_Pointer + 1) & 255);
		
		//передаем управление
		Flag_IF = false;//запрет внешних прерываний
		Flag_TF = false;
		*CS = new_CS;
		Instruction_Pointer = new_IP;
		if (log_to_console) cout << "INT_0 " << " -> " << (int)new_CS << ":" << (int)Instruction_Pointer;
	}
	else
	{
		Instruction_Pointer++;
	}
}
void I_RET()			//Interrupt Return
{
	//POP IP
	Instruction_Pointer = memory.read_2(SS_data * 16 + Stack_Pointer);
	Stack_Pointer ++;
	Instruction_Pointer += memory.read_2(SS_data * 16 + Stack_Pointer) * 256;
	Stack_Pointer ++;

	//POP CS
	*CS = memory.read_2(SS_data * 16 + Stack_Pointer);
	Stack_Pointer++;
	*CS += memory.read_2(SS_data * 16 + Stack_Pointer) * 256;
	Stack_Pointer++;
	
	
	//POP Flags
	int Flags = memory.read_2(SS_data * 16 + Stack_Pointer);
	Stack_Pointer++;
	Flags += memory.read_2(SS_data * 16 + Stack_Pointer) * 256;
	Stack_Pointer++;
	
	Flag_OF = (Flags >> 11) & 1;
	Flag_DF = (Flags >> 10) & 1;
	Flag_IF = (Flags >> 9) & 1;
	Flag_TF = (Flags >> 8) & 1;
	Flag_SF = (Flags >> 7) & 1;
	Flag_ZF = (Flags >> 6) & 1;
	Flag_AF = (Flags >> 4) & 1;
	Flag_PF = (Flags >> 2) & 1;
	Flag_CF = (Flags) & 1;

	SetConsoleTextAttribute(hConsole, 10);
	if (log_to_console || last_INT == 0x21) cout << "I_RET to " << *CS << ":" << Instruction_Pointer << " AX(" << (int)AX << ") CF=" << (int)Flag_CF;
	if (!log_to_console && last_INT == 0x21) cout << endl;
	SetConsoleTextAttribute(hConsole, 7);
}

//==========Processor Control=================
void CLC()					//Clear Carry
{
	Flag_CF = false;
	if (log_to_console) cout << "Clear Carry";
	Instruction_Pointer++;
}
void STC()					//Set Carry
{
	Flag_CF = true;
	if (log_to_console) cout << "Set Carry";
	Instruction_Pointer++;
}
void CMC()					//Complement Carry
{
	Flag_CF = !Flag_CF;
	if (log_to_console) cout << "Complement Carry";
	Instruction_Pointer++;
}
void CLD()					//Clear Direction
{
	Flag_DF = false;
	if (log_to_console) cout << "Clear Direction Flag";
	Instruction_Pointer++;
}
void STD()					//Set Direction
{
	Flag_DF = true;
	if (log_to_console) cout << "Set Direction Flag";
	Instruction_Pointer++;
}
void CLI()					//Clear Interrupt
{
	Flag_IF = false;
	if (log_to_console) cout << "Clear Interrupt Flag (INT disable)";
	Instruction_Pointer++;
}
void STI()					//Set Interrupt
{
	Flag_IF = true;
	if (log_to_console) cout << "Set Interrupt Flag (INT enable)";
	Instruction_Pointer++;
}
void HLT()					//Halt
{
	cont_exec = false;
	if (log_to_console) cout << "Halt!";
	Instruction_Pointer++;
}
void Wait()					//Wait
{
	if (log_to_console) cout << "Wait 8087 execute command (NOP)";
	Instruction_Pointer++;
}
void Lock()					//Bus lock prefix
{
	if (log_to_console) cout << "Lock command";
	Instruction_Pointer++;
}
void Esc_8087()				//Call 8087
{
	//Декодируем инструкцию 8087
	uint8 f_opcode = ((memory.read_2(Instruction_Pointer + *CS * 16) & 7) << 3) | ((memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] >> 3) & 7);
	uint8 rm_code = ((memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] >> 3) & 0b11000) | (memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] & 7);
	uint8 add_IP = 0;

	switch (f_opcode)
	{
	case 0b011100:  //FINIT
		if (log_to_console) cout << "FPU: INIT";
		Instruction_Pointer += 2;
		break;

	case 0b101111:  //FSTSW (Store Status Word)
		if (log_to_console) cout << "FPU: FSTSW";
		if (rm_code == 0b00110) add_IP = 1;
		Instruction_Pointer += 3 + add_IP;
		break;
	
	case 0b011101:  //FLD (Load TEMP to ST0)
		if (log_to_console) cout << "FPU: FSTSW";
		if (rm_code == 0b00110) add_IP = 1;
		Instruction_Pointer += 3 + add_IP;
		break;

	case 0b101110:  //FSAVE (Save State)
		if (log_to_console) cout << "FPU: FSAVE";
		if (rm_code == 0b00110) add_IP = 1;
		Instruction_Pointer += 3 + add_IP;
		break;

	default:
		if (log_to_console) cout << "FPU: unknown command";
	}

}

// ================ UNDOC ==============

void CALC()
{
	if (Flag_CF) AX = AX | 0xFF;
	else AX = AX & 0xFF00;
	if (log_to_console) cout << "CALC AL = " << (int)(AX & 255);
	Instruction_Pointer++;
}