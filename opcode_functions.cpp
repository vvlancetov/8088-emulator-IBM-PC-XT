#include "opcode_functions.h"
#include "custom_classes.h"
#include "video.h"
#include "audio.h"
#include <Windows.h>
#include <iostream>
#include <string>
#include <conio.h>
#include <bitset>
#include <sstream>

//#define DEBUG
typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

//Доп функции
template< typename T >
extern std::string int_to_hex(T i, int w);

// переменные из основного файла программы
using namespace std;
extern HANDLE hConsole;

extern bool exeption_0;
extern bool exeption_1;

extern FDD_mon_device FDD_monitor;
extern HDD_mon_device HDD_monitor;

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

extern bool halt_cpu;
extern bool log_to_console_FDD;
extern bool log_to_console_HDD;
extern bool log_to_console_DOS;
extern bool cont_exec;

//временные регистры
extern uint16 temp_ACC_16;
extern uint8 temp_ACC_8;
extern uint16 temp_Addr;

//вспомогательные переменные для рассчета адресов операндов
uint8 additional_IPs;
uint8 byte2;
uint32 New_Addr_32;
uint32 New_Addr_32_2;
uint16 New_Addr_16;
string OPCODE_comment;
bool OF_Carry = false;

uint16 operand_RM_seg;
uint16 operand_RM_offset;

//вспомогательные регистры для расчетов

uint16 Src = 0; //DEL
uint16 Dst = 0; //DEL
uint16* ptr_Src = &Src;
uint8* ptr_Src_L = (uint8*)ptr_Src;
uint8* ptr_Src_H = ptr_Src_L + 1;
uint16* ptr_Dst = &Dst;
uint8* ptr_Dst_L = (uint8*)ptr_Dst;
uint8* ptr_Dst_H = ptr_Dst_L + 1;

//префикс замены сегмента
extern uint8 Flag_segment_override;
// 00 - нет замены
// 01 - ES
// 02 - CS
// 03 - SS
// 04 - DS
extern bool keep_segment_override; //сохранение флага

//флаг аппаратных прерываний
extern bool Flag_hardware_INT;

extern Mem_Ctrl memory;
extern uint8 memory_2[1024 * 1024 + 1024 * 1024]; //память 2.0

extern SoundMaker speaker;
extern IO_Ctrl IO_device;
extern IC8259 int_ctrl;

//extern uint16 Stack_Pointer;

extern uint16 registers[8];

//функция проверки четности
bool parity_check[256] = { 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1 };

extern bool log_to_console;
extern bool step_mode;
extern bool log_to_console_8087;

extern string regnames[8];
extern string pairnames[4];
extern Monitor monitor;

extern void (*op_code_table[256])();
extern void (*op_code_table_8087[64])();
extern void (*backup_table[256])();

extern bool test_mode;
extern bool repeat_test_op;
extern bool negate_IDIV;
int last_INT = 0; //номер последнего прерывания (для отладки)
extern uint8 bus_lock;

//счетчик команд
extern int command_counter[256];
extern bool command_counter_ON;


//=============
__int16 DispCalc16(uint16 data); //декларация
__int8 DispCalc8(uint8 data);
//==============Service functions===============

//инициализация таблицы функций 8087
void opcode_8087_table_init()
{
	op_code_table_8087[0b001000] = &Esc_8087_001_000_Load;  //LOAD int/real to ST0
	op_code_table_8087[0b011000] = &Esc_8087_011_000_Load;	//LOAD int/real to ST0
	op_code_table_8087[0b101000] = &Esc_8087_101_000_Load;	//LOAD int/real to ST0
	op_code_table_8087[0b111000] = &Esc_8087_111_000_Load;	//LOAD int/real to ST0

	op_code_table_8087[0b111101] = &Esc_8087_111_101_Load;  //LOAD long INT to ST0
	op_code_table_8087[0b011101] = &Esc_8087_011_101_Load;	//LOAD temp real to ST0
	op_code_table_8087[0b111100] = &Esc_8087_111_100_Load;	//LOAD BCD to ST0

	op_code_table_8087[0b001010] = &Esc_8087_001_010_Store;	//STORE ST0 to int/real
	op_code_table_8087[0b011010] = &Esc_8087_011_010_Store;	//STORE ST0 to int/real
	op_code_table_8087[0b101010] = &Esc_8087_101_010_Store;	//STORE ST0 to int/real
	op_code_table_8087[0b111010] = &Esc_8087_111_010_Store;	//STORE ST0 to int/real

	op_code_table_8087[0b001011] = &Esc_8087_001_011_StorePop; //FSTP = Store and Pop
	op_code_table_8087[0b011011] = &Esc_8087_011_011_StorePop; //FSTP = Store and Pop
	op_code_table_8087[0b101011] = &Esc_8087_101_011_StorePop; //FSTP = Store and Pop + ST0 to STi 
	op_code_table_8087[0b111011] = &Esc_8087_111_011_StorePop; //FSTP = Store and Pop

	op_code_table_8087[0b111111] = &Esc_8087_111_111_StorePop; //StorePop ST0 Long Int to MEM
	op_code_table_8087[0b011111] = &Esc_8087_011_111_StorePop; //StorePop ST0 to TMP real mem
	op_code_table_8087[0b111110] = &Esc_8087_111_110_StorePop; //StorePop ST0 to BCD mem

	op_code_table_8087[0b001001] = &Esc_8087_001_001_FXCH;		//EXchange ST0 - STi

	op_code_table_8087[0b000010] = &Esc_8087_000_010_FCOM;		//FCOM = Compare + STi to ST0
	op_code_table_8087[0b010010] = &Esc_8087_010_010_FCOM;		//FCOM = Compare
	op_code_table_8087[0b100010] = &Esc_8087_100_010_FCOM;		//FCOM = Compare
	op_code_table_8087[0b110010] = &Esc_8087_110_010_FCOM;		//FCOM = Compare

	op_code_table_8087[0b000011] = &Esc_8087_000_011_FCOM;		//FcomPop + STi to ST0
	op_code_table_8087[0b010011] = &Esc_8087_010_011_FCOM;		//FcomPop
	op_code_table_8087[0b100011] = &Esc_8087_100_011_FCOM;		//FcomPop
	op_code_table_8087[0b110011] = &Esc_8087_110_011_FCOM;		//FcomPop + FCOMPP

	op_code_table_8087[0b001100] = &Esc_8087_001_100_TEST;		//FTST/FXAM/FABS

	op_code_table_8087[0b000000] = &Esc_8087_000_000_FADD;		//FADD
	op_code_table_8087[0b010000] = &Esc_8087_010_000_FADD;		//FADD
	op_code_table_8087[0b100000] = &Esc_8087_100_000_FADD;		//FADD
	op_code_table_8087[0b110000] = &Esc_8087_110_000_FADD;		//FADD

	op_code_table_8087[0b000100] = &Esc_8087_000_100_FSUB;		//FSUB R = 0
	op_code_table_8087[0b000101] = &Esc_8087_000_101_FSUB;		//FSUB R = 1
	op_code_table_8087[0b010100] = &Esc_8087_010_100_FSUB;		//FSUB R = 0
	op_code_table_8087[0b010101] = &Esc_8087_010_101_FSUB;		//FSUB R = 1
	op_code_table_8087[0b100100] = &Esc_8087_100_100_FSUB;		//FSUB R = 0
	op_code_table_8087[0b100101] = &Esc_8087_100_101_FSUB;		//FSUB R = 1
	op_code_table_8087[0b110100] = &Esc_8087_110_100_FSUB;		//FSUB R = 0
	op_code_table_8087[0b110101] = &Esc_8087_110_101_FSUB;		//FSUB R = 1

	op_code_table_8087[0b000001] = &Esc_8087_000_001_FMUL;		//FMUL
	op_code_table_8087[0b010001] = &Esc_8087_010_001_FMUL;		//FMUL
	op_code_table_8087[0b100001] = &Esc_8087_100_001_FMUL;		//FMUL
	op_code_table_8087[0b110001] = &Esc_8087_110_001_FMUL;		//FMUL

	op_code_table_8087[0b000110] = &Esc_8087_000_110_FDIV;		//FDIV R = 0
	op_code_table_8087[0b000111] = &Esc_8087_000_111_FDIV;		//FDIV R = 1
	op_code_table_8087[0b010110] = &Esc_8087_010_110_FDIV;		//FDIV R = 0
	op_code_table_8087[0b010111] = &Esc_8087_010_111_FDIV;		//FDIV R = 1
	op_code_table_8087[0b100110] = &Esc_8087_100_110_FDIV;		//FDIV R = 0
	op_code_table_8087[0b100111] = &Esc_8087_100_111_FDIV;		//FDIV R = 1
	op_code_table_8087[0b110110] = &Esc_8087_110_110_FDIV;		//FDIV R = 0
	op_code_table_8087[0b110111] = &Esc_8087_110_111_FDIV;		//FDIV R = 1

	op_code_table_8087[0b001111] = &Esc_8087_001_111_FSQRT;		//FSQRT/FSCALE/FPREM/FRNDINIT
	op_code_table_8087[0b001110] = &Esc_8087_001_110_FXTRACT;	//FXTRACT

	op_code_table_8087[0b001101] = &Esc_8087_001_101_FLDZ;		//FLDZ/FLD1/FLOPI/FLOL2T
	
	op_code_table_8087[0b011100] = &Esc_8087_011_100_FINIT;		//FINIT/FENI/FDISI

	op_code_table_8087[0b101111] = &Esc_8087_101_111_FSTSW;		//FSTSW

	op_code_table_8087[0b101110] = &Esc_8087_101_110_FSAVE;		//FSAVE
	op_code_table_8087[0b101100] = &Esc_8087_101_100_FRSTOR;	//FRSTOR

	op_code_table_8087[0b011001] = &op_code_8087_unknown;
	op_code_table_8087[0b011110] = &op_code_8087_unknown;
	op_code_table_8087[0b101001] = &op_code_8087_unknown;
	op_code_table_8087[0b101101] = &op_code_8087_unknown;
	op_code_table_8087[0b111001] = &op_code_8087_unknown;
}

//инициализация таблицы функций 8086
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
	op_code_table[0b00010000] = &ADC_R_to_RM_8;		// ADC R/M -> R/M 8bit
	op_code_table[0b00010001] = &ADC_R_to_RM_16;		// ADC R/M -> R/M 16bit
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
	op_code_table[0b11110001] = &Lock;					//Bus lock prefix (Undoc)
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
		Flag_segment_override = 1;
		//DS = &ES_data;
		//SS = &ES_data;
		break;
	case 1:
#ifdef DEBUG
		if (log_to_console) cout << "CS(" << *CS << ")";
#endif
		Flag_segment_override = 2;
		//DS = &CS_data;
		//SS = &CS_data;
		break;
	case 2:
#ifdef DEBUG
		if (log_to_console) cout << "SS(" << *SS << ")";
#endif
		Flag_segment_override = 3;
		//DS = &SS_data;
		break;
	case 3:
#ifdef DEBUG
		if (log_to_console) cout << "DS(" << *DS << ")";
#endif
		Flag_segment_override = 4;
		//SS = &DS_data;
		break;
	}
	keep_segment_override = true; //сохранить флаг до следующей команды
#ifdef DEBUG
	if (log_to_console) cout << endl;
#endif
	Instruction_Pointer++;

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
void mod_RM_3_old(uint8 byte2)		//расчет адреса операнда по биту 2
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
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(*SS, 4) + ":" + int_to_hex((Base_Pointer + d16) & 0xFFFF, 4) + "]";
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
void mod_RM_3(uint8 byte2)		//расчет адреса операнда по биту 2
{
	if (bus_lock == 1)
	{
		cout << "Bus lock exeption (RM)" << endl;
		bus_lock = 0;		//сбрасываем флаг блокировки шины
		//exeption = 0x14;	//бросаем исключение
	}
	if ((byte2 >> 6) == 0) //no displacement
	{
		additional_IPs = 0;
		switch (byte2 & 7)
		{
		case 0:
			switch (Flag_segment_override)
			{
			case 0:
				//ничего не меняем
				operand_RM_seg = *DS;
				break;
			case 1:
				//меняем на ES
				operand_RM_seg = *ES;
				break;
			case 2:
				//меняем на CS
				operand_RM_seg = *CS;
				break;
			case 3:
				//меняем на SS
				operand_RM_seg = *SS;
				break;
			case 4:
				//ничего не меняем (уже DS)
				operand_RM_seg = *DS;
				break;
			}
			operand_RM_offset = BX + Source_Index;
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(operand_RM_seg, 4) + ":" + int_to_hex(operand_RM_offset & 0xFFFF, 4) + "]";
			break;
		case 1:
			switch (Flag_segment_override)
			{
			case 0:
				//ничего не меняем
				operand_RM_seg = *DS;
				break;
			case 1:
				//меняем на ES
				operand_RM_seg = *ES;
				break;
			case 2:
				//меняем на CS
				operand_RM_seg = *CS;
				break;
			case 3:
				//меняем на SS
				operand_RM_seg = *SS;
				break;
			case 4:
				//ничего не меняем (уже DS)
				operand_RM_seg = *DS;
				break;
			}
			operand_RM_offset = BX + Destination_Index;
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(operand_RM_seg, 4) + ":" + int_to_hex(operand_RM_offset & 0xFFFF, 4) + "]";
			break;
		case 2:
			switch (Flag_segment_override)
			{
			case 0:
				//ничего не меняем
				operand_RM_seg = *SS;
				break;
			case 1:
				//меняем на ES
				operand_RM_seg = *ES;
				break;
			case 2:
				//меняем на CS
				operand_RM_seg = *CS;
				break;
			case 3:
				//ничего не меняем (уже SS)
				operand_RM_seg = *SS;
				break;
			case 4:
				//меняем на DS
				operand_RM_seg = *DS;
				break;
			}
			operand_RM_offset = Base_Pointer + Source_Index;
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(operand_RM_seg, 4) + ":" + int_to_hex((operand_RM_offset) & 0xFFFF, 4) + "]";
			break;
		case 3:
			switch (Flag_segment_override)
			{
			case 0:
				//ничего не меняем
				operand_RM_seg = *SS;
				break;
			case 1:
				//меняем на ES
				operand_RM_seg = *ES;
				break;
			case 2:
				//меняем на CS
				operand_RM_seg = *CS;
				break;
			case 3:
				//ничего не меняем (уже SS)
				operand_RM_seg = *SS;
				break;
			case 4:
				//меняем на DS
				operand_RM_seg = *DS;
				break;
			}
			operand_RM_offset = Base_Pointer + Destination_Index;
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(operand_RM_seg, 4) + ":" + int_to_hex(operand_RM_offset & 0xFFFF, 4) + "]";
			break;
		case 4:
			switch (Flag_segment_override)
			{
			case 0:
				//ничего не меняем
				operand_RM_seg = *DS;
				break;
			case 1:
				//меняем на ES
				operand_RM_seg = *ES;
				break;
			case 2:
				//меняем на CS
				operand_RM_seg = *CS;
				break;
			case 3:
				//меняем на SS
				operand_RM_seg = *SS;
				break;
			case 4:
				//ничего не меняем (уже DS)
				operand_RM_seg = *DS;
				break;
			}
			operand_RM_offset = Source_Index;
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(operand_RM_seg, 4) + ":" + int_to_hex(operand_RM_offset & 0xFFFF, 4) + "]";
			break;
		case 5:
			switch (Flag_segment_override)
			{
			case 0:
				//ничего не меняем
				operand_RM_seg = *DS;
				break;
			case 1:
				//меняем на ES
				operand_RM_seg = *ES;
				break;
			case 2:
				//меняем на CS
				operand_RM_seg = *CS;
				break;
			case 3:
				//меняем на SS
				operand_RM_seg = *SS;
				break;
			case 4:
				//ничего не меняем (уже DS)
				operand_RM_seg = *DS;
				break;
			}
			operand_RM_offset = Destination_Index;
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(operand_RM_seg, 4) + ":" + int_to_hex((operand_RM_offset) & 0xFFFF, 4) + "]";
			break;
		case 6:
			additional_IPs = 2;
			switch (Flag_segment_override)
			{
			case 0:
				//ничего не меняем
				operand_RM_seg = *DS;
				break;
			case 1:
				//меняем на ES
				operand_RM_seg = *ES;
				break;
			case 2:
				//меняем на CS
				operand_RM_seg = *CS;
				break;
			case 3:
				//меняем на SS
				operand_RM_seg = *SS;
				break;
			case 4:
				//ничего не меняем (уже DS)
				operand_RM_seg = *DS;
				break;
			}
			operand_RM_offset = memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF] + memory_2[(Instruction_Pointer + 3 + *CS * 16) & 0xFFFFF] * 256;
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(operand_RM_seg, 4) + ":" + int_to_hex(operand_RM_offset, 4) + "]";
			break;
		case 7:
			switch (Flag_segment_override)
			{
			case 0:
				//ничего не меняем
				operand_RM_seg = *DS;
				break;
			case 1:
				//меняем на ES
				operand_RM_seg = *ES;
				break;
			case 2:
				//меняем на CS
				operand_RM_seg = *CS;
				break;
			case 3:
				//меняем на SS
				operand_RM_seg = *SS;
				break;
			case 4:
				//ничего не меняем (уже DS)
				operand_RM_seg = *DS;
				break;
			}
			operand_RM_offset = BX;
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(operand_RM_seg, 4) + ":" + int_to_hex(operand_RM_offset, 4) + "]";
			break;
		}
		Flag_segment_override = 0; //отменяем смену сегмента

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
			switch (Flag_segment_override)
			{
			case 0:
				//ничего не меняем
				operand_RM_seg = *DS;
				break;
			case 1:
				//меняем на ES
				operand_RM_seg = *ES;
				break;
			case 2:
				//меняем на CS
				operand_RM_seg = *CS;
				break;
			case 3:
				//меняем на SS
				operand_RM_seg = *SS;
				break;
			case 4:
				//ничего не меняем (уже DS)
				operand_RM_seg = *DS;
				break;
			}
			operand_RM_offset = BX + Source_Index + d8;
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(operand_RM_seg, 4) + ":" + int_to_hex(operand_RM_offset & 0xFFFF, 4) + "]";
			break;
		case 1:
			switch (Flag_segment_override)
			{
			case 0:
				//ничего не меняем
				operand_RM_seg = *DS;
				break;
			case 1:
				//меняем на ES
				operand_RM_seg = *ES;
				break;
			case 2:
				//меняем на CS
				operand_RM_seg = *CS;
				break;
			case 3:
				//меняем на SS
				operand_RM_seg = *SS;
				break;
			case 4:
				//ничего не меняем (уже DS)
				operand_RM_seg = *DS;
				break;
			}
			operand_RM_offset = BX + Destination_Index + d8;
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(operand_RM_seg, 4) + ":" + int_to_hex(operand_RM_offset & 0xFFFF, 4) + "]";
			break;
		case 2:
			switch (Flag_segment_override)
			{
			case 0:
				//ничего не меняем
				operand_RM_seg = *SS;
				break;
			case 1:
				//меняем на ES
				operand_RM_seg = *ES;
				break;
			case 2:
				//меняем на CS
				operand_RM_seg = *CS;
				break;
			case 3:
				//ничего не меняем (уже SS)
				operand_RM_seg = *SS;
				break;
			case 4:
				//меняем на DS
				operand_RM_seg = *DS;
				break;
			}
			operand_RM_offset = Base_Pointer + Source_Index + d8;
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(operand_RM_seg, 4) + ":" + int_to_hex(operand_RM_offset & 0xFFFF, 4) + "]";
			break;
		case 3:
			switch (Flag_segment_override)
			{
			case 0:
				//ничего не меняем
				operand_RM_seg = *SS;
				break;
			case 1:
				//меняем на ES
				operand_RM_seg = *ES;
				break;
			case 2:
				//меняем на CS
				operand_RM_seg = *CS;
				break;
			case 3:
				//ничего не меняем (уже SS)
				operand_RM_seg = *SS;
				break;
			case 4:
				//меняем на DS
				operand_RM_seg = *DS;
				break;
			}
			operand_RM_offset = Base_Pointer + Destination_Index + d8;
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(operand_RM_seg, 4) + ":" + int_to_hex(operand_RM_offset & 0xFFFF, 4) + "]";
			break;
		case 4:
			switch (Flag_segment_override)
			{
			case 0:
				//ничего не меняем
				operand_RM_seg = *DS;
				break;
			case 1:
				//меняем на ES
				operand_RM_seg = *ES;
				break;
			case 2:
				//меняем на CS
				operand_RM_seg = *CS;
				break;
			case 3:
				//меняем на SS
				operand_RM_seg = *SS;
				break;
			case 4:
				//ничего не меняем (уже DS)
				operand_RM_seg = *DS;
				break;
			}
			operand_RM_offset = Source_Index + d8;
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(operand_RM_seg, 4) + ":" + int_to_hex(operand_RM_offset & 0xFFFF, 4) + "]";
			break;
		case 5:
			switch (Flag_segment_override)
			{
			case 0:
				//ничего не меняем
				operand_RM_seg = *DS;
				break;
			case 1:
				//меняем на ES
				operand_RM_seg = *ES;
				break;
			case 2:
				//меняем на CS
				operand_RM_seg = *CS;
				break;
			case 3:
				//меняем на SS
				operand_RM_seg = *SS;
				break;
			case 4:
				//ничего не меняем (уже DS)
				operand_RM_seg = *DS;
				break;
			}
			operand_RM_offset = Destination_Index + d8;
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(operand_RM_seg, 4) + ":" + int_to_hex(operand_RM_offset & 0xFFFF, 4) + "]";
			break;
		case 6:
			switch (Flag_segment_override)
			{
			case 0:
				//ничего не меняем
				operand_RM_seg = *SS;
				break;
			case 1:
				//меняем на ES
				operand_RM_seg = *ES;
				break;
			case 2:
				//меняем на CS
				operand_RM_seg = *CS;
				break;
			case 3:
				//ничего не меняем (уже SS)
				operand_RM_seg = *SS;
				break;
			case 4:
				//меняем на DS
				operand_RM_seg = *DS;
				break;
			}
			operand_RM_offset = Base_Pointer + d8;
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(operand_RM_seg, 4) + ":" + int_to_hex(operand_RM_offset & 0xFFFF, 4) + "]";
			break;
		case 7:
			switch (Flag_segment_override)
			{
			case 0:
				//ничего не меняем
				operand_RM_seg = *DS;
				break;
			case 1:
				//меняем на ES
				operand_RM_seg = *ES;
				break;
			case 2:
				//меняем на CS
				operand_RM_seg = *CS;
				break;
			case 3:
				//меняем на SS
				operand_RM_seg = *SS;
				break;
			case 4:
				//ничего не меняем (уже DS)
				operand_RM_seg = *DS;
				break;
			}
			operand_RM_offset = BX + d8;
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(operand_RM_seg, 4) + ":" + int_to_hex(operand_RM_offset & 0xFFFF, 4) + "]";
			break;
		}
		Flag_segment_override = 0; //отменяем смену сегмента
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
			switch (Flag_segment_override)
			{
			case 0:
				//ничего не меняем
				operand_RM_seg = *DS;
				break;
			case 1:
				//меняем на ES
				operand_RM_seg = *ES;
				break;
			case 2:
				//меняем на CS
				operand_RM_seg = *CS;
				break;
			case 3:
				//меняем на SS
				operand_RM_seg = *SS;
				break;
			case 4:
				//ничего не меняем (уже DS)
				operand_RM_seg = *DS;
				break;
			}
			operand_RM_offset = BX + Source_Index + d16;
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(operand_RM_seg, 4) + ":" + int_to_hex(operand_RM_offset & 0xFFFF, 4) + "]";
			break;
		case 1:
			switch (Flag_segment_override)
			{
			case 0:
				//ничего не меняем
				operand_RM_seg = *DS;
				break;
			case 1:
				//меняем на ES
				operand_RM_seg = *ES;
				break;
			case 2:
				//меняем на CS
				operand_RM_seg = *CS;
				break;
			case 3:
				//меняем на SS
				operand_RM_seg = *SS;
				break;
			case 4:
				//ничего не меняем (уже DS)
				operand_RM_seg = *DS;
				break;
			}
			operand_RM_offset = BX + Destination_Index + d16;
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(operand_RM_seg, 4) + ":" + int_to_hex(operand_RM_offset & 0xFFFF, 4) + "]";
			break;
		case 2:
			switch (Flag_segment_override)
			{
			case 0:
				//ничего не меняем
				operand_RM_seg = *SS;
				break;
			case 1:
				//меняем на ES
				operand_RM_seg = *ES;
				break;
			case 2:
				//меняем на CS
				operand_RM_seg = *CS;
				break;
			case 3:
				//ничего не меняем (уже SS)
				operand_RM_seg = *SS;
				break;
			case 4:
				//меняем на DS
				operand_RM_seg = *DS;
				break;
			}
			operand_RM_offset = Base_Pointer + Source_Index + d16;
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(operand_RM_seg, 4) + ":" + int_to_hex(operand_RM_offset & 0xFFFF, 4) + "]";
			break;
		case 3:
			switch (Flag_segment_override)
			{
			case 0:
				//ничего не меняем
				operand_RM_seg = *SS;
				break;
			case 1:
				//меняем на ES
				operand_RM_seg = *ES;
				break;
			case 2:
				//меняем на CS
				operand_RM_seg = *CS;
				break;
			case 3:
				//ничего не меняем (уже SS)
				operand_RM_seg = *SS;
				break;
			case 4:
				//меняем на DS
				operand_RM_seg = *DS;
				break;
			}
			operand_RM_offset = Base_Pointer + Destination_Index + d16;
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(operand_RM_seg, 4) + ":" + int_to_hex(operand_RM_offset & 0xFFFF, 4) + "]";
			break;
		case 4:
			switch (Flag_segment_override)
			{
			case 0:
				//ничего не меняем
				operand_RM_seg = *DS;
				break;
			case 1:
				//меняем на ES
				operand_RM_seg = *ES;
				break;
			case 2:
				//меняем на CS
				operand_RM_seg = *CS;
				break;
			case 3:
				//меняем на SS
				operand_RM_seg = *SS;
				break;
			case 4:
				//ничего не меняем (уже DS)
				operand_RM_seg = *DS;
				break;
			}
			operand_RM_offset = Source_Index + d16;
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(operand_RM_seg, 4) + ":" + int_to_hex(operand_RM_offset & 0xFFFF, 4) + "]";
			break;
		case 5:
			switch (Flag_segment_override)
			{
			case 0:
				//ничего не меняем
				operand_RM_seg = *DS;
				break;
			case 1:
				//меняем на ES
				operand_RM_seg = *ES;
				break;
			case 2:
				//меняем на CS
				operand_RM_seg = *CS;
				break;
			case 3:
				//меняем на SS
				operand_RM_seg = *SS;
				break;
			case 4:
				//ничего не меняем (уже DS)
				operand_RM_seg = *DS;
				break;
			}
			operand_RM_offset = Destination_Index + d16;
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(operand_RM_seg, 4) + ":" + int_to_hex(operand_RM_offset & 0xFFFF, 4) + "]";
			break;
		case 6:
			switch (Flag_segment_override)
			{
			case 0:
				//ничего не меняем
				operand_RM_seg = *SS;
				break;
			case 1:
				//меняем на ES
				operand_RM_seg = *ES;
				break;
			case 2:
				//меняем на CS
				operand_RM_seg = *CS;
				break;
			case 3:
				//ничего не меняем (уже SS)
				operand_RM_seg = *SS;
				break;
			case 4:
				//меняем на DS
				operand_RM_seg = *DS;
				break;
			}
			operand_RM_offset = Base_Pointer + d16;
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(operand_RM_seg, 4) + ":" + int_to_hex(operand_RM_offset & 0xFFFF, 4) + "]";
			break;
		case 7:
			switch (Flag_segment_override)
			{
			case 0:
				//ничего не меняем
				operand_RM_seg = *DS;
				break;
			case 1:
				//меняем на ES
				operand_RM_seg = *ES;
				break;
			case 2:
				//меняем на CS
				operand_RM_seg = *CS;
				break;
			case 3:
				//меняем на SS
				operand_RM_seg = *SS;
				break;
			case 4:
				//ничего не меняем (уже DS)
				operand_RM_seg = *DS;
				break;
			}
			operand_RM_offset = BX + d16;
			if (log_to_console) OPCODE_comment = "[" + int_to_hex(operand_RM_seg, 4) + ":" + int_to_hex(operand_RM_offset & 0xFFFF, 4) + "]";
			break;
		}
		Flag_segment_override = 0; //отменяем смену сегмента
		return;
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
		out = "Select disk #" + to_string(*ptr_DL);
		return out;
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
		return "Get int vector #" + int_to_hex(*ptr_AL,2) + "h = " + int_to_hex(memory_2[*ptr_AL + 3] * 256 + memory_2[*ptr_AL + 2], 4) + ":" + int_to_hex(memory_2[*ptr_AL + 1] * 256 + memory_2[*ptr_AL], 4);
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
		out += "name (" + int_to_hex(DS_data,4) + ":" + int_to_hex(DX, 4) + "):";
		for (int i = 0; i < 12; ++i)
		{
			if (memory_2[(DS_data * 16 + DX + i) & 0xFFFFF]) out += memory_2[(DS_data * 16 + DX + i) & 0xFFFFF];
			else break;
		}
		return out;
	case 62:
		return "Close file using handle #" + int_to_hex(*ptr_BX, 4);
	case 63:
		return "Read file or device using handle";
	case 64:
		out = "Write file or device using handle (" + int_to_hex(BX,4) + ") " + to_string(CX) + " bytes\n";
		out += "Output buffer: ";
		for (int i = 0; i < CX; ++i)
		{
			if (memory_2[(DS_data * 16 + DX + i) & 0xFFFFF]) out += memory_2[(DS_data * 16 + DX + i) & 0xFFFFF];
			//monitor.teletype(memory_2[(DS_data * 16 + DX + i) & 0xFFFFF]);
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
			for (int i = 0; i < 40; ++i)
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
std::string get_int10_data()
{
	string out;
	if (*ptr_AH == 0)
	{
		//(AH)= 00H	SET MODE (AL) CONTAINS MODE VALUE
		step_mode = 0;
		switch (*ptr_AL)
		{
		case 0:
			return "SET MODE -> 40X25 BW TEXT MODE";
		case 1:
			return "SET MODE -> 40X25 COLOR TEXT MODE";
		case 2:
			return "SET MODE -> 80X25 BW TEXT MODE";
		case 3:
			return "SET MODE -> 80X25 COLOR TEXT MODE";
		case 4:
			return "SET MODE -> 320X200 COLOR GFX MODE";
		case 5:
			return "SET MODE -> 320X200 BW GFX MODE";
		case 6:
			return "SET MODE -> 640X200 BW GFX MODE";
		case 7:
			return "SET MODE -> 80X25 MONOCHROME TEXT MODE";
		}
	}

	if (*ptr_AH == 1)
	{
		//(AH) = 01H	SET CURSOR TYPE
		return "SET CURSOR TYPE -> start line = " + int_to_hex((int)*ptr_CH, 2) + "H end line = " + int_to_hex((int)*ptr_CL, 2) + "H";
	}
	
	if (*ptr_AH == 2)
	{
		//(AH) = 02H SET CURSOR POSITION
		return "SET CURSOR POSITION -> row=" + int_to_hex((int)*ptr_DH, 2 ) + "H column=" + int_to_hex((int)*ptr_DL, 2 ) + "H page=" + int_to_hex((int)*ptr_BH, 2) + "H";
	}

	if (*ptr_AH == 3)
	{
		//(AH)= 03H	READ CURSOR POSITION
		return "READ CURSOR POSITION -> on page=" + int_to_hex((int)*ptr_BH, 2) + "H";
	}
	

	//(AH)= 05H	SELECT ACTIVE DISPLAY PAGE
	
	if (*ptr_AH == 6)
	{
		//(AH)= 06H	SCROLL ACTIVE PAGE UP
		if (*ptr_AL == 0) return "SCROLL ACTIVE PAGE UP - > BLANK ENTIRE WINDOW";
		else
		{
			return "SCROLL ACTIVE PAGE UP - > by " + int_to_hex((int)*ptr_AL, 2) + "H lines";
		}
	}

	//(AH)= 07H	SCROLL ACTIVE PAGE DOWN
	
	if (*ptr_AH == 9)
	{
		//(AH)= 09H	WRITE ATTRIBUTE/CHARACTER AT CURRENT CURSOR POSITION
		return "WRITE CHARACTER/ATTRIBUTE AT CURSOR -> char_code=" + int_to_hex((int)*ptr_AL, 2) + "H attribute=" + int_to_hex((int)*ptr_BL, 2) + "H page=" + int_to_hex((int)*ptr_BH, 2) + "H count=" + int_to_hex((int)*ptr_CX, 4) + "H";
	}

	if (*ptr_AH == 0xa)
	{
		//(AH) = 0AH	WRITE CHARACTER ONLY AT CURRENT CURSOR POSITION
		return "WRITE CHARACTER AT CURSOR -> char_code=" + int_to_hex((int)*ptr_AL, 2) + "H page=" + int_to_hex((int)*ptr_BH, 2) + "H count=" + int_to_hex((int)*ptr_CX, 4) + "H";
	}

	if (*ptr_AH == 0xb)
	{
		//(AH)= 0BH	SET COLOR PALETTE
		if (*ptr_BH == 0) { step_mode = 0; return "SET COLOR PALETTE -> SET BG_COLOR(color=" + int_to_hex((int)*ptr_BL, 2) + "h)"; }
		if (*ptr_BH == 1) { step_mode = 0; return "SET COLOR PALETTE -> SET PALETTE(palette=" + int_to_hex((int)*ptr_BL, 2) + "h)"; }
		return "SET COLOR PALETTE -> unknown (BH=" + int_to_hex((int)*ptr_BH, 2) + "H BL=" + int_to_hex((int)*ptr_BL, 2) + "H)";
	}

	if (*ptr_AH == 0xe)
	{
		//(AH)= 0EH	WRITE TELETYPE TO ACTIVE PAGE
		return "TELETYPE TO AP -> char_code=" + int_to_hex((int)*ptr_AL, 2) + "H color=" + int_to_hex((int)*ptr_BL, 2) + "H";
	}

	if (*ptr_AH == 0xf)
	{
		//(AH)= 0FH	CURRENT VIDEO STATE		
		return "GET CURRENT VIDEO STATE";
	}
	
	return "INT 10H unknown function AH=" + int_to_hex((int)*ptr_AH,2) + "H AL = " + int_to_hex((int)*ptr_AL,2) + "H";

}
//============== Data Transfer Group ===========

void mov_R_to_RM_8() //Move 8 bit R->R/M
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
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
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

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
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
		
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
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

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
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
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
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	
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
	New_Addr_32 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF] * 256;
	switch (Flag_segment_override)
	{
	case 0:
		//ничего не меняем
		operand_RM_seg = *DS;
		break;
	case 1:
		//меняем на ES
		operand_RM_seg = *ES;
		break;
	case 2:
		//меняем на CS
		operand_RM_seg = *CS;
		break;
	case 3:
		//меняем на SS
		operand_RM_seg = *SS;
		break;
	case 4:
		//ничего не меняем (уже DS)
		operand_RM_seg = *DS;
		break;
	}
	*ptr_AL = memory_2[(New_Addr_32 + operand_RM_seg * 16) & 0xFFFFF];
	Instruction_Pointer += 3;
	if (log_to_console) cout << "M[" << (int)operand_RM_seg << ":" << (int)New_Addr_32 << "] to AL(" << (int)(AX & 255) << ")";
}
void M_16_to_ACC()		//Memory to Accumulator 16
{
	New_Addr_32 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF] * 256;
	switch (Flag_segment_override)
	{
	case 0:
		//ничего не меняем
		operand_RM_seg = *DS;
		break;
	case 1:
		//меняем на ES
		operand_RM_seg = *ES;
		break;
	case 2:
		//меняем на CS
		operand_RM_seg = *CS;
		break;
	case 3:
		//меняем на SS
		operand_RM_seg = *SS;
		break;
	case 4:
		//ничего не меняем (уже DS)
		operand_RM_seg = *DS;
		break;
	}
	*ptr_AL = memory_2[(New_Addr_32 + operand_RM_seg * 16) & 0xFFFFF];
	*ptr_AH = memory_2[(New_Addr_32 + 1 + operand_RM_seg * 16) & 0xFFFFF];
	Instruction_Pointer += 3;
	if (log_to_console) cout << "M[" << (int)operand_RM_seg << ":" << (int)New_Addr_32 << "] to AX(" << (int)(AX) << ")";
}
void ACC_8_to_M()		//Accumulator to Memory 8
{
	New_Addr_32 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF] * 256;
	switch (Flag_segment_override)
	{
	case 0:
		//ничего не меняем
		operand_RM_seg = *DS;
		break;
	case 1:
		//меняем на ES
		operand_RM_seg = *ES;
		break;
	case 2:
		//меняем на CS
		operand_RM_seg = *CS;
		break;
	case 3:
		//меняем на SS
		operand_RM_seg = *SS;
		break;
	case 4:
		//ничего не меняем (уже DS)
		operand_RM_seg = *DS;
		break;
	}
	memory_2[(New_Addr_32 + operand_RM_seg * 16) & 0xFFFFF] = *ptr_AL;
	Instruction_Pointer += 3;
	if (log_to_console) cout << "AL(" << (int)(AX & 255) << ") to M[" << (int)operand_RM_seg << ":" << (int)New_Addr_32 << "]";
}
void ACC_16_to_M()		//Accumulator to Memory 16
{
	operand_RM_offset = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF] * 256;
	switch (Flag_segment_override)
	{
	case 0:
		//ничего не меняем
		operand_RM_seg = *DS;
		break;
	case 1:
		//меняем на ES
		operand_RM_seg = *ES;
		break;
	case 2:
		//меняем на CS
		operand_RM_seg = *CS;
		break;
	case 3:
		//меняем на SS
		operand_RM_seg = *SS;
		break;
	case 4:
		//ничего не меняем (уже DS)
		operand_RM_seg = *DS;
		break;
	}
	memory_2[(operand_RM_offset + operand_RM_seg * 16) & 0xFFFFF] = *ptr_AL;
	operand_RM_offset++;
	memory_2[(operand_RM_offset + operand_RM_seg * 16) & 0xFFFFF] = *ptr_AH;
	Instruction_Pointer += 3;
	if (log_to_console) cout << "AX(" << (int)AX << ") to M[" << (int)operand_RM_seg << ":" << (int)New_Addr_32 << "]";
}

void RM_to_Segment_Reg()	//Register/Memory to Segment Register
{
	operand_RM_offset = Instruction_Pointer;
	operand_RM_offset++;
	byte2 = memory_2[(*CS * 16 + operand_RM_offset) & 0xFFFFF]; //mod / reg / rm

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
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

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
	uint16 reg_data = *ptr_r16[reg];
	if (reg == 4) reg_data -= 2;

	if (log_to_console) cout << "PUSH " << reg16_name[reg] << "(" << (int)reg_data << ")";
		
	//пушим число
	Stack_Pointer--;
	memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFf] = reg_data >> 8;
	Stack_Pointer--;
	memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] = reg_data & 255;
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

void Pop_RM()			//POP Register/Memory (8F)
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];//второй байт
	uint8 OP = (byte2 >> 3) & 7;
	uint16 Src = 0;

	if (OP == 0 || 1) //пока отключим, эти биты вроде игнорируются
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
		cout << "Error: POP RM with REG!=0  command = " << (int)memory_2[CS_data * 16 + Instruction_Pointer] << " " << (int)memory_2[CS_data * 16 + Instruction_Pointer + 1];
		//log_to_console = 1;
		//step_mode = 1;
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
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		// mod 11 получатель - регистр
		*ptr_Src_L = *ptr_r8[(byte2 >> 3) & 7]; //временное значение
		*ptr_r8[(byte2 >> 3) & 7] = *ptr_r8[byte2 & 7];
		*ptr_r8[byte2 & 7] = *ptr_Src_L;
		if (log_to_console) cout << "Exchange " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") with " << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ")";
		Instruction_Pointer += 2;
	}
	else
	{
		*ptr_Src_L = *ptr_r8[(byte2 >> 3) & 7]; //временное значение
		mod_RM_3(byte2);
		*ptr_r8[(byte2 >> 3) & 7] = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = *ptr_Src_L;
		if (log_to_console) cout << "Exchange " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") with M" << OPCODE_comment;
		Instruction_Pointer += 2 + additional_IPs;
	}
}
void XCHG_16()			//Exchange Register/Memory with Register 16bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		// mod 11 получатель - регистр
		*ptr_Src = *ptr_r16[(byte2 >> 3) & 7]; //временное значение
		*ptr_r16[(byte2 >> 3) & 7] = *ptr_r16[byte2 & 7];
		*ptr_r16[byte2 & 7] = *ptr_Src;
		if (log_to_console) cout << "Exchange " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") with " << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ")";
		Instruction_Pointer += 2;
	}
	else
	{
		*ptr_Src = *ptr_r16[(byte2 >> 3) & 7]; //временное значение
		mod_RM_3(byte2);
		*ptr_r16[(byte2 >> 3) & 7] = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		*ptr_r16[(byte2 >> 3) & 7] = *ptr_r16[(byte2 >> 3) & 7] + memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] * 256;
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = *ptr_Src_H;
		operand_RM_offset--;
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = *ptr_Src_L;

		if (log_to_console) cout << "Exchange " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") with M" << OPCODE_comment;
		Instruction_Pointer += 2 + additional_IPs;
	}

	
}
void XCHG_ACC_R()		//Exchange Register with Accumulator
{
	byte2 = memory.read_2(Instruction_Pointer + *CS * 16) & 7; //reg
	
	if (log_to_console) cout << "Exchange AX(" << (int)AX << ") with " << reg16_name[byte2] << "(" << (int)*ptr_r16[byte2] << ")";
	
	*ptr_Src = AX;
	AX = *ptr_r16[byte2];
	*ptr_r16[byte2] = *ptr_Src;

	Instruction_Pointer ++;
}

void In_8_to_ACC_from_port()	 //Input 8 to AL/AX AX from fixed PORT
{
	*ptr_AL = IO_device.input_from_port_8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]); //пишем в AL байт из порта
	if (log_to_console) SetConsoleTextAttribute(hConsole, 10);
	if (log_to_console) cout << "read (" << (int)(AX & 255) << ") from port " << (int)memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];
	if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
	Instruction_Pointer += 2;
}
void In_16_to_ACC_from_port()    //Input 16 to AL/AX AX from fixed PORT
{
	AX = IO_device.input_from_port_16(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]); //пишем в AX 2 байта из порта
	if (log_to_console) SetConsoleTextAttribute(hConsole, 10);
	if (log_to_console) cout << "read (" << (int)AX << ") from port " << (int)memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];
	if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
	Instruction_Pointer += 2;
}
void Out_8_from_ACC_to_port()    //Output 8 from AL/AX AX from fixed PORT
{
	IO_device.output_to_port_8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF], *ptr_AL);//выводим в порт байт AL
	if (log_to_console) SetConsoleTextAttribute(hConsole, 10);
	if (log_to_console) cout << "write AL(" << (int)(AX & 255) << ") to port " << (int)memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];
	if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
	Instruction_Pointer += 2;
}
void Out_16_from_ACC_to_port()   //Output 16 from AL/AX AX from fixed PORT
{
	IO_device.output_to_port_16(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF], AX);//выводим в порт 2 байта AX
	if (log_to_console) SetConsoleTextAttribute(hConsole, 10);
	if (log_to_console) cout << "write AX(" << (int)(AX) << ") to port " << (int)memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];
	if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
	Instruction_Pointer += 2;
}
void In_8_from_DX()		//Input 8 from variable PORT
{
	*ptr_AL = IO_device.input_from_port_8(DX); //пишем в AL байт из порта DX
	if (log_to_console) SetConsoleTextAttribute(hConsole, 10);
	if (log_to_console) cout << "read (" << (int)(AX & 255) << ") from port " << (int)DX;
	if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
	Instruction_Pointer += 1;
}
void In_16_from_DX()	//Input 16 from variable PORT
{
	AX = IO_device.input_from_port_16(DX); //пишем в AX байт из порта DX
	if (log_to_console) SetConsoleTextAttribute(hConsole, 10);
	if (log_to_console) cout << "read (" << (int)AX << ") from port " << (int)DX;
	if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
	Instruction_Pointer += 1;
}
void Out_8_to_DX()		//Output 8 to variable PORT
{
	IO_device.output_to_port_8(DX, *ptr_AL);//выводим в порт DX байт AL
	if (log_to_console) SetConsoleTextAttribute(hConsole, 10);
	if (log_to_console) cout << "write AL(" << (int)(AX & 255) << ") to port " << (int)DX;
	if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
	Instruction_Pointer += 1;
}
void Out_16_to_DX()		//Output 16 to variable PORT
{
	IO_device.output_to_port_16(DX, AX);//выводим в порт DX байт AL
	if (log_to_console) SetConsoleTextAttribute(hConsole, 10);
	if (log_to_console) cout << "write AX(" << (int)(AX) << ") to port " << (int)DX;
	if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
	Instruction_Pointer += 1;
}

void XLAT()			//Translate Byte to AL
{
	switch (Flag_segment_override)
	{
	case 0:
		//ничего не меняем
		operand_RM_seg = *DS;
		break;
	case 1:
		//меняем на ES
		operand_RM_seg = *ES;
		break;
	case 2:
		//меняем на CS
		operand_RM_seg = *CS;
		break;
	case 3:
		//меняем на SS
		operand_RM_seg = *SS;
		break;
	case 4:
		//ничего не меняем (уже DS)
		operand_RM_seg = *DS;
		break;
	}
	if (log_to_console) cout << "XLAT M[" << (int)((BX + *ptr_AL + operand_RM_seg * 16) & 0xFFFFF) << "] -> AL(";
	uint16 Addr = BX + *ptr_AL;
	*ptr_AL = memory_2[(Addr + operand_RM_seg * 16) & 0xFFFFF];
	if (log_to_console) cout << (int)(AX & 255) << ")";
	Instruction_Pointer++;
}
void LEA()			//Load EA to Register
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 EA = 0;

	//определяем смещение
	if ((byte2 >> 6) == 3)
	{
		//UNDOC
		//возвращает офсет, оставшийся от предыдущей операции

		if (log_to_console) cout << "UNDOC LEA (" << (int)operand_RM_offset << ")" << " to " << reg16_name[(byte2 >> 3) & 7];
	}
	else
	{
		mod_RM_3(byte2);
		if (log_to_console) cout << "LEA (" << (int)operand_RM_offset << ")" ;
		*ptr_r16[(byte2 >> 3) & 7] = operand_RM_offset;
		if (log_to_console) cout << " to " << reg16_name[(byte2 >> 3) & 7];
	}

	Instruction_Pointer += 2 + additional_IPs;
}
void LDS()			//Load Pointer to DS
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	//определяем источник
	if ((byte2 >> 6) == 3)
	{
		// UNDOC
		// возвращает данные с учетом старых данных во внутренних регистрах
		//уточнить порядок при необходимости
		*ptr_r16[(byte2 >> 3) & 7] = memory_2[(operand_RM_offset + operand_RM_seg * 16) & 0xFFFFF];
		operand_RM_offset++;
		*ptr_r16[(byte2 >> 3) & 7] = *ptr_r16[(byte2 >> 3) & 7] + memory_2[(operand_RM_offset + operand_RM_seg * 16) & 0xFFFFF] * 256;
		operand_RM_offset++;
		DS_data = memory_2[(operand_RM_offset + operand_RM_seg * 16) & 0xFFFFF];
		operand_RM_offset++;
		DS_data = DS_data + memory_2[(operand_RM_offset + operand_RM_seg * 16) & 0xFFFFF] * 256;
		if (log_to_console) cout << "UNDOC! LDS " << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") to " << reg16_name[(byte2 >> 3) & 7] << " + DS(" << (int)DS_data << ")";
	}
	else
	{
		mod_RM_3(byte2);
		*ptr_r16[(byte2 >> 3) & 7] = memory_2[(operand_RM_offset + operand_RM_seg * 16) & 0xFFFFF];
		operand_RM_offset++;
		*ptr_r16[(byte2 >> 3) & 7]  = *ptr_r16[(byte2 >> 3) & 7]  + memory_2[(operand_RM_offset + operand_RM_seg * 16) & 0xFFFFF] * 256;
		operand_RM_offset++;
		DS_data = memory_2[(operand_RM_offset + operand_RM_seg * 16) & 0xFFFFF];
		operand_RM_offset++;
		DS_data  = DS_data  + memory_2[(operand_RM_offset + operand_RM_seg * 16) & 0xFFFFF] * 256;
		if (log_to_console) cout << "LDS " << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") to " << reg16_name[(byte2 >> 3) & 7] << " + DS(" << (int)DS_data << ")";
	}

	Instruction_Pointer += 2 + additional_IPs;
}
void LES()			//Load Pointer to ES
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	//определяем источник
	if ((byte2 >> 6) == 3)
	{
		// UNDOC
		// возвращает данные с учетом старых данных во внутренних регистрах
		//уточнить порядок при необходимости
		*ptr_r16[(byte2 >> 3) & 7] = memory_2[(operand_RM_offset + operand_RM_seg * 16) & 0xFFFFF];
		operand_RM_offset++;
		*ptr_r16[(byte2 >> 3) & 7] = *ptr_r16[(byte2 >> 3) & 7] + memory_2[(operand_RM_offset + operand_RM_seg * 16) & 0xFFFFF] * 256;
		operand_RM_offset++;
		ES_data = memory_2[(operand_RM_offset + operand_RM_seg * 16) & 0xFFFFF];
		operand_RM_offset++;
		ES_data = ES_data + memory_2[(operand_RM_offset + operand_RM_seg * 16) & 0xFFFFF] * 256;
		if (log_to_console) cout << "UNDOC! LES " << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") to " << reg16_name[(byte2 >> 3) & 7] << " + ES(" << (int)ES_data << ")";
	}
	else
	{
		mod_RM_3(byte2);
		*ptr_r16[(byte2 >> 3) & 7] = memory_2[(operand_RM_offset + operand_RM_seg * 16) & 0xFFFFF];
		operand_RM_offset++;
		*ptr_r16[(byte2 >> 3) & 7]  = *ptr_r16[(byte2 >> 3) & 7]  + memory_2[(operand_RM_offset + operand_RM_seg * 16) & 0xFFFFF] * 256;
		operand_RM_offset++;
		ES_data = memory_2[(operand_RM_offset + operand_RM_seg * 16) & 0xFFFFF];
		operand_RM_offset++;
		ES_data  = ES_data + memory_2[(operand_RM_offset + operand_RM_seg * 16) & 0xFFFFF] * 256;
		if (log_to_console) cout << "LES " << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") to " << reg16_name[(byte2 >> 3) & 7] << " + ES(" << (int)ES_data << ")";
	}

	Instruction_Pointer += 2 + additional_IPs;
}

void LAHF()			// Load AH with Flags
{
	*ptr_AH = (Flag_SF << 7) | (Flag_ZF << 6) | (Flag_AF << 4) | (Flag_PF << 2) | (Flag_CF) | (2);
	if (log_to_console) cout << "Load AH with Flags (" << (bitset<8>) * ptr_AH << ")";
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
	memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] = 0xF0 | (Flag_OF * 8) | (Flag_DF * 4) | (Flag_IF * 2) | Flag_TF;
	//memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] = 0x00 | (Flag_OF * 8) | (Flag_DF * 4) | (Flag_IF * 2) | Flag_TF;
	Stack_Pointer--;
	memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] = 0x2 | (Flag_SF * 128) | (Flag_ZF * 64) | (Flag_AF * 16) | (Flag_PF * 4) | (Flag_CF);
	//memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] = 0x0 | (Flag_SF * 128) | (Flag_ZF * 64) | (Flag_AF * 16) | (Flag_PF * 4) | (Flag_CF);
	if (log_to_console) cout << "Push Flags";
	Instruction_Pointer++;
}
void POPF()			// Pop Flags
{
	uint32 stack_addr = (SS_data * 16 + Stack_Pointer) & 0xFFFFF;
	int Flags = memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF];
	Stack_Pointer++;
	Flags += memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] * 256;
	Stack_Pointer++;
	if ((Flags >> 8) & 1)
	{
		cout << "TF = 1" << endl; step_mode = 1; log_to_console = 1;
	}
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

void ADD_R_to_RM_8()		// ADD R -> R/M 8bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Result = 0;

	if (log_to_console) cout << "ADD " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") ";
	
	//определяем объект назначения и результат операции ADD
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		if (log_to_console) cout << "+ " << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") = ";
		
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

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
		Result = memory_2[New_Addr_32] + *ptr_r8[(byte2 >> 3) & 7];
		Flag_AF = (((memory_2[New_Addr_32] & 15) + (*ptr_r8[(byte2 >> 3) & 7] & 15)) >> 4) & 1;
		Flag_CF = Result >> 8;
		OF_Carry = ((memory_2[New_Addr_32] & 0x7F) + (*ptr_r8[(byte2 >> 3) & 7] & 0x7F)) >> 7;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		memory_2[New_Addr_32] = Result;
		if (log_to_console) cout << " + M" << OPCODE_comment << " = " << (int)(Result & 255);

		Instruction_Pointer += 2 + additional_IPs;
	}
}
void ADD_R_to_RM_16()		// ADD R -> R/M 16bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint32 Result = 0;

	if (log_to_console) cout << "ADD " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") ";
	
	//определяем объект назначения и результат операции ADD
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		Result = *ptr_r16[(byte2 >> 3) & 7] + *ptr_r16[byte2 & 7];
		if (log_to_console) cout << "+ " << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") = " << (int)(Result & 0xFFFF);
		Flag_AF = (((*ptr_r16[(byte2 >> 3) & 7] & 15) + (*ptr_r16[byte2 & 7] & 15)) >> 4) & 1;
		OF_Carry = ((*ptr_r16[(byte2 >> 3) & 7] & 0x7FFF) + (*ptr_r16[byte2 & 7] & 0x7FFF)) >> 15;
		*ptr_r16[byte2 & 7] = Result & 0xFFFF;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		Result =  *ptr_Src + *ptr_r16[(byte2 >> 3) & 7];
		Flag_AF = (((*ptr_Src & 15) + (*ptr_r16[(byte2 >> 3) & 7] & 15)) >> 4) & 1;
		OF_Carry = ((*ptr_Src & 0x7FFF) + (*ptr_r16[(byte2 >> 3) & 7] & 0x7FFF)) >> 15;
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = Result >> 8;
		operand_RM_offset--;
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = Result;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " + M" << OPCODE_comment << " = " << (int)(Result & 0xFFFF);

		Instruction_Pointer += 2 + additional_IPs;
	}
}
void ADD_RM_to_R_8()		// INC R/M -> R 8bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Result = 0;
	
	//определяем 1-й операнд
	if((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		Result = *ptr_r8[byte2 & 7] + *ptr_r8[(byte2 >> 3) & 7];
		if (log_to_console) cout << "ADD " << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") to " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") = " << (int)(Result & 255);
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = ((*ptr_r8[byte2 & 7] & 0x7F) + (*ptr_r8[(byte2 >> 3) & 7] & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((*ptr_r8[byte2 & 7] & 15) + (*ptr_r8[(byte2 >> 3) & 7] & 15)) >> 4) & 1;
		*ptr_r8[(byte2 >> 3) & 7] = Result & 255;

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
		Result = memory_2[New_Addr_32] + *ptr_r8[(byte2 >> 3) & 7];
		if (log_to_console) cout << "ADD M" << OPCODE_comment << " to " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") = " << (int)(Result & 255);
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = ((memory_2[New_Addr_32] & 0x7F) + (*ptr_r8[(byte2 >> 3) & 7] & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((memory_2[New_Addr_32] & 15) + (*ptr_r8[(byte2 >> 3) & 7] & 15)) >> 4) & 1;
		*ptr_r8[(byte2 >> 3) & 7] = Result & 255;
		
		Instruction_Pointer += 2 + additional_IPs;
	}
	
}
void ADD_RM_to_R_16()		// INC R/M -> R 16bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint32 Result = 0;

	//определяем 1-й операнд
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		Result = *ptr_r16[byte2 & 7] + *ptr_r16[(byte2 >> 3) & 7];
		if (log_to_console) cout << "ADD " << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") to " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") = " << (int)(Result & 0xFFFF);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = ((*ptr_r16[byte2 & 7] & 0x7FFF) + (*ptr_r16[(byte2 >> 3) & 7] & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((*ptr_r16[byte2 & 7] & 15) + (*ptr_r16[(byte2 >> 3) & 7] & 15)) >> 4) & 1;
		*ptr_r16[(byte2 >> 3) & 7] = Result & 0xFFFF;

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		Result = *ptr_Src + *ptr_r16[(byte2 >> 3) & 7];
		if (log_to_console) cout << "ADD M" << OPCODE_comment << " to " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") = " << (int)(Result & 0xFFFF);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = ((*ptr_Src & 0x7FFF) + (*ptr_r16[(byte2 >> 3) & 7] & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((*ptr_Src & 15) + (*ptr_r16[(byte2 >> 3) & 7] & 15)) >> 4) & 1;
		*ptr_r16[(byte2 >> 3) & 7] = Result & 0xFFFF;

		Instruction_Pointer += 2 + additional_IPs;
	}
}
void ADD_IMM_RM_16s()		// ADD/ADC IMM -> R/M 16 bit sign ext.
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 OP = (byte2 >> 3) & 7;
	uint16 Src = 0;
	uint32 Result_32 = 0;
	uint16 Result = 0;
	uint16 imm = 0;

	switch (OP)
	{
	case 0: //  ADD  mod 000 r/m
			//определяем объект назначения и результат операции
		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			//непосредственный операнд
			imm = memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF];
			if ((imm >> 7) & 1) imm = imm | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "ADD IMMs(" << (int)imm << ") + " << reg16_name[byte2 & 7] << "(" << *ptr_r16[byte2 & 7] << ") = ";
			//switch (byte2 & 7)
			Result_32 = imm + *ptr_r16[byte2 & 7];
			Flag_AF = (((imm & 15) + (*ptr_r16[byte2 & 7] & 15)) >> 4) & 1;
			OF_Carry = ((imm & 0x7FFF) + (*ptr_r16[byte2 & 7] & 0x7FFF)) >> 15;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			*ptr_r16[byte2 & 7] = Result_32 & 0xFFFF;
			if (log_to_console) cout << (int)(Result_32 & 0xFFFF);

			Instruction_Pointer += 3;
		}
		else
		{
			mod_RM_3(byte2);
			//непосредственный операнд
			imm = memory_2[(Instruction_Pointer + 2 + additional_IPs + *CS * 16) & 0xFFFFF];
			if ((imm >> 7) & 1) imm = imm | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "ADD IMMs(" << (int)imm << ") + ";
			*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			operand_RM_offset++;
			*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			Result_32 = *ptr_Src + imm;
			Flag_AF = (((*ptr_Src & 15) + (imm & 15)) >> 4) & 1;
			OF_Carry = ((*ptr_Src & 0x7FFF) + (imm & 0x7FFF)) >> 15;
			Flag_CF = (Result_32 >> 16) & 1;
			Flag_SF = (Result_32 >> 15) & 1;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = (Result_32 >> 8) & 255;
			operand_RM_offset--;
			memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = Result_32 & 255;
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);

			Instruction_Pointer += 3 + additional_IPs;
		}
		
		break;
	
	case 1:  //OR mod 001 r/m
		//определяем объект назначения и результат операции
	
		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			//непосредственный операнд
			imm = memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF];
			if ((imm >> 7) & 1) imm = imm | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)imm << ") OR " << reg16_name[byte2 & 7] << "(" << *ptr_r16[byte2 & 7] << ") = ";
			//switch (byte2 & 7)
			Result_32 = imm | *ptr_r16[byte2 & 7];
			Flag_CF = 0;
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = 0;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			*ptr_r16[byte2 & 7] = Result_32 & 0xFFFF;
			if (log_to_console) cout << (int)(Result_32 & 0xFFFF);

			Instruction_Pointer += 3;
		}
		else
		{
			mod_RM_3(byte2);
			//непосредственный операнд
			imm = memory_2[(Instruction_Pointer + 2 + additional_IPs + *CS * 16) & 0xFFFFF];
			if ((imm >> 7) & 1) imm = imm | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)imm << ") OR ";
			*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			operand_RM_offset++;
			*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			Result_32 = *ptr_Src | imm;
			Flag_CF = 0;
			Flag_SF = (Result_32 >> 15) & 1;
			Flag_OF = 0;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = (Result_32 >> 8) & 255;
			operand_RM_offset--;
			memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = Result_32 & 255;
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);

			Instruction_Pointer += 3 + additional_IPs;
		}

		break;

	case 2:	//ADC  mod 010 r/m

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			//непосредственный операнд
			imm = memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF];
			if ((imm >> 7) & 1) imm = imm | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "ADC IMMs(" << (int)imm << ") + " << reg16_name[byte2 & 7] << "(" << *ptr_r16[byte2 & 7] << ") + CF(" << (int)Flag_CF << ") = ";
			//switch (byte2 & 7)
			Result_32 = imm + *ptr_r16[byte2 & 7] + Flag_CF;
			Flag_AF = (((imm & 15) + (*ptr_r16[byte2 & 7] & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = ((imm & 0x7FFF) + (*ptr_r16[byte2 & 7] & 0x7FFF) + Flag_CF) >> 15;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			*ptr_r16[byte2 & 7] = Result_32 & 0xFFFF;
			if (log_to_console) cout << (int)(Result_32 & 0xFFFF);

			Instruction_Pointer += 3;
		}
		else
		{
			mod_RM_3(byte2);
			//непосредственный операнд
			imm = memory_2[(Instruction_Pointer + 2 + additional_IPs + *CS * 16) & 0xFFFFF];
			if ((imm >> 7) & 1) imm = imm | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "ADC IMMs(" << (int)imm << ") +  CF(" << (int)Flag_CF << ") + ";
			*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			operand_RM_offset++;
			*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			Result_32 = *ptr_Src + imm + Flag_CF;
			Flag_AF = (((*ptr_Src & 15) + (imm & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = ((*ptr_Src & 0x7FFF) + (imm & 0x7FFF) + Flag_CF) >> 15;
			Flag_CF = (Result_32 >> 16) & 1;
			Flag_SF = (Result_32 >> 15) & 1;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = (Result_32 >> 8) & 255;
			operand_RM_offset--;
			memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = Result_32 & 255;
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);

			Instruction_Pointer += 3 + additional_IPs;
		}

		break;

	case 3:	//   SBB  mod 011 r/m

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			//непосредственный операнд
			imm = memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF];
			if ((imm >> 7) & 1) imm = imm | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << reg16_name[byte2 & 7] << "(" << *ptr_r16[byte2 & 7] << ") - SBB IMMs(" << (int)imm << ") - CF(" << (int)Flag_CF << ") = ";
			//switch (byte2 & 7)
			Result_32 = *ptr_r16[byte2 & 7] - imm - Flag_CF;
			Flag_AF = (((*ptr_r16[byte2 & 7] & 15) - (imm & 15) - Flag_CF) >> 4) & 1;
			OF_Carry = ((*ptr_r16[byte2 & 7] & 0x7FFF) - (imm & 0x7FFF) - Flag_CF) >> 15;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			*ptr_r16[byte2 & 7] = Result_32 & 0xFFFF;
			if (log_to_console) cout << (int)(Result_32 & 0xFFFF);

			Instruction_Pointer += 3;
		}
		else
		{
			mod_RM_3(byte2);
			//непосредственный операнд
			imm = memory_2[(Instruction_Pointer + 2 + additional_IPs + *CS * 16) & 0xFFFFF];
			if ((imm >> 7) & 1) imm = imm | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "SUB M" << OPCODE_comment << " - " << "SBB IMMs(" << (int)imm << ") - CF(" << (int)Flag_CF << ") = " << (int)(Result_32 & 0xFFFF);
			*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			operand_RM_offset++;
			*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			Result_32 = *ptr_Src - imm - Flag_CF;
			Flag_AF = (((*ptr_Src & 15) - (imm & 15) - Flag_CF) >> 4) & 1;
			OF_Carry = ((*ptr_Src & 0x7FFF) - (imm & 0x7FFF) - Flag_CF) >> 15;
			Flag_CF = (Result_32 >> 16) & 1;
			Flag_SF = (Result_32 >> 15) & 1;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = (Result_32 >> 8) & 255;
			operand_RM_offset--;
			memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = Result_32 & 255;

			Instruction_Pointer += 3 + additional_IPs;
		}

		break;

	case 4:  //AND mod 001 r/m
		//определяем объект назначения и результат операции

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			//непосредственный операнд
			imm = memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF];
			if ((imm >> 7) & 1) imm = imm | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)imm << ") AND " << reg16_name[byte2 & 7] << "(" << *ptr_r16[byte2 & 7] << ") = ";
			//switch (byte2 & 7)
			Result_32 = imm & *ptr_r16[byte2 & 7];
			Flag_CF = 0;
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = 0;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			*ptr_r16[byte2 & 7] = Result_32 & 0xFFFF;
			if (log_to_console) cout << (int)(Result_32 & 0xFFFF);

			Instruction_Pointer += 3;
		}
		else
		{
			mod_RM_3(byte2);
			//непосредственный операнд
			imm = memory_2[(Instruction_Pointer + 2 + additional_IPs + *CS * 16) & 0xFFFFF];
			if ((imm >> 7) & 1) imm = imm | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)imm << ") AND ";
			*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			operand_RM_offset++;
			*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			Result_32 = *ptr_Src & imm;
			Flag_CF = 0;
			Flag_SF = (Result_32 >> 15) & 1;
			Flag_OF = 0;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = (Result_32 >> 8) & 255;
			operand_RM_offset--;
			memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = Result_32 & 255;
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);

			Instruction_Pointer += 3 + additional_IPs;
		}
		break;

	case 5:  //   SUB  mod 101 r/m

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			//непосредственный операнд
			imm = memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF];
			if ((imm >> 7) & 1) imm = imm | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << reg16_name[byte2 & 7] << "(" << *ptr_r16[byte2 & 7] << ") - SUB IMMs(" << (int)imm << ") = ";
			//switch (byte2 & 7)
			Result_32 = *ptr_r16[byte2 & 7] - imm;
			Flag_AF = (((*ptr_r16[byte2 & 7] & 15) - (imm & 15)) >> 4) & 1;
			OF_Carry = ((*ptr_r16[byte2 & 7] & 0x7FFF) - (imm & 0x7FFF)) >> 15;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			*ptr_r16[byte2 & 7] = Result_32 & 0xFFFF;
			if (log_to_console) cout << (int)(Result_32 & 0xFFFF);

			Instruction_Pointer += 3;
		}
		else
		{
			mod_RM_3(byte2);
			//непосредственный операнд
			imm = memory_2[(Instruction_Pointer + 2 + additional_IPs + *CS * 16) & 0xFFFFF];
			if ((imm >> 7) & 1) imm = imm | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "SUB M" << OPCODE_comment << " - " << "SUB IMMs(" << (int)imm << ") = " << (int)(Result_32 & 0xFFFF);
			*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			operand_RM_offset++;
			*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			Result_32 = *ptr_Src - imm;
			Flag_AF = (((*ptr_Src & 15) - (imm & 15)) >> 4) & 1;
			OF_Carry = ((*ptr_Src & 0x7FFF) - (imm & 0x7FFF)) >> 15;
			Flag_CF = (Result_32 >> 16) & 1;
			Flag_SF = (Result_32 >> 15) & 1;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = (Result_32 >> 8) & 255;
			operand_RM_offset--;
			memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = Result_32 & 255;
			

			Instruction_Pointer += 3 + additional_IPs;
		}
		
		break;

	case 6:  //XOR mod 110 r/m
		//определяем объект назначения и результат операции

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			//непосредственный операнд
			imm = memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF];
			if ((imm >> 7) & 1) imm = imm | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)imm << ") XOR " << reg16_name[byte2 & 7] << "(" << *ptr_r16[byte2 & 7] << ") = ";
			//switch (byte2 & 7)
			Result_32 = imm ^ *ptr_r16[byte2 & 7];
			Flag_CF = 0;
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = 0;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			*ptr_r16[byte2 & 7] = Result_32 & 0xFFFF;
			if (log_to_console) cout << (int)(Result_32 & 0xFFFF);

			Instruction_Pointer += 3;
		}
		else
		{
			mod_RM_3(byte2);
			//непосредственный операнд
			imm = memory_2[(Instruction_Pointer + 2 + additional_IPs + *CS * 16) & 0xFFFFF];
			if ((imm >> 7) & 1) imm = imm | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)imm << ") XOR ";
			*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			operand_RM_offset++;
			*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			Result_32 = *ptr_Src ^ imm;
			Flag_CF = 0;
			Flag_SF = (Result_32 >> 15) & 1;
			Flag_OF = 0;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = (Result_32 >> 8) & 255;
			operand_RM_offset--;
			memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = Result_32 & 255;
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);

			Instruction_Pointer += 3 + additional_IPs;
		}

		break;

	case 7:  //   CMP mod 111 r/m

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			//непосредственный операнд
			imm = memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF];
			if ((imm >> 7) & 1) imm = imm | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "CMP "  << reg16_name[byte2 & 7] << "(" << *ptr_r16[byte2 & 7] << ") with IMMs(" << (int)imm << ") = ";
			//switch (byte2 & 7)
			Result_32 = *ptr_r16[byte2 & 7] - imm;
			Flag_AF = (((*ptr_r16[byte2 & 7] & 15) - (imm & 15)) >> 4) & 1;
			OF_Carry = ((*ptr_r16[byte2 & 7] & 0x7FFF) - (imm & 0x7FFF)) >> 15;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << (int)(Result_32 & 0xFFFF);

			Instruction_Pointer += 3;
		}
		else
		{
			mod_RM_3(byte2);
			//непосредственный операнд
			imm = memory_2[(Instruction_Pointer + 2 + additional_IPs + *CS * 16) & 0xFFFFF];
			if ((imm >> 7) & 1) imm = imm | 0xFF00; //продолжаем знак на старший байт
			*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			operand_RM_offset++;
			*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			Result_32 = *ptr_Src - imm;
			if (log_to_console) cout << "CMP M" << OPCODE_comment << "("<< (int)*ptr_Src << ") with " << " IMMs(" << (int)imm << ") = " << (int)(Result_32 & 0xFFFF);
			Flag_AF = (((*ptr_Src & 15) - (imm & 15)) >> 4) & 1;
			OF_Carry = ((*ptr_Src & 0x7FFF) - (imm & 0x7FFF)) >> 15;
			Flag_CF = (Result_32 >> 16) & 1;
			Flag_SF = (Result_32 >> 15) & 1;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			Instruction_Pointer += 3 + additional_IPs;
		}
		break;
	}
}

void ADD_IMM_to_ACC_8()	// ADD IMM -> ACC 8bit
{
	uint16 Result = 0;
	
	uint8 imm = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];
	if (log_to_console) cout << "ADD IMM (" << (int)imm << ") to AL(" << (int)(AX & 255) << ") = ";
	OF_Carry = ((imm & 0x7F) + (AX & 0x7F)) >> 7;
	Result = imm + *ptr_AL;
	Flag_AF = (((AX & 15) + (imm & 15)) >> 4) & 1;
	Flag_CF = (Result >> 8) & 1;
	Flag_SF = (Result >> 7) & 1;
	Flag_OF = Flag_CF ^ OF_Carry;
	if (Result & 255) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result & 255];
	*ptr_AL = Result & 255;
	if (log_to_console) cout << (int)(Result);
	Instruction_Pointer += 2;
}
void ADD_IMM_to_ACC_16()	// ADD IMM -> ACC 16bit 
{
	uint32 Result = 0;
	operand_RM_offset = Instruction_Pointer;
	operand_RM_offset++;
	uint16 imm = memory_2[(operand_RM_offset + *CS * 16) & 0xFFFFF];
	operand_RM_offset++;
	imm += memory.read_2(operand_RM_offset + *CS * 16) * 256;
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

void ADC_R_to_RM_8()		// ADC R -> R/M 8bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Result = 0;

	if (log_to_console) cout << "ADC " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ")";

	//определяем объект назначения и результат операции ADD
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		//складываем два регистра
		Result = *ptr_r8[(byte2 >> 3) & 7] + *ptr_r8[byte2 & 7] + Flag_CF;
		if (log_to_console) cout << " + " << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") + CF(" << (int)Flag_CF << ") = " << (int)(Result & 255);
		Flag_AF = (((*ptr_r8[(byte2 >> 3) & 7] & 15) + (*ptr_r8[byte2 & 7] & 15) + Flag_CF) >> 4) & 1;
		OF_Carry = ((*ptr_r8[(byte2 >> 3) & 7] & 0x7F) + (*ptr_r8[byte2 & 7] & 0x7F) + Flag_CF) >> 7;
		*ptr_r8[byte2 & 7] = Result;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
		Result = memory_2[New_Addr_32] + *ptr_r8[(byte2 >> 3) & 7] + Flag_CF;
		if (log_to_console) cout << " + M" << OPCODE_comment << " + CF(" << (int)Flag_CF << ") = " << (int)(Result & 255);
		Flag_AF = (((memory_2[New_Addr_32] & 15) + (*ptr_r8[(byte2 >> 3) & 7] & 15) + Flag_CF) >> 4) & 1;
		OF_Carry = ((memory_2[New_Addr_32] & 0x7F) + (*ptr_r8[(byte2 >> 3) & 7] & 0x7F) + Flag_CF) >> 7;
		Flag_CF = Result >> 8;
		if (log_to_console) cout << "  " << (int)OF_Carry;
		Flag_SF = (Result >> 7) & 1;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		memory_2[New_Addr_32] = Result;

		Instruction_Pointer += 2 + additional_IPs;
	}
}
void ADC_R_to_RM_16()		// ADC R -> R/M 16bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint32 Result = 0;

	if (log_to_console) cout << "ADC " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") ";

	//определяем объект назначения и результат операции ADD
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		Result = *ptr_r16[(byte2 >> 3) & 7] + *ptr_r16[byte2 & 7] + Flag_CF;
		if (log_to_console) cout << "+ " << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") + CF(" << (int)Flag_CF << ") = " << (int)(Result & 0xFFFF);
		Flag_AF = (((*ptr_r16[(byte2 >> 3) & 7] & 15) + (*ptr_r16[byte2 & 7] & 15) + Flag_CF) >> 4) & 1;
		OF_Carry = ((*ptr_r16[(byte2 >> 3) & 7] & 0x7FFF) + (*ptr_r16[byte2 & 7] & 0x7FFF) + Flag_CF) >> 15;
		*ptr_r16[byte2 & 7] = Result & 0xFFFF;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		Result = *ptr_Src + *ptr_r16[(byte2 >> 3) & 7] + Flag_CF;
		Flag_AF = (((*ptr_Src & 15) + (*ptr_r16[(byte2 >> 3) & 7] & 15) + Flag_CF) >> 4) & 1;
		OF_Carry = ((*ptr_Src & 0x7FFF) + (*ptr_r16[(byte2 >> 3) & 7] & 0x7FFF) + Flag_CF) >> 15;
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = Result >> 8;
		operand_RM_offset--;
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = Result;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " + M" << OPCODE_comment << " + CF(" << (int)Flag_CF << ") = " << (int)(Result & 0xFFFF);

		Instruction_Pointer += 2 + additional_IPs;
	}
}
void ADC_RM_to_R_8()		// ADC R/M -> R 8bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Result = 0;

	//определяем 1-й операнд
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		Result = *ptr_r8[byte2 & 7] + *ptr_r8[(byte2 >> 3) & 7] + Flag_CF;
		if (log_to_console) cout << "ADС " << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") to " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") + CF(" << (int)Flag_CF << ") = " << (int)(Result & 255);
		OF_Carry = ((*ptr_r8[byte2 & 7] & 0x7F) + (*ptr_r8[(byte2 >> 3) & 7] & 0x7F) + Flag_CF) >> 7;
		Flag_AF = (((*ptr_r8[byte2 & 7] & 15) + (*ptr_r8[(byte2 >> 3) & 7] & 15) + Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		*ptr_r8[(byte2 >> 3) & 7] = Result & 255;

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
		Result = memory_2[New_Addr_32] + *ptr_r8[(byte2 >> 3) & 7] + Flag_CF;
		if (log_to_console) cout << "ADD M" << OPCODE_comment << " to " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") + CF(" << (int)Flag_CF << ") = " << (int)(Result & 255);
		OF_Carry = ((memory_2[New_Addr_32] & 0x7F) + (*ptr_r8[(byte2 >> 3) & 7] & 0x7F) + Flag_CF) >> 7;
		Flag_AF = (((memory_2[New_Addr_32] & 15) + (*ptr_r8[(byte2 >> 3) & 7] & 15) + Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		*ptr_r8[(byte2 >> 3) & 7] = Result & 255;

		Instruction_Pointer += 2 + additional_IPs;
	}
}
void ADC_RM_to_R_16()		// ADC R/M -> R 16bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint32 Result = 0;

	//определяем 1-й операнд
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		Result = *ptr_r16[byte2 & 7] + *ptr_r16[(byte2 >> 3) & 7] + Flag_CF;
		if (log_to_console) cout << "ADC " << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") to " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") + CF(" << (int)Flag_CF << ") = " << (int)(Result & 0xFFFF);
		OF_Carry = ((*ptr_r16[byte2 & 7] & 0x7FFF) + (*ptr_r16[(byte2 >> 3) & 7] & 0x7FFF) + Flag_CF) >> 15;
		Flag_AF = (((*ptr_r16[byte2 & 7] & 15) + (*ptr_r16[(byte2 >> 3) & 7] & 15) + Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		*ptr_r16[(byte2 >> 3) & 7] = Result & 0xFFFF;

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		Result = *ptr_Src + *ptr_r16[(byte2 >> 3) & 7] + Flag_CF;
		if (log_to_console) cout << "ADD M" << OPCODE_comment << " to " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") + CF(" << (int)Flag_CF << ") = " << (int)(Result & 0xFFFF);
		OF_Carry = ((*ptr_Src & 0x7FFF) + (*ptr_r16[(byte2 >> 3) & 7] & 0x7FFF) + Flag_CF) >> 15;
		Flag_AF = (((*ptr_Src & 15) + (*ptr_r16[(byte2 >> 3) & 7] & 15) + Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		*ptr_r16[(byte2 >> 3) & 7] = Result & 0xFFFF;

		Instruction_Pointer += 2 + additional_IPs;
	}
}

void ADC_IMM_to_ACC_8()		// ADC IMM->ACC 8bit
{
	uint16 Result = 0;
	
	uint8 imm = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];
	if (log_to_console) cout << "ADC IMM (" << (int)imm << ") to AL(" << (int)(AX & 255) << ") + CF("<<(int)Flag_CF<<") = ";
	OF_Carry = ((imm & 0x7F) + (AX & 0x7F) + Flag_CF) >> 7;
	Flag_AF = (((AX & 15) + (imm & 15) + Flag_CF) >> 4) & 1;
	Result = imm + *ptr_AL + Flag_CF;
	Flag_CF = (Result >> 8) & 1;
	Flag_SF = (Result >> 7) & 1;
	Flag_OF = Flag_CF ^ OF_Carry;
	if (Result & 255) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result & 255];
	*ptr_AL = Result;
	if (log_to_console) cout << (int)(Result & 255);

	Instruction_Pointer += 2;
}
void ADC_IMM_to_ACC_16()	// ADC IMM->ACC 16bit 
{
	uint32 Result = 0;
	
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

//INC/DEC 8
void INC_RM_8()		// INC R/M 8bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];//второй байт
	uint8 Src = 0;		//источник данных
	additional_IPs = 0;
	uint16 old_IP = 0;
	uint16 new_IP = 0;
	uint16 new_CS = 0;
	uint32 stack_addr = 0;

	switch ((byte2 >> 3) & 7)
	{
	case 0: //INC RM
		
		if ((byte2 >> 6) == 3)
		{
			//увеличиваем регистр
			
			Flag_AF = (((*ptr_r8[byte2 & 7] & 0x0F) + 1) >> 4) & 1;
			Flag_OF = ((*ptr_r8[byte2 & 7] & 0x7F) + 1) >> 7;
			++(*ptr_r8[byte2 & 7]);
			if (*ptr_r8[byte2 & 7]) Flag_ZF = 0;
			else {
				Flag_ZF = 1;
				Flag_OF = 0;
			}
			Flag_SF = (*ptr_r8[byte2 & 7] >> 7) & 1;
			Flag_PF = parity_check[*ptr_r8[byte2 & 7]];
			if (log_to_console) cout << "INC " << reg8_name[byte2 & 7] << " = " << (int)*ptr_r8[byte2 & 7];

			Instruction_Pointer += 2;
		} 
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			Flag_AF = (((memory_2[New_Addr_32] & 0x0F) + 1) >> 4) & 1;
			Flag_OF = ((memory_2[New_Addr_32] & 0x7F) + 1) >> 7;
			memory_2[New_Addr_32]++;
			if (memory_2[New_Addr_32]) Flag_ZF = 0;
			else 
			{
				Flag_ZF = 1;
				Flag_OF = 0;
			}
			Flag_SF = (memory_2[New_Addr_32] >> 7) & 1;
			Flag_PF = parity_check[memory_2[New_Addr_32]];
			if (log_to_console) cout << "INC M" << OPCODE_comment << " = " << (int)memory_2[New_Addr_32];
			
			Instruction_Pointer += 2 + additional_IPs;
		}
		break;

	case 1: //DEC RM
	
			
		if ((byte2 >> 6) == 3)
		{
			//увеличиваем регистр

			Flag_AF = (((*ptr_r8[byte2 & 7] & 0x0F) - 1) >> 4) & 1;
			Flag_OF = ((*ptr_r8[byte2 & 7] & 0x7F) - 1) >> 7;
			if (!*ptr_r8[byte2 & 7]) Flag_OF = 0;
			--(*ptr_r8[byte2 & 7]);
			if (*ptr_r8[byte2 & 7]) Flag_ZF = 0;
			else Flag_ZF = 1;
			Flag_SF = (*ptr_r8[byte2 & 7] >> 7) & 1;
			Flag_PF = parity_check[*ptr_r8[byte2 & 7]];
			if (log_to_console) cout << "DEC " << reg8_name[byte2 & 7] << " = " << (int)*ptr_r8[byte2 & 7];

			Instruction_Pointer += 2;
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			Flag_AF = (((memory_2[New_Addr_32] & 0x0F) - 1) >> 4) & 1;
			Flag_OF = ((memory_2[New_Addr_32] & 0x7F) - 1) >> 7;
			if (!memory_2[New_Addr_32]) Flag_OF = 0;
			memory_2[New_Addr_32]--;
			if (memory_2[New_Addr_32]) Flag_ZF = 0;
			else Flag_ZF = 1;
			Flag_SF = (memory_2[New_Addr_32] >> 7) & 1;
			Flag_PF = parity_check[memory_2[New_Addr_32]];
			if (log_to_console) cout << "DEC M" << OPCODE_comment << " = " << (int)memory_2[New_Addr_32];

			Instruction_Pointer += 2 + additional_IPs;
		}
		
		break;

	case 2: //Indirect near call (Undoc)
		
		//портит адреса вызова и адреса возврата, так как работает только с 1 байтом
		if ((byte2 >> 6) == 3)
		{
			// mod 11 - адрес в регистре
			new_IP = *ptr_r8[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			new_IP = memory_2[New_Addr_32];
		}

		old_IP = Instruction_Pointer;
		stack_addr = SS_data * 16 + Stack_Pointer - 1;
		Stack_Pointer--;
		Stack_Pointer--;
		memory.write_2(SS_data * 16 + Stack_Pointer, (Instruction_Pointer + 2 + additional_IPs) & 255);

		Instruction_Pointer = new_IP;
		if (log_to_console) SetConsoleTextAttribute(hConsole, 13);
		if (log_to_console) cout << "UNDOC! Near Indirect Call to " << (int)*CS << ":" << (int)Instruction_Pointer << " (ret to " << (int)*CS << ":" << (int)(old_IP + 2 + additional_IPs) << ")";
		//else cout << "Near Indirect Call to " << (int)*CS << ":" << (int)Instruction_Pointer << " (ret to " << (int)*CS << ":" << (int)(old_IP + 2 + additional_IPs) << ")";
		if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
		
		break;

	case 3:  //Indirect Intersegment Call (Undoc)
			//портит адреса вызова и возврата так как работает только с 1 байтом

		if ((byte2 >> 6) == 3)
		{
			operand_RM_seg = DS_data * 16;
			operand_RM_offset = *ptr_r8[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
		}

		new_IP = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		//new_IP = new_IP + memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] * 256;
		operand_RM_offset++;
		new_CS = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		//new_CS = new_CS + memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] * 256;

		Stack_Pointer--;
		//memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] = CS_data >> 8;
		Stack_Pointer--;
		memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] = CS_data & 255;
		Stack_Pointer--;
		//memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] = (Instruction_Pointer + 2 + additional_IPs) >> 8;
		Stack_Pointer--;
		memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] = (Instruction_Pointer + 2 + additional_IPs) & 255;

		Instruction_Pointer = new_IP;
		*CS = new_CS;

		if (log_to_console) SetConsoleTextAttribute(hConsole, 13);
		if (log_to_console) cout << "Undoc! Indirect Intersegment Call to " << (int)*CS << ":" << (int)Instruction_Pointer;
		//if (log_to_console) cout << " DEBUG addr = " << (int)New_Addr_32;
		if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
		break;

	
	case 4:  //Indirect Jump within Segment   (Undoc)
			 //портит адрес вызова так как работает только с 1 байтом
		if ((byte2 >> 6) == 3)
		{
			Instruction_Pointer = *ptr_r8[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			Instruction_Pointer = memory_2[New_Addr_32];
		}

		if (log_to_console) SetConsoleTextAttribute(hConsole, 13);
		if (log_to_console) cout << "UNDOC! Indirect Jump within Segment to " << (int)*CS << ":" << (int)Instruction_Pointer;
		if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
		break;

	case 5:  //Indirect Intersegment Jump (Undoc)
			 //портит адрес вызова так как работает только с 1 байтом
		if ((byte2 >> 6) == 3)
		{
			New_Addr_32 = *ptr_r8[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
		}

		Instruction_Pointer = memory_2[New_Addr_32];
		*CS = memory_2[New_Addr_32 + 2];
		if (log_to_console) SetConsoleTextAttribute(hConsole, 13);
		if (log_to_console) cout << "UNDOC! Indirect Intersegment Jump to " << (int)*CS << ":" << (int)Instruction_Pointer;
		if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
		break;

	case 6:  // PUSH RM (Undoc)

		if ((byte2 >> 6) == 3)
		{
			//пушим в стек регистр
			Stack_Pointer--;
			Stack_Pointer--;
			memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] = *ptr_r8[byte2 & 7];
			cout << "Undoc PUSH " << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ")";
			Instruction_Pointer += 2;
		}
		else
		{
			//пушим байт из памяти 
			//если установлен bus_lock - бросаем исключение 1(?)
			//if (bus_lock == 1) exeption = 0x11;
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			Stack_Pointer--;
			Stack_Pointer--;
			memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] = memory_2[New_Addr_32];
			cout << "Undoc PUSH " << OPCODE_comment << "(" << (int)memory_2[New_Addr_32] << ") IP " << (int)memory_2[CS_data * 16 + Instruction_Pointer] << " " << (int)memory_2[CS_data * 16 + Instruction_Pointer + 1] <<endl;
			Instruction_Pointer += 2 + additional_IPs;
		}
		break;

	case 7: 
		
		cout << "INC FE/0 Illegal Instruction byte2=" << (bitset<8>)byte2 << endl;
		if ((byte2 >> 6) == 3)
		{
			Instruction_Pointer += 2 + additional_IPs;
		}
		else
		{
			mod_RM_3(byte2);
			Instruction_Pointer += 2 + additional_IPs;
		}
		break;
	}
}
void INC_Reg()			//  INC reg 16 bit
{
	uint8 reg = memory_2[(Instruction_Pointer + *CS * 16) & 0xFFFFF] & 7;//регистр
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

	if (log_to_console) cout << "INC " << reg16_name[reg] << " = " << (int)*ptr_r16[reg];

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
	if (temp_ACC_8 > 9)
	{
		temp_ACC_16 = temp_ACC_16 + 6;
		Flag_AF = true;
	}
	else
	{
		if (Flag_AF)
		{
			temp_ACC_16 = (temp_ACC_16 & 0xF0) | ((temp_ACC_8 + 6) & 0xF);
		}
	}
	
	temp_ACC_8 = (temp_ACC_16 >> 4) & 31; //старшие 4 бита

	if (temp_ACC_8 > 9 || Flag_CF)
	{
		temp_ACC_8 += 6; // +6 к старшим битам
		if (temp_ACC_8 > 15) { Flag_CF = true;}
		temp_ACC_8 = temp_ACC_8 & 15;
	}

	*ptr_AL = (temp_ACC_16 & 15)|(temp_ACC_8 << 4);
	
	if (*ptr_AL) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_SF = (AX >> 7) & 1;
	Flag_PF = parity_check[*ptr_AL];
	if (log_to_console) cout << "DAA AL = " << (int)((AX >> 4) & 15) << " + " << (int)(AX & 15);
	Instruction_Pointer += 1;
}

//SUB

void SUB_RM_from_RM_8()			// SUB R from R/M 8bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Result = 0;

	if (log_to_console) cout << "SUB " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") from ";

	//определяем объект назначения и результат операции ADD
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		if (log_to_console) cout << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") = ";

		//складываем два регистра
		Result = *ptr_r8[byte2 & 7] - *ptr_r8[(byte2 >> 3) & 7];
		if (log_to_console) cout << (int)(Result & 255);
		Flag_AF = (((*ptr_r8[byte2 & 7] & 15) - (*ptr_r8[(byte2 >> 3) & 7] & 15)) >> 4) & 1;
		OF_Carry = ((*ptr_r8[byte2 & 7] & 0x7F) - (*ptr_r8[(byte2 >> 3) & 7] & 0x7F)) >> 7;
		*ptr_r8[byte2 & 7] = Result;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
		Result = memory_2[New_Addr_32] - *ptr_r8[(byte2 >> 3) & 7];
		Flag_AF = (((memory_2[New_Addr_32] & 15) - (*ptr_r8[(byte2 >> 3) & 7] & 15)) >> 4) & 1;
		Flag_CF = Result >> 8;
		OF_Carry = ((memory_2[New_Addr_32] & 0x7F) - (*ptr_r8[(byte2 >> 3) & 7] & 0x7F)) >> 7;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		memory_2[New_Addr_32] = Result;
		if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)(Result & 255);

		Instruction_Pointer += 2 + additional_IPs;
	}
}
void SUB_RM_from_RM_16()		// SUB R from R/M 16bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint32 Result = 0;

	if (log_to_console) cout << "SUB " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") from ";

	//определяем объект назначения и результат операции ADD
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		Result = *ptr_r16[byte2 & 7] - *ptr_r16[(byte2 >> 3) & 7];
		if (log_to_console) cout << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") = " << (int)(Result & 0xFFFF);
		Flag_AF = (((*ptr_r16[byte2 & 7] & 15) - (*ptr_r16[(byte2 >> 3) & 7] & 15)) >> 4) & 1;
		OF_Carry = ((*ptr_r16[byte2 & 7] & 0x7FFF) - (*ptr_r16[(byte2 >> 3) & 7] & 0x7FFF)) >> 15;
		*ptr_r16[byte2 & 7] = Result & 0xFFFF;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		Result = *ptr_Src - *ptr_r16[(byte2 >> 3) & 7];
		Flag_AF = (((*ptr_Src & 15) - (*ptr_r16[(byte2 >> 3) & 7] & 15)) >> 4) & 1;
		OF_Carry = ((*ptr_Src & 0x7FFF) - (*ptr_r16[(byte2 >> 3) & 7] & 0x7FFF)) >> 15;
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = Result >> 8;
		operand_RM_offset--;
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = Result;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)(Result & 0xFFFF);

		Instruction_Pointer += 2 + additional_IPs;
	}
}
void SUB_RM_from_R_8()			// SUB R/M -> R 8bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Result = 0;

	//определяем 1-й операнд
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		Result = - *ptr_r8[byte2 & 7] + *ptr_r8[(byte2 >> 3) & 7];
		if (log_to_console) cout << "SUB " << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") from " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") = " << (int)(Result & 255);
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = (-(*ptr_r8[byte2 & 7] & 0x7F) + (*ptr_r8[(byte2 >> 3) & 7] & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((-(*ptr_r8[byte2 & 7] & 15) + (*ptr_r8[(byte2 >> 3) & 7] & 15)) >> 4) & 1;
		*ptr_r8[(byte2 >> 3) & 7] = Result & 255;

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
		Result = -memory_2[New_Addr_32] + *ptr_r8[(byte2 >> 3) & 7];
		if (log_to_console) cout << "SUB M" << OPCODE_comment << " from " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") = " << (int)(Result & 255);
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = (-(memory_2[New_Addr_32] & 0x7F) + (*ptr_r8[(byte2 >> 3) & 7] & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((-(memory_2[New_Addr_32] & 15) + (*ptr_r8[(byte2 >> 3) & 7] & 15)) >> 4) & 1;
		*ptr_r8[(byte2 >> 3) & 7] = Result & 255;

		Instruction_Pointer += 2 + additional_IPs;
	}
}
void SUB_RM_from_R_16()			// SUB R/M -> R 16bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint32 Result = 0;

	//определяем 1-й операнд
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		Result = -*ptr_r16[byte2 & 7] + *ptr_r16[(byte2 >> 3) & 7];
		if (log_to_console) cout << "SUB " << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") from " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") = " << (int)(Result & 0xFFFF);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = (-(*ptr_r16[byte2 & 7] & 0x7FFF) + (*ptr_r16[(byte2 >> 3) & 7] & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((-(*ptr_r16[byte2 & 7] & 15) + (*ptr_r16[(byte2 >> 3) & 7] & 15)) >> 4) & 1;
		*ptr_r16[(byte2 >> 3) & 7] = Result & 0xFFFF;

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		Result = -*ptr_Src + *ptr_r16[(byte2 >> 3) & 7];
		if (log_to_console) cout << "SUB M" << OPCODE_comment << " from " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") = " << (int)(Result & 0xFFFF);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = (-(*ptr_Src & 0x7FFF) + (*ptr_r16[(byte2 >> 3) & 7] & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((-(*ptr_Src & 15) + (*ptr_r16[(byte2 >> 3) & 7] & 15)) >> 4) & 1;
		*ptr_r16[(byte2 >> 3) & 7] = Result & 0xFFFF;

		Instruction_Pointer += 2 + additional_IPs;
	}
}

void SUB_IMM_from_ACC_8()		// SUB ACC  8bit - IMM -> ACC
{
	uint16 Result = 0;
	uint8 imm = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];
	bool OF_Carry = 0;

	if (log_to_console) cout << "SUB IMM (" << (int)imm << ") from AL(" << setw(2) << (int)*ptr_AL << ") = ";
	OF_Carry = ((*ptr_AL & 0x7F) - (imm & 0x7F)) >> 7;
	Result = *ptr_AL - imm;
	Flag_CF = (Result >> 8) & 1;
	Flag_SF = (Result >> 7) & 1;
	Flag_OF = Flag_CF ^ OF_Carry;
	if (Result & 255) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result & 255];
	Flag_AF = ((((AX) & 15) - (imm & 15)) >> 4) & 1;
	*ptr_AL = Result;
	if (log_to_console) cout << (int)*ptr_AL;

	Instruction_Pointer += 2;
}
void SUB_IMM_from_ACC_16()		// SUB ACC 16bit - IMM -> ACC
{
	uint32 Result = 0;
	bool OF_Carry = 0;
	operand_RM_offset = Instruction_Pointer;
	operand_RM_offset++;
	uint16 imm = memory_2[(operand_RM_offset + *CS * 16) & 0xFFFFF];
	operand_RM_offset++;
	imm += memory.read_2(operand_RM_offset + *CS * 16) * 256;
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
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Result = 0;

	if (log_to_console) cout << "SBB " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") (CF=" << (int)Flag_CF << ") from ";

	//определяем объект назначения и результат операции ADD
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		if (log_to_console) cout << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") = ";

		//складываем два регистра
		Result = *ptr_r8[byte2 & 7] - *ptr_r8[(byte2 >> 3) & 7] - Flag_CF;
		if (log_to_console) cout << (int)(Result & 255);
		Flag_AF = (((*ptr_r8[byte2 & 7] & 15) - (*ptr_r8[(byte2 >> 3) & 7] & 15) - Flag_CF) >> 4) & 1;
		OF_Carry = ((*ptr_r8[byte2 & 7] & 0x7F) - (*ptr_r8[(byte2 >> 3) & 7] & 0x7F) - Flag_CF) >> 7;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (!*ptr_r8[byte2 & 7]) Flag_OF = 0; //если вычитаем из ноля, OF = 0
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		*ptr_r8[byte2 & 7] = Result;
		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
		Result = memory_2[New_Addr_32] - *ptr_r8[(byte2 >> 3) & 7] - Flag_CF;
		Flag_AF = (((memory_2[New_Addr_32] & 15) - (*ptr_r8[(byte2 >> 3) & 7] & 15) - Flag_CF) >> 4) & 1;
		OF_Carry = ((memory_2[New_Addr_32] & 0x7F) - (*ptr_r8[(byte2 >> 3) & 7] & 0x7F) - Flag_CF) >> 7;
		Flag_CF = Result >> 8;
		Flag_OF = Flag_CF ^ OF_Carry;
		if(!memory_2[New_Addr_32]) Flag_OF = 0; //если вычитаем из ноля, OF = 0
		Flag_SF = ((Result >> 7) & 1);
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		memory_2[New_Addr_32] = Result;
		if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)(Result & 255);
		Instruction_Pointer += 2 + additional_IPs;
	}
}
void SBB_RM_from_RM_16()		// SBB R/M -> R/M 16bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint32 Result = 0;

	if (log_to_console) cout << "SBB " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") (CF=" << (int)Flag_CF << ") from ";

	//определяем объект назначения и результат операции ADD
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		Result = *ptr_r16[byte2 & 7] - *ptr_r16[(byte2 >> 3) & 7] - Flag_CF;
		if (log_to_console) cout << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") = " << (int)(Result & 0xFFFF);
		Flag_AF = (((*ptr_r16[byte2 & 7] & 15) - (*ptr_r16[(byte2 >> 3) & 7] & 15) - Flag_CF) >> 4) & 1;
		OF_Carry = ((*ptr_r16[byte2 & 7] & 0x7FFF) - (*ptr_r16[(byte2 >> 3) & 7] & 0x7FFF) - Flag_CF) >> 15;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = (Result >> 15) & 1;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (!*ptr_r16[byte2 & 7]) Flag_OF = 0; //если вычитаем из ноля, OF = 0
		*ptr_r16[byte2 & 7] = Result & 0xFFFF;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		Result = *ptr_Src - *ptr_r16[(byte2 >> 3) & 7] - Flag_CF;
		Flag_AF = (((*ptr_Src & 15) - (*ptr_r16[(byte2 >> 3) & 7] & 15) - Flag_CF) >> 4) & 1;
		OF_Carry = ((*ptr_Src & 0x7FFF) - (*ptr_r16[(byte2 >> 3) & 7] & 0x7FFF) - Flag_CF) >> 15;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = (Result >> 15) & 1;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (!*ptr_Src) Flag_OF = 0; //если вычитаем из ноля, OF = 0
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = Result >> 8;
		operand_RM_offset--;
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = Result;
		if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)(Result & 0xFFFF);
		Instruction_Pointer += 2 + additional_IPs;
	}
}
void SBB_RM_from_R_8()			// SBB R/M -> R 8bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Result = 0;

	//определяем 1-й операнд
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		Result = -*ptr_r8[byte2 & 7] + *ptr_r8[(byte2 >> 3) & 7] - Flag_CF;
		if (log_to_console) cout << "SBB " << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") (CF=" << (int)Flag_CF << ") from " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") = " << (int)(Result & 255);
		OF_Carry = (-(*ptr_r8[byte2 & 7] & 0x7F) + (*ptr_r8[(byte2 >> 3) & 7] & 0x7F) - Flag_CF) >> 7;
		Flag_AF = ((-(*ptr_r8[byte2 & 7] & 15) + (*ptr_r8[(byte2 >> 3) & 7] & 15) - Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (!*ptr_r8[(byte2 >> 3) & 7]) Flag_OF = 0; //если вычитаем из ноля, OF = 0
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		*ptr_r8[(byte2 >> 3) & 7] = Result & 255;

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
		Result = -memory_2[New_Addr_32] + *ptr_r8[(byte2 >> 3) & 7] - Flag_CF;
		OF_Carry = (-(memory_2[New_Addr_32] & 0x7F) + (*ptr_r8[(byte2 >> 3) & 7] & 0x7F) - Flag_CF) >> 7;
		Flag_AF = ((-(memory_2[New_Addr_32] & 15) + (*ptr_r8[(byte2 >> 3) & 7] & 15) - Flag_CF) >> 4) & 1;
		if (log_to_console) cout << "SBB M" << OPCODE_comment << " (CF=" << (int)Flag_CF << ") from " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") = " << (int)(Result & 255);
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (!*ptr_r8[(byte2 >> 3) & 7]) Flag_OF = 0; //если вычитаем из ноля, OF = 0
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		*ptr_r8[(byte2 >> 3) & 7] = Result & 255;

		Instruction_Pointer += 2 + additional_IPs;
	}

}
void SBB_RM_from_R_16()			// SBB R/M -> R 16bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint32 Result = 0;

	//определяем 1-й операнд
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		Result = -*ptr_r16[byte2 & 7] + *ptr_r16[(byte2 >> 3) & 7] - Flag_CF;
		if (log_to_console) cout << "SBB " << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") (CF=" << (int)Flag_CF << ") from " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") = " << (int)(Result & 0xFFFF);
		OF_Carry = (-(*ptr_r16[byte2 & 7] & 0x7FFF) + (*ptr_r16[(byte2 >> 3) & 7] & 0x7FFF) - Flag_CF) >> 15;
		Flag_AF = ((-(*ptr_r16[byte2 & 7] & 15) + (*ptr_r16[(byte2 >> 3) & 7] & 15) - Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (!*ptr_r16[(byte2 >> 3) & 7]) Flag_OF = 0; //если вычитаем из ноля, OF = 0
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		*ptr_r16[(byte2 >> 3) & 7] = Result & 0xFFFF;
		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		Result = -*ptr_Src + *ptr_r16[(byte2 >> 3) & 7] - Flag_CF;
		if (log_to_console) cout << "SBB M" << OPCODE_comment << " (CF=" << (int)Flag_CF << ") from " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") = " << (int)(Result & 0xFFFF);
		OF_Carry = (-(*ptr_Src & 0x7FFF) + (*ptr_r16[(byte2 >> 3) & 7] & 0x7FFF) - Flag_CF) >> 15;
		Flag_AF = ((-(*ptr_Src & 15) + (*ptr_r16[(byte2 >> 3) & 7] & 15) - Flag_CF) >> 4) & 1;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (!*ptr_r16[(byte2 >> 3) & 7]) Flag_OF = 0; //если вычитаем из ноля, OF = 0
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		*ptr_r16[(byte2 >> 3) & 7] = Result & 0xFFFF;
		Instruction_Pointer += 2 + additional_IPs;
	}
}

void SBB_IMM_from_ACC_8()		// SBB ACC  8bit - IMM -> ACC
{
	uint16 Result = 0;
	uint8 imm = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];
	bool OF_Carry = 0;
	if (log_to_console) cout << "SBB IMM (" << (int)imm << ") from AL(" << setw(2) << (int)*ptr_AL << ") = ";
	OF_Carry = ((AX & 0x7F) - (imm & 0x7F) - Flag_CF) >> 7;
	Flag_AF = (((AX & 15) - (imm & 15) - Flag_CF) >> 4) & 1;
	Result = (AX & 255) - imm - Flag_CF;
	Flag_CF = (Result >> 8) & 1;
	Flag_SF = (Result >> 7) & 1;
	Flag_OF = Flag_CF ^ OF_Carry;
	if (!*ptr_AL) Flag_OF = 0; //если вычитаем из ноля, OF = 0
	if (Result & 255) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result & 255];
	*ptr_AL = Result & 255;
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
	if (!AX) Flag_OF = 0; //если вычитаем из ноля, OF = 0
	if (Result & 0xFFFF) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result & 255];
	AX = Result;
	if (log_to_console) cout << (int)(AX);

	Instruction_Pointer += 3;
}

void DEC_Reg()			//  DEC reg 16 bit
{
	uint8 reg = memory_2[(Instruction_Pointer + *CS * 16) & 0xFFFFF] & 7;//регистр
	Flag_AF = (((*ptr_r16[reg] & 0x0F) - 1) >> 4) & 1;
	Flag_OF = (((*ptr_r16[reg] & 0x7FFF) - 1) >> 15);
	if (!*ptr_r16[reg]) Flag_OF = 0;
	(*ptr_r16[reg])--;
	if (*ptr_r16[reg]) Flag_ZF = 0;
	else Flag_ZF = 1;
	Flag_SF = (*ptr_r16[reg] >> 15) & 1;
	Flag_PF = parity_check[*ptr_r16[reg] & 255];

	if (log_to_console) cout << "DEC " << reg16_name[reg] << " = " << (int)*ptr_r16[reg];

	Instruction_Pointer += 1;
}

void CMP_Reg_RM_8()		//  CMP Reg with R/M 8 bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Result = 0;

	if (log_to_console) cout << "CMP " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") with ";

	//определяем объект назначения и результат операции ADD
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		if (log_to_console) cout << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") = ";

		//складываем два регистра
		Result = *ptr_r8[byte2 & 7] - *ptr_r8[(byte2 >> 3) & 7];
		if (log_to_console) cout << (int)(Result & 255);
		Flag_AF = (((*ptr_r8[byte2 & 7] & 15) - (*ptr_r8[(byte2 >> 3) & 7] & 15)) >> 4) & 1;
		OF_Carry = ((*ptr_r8[byte2 & 7] & 0x7F) - (*ptr_r8[(byte2 >> 3) & 7] & 0x7F)) >> 7;
		//*ptr_r8[byte2 & 7] = Result;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
		Result = memory_2[New_Addr_32] - *ptr_r8[(byte2 >> 3) & 7];
		Flag_AF = (((memory_2[New_Addr_32] & 15) - (*ptr_r8[(byte2 >> 3) & 7] & 15)) >> 4) & 1;
		Flag_CF = Result >> 8;
		OF_Carry = ((memory_2[New_Addr_32] & 0x7F) - (*ptr_r8[(byte2 >> 3) & 7] & 0x7F)) >> 7;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		//memory_2[New_Addr_32] = Result;
		if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)(Result & 255);

		Instruction_Pointer += 2 + additional_IPs;
	}
}
void CMP_Reg_RM_16()	//  CMP Reg with R/M 16 bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint32 Result = 0;

	if (log_to_console) cout << "CMP " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") with ";

	//определяем объект назначения и результат операции ADD
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		Result = *ptr_r16[byte2 & 7] - *ptr_r16[(byte2 >> 3) & 7];
		if (log_to_console) cout << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") = " << (int)(Result & 0xFFFF);
		Flag_AF = (((*ptr_r16[byte2 & 7] & 15) - (*ptr_r16[(byte2 >> 3) & 7] & 15)) >> 4) & 1;
		OF_Carry = ((*ptr_r16[byte2 & 7] & 0x7FFF) - (*ptr_r16[(byte2 >> 3) & 7] & 0x7FFF)) >> 15;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		Result = *ptr_Src - *ptr_r16[(byte2 >> 3) & 7];
		Flag_AF = (((*ptr_Src & 15) - (*ptr_r16[(byte2 >> 3) & 7] & 15)) >> 4) & 1;
		OF_Carry = ((*ptr_Src & 0x7FFF) - (*ptr_r16[(byte2 >> 3) & 7] & 0x7FFF)) >> 15;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)(Result & 0xFFFF);

		Instruction_Pointer += 2 + additional_IPs;
	}
}
//доработать
void CMP_RM_Reg_8()		//  CMP R/M with Reg 8 bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Result = 0;

	//определяем 1-й операнд
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		Result = -*ptr_r8[byte2 & 7] + *ptr_r8[(byte2 >> 3) & 7];
		if (log_to_console) cout << "CMP " << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") with " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") = " << (int)(Result & 255);
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = (-(*ptr_r8[byte2 & 7] & 0x7F) + (*ptr_r8[(byte2 >> 3) & 7] & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((-(*ptr_r8[byte2 & 7] & 15) + (*ptr_r8[(byte2 >> 3) & 7] & 15)) >> 4) & 1;
		//*ptr_r8[(byte2 >> 3) & 7] = Result & 255;

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
		Result = -memory_2[New_Addr_32] + *ptr_r8[(byte2 >> 3) & 7];
		if (log_to_console) cout << "CMP M" << OPCODE_comment << " with " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") = " << (int)(Result & 255);
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = (-(memory_2[New_Addr_32] & 0x7F) + (*ptr_r8[(byte2 >> 3) & 7] & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((-(memory_2[New_Addr_32] & 15) + (*ptr_r8[(byte2 >> 3) & 7] & 15)) >> 4) & 1;
		//*ptr_r8[(byte2 >> 3) & 7] = Result & 255;

		Instruction_Pointer += 2 + additional_IPs;
	}
}
void CMP_RM_Reg_16()	//  CMP R/M with Reg 16 bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint32 Result = 0;

	//определяем 1-й операнд
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		Result = -*ptr_r16[byte2 & 7] + *ptr_r16[(byte2 >> 3) & 7];
		if (log_to_console) cout << "CMP " << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") with " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") = " << (int)(Result & 0xFFFF);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = (-(*ptr_r16[byte2 & 7] & 0x7FFF) + (*ptr_r16[(byte2 >> 3) & 7] & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((-(*ptr_r16[byte2 & 7] & 15) + (*ptr_r16[(byte2 >> 3) & 7] & 15)) >> 4) & 1;

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		Result = -*ptr_Src + *ptr_r16[(byte2 >> 3) & 7];
		if (log_to_console) cout << "CMP M" << OPCODE_comment << "(" << (int)*ptr_Src << ") with " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") = " << (int)(Result & 0xFFFF);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		OF_Carry = (-(*ptr_Src & 0x7FFF) + (*ptr_r16[(byte2 >> 3) & 7] & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((-(*ptr_Src & 15) + (*ptr_r16[(byte2 >> 3) & 7] & 15)) >> 4) & 1;

		Instruction_Pointer += 2 + additional_IPs;
	}
}
void CMP_IMM_with_ACC_8()		// CMP IMM  8bit - ACC
{
	uint16 Result = 0;
	uint8 imm = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];

	if (log_to_console) cout << "CMP IMM (" << (int)(imm) << ") with AL(" << (int)*ptr_AL << ") = ";

	Result = *ptr_AL - imm;
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
	
	uint16 imm = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory.read_2(Instruction_Pointer + 2 + *CS * 16) * 256;
	
	if (log_to_console) cout << "CMP IMM (" << (int)(imm) << ") with AX(" << (int)AX << ") = ";

	Result = AX - imm;
	Flag_CF = (Result >> 16) & 1;
	Flag_SF = (Result >> 15) & 1;
	OF_Carry = ((AX & 0x7FFF) - (imm & 0x7FFF)) >> 15;
	Flag_OF = Flag_CF ^ OF_Carry;
	if (Result & 0xFFFF) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result & 255];
	Flag_AF = (((AX & 15) - (imm & 15)) >> 4) & 1;
	if (log_to_console) cout << (int)(Result & 0xFFFF);

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
	uint8 high_AL = (AL >> 4) & 15;

	if ((*ptr_AL & 15) > 9)
	{
		*ptr_AL = *ptr_AL - 6;
		Flag_AF = 1;
	}
	else
	{
		if (Flag_AF)
		{
			*ptr_AL = *ptr_AL - 6;
		}
	}

	if ((*ptr_AL >> 4) > 9 || Flag_CF)
	{
		*ptr_AL = *ptr_AL - 0x60; // -6 к старшим битам
		Flag_CF = true;
	}
	if (*ptr_AL) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_SF = (*ptr_AL >> 7) & 1;
	Flag_PF = parity_check[*ptr_AL];
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
		exeption_0 = 1;
		Flag_ZF = true;  //недокументированное поведение
		Flag_SF = false; //недокументированное поведение
		Flag_PF = true;  //недокументированное поведение
		Flag_CF = 0;  //недокументированное поведение
		Flag_OF = 0;  //недокументированное поведение
		Instruction_Pointer += 2;
		if (log_to_console) cout << " [DIV0] ";
		return;
	}
	
	*ptr_AH = *ptr_AL / base;
	*ptr_AL = *ptr_AL % base;

	if (*ptr_AL) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_SF = ((AX >> 7) & 1);
	Flag_PF = parity_check[*ptr_AL];
	Flag_AF = 0;
	Flag_CF = 0;
	Flag_OF = 0;
	if (log_to_console) cout << (int)*ptr_AH << " + " << (int)*ptr_AL;
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
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];//второй байт
	uint16 disp = 0;		//смещение
	uint16 Src = 0;		//источник данных
	uint8 imm = 0;
	uint8 Result = 0;
	uint16 Result_16 = 0;
	uint8 rem; //остаток от деления
	bool old_SF = 0;

	switch ((byte2 >> 3) & 7)
	{
	case 0:  //TEST_IMM_8
	case 1:  //недокументированный алиас
		//определяем объект назначения и результат операции
		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			imm = memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF];
			if (log_to_console) cout << "TEST IMM(" << (int)imm << ") AND ";
			Result = *ptr_r8[byte2 & 7] & imm;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = (Result >> 7) & 1;
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result];
			if (log_to_console) cout << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") = " << (int)Result;
			Instruction_Pointer += 3;
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			imm = memory_2[(Instruction_Pointer + 2 + additional_IPs + *CS * 16) & 0xFFFFF];//IMM
			if (log_to_console) cout << "TEST IMM(" << (int)imm << ") AND ";
			Result = memory_2[New_Addr_32] & imm;
			if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)Result;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Instruction_Pointer += 3 + additional_IPs;
		}
		break;

	case 2:  //NOT(Invert) RM_8
		//определяем адрес или регистр
		if((byte2 >> 6) == 3)
		{
			// mod 11 - регистр 8bit
			if (log_to_console) cout << "Invert " << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") -> ";
			*ptr_r8[byte2 & 7] = ~(*ptr_r8[byte2 & 7]);
			if (log_to_console) cout << *ptr_r8[byte2 & 7];
			Instruction_Pointer += 2;
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			if (log_to_console) cout << "Invert M" << OPCODE_comment << "(" << (int)memory_2[New_Addr_32] << ") -> ";
			memory_2[New_Addr_32] = ~memory_2[New_Addr_32];
			if (log_to_console) cout << (int)memory_2[New_Addr_32];
			Instruction_Pointer += 2 + additional_IPs;
		}
		break;

	case 3: //NEG_8
		
		if (log_to_console) cout << "NEGATE ";
		if ((byte2 >> 6) == 3)
		{
			// mod 11 - регистр 8bit
			

			if (*ptr_r8[byte2 & 7]) Flag_CF = 1;
			else Flag_CF = 0;
			if (*ptr_r8[byte2 & 7] == 0x80) Flag_OF = 1;
			else Flag_OF = 0;
			*ptr_r8[byte2 & 7] = ~*ptr_r8[byte2 & 7] + 1;
			Flag_SF = (*ptr_r8[byte2 & 7] >> 7) & 1;
			if (*ptr_r8[byte2 & 7]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_r8[byte2 & 7]];
			if (*ptr_r8[byte2 & 7] & 15) Flag_AF = 1;
			else Flag_AF = 0;
			if (log_to_console) cout << reg8_name[byte2 & 7] << " = " << (int)*ptr_r8[byte2 & 7];
			Instruction_Pointer += 2;
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			if (memory_2[New_Addr_32]) Flag_CF = 1;
			else Flag_CF = 0;
			if (memory_2[New_Addr_32] == 0x80) Flag_OF = 1;
			else Flag_OF = 0;
			memory_2[New_Addr_32] = ~memory_2[New_Addr_32] + 1;
			Flag_SF = (memory_2[New_Addr_32] >> 7) & 1;
			if (memory_2[New_Addr_32]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[memory_2[New_Addr_32]];
			if (memory_2[New_Addr_32] & 15) Flag_AF = 1;
			else Flag_AF = 0;
			if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)(memory_2[New_Addr_32]);
			Instruction_Pointer += 2 + additional_IPs;
		}
		break;
		
	case 4: //MUL = Multiply (Unsigned)

		if (log_to_console) cout << "MUL AL(" << (int)*ptr_AL << ") x ";
		//определяем множитель
		if((byte2 >> 6) == 3)
		{
			// mod 11 - регистр 8bit
			Src = *ptr_r8[byte2 & 7];
			if (log_to_console) cout << reg8_name[byte2 & 7] << "(" << int(Src) << ") = ";
			additional_IPs = 0;
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			Src = memory_2[New_Addr_32];
			if (log_to_console) cout << "M" << OPCODE_comment << "=";
		}

		AX = *ptr_AL * Src;
		if (AX >> 8)
		{
			Flag_CF = true; Flag_OF = true; 
		}
		else 
		{
			Flag_CF = false; Flag_OF = false;
		}

		Flag_AF = 0; //undefined
		if (log_to_console) cout << (int)(AX);
		Instruction_Pointer += 2 + additional_IPs;
		break;

	case 5: //IMUL = Integer Multiply(Signed)
		if (log_to_console) cout << "IMUL AL(" << (int)*ptr_AL  << ") x ";
		//определяем множитель
		if((byte2 >> 6) == 3)
		{
			// mod 11 - регистр 8bit
			Src = *ptr_r8[byte2 & 7];
			if (log_to_console) cout << reg8_name[byte2 & 7] << "(" << int(Src) << ") = ";
			additional_IPs = 0;
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			Src = memory_2[New_Addr_32];
			if (log_to_console) cout << "M" << OPCODE_comment << " = ";
		}

		AX = ((__int8)*ptr_AL * (__int8)Src);
		if ((AX >> 7) == 0 || (AX >> 7) == 0b111111111)
		{
			Flag_CF = false; Flag_OF = false;
		}
		else
		{
			Flag_CF = true; Flag_OF = true; 
		}
		if (log_to_console) cout << (__int16)(AX);
		Instruction_Pointer += 2 + additional_IPs;
		break;
	
	case 6: //DIV = Divide (Unsigned)

		if (log_to_console) cout << "DIV AX(" << int(AX) << ") : ";
		//определяем делитель
		if((byte2 >> 6) == 3)
		{
			// mod 11 - регистр 8bit
			Src = *ptr_r8[byte2 & 7];
			if (log_to_console) cout << reg8_name[byte2 & 7] << "(" << int(Src) << ") = ";
			additional_IPs = 0;
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			Src = memory_2[New_Addr_32];
			if (log_to_console) cout << "M" << OPCODE_comment << "(" << (int)Src << ") = ";
		}

		//бросаем исключение при делении на 0
		if (Src == 0) {
			exeption_0 = 1;
			Instruction_Pointer += (2 + additional_IPs);
			if (log_to_console) cout << " [DIV0] ";
			return;
		}

		rem = AX % Src;
		Result_16 = AX / Src;

		//бросаем исключение при переполнении
		if (Result_16 > 0xFF) {
			exeption_0 = 1;
			Instruction_Pointer += (2 + additional_IPs);
			if (log_to_console) cout << " [OVERFLOW] ";
			return;
		}

		*ptr_AH = rem;
		*ptr_AL = Result_16;

		if (log_to_console) cout << (int)(Result_16) << " rem " << (int)rem;
		Instruction_Pointer += 2 + additional_IPs;
		break;

	case 7: //IDIV = Integer Divide (Signed)
		
		if (log_to_console) cout << "IDIV AX(" << setw(4) << (int)AX << ") : ";
		//определяем делитель
		if ((byte2 >> 6) == 3)
		{
			// mod 11 - регистр 8bit
			Src = *ptr_r8[byte2 & 7];
			if (log_to_console) cout << reg8_name[byte2 & 7] << "(" << int(Src) << ") = ";
			additional_IPs = 0;
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			Src = memory_2[New_Addr_32];
			if (log_to_console) cout << "M" << OPCODE_comment << "(" << (int)Src << ") = ";
		}

		//бросаем исключение при делении на 0
		if (Src == 0) {
			exeption_0 = 1;
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
		if (quotient > 127 || quotient < -128) {
			exeption_0 = 1;
			Instruction_Pointer += (2 + additional_IPs);
			if (log_to_console) cout << " [OVERFLOW] ";
			return;
		}

		rem = ((__int16)AX % (__int8)Src);
		*ptr_AH = rem;
		*ptr_AL = quotient;
		if (log_to_console) cout << setw(2) << (int)(AX & 255) << " rem " << (int)rem;
		Instruction_Pointer += 2 + additional_IPs;
		break;
	}
	
}
// TEST/NOT/NEG/MUL/IMUL/DIV/IDIV   16 bit
void Invert_RM_16()		
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];//второй байт
	uint16 disp = 0;		//смещение
	uint16 Result = 0;
	uint32 Result_32 = 0;
	additional_IPs = 0;
	uint16 rem = 0;   //остаток деления

	switch ((byte2 >> 3) & 7)
	{
	case 0:  //TEST_IMM_16
	case 1:  //Alias
		
		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			operand_RM_offset = Instruction_Pointer;
			operand_RM_offset++;
			operand_RM_offset++;
			*ptr_Src_L = memory_2[(operand_RM_offset + *CS * 16) & 0xFFFFF];
			operand_RM_offset++;
			*ptr_Src_H = memory_2[(operand_RM_offset + *CS * 16) & 0xFFFFF];
			if (log_to_console) cout << "TEST IMM[" << (int)*ptr_Src << "] AND ";
			Result = *ptr_r16[byte2 & 7] & Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			Flag_AF = 0; //undefined
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			if (log_to_console) cout << reg16_name[byte2 & 7] << " = " << (int)Result;
			Flag_PF = parity_check[Result & 255];
			Instruction_Pointer += 4;
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			operand_RM_offset = Instruction_Pointer;
			operand_RM_offset++;
			operand_RM_offset++;
			*ptr_Src_L = memory_2[(operand_RM_offset + additional_IPs + *CS * 16) & 0xFFFFF];
			operand_RM_offset++;
			*ptr_Src_H = memory_2[(operand_RM_offset + additional_IPs + *CS * 16) & 0xFFFFF];
			if (log_to_console) cout << "TEST IMM[" << (int)*ptr_Src << "] AND ";
			Result = (memory_2[New_Addr_32] + memory_2[New_Addr_32 + 1] * 256) & *ptr_Src;
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)Result;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			Flag_AF = 0; //undefined
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Instruction_Pointer += 4 + additional_IPs;
		}
		break;

	case 2: //NOT (Invert) 16 R/M

		//определяем адрес или регистр
		if ((byte2 >> 6) == 3)
		{
			// mod 11 - регистр 16bit
			*ptr_r16[byte2 & 7] = ~(*ptr_r16[byte2 & 7]);
			if (log_to_console) cout << "Invert " << reg16_name[byte2 & 7] << " = " << (int)*ptr_r16[byte2 & 7];
			Instruction_Pointer += 2;
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = ~memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			operand_RM_offset++;
			memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = ~memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			if (log_to_console) cout << "Invert M" << OPCODE_comment << " = " << (int)(memory_2[New_Addr_32] + memory_2[New_Addr_32 + 1] * 256);
			Instruction_Pointer += 2 + additional_IPs;
		}
		
		break;

	case 3:  //NEG 16 R/M

		if (log_to_console) cout << "NEGATE ";
		if ((byte2 >> 6) == 3)
		{
			// mod 11 - регистр 8bit

			if (*ptr_r16[byte2 & 7]) Flag_CF = 1;
			else Flag_CF = 0;
			if (*ptr_r16[byte2 & 7] == 0x80) Flag_OF = 1;
			else Flag_OF = 0;
			*ptr_r16[byte2 & 7] = ~*ptr_r16[byte2 & 7] + 1;
			Flag_SF = (*ptr_r16[byte2 & 7] >> 15) & 1;
			if (*ptr_r16[byte2 & 7]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_r16[byte2 & 7] & 255];
			if (*ptr_r16[byte2 & 7] & 15) Flag_AF = 1;
			else Flag_AF = 0;
			if (log_to_console) cout << reg16_name[byte2 & 7] << " = " << (int)*ptr_r16[byte2 & 7];
			Instruction_Pointer += 2;
		}
		else
		{
			mod_RM_3(byte2);
			//New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			operand_RM_offset++;
			*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			if (*ptr_Src) Flag_CF = 1;
			else Flag_CF = 0;
			if (*ptr_Src == 0x8000) Flag_OF = 1;
			else Flag_OF = 0;
			*ptr_Src = ~(*ptr_Src) + 1;
			memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = *ptr_Src_H;
			operand_RM_offset--;
			memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = *ptr_Src_L;
			Flag_SF = (*ptr_Src_H) >> 7;
			if (*ptr_Src) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_Src_L];
			if (*ptr_Src_L & 15) Flag_AF = 1;
			else Flag_AF = 0;
			if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)*ptr_Src;
			Instruction_Pointer += 2 + additional_IPs;
		}
		break;

	case 4: //MUL = Multiply (Unsigned)

		if (log_to_console) cout << "MUL AX(" << int(AX) << ") x ";
		//определяем множитель
		if ((byte2 >> 6) == 3)
		{
			// mod 11 - регистр 16bit
			Result_32 = AX * (*ptr_r16[byte2 & 7]);
			if (log_to_console) cout << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") = ";
			additional_IPs = 0;
		}
		else
		{
			mod_RM_3(byte2);
			//New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			operand_RM_offset++;
			*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			if (log_to_console) cout << "M" << OPCODE_comment << " = ";
			Result_32 = AX * (*ptr_Src);
		}
		
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
		Flag_AF = 0; //undefined
		if (log_to_console) cout << (uint32)(Result_32);
		Instruction_Pointer += 2 + additional_IPs;
		break;

	case 5: //IMUL = Integer Multiply(Signed)
		if (log_to_console) cout << "IMUL AX(" << __int16(AX) << ") * ";
		//определяем множитель
		if ((byte2 >> 6) == 3)
		{
			// mod 11 - регистр 16bit
			Src = *ptr_r16[byte2 & 7];
			Result_32 = ((__int16)AX * (__int16)Src);
			if (log_to_console) cout <<reg16_name[byte2 & 7] << "(" << __int16(Src) << ")=";
			Instruction_Pointer += 2;
		}
		else
		{
			mod_RM_3(byte2);
			//New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			operand_RM_offset++;
			*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			Result_32 = ((__int16)AX * (__int16)*ptr_Src);
			if (log_to_console) cout << "M" << OPCODE_comment << " = ";
			Instruction_Pointer += 2 + additional_IPs;
		}

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
		
		break;

	case 6: //DIV = Divide (Unsigned)

		if (log_to_console) cout << "DIV DXAX(" << int(DX * 256 * 256 + AX) << ") : ";
		//определяем делитель
		if ((byte2 >> 6) == 3)
		{
			// mod 11 - регистр 16bit
			Src = *ptr_r16[byte2 & 7];
			if (log_to_console) cout << reg16_name[byte2 & 7] << "(" << int(Src) << ") = ";
			Instruction_Pointer += 2;
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			operand_RM_offset++;
			*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			Src = *ptr_Src;
			if (log_to_console) cout << "M" << OPCODE_comment << " = ";
			Instruction_Pointer += 2 + additional_IPs;
		}

		//бросаем исключение при делении на 0
		if (Src == 0) {
			exeption_0 = 1;
			if (log_to_console) cout << " [DIV 0] ";
			Instruction_Pointer += 2 + additional_IPs;
			return;
		}

		Result_32 = ((uint32)(DX * 256 * 256 + AX) / Src);
		//бросаем исключение при переполнении
		if (Result_32 > 0xFFFF) {
			exeption_0 = 1;
			if (log_to_console) cout << " [OVERFLOW] ";
			Instruction_Pointer += 2 + additional_IPs;
			return;
		}

		rem = (uint32)(DX * 256 * 256 + AX) % Src; //остаток от деления
		AX = Result_32;
		DX = rem;
		if (log_to_console) cout << (int)(AX) << " rem " << (int)rem;
		
		break;

	case 7: //IDIV = Integer Divide (Signed)

		if (log_to_console) cout << "IDIV DXAX(" << setw(8) << (int)(DX * 256 * 256 + AX) << ") : ";
		//определяем делитель
		if ((byte2 >> 6) == 3)
		{
			// mod 11 - регистр 16bit
			Src = *ptr_r16[byte2 & 7];
			if (log_to_console) cout << reg16_name[byte2 & 7] << "(" << int(Src) << ") = ";
			additional_IPs = 0;
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			operand_RM_offset++;
			*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			Src = *ptr_Src;
			if (log_to_console) cout << "M" << OPCODE_comment << "(" << (int)Src << ") = ";
		}

		//бросаем исключение при делении на 0
		if (Src == 0) {
			exeption_0 = 1;
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
		if (quotient > 32767 || quotient < -32768) {
			exeption_0 = 1;
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
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 Result = 0;
	uint16 Src = 0;
	uint8 MSB = 0;
	additional_IPs = 0;

	switch ((byte2 >> 3) & 7)
	{
	case 0:  //ROL Rotate Left

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			Flag_CF = (*ptr_r8[byte2 & 7] >> 7) & 1;
			*ptr_r8[byte2 & 7] = (*ptr_r8[byte2 & 7] << 1) | Flag_CF;
			Flag_OF = (*ptr_r8[byte2 & 7] >> 7) ^ Flag_CF;
			if (log_to_console) cout << "ROL " << reg8_name[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			Flag_CF = (memory_2[New_Addr_32] >> 7) & 1;
			memory_2[New_Addr_32] = (memory_2[New_Addr_32] << 1) | Flag_CF;
			Flag_OF = (memory_2[New_Addr_32] >> 7) ^ Flag_CF;
			if (log_to_console) cout << "ROL M" << OPCODE_comment << "";
		}
		break;

	case 1:  //ROR = Rotate Right

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			Flag_CF = *ptr_r8[byte2 & 7] & 1;
			*ptr_r8[byte2 & 7] = (*ptr_r8[byte2 & 7] >> 1) | (Flag_CF * 0x80);
			Flag_OF = !parity_check[*ptr_r8[byte2 & 7] & 0b11000000];
			if (log_to_console) cout << "ROR " << reg8_name[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			Flag_CF = memory_2[New_Addr_32] & 1;
			memory_2[New_Addr_32] = (memory_2[New_Addr_32] >> 1) | (Flag_CF * 0x80);
			Flag_OF = !parity_check[memory_2[New_Addr_32] & 0b11000000];
			if (log_to_console) cout << "ROR M" << OPCODE_comment << "";
		}
		break;
	
	case 2:  //RCL Rotate Left throught carry

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			Src = (*ptr_r8[byte2 & 7] << 1) | Flag_CF;
			Flag_CF = (Src >> 8) & 1;
			*ptr_r8[byte2 & 7] = Src;
			Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
			if (log_to_console) cout << "RCL " << reg8_name[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			Src = (memory_2[New_Addr_32] << 1) | Flag_CF;
			Flag_CF = (Src >> 8) & 1;
			memory_2[New_Addr_32] = Src;
			Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
			if (log_to_console) cout << "RCL M" << OPCODE_comment << "";
		}
		break;

	case 3:  //RCR = Rotate Right throught carry

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			Src = (*ptr_r8[byte2 & 7] >> 1) | (Flag_CF << 7);
			Flag_CF = *ptr_r8[byte2 & 7] & 1;
			*ptr_r8[byte2 & 7] = Src;
			Flag_OF = !parity_check[Src & 0b11000000];
			if (log_to_console) cout << "RCR " << reg8_name[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			Src = (memory_2[New_Addr_32] >> 1) | (Flag_CF << 7);
			Flag_CF = memory_2[New_Addr_32] & 1;
			memory_2[New_Addr_32] = Src;
			Flag_OF = !parity_check[Src & 0b11000000];
			if (log_to_console) cout << "RCR M" << OPCODE_comment << "";
		}
		break;
	
	case 4:  //Shift Left (SHL/SAL)

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			Src = *ptr_r8[byte2 & 7] << 1;
			*ptr_r8[byte2 & 7] = Src;
			Flag_CF = (Src >> 8) & 1;
			Flag_SF = (Src >> 7) & 1;
			Flag_OF = !parity_check[Src >> 7];
			if (*ptr_r8[byte2 & 7]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_r8[byte2 & 7]];
			if (log_to_console) cout << "Shift left " << reg8_name[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			Src = memory_2[New_Addr_32] << 1;
			memory_2[New_Addr_32] = Src;
			Flag_CF = (Src >> 8) & 1;
			Flag_SF = (Src >> 7) & 1;
			Flag_OF = !parity_check[Src >> 7];
			if (memory_2[New_Addr_32]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[memory_2[New_Addr_32]];
			if (log_to_console) cout << "Shift left M" << OPCODE_comment << "";
		}
		break;
			 
	case 5:  //Shift Right (SHR)

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			Flag_CF = *ptr_r8[byte2 & 7] & 1;
			Src = *ptr_r8[byte2 & 7] >> 1;
			*ptr_r8[byte2 & 7] = Src;
			Flag_SF = 0;
			Flag_OF = Src >> 6;
			if (*ptr_r8[byte2 & 7]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_r8[byte2 & 7]];
			if (log_to_console) cout << "Shift(SHR) right " << reg8_name[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			Flag_CF = memory_2[New_Addr_32] & 1;
			Src = memory_2[New_Addr_32] >> 1;
			memory_2[New_Addr_32] = Src;
			Flag_SF = 0;
			Flag_OF = Src >> 6;
			if (memory_2[New_Addr_32]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[memory_2[New_Addr_32]];
			if (log_to_console) cout << "Shift(SHR) right M" << OPCODE_comment << "";
		}
		break;
		
	case 6: //SETMO byte R/M

		if (log_to_console) cout << "SETMO_B ";
		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			*ptr_r8[byte2 & 7] = 0xFF;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = true;
			Flag_ZF = false;
			Flag_PF = true;
			if (log_to_console) cout << reg8_name[byte2 & 7] << " = " << (int)*ptr_r8[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			memory_2[New_Addr_32] = 0xFF;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = true;
			Flag_ZF = false;
			Flag_PF = true;
			if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)memory_2[New_Addr_32];
		}

		Flag_AF = 0;
		break;

	case 7:  //Shift Arithm Right (SAR)

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			Flag_CF = *ptr_r8[byte2 & 7] & 1;
			MSB = *ptr_r8[byte2 & 7] & 128;
			*ptr_r8[byte2 & 7] = (*ptr_r8[byte2 & 7] >> 1) | MSB;
			Flag_SF = ((*ptr_r8[byte2 & 7] >> 7) & 1);
			Flag_OF = !parity_check[*ptr_r8[byte2 & 7] & 0b11000000];
			if (*ptr_r8[byte2 & 7]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_r8[byte2 & 7]];
			if (log_to_console) cout << "Shift(SAR) right " << reg8_name[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			Flag_CF = memory_2[New_Addr_32] & 1;
			MSB = memory_2[New_Addr_32] & 128;
			memory_2[New_Addr_32] = (memory_2[New_Addr_32] >> 1) | MSB;
			Flag_SF = ((memory_2[New_Addr_32] >> 7) & 1);
			Flag_OF = !parity_check[memory_2[New_Addr_32] & 0b11000000];
			if (memory_2[New_Addr_32]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[memory_2[New_Addr_32]];
			if (log_to_console) cout << "Shift(SAR) right M" << OPCODE_comment << "";
		}
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void SHL_ROT_16()			// Shift Logical / Arithmetic Left / 16bit / once
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint32 Src = 0;
	uint16 MSB = 0;
	additional_IPs = 0;
	uint16 Result = 0;

	switch ((byte2 >> 3) & 7)
	{

	case 0:  //ROL Rotate Left
			
		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			Flag_CF = (*ptr_r16[byte2 & 7] >> 15) & 1;
			*ptr_r16[byte2 & 7] = (*ptr_r16[byte2 & 7] << 1) | Flag_CF;
			Flag_OF = (*ptr_r16[byte2 & 7] >> 15) ^ Flag_CF;
			if (log_to_console) cout << "ROL " << reg16_name[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[New_Addr_32];
			*ptr_Src_H = memory_2[New_Addr_32 + 1];
			Flag_CF = (*ptr_Src >> 15) & 1;
			*ptr_Src = (*ptr_Src << 1) | Flag_CF;
			Flag_OF = (*ptr_Src >> 15) ^ Flag_CF;
			memory_2[New_Addr_32 + 1] = *ptr_Src_H;
			memory_2[New_Addr_32] = *ptr_Src_L;
			if (log_to_console) cout << "ROL M" << OPCODE_comment << "";
		}
		break;
	
	case 1:  //ROR = Rotate Right
	
		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			Flag_CF = *ptr_r16[byte2 & 7] & 1;
			*ptr_r16[byte2 & 7] = (*ptr_r16[byte2 & 7] >> 1) | (Flag_CF * 0x8000);
			Flag_OF = !parity_check[(*ptr_r16[byte2 & 7] >> 8) & 0b11000000];
			if (log_to_console) cout << "ROR " << reg16_name[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[New_Addr_32];
			*ptr_Src_H = memory_2[New_Addr_32 + 1];
			Flag_CF = *ptr_Src & 1;
			*ptr_Src = (*ptr_Src >> 1) | (Flag_CF * 0x8000);
			memory_2[New_Addr_32 + 1] = *ptr_Src_H;
			memory_2[New_Addr_32] = *ptr_Src_L;
			Flag_OF = !parity_check[(*ptr_Src >> 8) & 0b11000000];
			if (log_to_console) cout << "ROR M" << OPCODE_comment << "";
		}
		break;
	
	case 2:  //RCL Rotate Left throught carry
	
		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			Src = (*ptr_r16[byte2 & 7] << 1) | Flag_CF;
			Flag_CF = (Src >> 16) & 1;
			*ptr_r16[byte2 & 7] = Src;
			Flag_OF = !parity_check[(Src >> 9) & 0b11000000];
			if (log_to_console) cout << "RCL " << reg16_name[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[New_Addr_32];
			*ptr_Src_H = memory_2[New_Addr_32 + 1];
			Src = (*ptr_Src << 1) | Flag_CF;
			Flag_CF = (Src >> 16) & 1;
			*ptr_Src = Src;
			memory_2[New_Addr_32 + 1] = *ptr_Src_H;
			memory_2[New_Addr_32] = *ptr_Src_L;
			Flag_OF = !parity_check[(Src >> 9) & 0b11000000];
			if (log_to_console) cout << "RCL M" << OPCODE_comment << "";
		}
		break;
	
	case 3:  //RCR = Rotate Right throught carry
	
		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			Src = (*ptr_r16[byte2 & 7] >> 1) | (Flag_CF << 15);
			Flag_CF = *ptr_r16[byte2 & 7] & 1;
			*ptr_r16[byte2 & 7] = Src;
			Flag_OF = !parity_check[(Src >> 8) & 0b11000000];
			if (log_to_console) cout << "RCR " << reg16_name[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[New_Addr_32];
			*ptr_Src_H = memory_2[New_Addr_32 + 1];
			Src = (*ptr_Src >> 1) | (Flag_CF << 15);
			Flag_CF = *ptr_Src & 1;
			*ptr_Src = Src;
			memory_2[New_Addr_32 + 1] = *ptr_Src_H;
			memory_2[New_Addr_32] = *ptr_Src_L;
			Flag_OF = !parity_check[(Src >> 8) & 0b11000000];
			if (log_to_console) cout << "RCR M" << OPCODE_comment << "";
		}
		break;
	
	case 4:  //Shift Left

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			Src = *ptr_r16[byte2 & 7] << 1;
			*ptr_r16[byte2 & 7] = Src;
			Flag_CF = (Src >> 16) & 1;
			Flag_SF = (Src >> 15) & 1;
			Flag_OF = !parity_check[Src >> 15];
			if (*ptr_r16[byte2 & 7]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_r16[byte2 & 7] & 255];
			if (log_to_console) cout << "Shift left " << reg16_name[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[New_Addr_32];
			*ptr_Src_H = memory_2[New_Addr_32 + 1];
			Src = *ptr_Src << 1;
			*ptr_Src = Src;
			memory_2[New_Addr_32 + 1] = *ptr_Src_H;
			memory_2[New_Addr_32] = *ptr_Src_L;
			Flag_CF = (Src >> 16) & 1;
			Flag_SF = (Src >> 15) & 1;
			Flag_OF = !parity_check[Src >> 15];
			if (*ptr_Src) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_Src_L];
			if (log_to_console) cout << "Shift left M" << OPCODE_comment << "";
		}
		break;

	case 5:  //Shift right (SHR)

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			Flag_CF = *ptr_r16[byte2 & 7] & 1;
			Src = *ptr_r16[byte2 & 7] >> 1;
			*ptr_r16[byte2 & 7] = Src;
			Flag_SF = 0;
			Flag_OF = Src >> 14;
			if (*ptr_r16[byte2 & 7]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_r16[byte2 & 7] & 255];
			if (log_to_console) cout << "Shift(SHR) right " << reg16_name[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[New_Addr_32];
			*ptr_Src_H = memory_2[New_Addr_32 + 1];
			Flag_CF = *ptr_Src & 1;
			Src = *ptr_Src >> 1;
			*ptr_Src = Src;
			Flag_SF = 0;
			Flag_OF = Src >> 14;
			if (*ptr_Src) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_Src_L];
			memory_2[New_Addr_32 + 1] = *ptr_Src_H;
			memory_2[New_Addr_32] = *ptr_Src_L;
			if (log_to_console) cout << "Shift(SHR) right M" << OPCODE_comment << "";
		}
		break;

	case 6:  //SETMO 

		if (log_to_console) cout << "SETMO WORD ";
		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			*ptr_r16[byte2 & 7] = 0xFFFF;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = true;
			Flag_ZF = false;
			Flag_PF = true;
			if (log_to_console) cout << reg16_name[byte2 & 7] << " = " << (int)*ptr_r16[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			memory_2[New_Addr_32] = 0xFF;
			memory_2[New_Addr_32 + 1] = 0xFF;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = true;
			Flag_ZF = false;
			Flag_PF = true;
			if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)0xFFFF;
		}

		Flag_AF = 0;
		break;
	
	case 7:  //Shift arithm right (SAR)

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			Flag_CF = *ptr_r16[byte2 & 7] & 1;
			MSB = *ptr_r16[byte2 & 7] & 0x8000;
			*ptr_r16[byte2 & 7] = (*ptr_r16[byte2 & 7] >> 1) | MSB;
			Flag_SF = ((*ptr_r16[byte2 & 7] >> 15) & 1);
			Flag_OF = !parity_check[(*ptr_r16[byte2 & 7] >> 8) & 0b11000000];
			if (*ptr_r16[byte2 & 7]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_r16[byte2 & 7] & 255];
			if (log_to_console) cout << "Shift(SAR) right " << reg16_name[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[New_Addr_32];
			*ptr_Src_H = memory_2[New_Addr_32 + 1];
			Flag_CF = *ptr_Src & 1;
			MSB = *ptr_Src & 0x8000;
			*ptr_Src = (*ptr_Src >> 1) | MSB;
			Flag_SF = ((*ptr_Src >> 15) & 1);
			Flag_OF = !parity_check[(*ptr_Src >> 8) & 0b11000000];
			if (*ptr_Src) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_Src & 255];
			memory_2[New_Addr_32 + 1] = *ptr_Src_H;
			memory_2[New_Addr_32] = *ptr_Src_L;
			if (log_to_console) cout << "Shift(SAR) right M" << OPCODE_comment << "";
		}
		break;
		
	default:
		cout << "unknown OP = " << memory.read_2(Instruction_Pointer + *CS * 16);
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void SHL_ROT_8_mult()		// Shift Logical / Arithmetic Left / 8bit / CL
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Src = 0;
	uint16 Result = 0;
	uint8 repeats = *ptr_CL;
	

	uint8 MSB = 0;
	additional_IPs = 0;

	switch ((byte2 >> 3) & 7)
	{
	case 0:  //ROL Rotate Left

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = (*ptr_r8[byte2 & 7] >> 7) & 1;
				*ptr_r8[byte2 & 7] = (*ptr_r8[byte2 & 7] << 1) | Flag_CF;
			}
			if (repeats == 1) Flag_OF = (*ptr_r8[byte2 & 7] >> 7) ^ Flag_CF;
			if (log_to_console) cout << "ROL " << reg8_name[byte2 & 7] << " " << (int)repeats << " times";
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = (memory_2[New_Addr_32] >> 7) & 1;
				memory_2[New_Addr_32] = (memory_2[New_Addr_32] << 1) | Flag_CF;
			}
			if (repeats == 1) Flag_OF = (memory_2[New_Addr_32] >> 7) ^ Flag_CF;
			if (log_to_console) cout << "ROL M" << OPCODE_comment << " " << (int)repeats << " times";
		}
		break;

	case 1:  //ROR = Rotate Right

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = *ptr_r8[byte2 & 7] & 1;
				*ptr_r8[byte2 & 7] = (*ptr_r8[byte2 & 7] >> 1) | (Flag_CF * 0x80);
			}
			if (repeats == 1) Flag_OF = !parity_check[*ptr_r8[byte2 & 7] & 0b11000000];
			if (log_to_console) cout << "ROR " << reg8_name[byte2 & 7] << " " << (int)repeats << " times";;
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = memory_2[New_Addr_32] & 1;
				memory_2[New_Addr_32] = (memory_2[New_Addr_32] >> 1) | (Flag_CF * 0x80);
			}
			if (repeats == 1) Flag_OF = !parity_check[memory_2[New_Addr_32] & 0b11000000];
			if (log_to_console) cout << "ROR M" << OPCODE_comment << " " << (int)repeats << " times";
		}
		break;

	case 2:  //RCL Rotate Left throught carry

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			for (int i = 0; i < repeats; i++)
			{
				Src = (*ptr_r8[byte2 & 7] << 1) | Flag_CF;
				Flag_CF = (Src >> 8) & 1;
				*ptr_r8[byte2 & 7] = Src;
			}
			if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
			if (log_to_console) cout << "RCL " << reg8_name[byte2 & 7] << " " << (int)repeats << " times";
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			for (int i = 0; i < repeats; i++)
			{
				Src = (memory_2[New_Addr_32] << 1) | Flag_CF;
				Flag_CF = (Src >> 8) & 1;
				memory_2[New_Addr_32] = Src;
			}
			if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
			if (log_to_console) cout << "RCL M" << OPCODE_comment << " " << (int)repeats << " times";
		}
		break;

	case 3:  //RCR = Rotate Right throught carry

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			for (int i = 0; i < repeats; i++)
			{
				Src = (*ptr_r8[byte2 & 7] >> 1) | (Flag_CF << 7);
				Flag_CF = *ptr_r8[byte2 & 7] & 1;
				*ptr_r8[byte2 & 7] = Src;
			}
			if (repeats == 1) Flag_OF = !parity_check[Src & 0b11000000];
			if (log_to_console) cout << "RCR " << reg8_name[byte2 & 7] << " " << (int)repeats << " times";
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			for (int i = 0; i < repeats; i++)
			{
				Src = (memory_2[New_Addr_32] >> 1) | (Flag_CF << 7);
				Flag_CF = memory_2[New_Addr_32] & 1;
				memory_2[New_Addr_32] = Src;
			}
			if (repeats == 1) Flag_OF = !parity_check[Src & 0b11000000];
			if (log_to_console) cout << "RCR M" << OPCODE_comment << " " << (int)repeats << " times";
		}
		break;

	case 4:  //Shift Left
		
		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			if (!repeats) break;
			for (int i = 0; i < repeats; i++)
			{
				Src = *ptr_r8[byte2 & 7] << 1;
				*ptr_r8[byte2 & 7] = Src;
			}
			Flag_CF = (Src >> 8) & 1;
			Flag_SF = (Src >> 7) & 1;
			if (repeats == 1) Flag_OF = !parity_check[Src >> 7];
			if (*ptr_r8[byte2 & 7]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_r8[byte2 & 7]];
			if (log_to_console) cout << "Shift left " << reg8_name[byte2 & 7] << " " << (int)repeats << " times";
		}
		else
		{
			mod_RM_3(byte2);
			if (!repeats) break;
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			for (int i = 0; i < repeats; i++)
			{
				Src = memory_2[New_Addr_32] << 1;
				memory_2[New_Addr_32] = Src;
			}
			Flag_CF = (Src >> 8) & 1;
			Flag_SF = (Src >> 7) & 1;
			if (repeats == 1) Flag_OF = !parity_check[Src >> 7];
			if (memory_2[New_Addr_32]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[memory_2[New_Addr_32]];
			if (log_to_console) cout << "Shift left M" << OPCODE_comment << " " << (int)repeats << " times";
		}
		break;

	case 5:  //Shift Right (SHR)

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			if (!repeats) break;
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = *ptr_r8[byte2 & 7] & 1;
				Src = *ptr_r8[byte2 & 7] >> 1;
				*ptr_r8[byte2 & 7] = Src;
			}
			Flag_SF = 0;
			if (repeats == 1) Flag_OF = Src >> 6;
			if (*ptr_r8[byte2 & 7]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_r8[byte2 & 7]];
			if (log_to_console) cout << "Shift(SHR) right AL" << " " << (int)repeats << " times";
		}
		else
		{
			mod_RM_3(byte2);
			if (!repeats) break;
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = memory_2[New_Addr_32] & 1;
				Src = memory_2[New_Addr_32] >> 1;
				memory_2[New_Addr_32] = Src;
			}
			Flag_SF = 0;
			if (repeats == 1) Flag_OF = Src >> 6;
			if (memory_2[New_Addr_32]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[memory_2[New_Addr_32]];
			if (log_to_console) cout << "Shift(SHR) right M" << OPCODE_comment << " " << (int)repeats << " times";
		}
		break;

	case 6: //SETMOC byte R/M

		if (log_to_console) cout << "SETMO_B ";
		
		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			if (!*ptr_CL) break;
			*ptr_r8[byte2 & 7] = 0xFF;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = true;
			Flag_ZF = false;
			Flag_PF = true;
			if (log_to_console) cout << reg8_name[byte2 & 7] << " = " << (int)*ptr_r8[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			if (!*ptr_CL) break;
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			memory_2[New_Addr_32] = 0xFF;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = true;
			Flag_ZF = false;
			Flag_PF = true;
			if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)memory_2[New_Addr_32];
		}

		Flag_AF = 0;
		break;

	case 7:  //Shift Right (SAR)

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			if (!repeats) break;
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = *ptr_r8[byte2 & 7] & 1;
				MSB = *ptr_r8[byte2 & 7] & 128;
				*ptr_r8[byte2 & 7] = (*ptr_r8[byte2 & 7] >> 1) | MSB;
			}
			Flag_SF = ((*ptr_r8[byte2 & 7] >> 7) & 1);
			if (repeats == 1) Flag_OF = !parity_check[*ptr_r8[byte2 & 7] & 0b11000000];
			if (*ptr_r8[byte2 & 7]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_r8[byte2 & 7]];
			if (log_to_console) cout << "Shift(SAR) right " << reg8_name[byte2 & 7] << " " << (int)repeats << " times";;
		}
		else
		{
			mod_RM_3(byte2);
			if (!repeats) break;
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = memory_2[New_Addr_32] & 1;
				MSB = memory_2[New_Addr_32] & 128;
				memory_2[New_Addr_32] = (memory_2[New_Addr_32] >> 1) | MSB;
			}
			Flag_SF = ((memory_2[New_Addr_32] >> 7) & 1);
			if (repeats == 1) Flag_OF = !parity_check[memory_2[New_Addr_32] & 0b11000000];
			if (memory_2[New_Addr_32]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[memory_2[New_Addr_32]];
			if (log_to_console) cout << "Shift(SAR) right M" << OPCODE_comment << " " << (int)repeats << " times";
		}
		break;
	
	default:
		cout << "unknown OP = " << memory.read_2(Instruction_Pointer + *CS * 16);
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void SHL_ROT_16_mult()		// Shift Logical / Arithmetic Left / 16bit / CL
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint32 Src = 0;
	uint32 Result = 0;
	uint8 repeats = *ptr_CL;
	uint16 MSB = 0;
	additional_IPs = 0;

	switch ((byte2 >> 3) & 7)
	{
	case 0:  //ROL Rotate Left

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр 
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = (*ptr_r16[byte2 & 7] >> 15) & 1;
				*ptr_r16[byte2 & 7] = (*ptr_r16[byte2 & 7] << 1) | Flag_CF;
			}
			if (repeats == 1) Flag_OF = (*ptr_r16[byte2 & 7] >> 15) ^ Flag_CF;
			if (log_to_console) cout << "ROL " << reg16_name[byte2 & 7] << " " << (int)repeats << " times";;
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[New_Addr_32];
			*ptr_Src_H = memory_2[New_Addr_32 + 1];
			
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = (*ptr_Src >> 15) & 1;
				*ptr_Src = (*ptr_Src << 1) | Flag_CF;
			}
			if (repeats == 1) Flag_OF = (*ptr_Src >> 15) ^ Flag_CF;
			memory_2[New_Addr_32 + 1] = *ptr_Src_H;
			memory_2[New_Addr_32] = *ptr_Src_L;
			if (log_to_console) cout << "ROL M" << OPCODE_comment << " " << (int)repeats << " times";
		}
		break;

	case 1:  //ROR = Rotate Right
		
		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = *ptr_r16[byte2 & 7] & 1;
				*ptr_r16[byte2 & 7] = (*ptr_r16[byte2 & 7] >> 1) | (Flag_CF * 0x8000);
			}
			if (repeats == 1) Flag_OF = !parity_check[(*ptr_r16[byte2 & 7] >> 8) & 0b11000000];
			if (log_to_console) cout << "ROR " << reg16_name[byte2 & 7] << " " << (int)repeats << " times";
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			operand_RM_offset++;
			New_Addr_32_2 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[New_Addr_32];
			*ptr_Src_H = memory_2[New_Addr_32_2];
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = *ptr_Src & 1;
				*ptr_Src = (*ptr_Src >> 1) | (Flag_CF * 0x8000);
			}
			memory_2[New_Addr_32_2] = *ptr_Src_H;
			memory_2[New_Addr_32] = *ptr_Src_L;
			if (repeats == 1) Flag_OF = !parity_check[(*ptr_Src >> 8) & 0b11000000];
			if (log_to_console) cout << "ROR M" << OPCODE_comment << " " << (int)repeats << " times";
		}
		break;

	case 2:  //RCL Rotate Left throught carry
		
		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			for (int i = 0; i < repeats; i++)
			{
				Src = (*ptr_r16[byte2 & 7] << 1) | Flag_CF;
				Flag_CF = (Src >> 16) & 1;
				*ptr_r16[byte2 & 7] = Src;
			}
			if (repeats == 1) Flag_OF = !parity_check[(Src >> 9) & 0b11000000];
			if (log_to_console) cout << "RCL " << reg16_name[byte2 & 7] << " " << (int)repeats << " times";
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[New_Addr_32];
			*ptr_Src_H = memory_2[New_Addr_32 + 1];
			for (int i = 0; i < repeats; i++)
			{
				Src = (*ptr_Src << 1) | Flag_CF;
				Flag_CF = (Src >> 16) & 1;
				*ptr_Src = Src;
			}
			memory_2[New_Addr_32 + 1] = *ptr_Src_H;
			memory_2[New_Addr_32] = *ptr_Src_L;
			if (repeats == 1) Flag_OF = !parity_check[(Src >> 9) & 0b11000000];
			if (log_to_console) cout << "RCL M" << OPCODE_comment << " " << (int)repeats << " times";
		}
		break;

	case 3:  //RCR = Rotate Right throught carry

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			for (int i = 0; i < repeats; i++)
			{
				Src = (*ptr_r16[byte2 & 7] >> 1) | (Flag_CF << 15);
				Flag_CF = *ptr_r16[byte2 & 7] & 1;
				*ptr_r16[byte2 & 7] = Src;
			}
			if (repeats == 1) Flag_OF = !parity_check[(Src >> 8) & 0b11000000];
			if (log_to_console) cout << "RCR " << reg16_name[byte2 & 7] << " " << (int)repeats << " times";
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[New_Addr_32];
			*ptr_Src_H = memory_2[New_Addr_32 + 1];
			for (int i = 0; i < repeats; i++)
			{
				Src = (*ptr_Src >> 1) | (Flag_CF << 15);
				Flag_CF = *ptr_Src & 1;
				*ptr_Src = Src;
			}
			memory_2[New_Addr_32 + 1] = *ptr_Src_H;
			memory_2[New_Addr_32] = *ptr_Src_L;
			if (repeats == 1) Flag_OF = !parity_check[(Src >> 8) & 0b11000000];
			if (log_to_console) cout << "RCR M" << OPCODE_comment << " " << (int)repeats << " times";
		}
		break;
	
	case 4:  //Shift Left

		
		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			if (!repeats) break;
			for (int i = 0; i < repeats; i++)
			{
				Src = *ptr_r16[byte2 & 7] << 1;
				*ptr_r16[byte2 & 7] = Src;
			}
			Flag_CF = (Src >> 16) & 1;
			Flag_SF = (Src >> 15) & 1;
			if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
			if (*ptr_r16[byte2 & 7]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_r16[byte2 & 7] & 255];
			if (log_to_console) cout << "Shift left " << reg16_name[byte2 & 7] << " " << (int)repeats << " times";
		}
		else
		{
			mod_RM_3(byte2);
			if (!repeats) break;
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[New_Addr_32];
			*ptr_Src_H = memory_2[New_Addr_32 + 1];
			for (int i = 0; i < repeats; i++)
			{
				Src = *ptr_Src << 1;
				*ptr_Src = Src;
			}
			memory_2[New_Addr_32 + 1] = *ptr_Src_H;
			memory_2[New_Addr_32] = *ptr_Src_L;
			Flag_CF = (Src >> 16) & 1;
			Flag_SF = (Src >> 15) & 1;
			if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
			if (*ptr_Src) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_Src_L];
			if (log_to_console) cout << "Shift left M" << OPCODE_comment << " " << (int)repeats << " times";
		}
		break;

	case 5:  //Shift right (SHR)
	
		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			if (!repeats) break;
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = *ptr_r16[byte2 & 7] & 1;
				Src = *ptr_r16[byte2 & 7] >> 1;
				*ptr_r16[byte2 & 7] = Src;
			}
			Flag_SF = 0;
			if (repeats == 1)Flag_OF = Src >> 14;
			if (*ptr_r16[byte2 & 7]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_r16[byte2 & 7] & 255];
			if (log_to_console) cout << "Shift(SHR) right " << reg16_name[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			if (!repeats) break;
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[New_Addr_32];
			operand_RM_offset++;
			New_Addr_32_2 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_H = memory_2[New_Addr_32_2];
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = *ptr_Src & 1;
				Src = *ptr_Src >> 1;
				*ptr_Src = Src;
			}
			Flag_SF = 0;
			if (repeats == 1) Flag_OF = Src >> 14;
			if (*ptr_Src) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_Src_L];
			memory_2[New_Addr_32_2] = *ptr_Src_H;
			memory_2[New_Addr_32] = *ptr_Src_L;
			if (log_to_console) cout << "Shift(SHR) right M" << OPCODE_comment << "";
		}
		break;

	case 6:  //SETMOC word R/M
		
		if (log_to_console) cout << "SETMOC WORD ";
		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			if (*ptr_CL == 0) break;
			*ptr_r16[byte2 & 7] = 0xFFFF;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = true;
			Flag_ZF = false;
			Flag_PF = true;
			if (log_to_console) cout << reg16_name[byte2 & 7] << " = " << (int)*ptr_r16[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			if (*ptr_CL == 0) break;
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			memory_2[New_Addr_32] = 0xFF;
			memory_2[New_Addr_32 + 1] = 0xFF;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = true;
			Flag_ZF = false;
			Flag_PF = true;
			if (log_to_console) cout << "M" << OPCODE_comment << " = " << (int)0xFFFF;
		}

		Flag_AF = 0;
		break;

	case 7:  //Shift right (SAR)

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			if (!repeats) break;
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = *ptr_r16[byte2 & 7] & 1;
				MSB = *ptr_r16[byte2 & 7] & 0x8000;
				*ptr_r16[byte2 & 7] = (*ptr_r16[byte2 & 7] >> 1) | MSB;
			}
			Flag_SF = ((*ptr_r16[byte2 & 7] >> 15) & 1);
			if (repeats == 1) Flag_OF = !parity_check[(*ptr_r16[byte2 & 7] >> 8) & 0b11000000];
			if (*ptr_r16[byte2 & 7]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_r16[byte2 & 7] & 255];
			if (log_to_console) cout << "Shift(SAR) right " << reg16_name[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			if (!repeats) break;
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[New_Addr_32];
			*ptr_Src_H = memory_2[New_Addr_32 + 1];
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = *ptr_Src & 1;
				MSB = *ptr_Src & 0x8000;
				*ptr_Src = (*ptr_Src >> 1) | MSB;
			}
			Flag_SF = ((*ptr_Src >> 15) & 1);
			if (repeats == 1) Flag_OF = !parity_check[(*ptr_Src >> 8) & 0b11000000];
			if (*ptr_Src) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_Src & 255];
			memory_2[New_Addr_32 + 1] = *ptr_Src_H;
			memory_2[New_Addr_32] = *ptr_Src_L;
			if (log_to_console) cout << "Shift(SAR) right M" << OPCODE_comment << "";
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
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Result = 0;

	if (log_to_console) cout << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") ";

	//определяем объект назначения и результат операции ADD
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		if (log_to_console) cout << " AND " << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") = ";

		//складываем два регистра
		Result = *ptr_r8[(byte2 >> 3) & 7] & *ptr_r8[byte2 & 7];
		if (log_to_console) cout << (int)(Result & 255);
		*ptr_r8[byte2 & 7] = Result;
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
		Result = memory_2[New_Addr_32] & *ptr_r8[(byte2 >> 3) & 7];
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		memory_2[New_Addr_32] = Result;
		if (log_to_console) cout << " AND M" << OPCODE_comment << " = " << (int)(Result & 255);

		Instruction_Pointer += 2 + additional_IPs;
	}
}
void AND_RM_16()			// AND R + R/M -> R/M 16bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint32 Result = 0;

	if (log_to_console) cout << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") ";

	//определяем объект назначения и результат операции ADD
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		Result = *ptr_r16[(byte2 >> 3) & 7] & *ptr_r16[byte2 & 7];
		if (log_to_console) cout << " AND " << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") = " << (int)(Result & 0xFFFF);
		*ptr_r16[byte2 & 7] = Result & 0xFFFF;
		Flag_CF = 0;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = 0;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		Result = *ptr_Src & *ptr_r16[(byte2 >> 3) & 7];
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = Result >> 8;
		operand_RM_offset--;
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = Result;
		Flag_CF = 0;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = 0;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " AND M" << OPCODE_comment << " = " << (int)(Result & 0xFFFF);

		Instruction_Pointer += 2 + additional_IPs;
	}
}
void AND_RM_R_8()			// AND R + R/M -> R 8bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Result = 0;

	//определяем 1-й операнд
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		Result = *ptr_r8[byte2 & 7] & *ptr_r8[(byte2 >> 3) & 7];
		if (log_to_console) cout << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") AND " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") = " << (int)(Result & 255);
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		*ptr_r8[(byte2 >> 3) & 7] = Result & 255;

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
		Result = memory_2[New_Addr_32] & *ptr_r8[(byte2 >> 3) & 7];
		if (log_to_console) cout << "M" << OPCODE_comment << " AND " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") = " << (int)(Result & 255);
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		*ptr_r8[(byte2 >> 3) & 7] = Result & 255;

		Instruction_Pointer += 2 + additional_IPs;
	}

}
void AND_RM_R_16()			// AND R + R/M -> R 16bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint32 Result = 0;

	//определяем 1-й операнд
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		Result = *ptr_r16[byte2 & 7] & *ptr_r16[(byte2 >> 3) & 7];
		if (log_to_console) cout << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") AND " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") = " << (int)(Result & 0xFFFF);
		Flag_CF = 0;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = 0;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		*ptr_r16[(byte2 >> 3) & 7] = Result & 0xFFFF;

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		Result = *ptr_Src & *ptr_r16[(byte2 >> 3) & 7];
		if (log_to_console) cout << "M" << OPCODE_comment << " AND " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") = " << (int)(Result & 0xFFFF);
		Flag_CF = 0;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = 0;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		*ptr_r16[(byte2 >> 3) & 7] = Result & 0xFFFF;

		Instruction_Pointer += 2 + additional_IPs;
	}
}
void AND_IMM_ACC_8()		//AND IMM to ACC 8bit
{
	uint8 Result = 0;

	//непосредственный операнд
	uint8 imm = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];

	if (log_to_console) cout << "IMM(" << (int)imm << ") AND AL(" << (int)(AX & 255) << ") ";

	//определяем объект назначения и результат операции
	*ptr_AL = *ptr_AL & imm;
	Flag_CF = 0;
	Flag_OF = 0;
	Flag_SF = ((*ptr_AL >> 7) & 1);
	if (*ptr_AL) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[*ptr_AL];
	if (log_to_console) cout << " = " << (int)*ptr_AL;

	Instruction_Pointer += 2;
}
void AND_IMM_ACC_16()		//AND IMM to ACC 16bit
{
	uint16 Result = 0;

	//непосредственный операнд
	uint16 imm = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory.read_2(Instruction_Pointer + 2 + *CS * 16) * 256;

	if (log_to_console) cout << "IMM(" << (int)imm << ") AND AX(" << (int)AX << ")";

	//определяем объект назначения и результат операции
	AX = AX & imm;
	Flag_CF = 0;
	Flag_OF = 0;
	Flag_SF = ((AX >> 15) & 1);
	if (AX) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[AX & 255];
	if (log_to_console) cout << " = " << (int)AX;

	Instruction_Pointer += 3;
}

//TEST

void TEST_8()		  //TEST = AND Function to Flags
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 Result = 0;

	//определяем источник
	if((byte2 >> 6 == 3))
	{
		// mod 11 источник - регистр (byte2 & 7)
		if (log_to_console) cout << "TEST " << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") AND " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") = ";
		Result = *ptr_r8[byte2 & 7] & *ptr_r8[(byte2 >> 3) & 7];
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result];
		if (log_to_console) cout << (int)Result;
		Instruction_Pointer += 2;
	}
	else
	{
		//память
		mod_RM_3(byte2);
		Result = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] & *ptr_r8[(byte2 >> 3) & 7];
		if (log_to_console) cout << "TEST M" << OPCODE_comment << " AND " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") = ";
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result];
		if (log_to_console) cout << (int)Result;
		Instruction_Pointer += 2 + additional_IPs;
	}
}
void TEST_16()       //TEST = AND Function to Flags
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Result = 0;

	//определяем источник
	if ((byte2 >> 6 == 3))
	{
		// mod 11 источник - регистр (byte2 & 7)
		if (log_to_console) cout << "TEST " << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") AND " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") = ";
		Result = *ptr_r16[byte2 & 7] & *ptr_r16[(byte2 >> 3) & 7];
		Flag_CF = 0;
		Flag_SF = (Result >> 15) & 1;
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)Result;
		Instruction_Pointer += 2;
	}
	else
	{
		//память
		mod_RM_3(byte2);
		*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		Result = *ptr_Src & *ptr_r16[(byte2 >> 3) & 7];
		if (log_to_console) cout << "TEST M" << OPCODE_comment << " AND " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") = ";
		Flag_CF = 0;
		Flag_SF = (Result >> 15) & 1;
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)Result;
		Instruction_Pointer += 2 + additional_IPs;
	}
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

	if (log_to_console) cout << "TEST IMM[" << (int)(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF] * 256) << "] AND AX(" << (int)AX << ") = ";

	//определяем объект назначения и результат операции
	Result = AX & (memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF] * 256);
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
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Result = 0;

	if (log_to_console) cout << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") ";

	//определяем объект назначения и результат операции ADD
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		if (log_to_console) cout << " OR " << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") = ";

		//складываем два регистра
		Result = *ptr_r8[(byte2 >> 3) & 7] | *ptr_r8[byte2 & 7];
		if (log_to_console) cout << (int)(Result & 255);
		*ptr_r8[byte2 & 7] = Result;
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
		Result = memory_2[New_Addr_32] | *ptr_r8[(byte2 >> 3) & 7];
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		memory_2[New_Addr_32] = Result;
		if (log_to_console) cout << " OR M" << OPCODE_comment << " = " << (int)(Result & 255);

		Instruction_Pointer += 2 + additional_IPs;
	}
}
void OR_RM_16()		// OR R + R/M -> R/M 16bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint32 Result = 0;

	if (log_to_console) cout << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") ";

	//определяем объект назначения и результат операции ADD
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		Result = *ptr_r16[(byte2 >> 3) & 7] | *ptr_r16[byte2 & 7];
		if (log_to_console) cout << " OR " << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") = " << (int)(Result & 0xFFFF);
		*ptr_r16[byte2 & 7] = Result & 0xFFFF;
		Flag_CF = 0;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = 0;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		Result = *ptr_Src | *ptr_r16[(byte2 >> 3) & 7];
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = Result >> 8;
		operand_RM_offset--;
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = Result;
		Flag_CF = 0;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = 0;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " OR M" << OPCODE_comment << " = " << (int)(Result & 0xFFFF);

		Instruction_Pointer += 2 + additional_IPs;
	}
}
void OR_RM_R_8()		// OR R + R/M -> R 8bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Result = 0;

	//определяем 1-й операнд
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		Result = *ptr_r8[byte2 & 7] | *ptr_r8[(byte2 >> 3) & 7];
		if (log_to_console) cout << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") OR " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") = " << (int)(Result & 255);
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		*ptr_r8[(byte2 >> 3) & 7] = Result & 255;

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
		Result = memory_2[New_Addr_32] | *ptr_r8[(byte2 >> 3) & 7];
		if (log_to_console) cout << "M" << OPCODE_comment << " OR " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") = " << (int)(Result & 255);
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		*ptr_r8[(byte2 >> 3) & 7] = Result & 255;

		Instruction_Pointer += 2 + additional_IPs;
	}

}
void OR_RM_R_16()		// OR R + R/M -> R 16bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint32 Result = 0;

	//определяем 1-й операнд
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		Result = *ptr_r16[byte2 & 7] | *ptr_r16[(byte2 >> 3) & 7];
		if (log_to_console) cout << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") OR " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") = " << (int)(Result & 0xFFFF);
		Flag_CF = 0;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = 0;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		*ptr_r16[(byte2 >> 3) & 7] = Result & 0xFFFF;

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		Result = *ptr_Src | *ptr_r16[(byte2 >> 3) & 7];
		if (log_to_console) cout << "M" << OPCODE_comment << " OR " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") = " << (int)(Result & 0xFFFF);
		Flag_CF = 0;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = 0;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		*ptr_r16[(byte2 >> 3) & 7] = Result & 0xFFFF;

		Instruction_Pointer += 2 + additional_IPs;
	}
}
void OR_IMM_ACC_8()    //OR IMM to ACC 8bit
{
	uint8 Result = 0;

	//непосредственный операнд
	uint8 imm = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];

	if (log_to_console) cout << "IMM(" << (int)imm << ") OR AL(" << (int)(AX & 255) << ") ";

	//определяем объект назначения и результат операции
	*ptr_AL = *ptr_AL | imm;

	Flag_CF = 0;
	Flag_OF = 0;
	Flag_SF = ((*ptr_AL >> 7) & 1);
	if (*ptr_AL) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[*ptr_AL];
	if (log_to_console) cout << " = " << (int)*ptr_AL;

	Instruction_Pointer += 2;
}
void OR_IMM_ACC_16()   //OR IMM to ACC 16bit
{
	uint16 Result = 0;

	//непосредственный операнд
	uint16 imm = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory.read_2(Instruction_Pointer + 2 + *CS * 16) * 256;

	if (log_to_console) cout << "IMM(" << (int)imm << ") OR AX(" << (int)AX << ")";

	//определяем объект назначения и результат операции
	AX = AX | imm;
	Flag_CF = 0;
	Flag_OF = 0;
	Flag_SF = ((AX >> 15) & 1);
	if (AX) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[AX & 255];
	if (log_to_console) cout << " = " << (int)AX;

	Instruction_Pointer += 3;
}

//XOR
void XOR_RM_8()		// XOR R + R/M -> R/M 8bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Result = 0;

	if (log_to_console) cout << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") ";

	//определяем объект назначения и результат операции ADD
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		if (log_to_console) cout << " XOR " << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") = ";

		//складываем два регистра
		Result = *ptr_r8[(byte2 >> 3) & 7] ^ *ptr_r8[byte2 & 7];
		if (log_to_console) cout << (int)(Result & 255);
		*ptr_r8[byte2 & 7] = Result;
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
		Result = memory_2[New_Addr_32] ^ *ptr_r8[(byte2 >> 3) & 7];
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		memory_2[New_Addr_32] = Result;
		if (log_to_console) cout << " XOR M" << OPCODE_comment << " = " << (int)(Result & 255);

		Instruction_Pointer += 2 + additional_IPs;
	}
}
void XOR_RM_16()		// XOR R + R/M -> R/M 16bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint32 Result = 0;

	if (log_to_console) cout << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") ";

	//определяем объект назначения и результат операции ADD
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		Result = *ptr_r16[(byte2 >> 3) & 7] ^ *ptr_r16[byte2 & 7];
		if (log_to_console) cout << " XOR " << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") = " << (int)(Result & 0xFFFF);
		*ptr_r16[byte2 & 7] = Result & 0xFFFF;
		Flag_CF = 0;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = 0;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		Result = *ptr_Src ^ *ptr_r16[(byte2 >> 3) & 7];
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = Result >> 8;
		operand_RM_offset--;
		memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = Result;
		Flag_CF = 0;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = 0;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " XOR M" << OPCODE_comment << " = " << (int)(Result & 0xFFFF);

		Instruction_Pointer += 2 + additional_IPs;
	}
}
void XOR_RM_R_8()		// XOR R + R/M -> R 8bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Result = 0;

	//определяем 1-й операнд
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		Result = *ptr_r8[byte2 & 7] ^ *ptr_r8[(byte2 >> 3) & 7];
		if (log_to_console) cout << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") XOR " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") = " << (int)(Result & 255);
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		*ptr_r8[(byte2 >> 3) & 7] = Result & 255;

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
		Result = memory_2[New_Addr_32] ^ *ptr_r8[(byte2 >> 3) & 7];
		if (log_to_console) cout << "M" << OPCODE_comment << " XOR " << reg8_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r8[(byte2 >> 3) & 7] << ") = " << (int)(Result & 255);
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		*ptr_r8[(byte2 >> 3) & 7] = Result & 255;

		Instruction_Pointer += 2 + additional_IPs;
	}

}
void XOR_RM_R_16()		// XOR R + R/M -> R 16bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint32 Result = 0;

	//определяем 1-й операнд
	if ((byte2 >> 6) == 3)
	{
		// mod 11 источник - регистр
		Result = *ptr_r16[byte2 & 7] ^ *ptr_r16[(byte2 >> 3) & 7];
		if (log_to_console) cout << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") XOR " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") = " << (int)(Result & 0xFFFF);
		Flag_CF = 0;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = 0;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		*ptr_r16[(byte2 >> 3) & 7] = Result & 0xFFFF;

		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		Result = *ptr_Src ^ *ptr_r16[(byte2 >> 3) & 7];
		if (log_to_console) cout << "M" << OPCODE_comment << " XOR " << reg16_name[(byte2 >> 3) & 7] << "(" << (int)*ptr_r16[(byte2 >> 3) & 7] << ") = " << (int)(Result & 0xFFFF);
		Flag_CF = 0;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = 0;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		*ptr_r16[(byte2 >> 3) & 7] = Result & 0xFFFF;

		Instruction_Pointer += 2 + additional_IPs;
	}
}

//XOR/OR/AND/ADD/ADC/SBB/SUB IMM to Register/Memory
void XOR_OR_IMM_RM_8()		
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 OP = (byte2 >> 3) & 7;
	uint8 imm = 0;
	uint8 Result = 0;
	uint16 Result_16 = 0;
	
	switch (OP)
	{
	case 0:   //ADD IMM -> R/M 8 bit

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			imm = memory.read_2(Instruction_Pointer + 2 + *CS * 16);
			if (log_to_console) cout << "ADD IMM[" << (int)imm << "] + " << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") = ";
			Flag_AF = (((*ptr_r8[byte2 & 7] & 15) + (imm & 15)) >> 4) & 1;
			OF_Carry = ((*ptr_r8[byte2 & 7] & 0x7F) + (imm & 0x7F)) >> 7;
			Result_16 = *ptr_r8[byte2 & 7] + imm;
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			*ptr_r8[byte2 & 7] = Result_16 & 255;
			if (*ptr_r8[byte2 & 7]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_r8[byte2 & 7]];
			if (log_to_console) cout << (int)(*ptr_r8[byte2 & 7]);
			Instruction_Pointer += 3;
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			//непосредственный операнд
			imm = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "ADD IMM[" << (int)imm << "] + " << "M" << OPCODE_comment << " = ";
			Flag_AF = (((memory_2[New_Addr_32] & 15) + (imm & 15)) >> 4) & 1;
			OF_Carry = ((memory_2[New_Addr_32] & 0x7F) + (imm & 0x7F)) >> 7;
			Result_16 = memory_2[New_Addr_32] + imm;
			memory_2[New_Addr_32] = Result_16 & 255;
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			if (log_to_console) cout << (int)(Result_16 & 255);
			Instruction_Pointer += 3 + additional_IPs;
		}
		break;

	case 1:   //OR

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			imm = memory.read_2(Instruction_Pointer + 2 + *CS * 16);
			if (log_to_console) cout << "IMM[" << (int)imm << "] OR " << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") = ";
			Result_16 = *ptr_r8[byte2 & 7] | imm;
			Flag_CF = 0;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = 0;
			*ptr_r8[byte2 & 7] = Result_16 & 255;
			if (*ptr_r8[byte2 & 7]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_r8[byte2 & 7]];
			if (log_to_console) cout << (int)(*ptr_r8[byte2 & 7]);
			Instruction_Pointer += 3;
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			//непосредственный операнд
			imm = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "IMM[" << (int)imm << "] OR " << "M" << OPCODE_comment << " = ";
			Result_16 = memory_2[New_Addr_32] | imm;
			memory_2[New_Addr_32] = Result_16 & 255;
			Flag_CF = 0;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = 0;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			if (log_to_console) cout << (int)(Result_16 & 255);
			Instruction_Pointer += 3 + additional_IPs;
		}
		break;

	case 2:   //ADC IMM -> R/M 8 bit

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			imm = memory.read_2(Instruction_Pointer + 2 + *CS * 16);
			if (log_to_console) cout << "ADC IMM[" << (int)imm << "] + " << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") + CF (" << (int)Flag_CF << ") = ";
			Flag_AF = (((*ptr_r8[byte2 & 7] & 15) + (imm & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = ((*ptr_r8[byte2 & 7] & 0x7F) + (imm & 0x7F) + Flag_CF) >> 7;
			Result_16 = *ptr_r8[byte2 & 7] + imm + Flag_CF;
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			*ptr_r8[byte2 & 7] = Result_16 & 255;
			if (*ptr_r8[byte2 & 7]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_r8[byte2 & 7]];
			if (log_to_console) cout << (int)(*ptr_r8[byte2 & 7]);
			Instruction_Pointer += 3;
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			//непосредственный операнд
			imm = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "ADC IMM[" << (int)imm << "] + " << "M" << OPCODE_comment << "+ CF (" << (int)Flag_CF << ") = ";
			Flag_AF = (((memory_2[New_Addr_32] & 15) + (imm & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = ((memory_2[New_Addr_32] & 0x7F) + (imm & 0x7F) + Flag_CF) >> 7;
			Result_16 = memory_2[New_Addr_32] + imm + Flag_CF;
			memory_2[New_Addr_32] = Result_16 & 255;
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			if (log_to_console) cout << (int)(Result_16 & 255);
			Instruction_Pointer += 3 + additional_IPs;
		}
		break;

	case 3:   //SBB IMM -> R/M 8 bit

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			imm = memory.read_2(Instruction_Pointer + 2 + *CS * 16);
			if (log_to_console) cout << "SBB " << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") - IMM[" << (int)imm << "] - CF(" << (int)Flag_CF << ") = ";
			Flag_AF = (((*ptr_r8[byte2 & 7] & 15) - (imm & 15) - Flag_CF) >> 4) & 1;
			OF_Carry = ((*ptr_r8[byte2 & 7] & 0x7F) - (imm & 0x7F) - Flag_CF) >> 7;
			Result_16 = *ptr_r8[byte2 & 7] - imm - Flag_CF;
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (!*ptr_r8[byte2 & 7]) Flag_OF = 0; //если вычитаем из ноля, OF = 0
			*ptr_r8[byte2 & 7] = Result_16 & 255;
			if (*ptr_r8[byte2 & 7]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_r8[byte2 & 7]];
			if (log_to_console) cout << (int)(*ptr_r8[byte2 & 7]);
			Instruction_Pointer += 3;
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			//непосредственный операнд
			imm = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "SBB M" << OPCODE_comment << " IMM[" << (int)imm << "] - CF (" << (int)Flag_CF << ") = ";
			Flag_AF = (((memory_2[New_Addr_32] & 15) - (imm & 15) - Flag_CF) >> 4) & 1;
			OF_Carry = ((memory_2[New_Addr_32] & 0x7F) - (imm & 0x7F) - Flag_CF) >> 7;
			Result_16 = memory_2[New_Addr_32] - imm - Flag_CF;
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (!memory_2[New_Addr_32]) Flag_OF = 0; //если вычитаем из ноля, OF = 0
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			memory_2[New_Addr_32] = Result_16 & 255;
			if (log_to_console) cout << (int)(Result_16 & 255);
			Instruction_Pointer += 3 + additional_IPs;
		}
		break;

	case 4:   //AND

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			imm = memory.read_2(Instruction_Pointer + 2 + *CS * 16);
			if (log_to_console) cout << "IMM[" << (int)imm << "] AND " << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") = ";
			Result_16 = *ptr_r8[byte2 & 7] & imm;
			Flag_CF = 0;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = 0;
			*ptr_r8[byte2 & 7] = Result_16 & 255;
			if (*ptr_r8[byte2 & 7]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_r8[byte2 & 7]];
			if (log_to_console) cout << (int)(*ptr_r8[byte2 & 7]);
			Instruction_Pointer += 3;
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			//непосредственный операнд
			imm = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "IMM[" << (int)imm << "] AND " << "M" << OPCODE_comment << " = ";
			Result_16 = memory_2[New_Addr_32] & imm;
			memory_2[New_Addr_32] = Result_16 & 255;
			Flag_CF = 0;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = 0;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			if (log_to_console) cout << (int)(Result_16 & 255);
			Instruction_Pointer += 3 + additional_IPs;
		}
		break;

	case 5:   //SUB IMM -> R/M 8 bit
		
		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			imm = memory.read_2(Instruction_Pointer + 2 + *CS * 16);
			if (log_to_console) cout << "SUB " << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") - IMM[" << (int)imm << "] = ";
			Flag_AF = (((*ptr_r8[byte2 & 7] & 15) - (imm & 15)) >> 4) & 1;
			OF_Carry = ((*ptr_r8[byte2 & 7] & 0x7F) - (imm & 0x7F)) >> 7;
			Result_16 = *ptr_r8[byte2 & 7] - imm;
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (!*ptr_r8[byte2 & 7]) Flag_OF = 0;
			*ptr_r8[byte2 & 7] = Result_16 & 255;
			if (*ptr_r8[byte2 & 7]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_r8[byte2 & 7]];
			if (log_to_console) cout << (int)(*ptr_r8[byte2 & 7]);
			Instruction_Pointer += 3;
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			//непосредственный операнд
			imm = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "SUB M" << OPCODE_comment << " IMM[" << (int)imm << "] = ";
			Flag_AF = (((memory_2[New_Addr_32] & 15) - (imm & 15)) >> 4) & 1;
			OF_Carry = ((memory_2[New_Addr_32] & 0x7F) - (imm & 0x7F)) >> 7;
			Result_16 = memory_2[New_Addr_32] - imm;
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (!memory_2[New_Addr_32]) Flag_OF = 0;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			if (log_to_console) cout << (int)(Result_16 & 255);
			memory_2[New_Addr_32] = Result_16 & 255;
			Instruction_Pointer += 3 + additional_IPs;
		}
		break;

	case 6: //XOR

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			imm = memory.read_2(Instruction_Pointer + 2 + *CS * 16);
			if (log_to_console) cout << "IMM[" << (int)imm << "] XOR " << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") = ";
			Result_16 = *ptr_r8[byte2 & 7] ^ imm;
			Flag_CF = 0;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = 0;
			*ptr_r8[byte2 & 7] = Result_16 & 255;
			if (*ptr_r8[byte2 & 7]) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[*ptr_r8[byte2 & 7]];
			if (log_to_console) cout << (int)(*ptr_r8[byte2 & 7]);
			Instruction_Pointer += 3;
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			//непосредственный операнд
			imm = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "IMM[" << (int)imm << "] XOR " << "M" << OPCODE_comment << " = ";
			Result_16 = memory_2[New_Addr_32] ^ imm;
			memory_2[New_Addr_32] = Result_16 & 255;
			Flag_CF = 0;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = 0;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			if (log_to_console) cout << (int)(Result_16 & 255);
			Instruction_Pointer += 3 + additional_IPs;
		}
		break;

	case 7:   //CMP IMM -> R/M 8 bit

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			imm = memory.read_2(Instruction_Pointer + 2 + *CS * 16);
			if (log_to_console) cout << "CMP " << reg8_name[byte2 & 7] << "(" << (int)*ptr_r8[byte2 & 7] << ") - IMM[" << (int)imm << "] = ";
			Flag_AF = (((*ptr_r8[byte2 & 7] & 15) - (imm & 15)) >> 4) & 1;
			OF_Carry = ((*ptr_r8[byte2 & 7] & 0x7F) - (imm & 0x7F)) >> 7;
			Result_16 = *ptr_r8[byte2 & 7] - imm;
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			if (log_to_console) cout << (int)(Result_16 & 255);
			Instruction_Pointer += 3;
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			//непосредственный операнд
			imm = memory.read_2(Instruction_Pointer + 2 + additional_IPs + *CS * 16);
			if (log_to_console) cout << "CMP M" << OPCODE_comment << "("<< (int)memory_2[New_Addr_32] << ") IMM[" << (int)imm << "] = ";
			Flag_AF = (((memory_2[New_Addr_32] & 15) - (imm & 15)) >> 4) & 1;
			OF_Carry = ((memory_2[New_Addr_32] & 0x7F) - (imm & 0x7F)) >> 7;
			Result_16 = memory_2[New_Addr_32] - imm;
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			if (log_to_console) cout << (int)(Result_16 & 255);
			Instruction_Pointer += 3 + additional_IPs;
		}
		break;
	}
	
}
void XOR_OR_IMM_RM_16()   //XOR/OR/ADD/ADC IMM to Register/Memory 16bit
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint8 OP = (byte2 >> 3) & 7;
	uint16 Result = 0;
	additional_IPs = 0;
	uint32 Result_32 = 0;

	switch (OP)
	{
	case 0:   //ADD IMM -> R/M 16 bit

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			*ptr_Src_L = memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF];
			*ptr_Src_H = memory_2[(Instruction_Pointer + 3 + *CS * 16) & 0xFFFFF];
			if (log_to_console) cout << "ADD IMM(" << (int)*ptr_Src << ") + " << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") = ";
			Flag_AF = (((*ptr_r16[byte2 & 7] & 15) + (*ptr_Src & 15)) >> 4) & 1;
			OF_Carry = ((*ptr_r16[byte2 & 7] & 0x7FFF) + (*ptr_Src & 0x7FFF)) >> 15;
			Result_32 = *ptr_r16[byte2 & 7] + *ptr_Src;
			*ptr_r16[byte2 & 7] = Result_32 & 0xFFFF;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << (int)*ptr_r16[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[(Instruction_Pointer + 2 + additional_IPs + *CS * 16) & 0xFFFFF];
			*ptr_Src_H = memory_2[(Instruction_Pointer + 3 + additional_IPs + *CS * 16) & 0xFFFFF];
			if (log_to_console) cout << "ADD IMM(" << (int)*ptr_Src << ") + ";
			Flag_AF = (((memory_2[New_Addr_32] & 15) + (*ptr_Src & 15)) >> 4) & 1;
			OF_Carry = (((memory_2[New_Addr_32] + memory_2[New_Addr_32 + 1] * 256) & 0x7FFF) + (*ptr_Src & 0x7FFF)) >> 15;
			Result_32 = memory_2[New_Addr_32] + memory_2[New_Addr_32 + 1] * 256 + *ptr_Src;
			memory_2[New_Addr_32] = Result_32 & 255;
			memory_2[New_Addr_32 + 1] = Result_32 >> 8;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);
		}
		break;

	case 1:   //OR
		
		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			*ptr_Src_L = memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF];
			*ptr_Src_H = memory_2[(Instruction_Pointer + 3 + *CS * 16) & 0xFFFFF];
			if (log_to_console) cout << "IMM(" << (int)*ptr_Src << ") OR " << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") = ";
			Result_32 = *ptr_r16[byte2 & 7] | *ptr_Src;
			*ptr_r16[byte2 & 7] = Result_32 & 0xFFFF;
			Flag_CF = 0;
			Flag_SF = (Result_32 >> 15) & 1;
			Flag_OF = 0;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << (int)*ptr_r16[byte2 & 7];

		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[(Instruction_Pointer + 2 + additional_IPs + *CS * 16) & 0xFFFFF];
			*ptr_Src_H = memory_2[(Instruction_Pointer + 3 + additional_IPs + *CS * 16) & 0xFFFFF];
			if (log_to_console) cout << "IMM(" << (int)*ptr_Src << ") OR ";
			Result_32 = (memory_2[New_Addr_32] + memory_2[New_Addr_32 + 1] * 256) | *ptr_Src;
			memory_2[New_Addr_32] = Result_32 & 255;
			memory_2[New_Addr_32 + 1] = Result_32 >> 8;
			Flag_CF = 0;
			Flag_SF = (Result_32 >> 15) & 1;
			Flag_OF = 0;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);
		}
		break;

	case 2:   //ADC IMM -> R/M 16 bit
		
		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			*ptr_Src_L = memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF];
			*ptr_Src_H = memory_2[(Instruction_Pointer + 3 + *CS * 16) & 0xFFFFF];
			if (log_to_console) cout << "ADC IMM(" << (int)*ptr_Src << ") + " << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") + CF(" << (int)Flag_CF << ") = ";
			Flag_AF = (((*ptr_r16[byte2 & 7] & 15) + (*ptr_Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = ((*ptr_r16[byte2 & 7] & 0x7FFF) + (*ptr_Src & 0x7FFF) + Flag_CF) >> 15;
			Result_32 = *ptr_r16[byte2 & 7] + *ptr_Src + Flag_CF;
			*ptr_r16[byte2 & 7] = Result_32 & 0xFFFF;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << (int)*ptr_r16[byte2 & 7];

		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[(Instruction_Pointer + 2 + additional_IPs + *CS * 16) & 0xFFFFF];
			*ptr_Src_H = memory_2[(Instruction_Pointer + 3 + additional_IPs + *CS * 16) & 0xFFFFF];
			if (log_to_console) cout << "ADC IMM(" << (int)*ptr_Src << ") + ";
			Flag_AF = (((memory_2[New_Addr_32] & 15) + (*ptr_Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = (((memory_2[New_Addr_32] + memory_2[New_Addr_32 + 1] * 256) & 0x7FFF) + (*ptr_Src & 0x7FFF) + Flag_CF) >> 15;
			Result_32 = memory_2[New_Addr_32] + memory_2[New_Addr_32 + 1] * 256 + *ptr_Src + Flag_CF;
			memory_2[New_Addr_32] = Result_32 & 255;
			memory_2[New_Addr_32 + 1] = Result_32 >> 8;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << " M" << OPCODE_comment << " + CF(" << (int)Flag_CF << ") = " << (int)(Result_32 & 0xFFFF);
		}
		break;

	case 3:   //SBB IMM -> R/M 16 bit

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			*ptr_Src_L = memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF];
			*ptr_Src_H = memory_2[(Instruction_Pointer + 3 + *CS * 16) & 0xFFFFF];
			if (log_to_console) cout << "SBB " << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") - IMM(" << (int)*ptr_Src << ") - CF(" << (int)Flag_CF << ") = ";
			Flag_AF = (((*ptr_r16[byte2 & 7] & 15) - (*ptr_Src & 15) - Flag_CF) >> 4) & 1;
			OF_Carry = ((*ptr_r16[byte2 & 7] & 0x7FFF) - (*ptr_Src & 0x7FFF) - Flag_CF) >> 15;
			Result_32 = *ptr_r16[byte2 & 7] - *ptr_Src - Flag_CF;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (!*ptr_r16[byte2 & 7]) Flag_OF = 0; //если вычитаем из ноля, OF = 0
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			*ptr_r16[byte2 & 7] = Result_32 & 0xFFFF;
			if (log_to_console) cout << (int)*ptr_r16[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[(Instruction_Pointer + 2 + additional_IPs + *CS * 16) & 0xFFFFF];
			*ptr_Src_H = memory_2[(Instruction_Pointer + 3 + additional_IPs + *CS * 16) & 0xFFFFF];
			if (log_to_console) cout << "SBB M" << OPCODE_comment << " - IMM(" << (int)*ptr_Src << ") - CF(" << (int)Flag_CF << ") = ";
			Flag_AF = (((memory_2[New_Addr_32] & 15) - (*ptr_Src & 15) - Flag_CF) >> 4) & 1;
			OF_Carry = (((memory_2[New_Addr_32] + memory_2[New_Addr_32 + 1] * 256) & 0x7FFF) - (*ptr_Src & 0x7FFF) - Flag_CF) >> 15;
			Result_32 = memory_2[New_Addr_32] + memory_2[New_Addr_32 + 1] * 256 - *ptr_Src - Flag_CF;
			memory_2[New_Addr_32] = Result_32 & 255;
			memory_2[New_Addr_32 + 1] = Result_32 >> 8;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (!*ptr_Src) Flag_OF = 0; //если вычитаем из ноля, OF = 0
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << (int)(Result_32 & 0xFFFF);
		}
		break;

	case 4:   //AND

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			*ptr_Src_L = memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF];
			*ptr_Src_H = memory_2[(Instruction_Pointer + 3 + *CS * 16) & 0xFFFFF];
			if (log_to_console) cout << "IMM(" << (int)*ptr_Src << ") AND " << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") = ";
			Result_32 = *ptr_r16[byte2 & 7] & *ptr_Src;
			*ptr_r16[byte2 & 7] = Result_32 & 0xFFFF;
			Flag_CF = 0;
			Flag_SF = (Result_32 >> 15) & 1;
			Flag_OF = 0;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << (int)*ptr_r16[byte2 & 7];

		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[(Instruction_Pointer + 2 + additional_IPs + *CS * 16) & 0xFFFFF];
			*ptr_Src_H = memory_2[(Instruction_Pointer + 3 + additional_IPs + *CS * 16) & 0xFFFFF];
			if (log_to_console) cout << "IMM(" << (int)*ptr_Src << ") AND ";
			Result_32 = (memory_2[New_Addr_32] + memory_2[New_Addr_32 + 1] * 256) & *ptr_Src;
			memory_2[New_Addr_32] = Result_32 & 255;
			memory_2[New_Addr_32 + 1] = Result_32 >> 8;
			Flag_CF = 0;
			Flag_SF = (Result_32 >> 15) & 1;
			Flag_OF = 0;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);
		}
		break;

	case 5:   //SUB IMM -> R/M 16 bit

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			*ptr_Src_L = memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF];
			*ptr_Src_H = memory_2[(Instruction_Pointer + 3 + *CS * 16) & 0xFFFFF];
			if (log_to_console) cout << "SUB " << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") - IMM(" << (int)*ptr_Src << ") = ";
			Flag_AF = (((*ptr_r16[byte2 & 7] & 15) - (*ptr_Src & 15)) >> 4) & 1;
			OF_Carry = ((*ptr_r16[byte2 & 7] & 0x7FFF) - (*ptr_Src & 0x7FFF)) >> 15;
			Result_32 = *ptr_r16[byte2 & 7] - *ptr_Src;
			*ptr_r16[byte2 & 7] = Result_32 & 0xFFFF;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << (int)*ptr_r16[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[(Instruction_Pointer + 2 + additional_IPs + *CS * 16) & 0xFFFFF];
			*ptr_Src_H = memory_2[(Instruction_Pointer + 3 + additional_IPs + *CS * 16) & 0xFFFFF];
			if (log_to_console) cout << "SUB M" << OPCODE_comment << " - IMM(" << (int)*ptr_Src << ") = ";
			Flag_AF = (((memory_2[New_Addr_32] & 15) - (*ptr_Src & 15)) >> 4) & 1;
			OF_Carry = (((memory_2[New_Addr_32] + memory_2[New_Addr_32 + 1] * 256) & 0x7FFF) - (*ptr_Src & 0x7FFF)) >> 15;
			Result_32 = memory_2[New_Addr_32] + memory_2[New_Addr_32 + 1] * 256 - *ptr_Src;
			memory_2[New_Addr_32] = Result_32 & 255;
			memory_2[New_Addr_32 + 1] = Result_32 >> 8;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << (int)(Result_32 & 0xFFFF);
		}
		break;

	case 6:  //XOR

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			*ptr_Src_L = memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF];
			*ptr_Src_H = memory_2[(Instruction_Pointer + 3 + *CS * 16) & 0xFFFFF];
			if (log_to_console) cout << "IMM(" << (int)*ptr_Src << ") XOR " << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") = ";
			Result_32 = *ptr_r16[byte2 & 7] ^ *ptr_Src;
			*ptr_r16[byte2 & 7] = Result_32 & 0xFFFF;
			Flag_CF = 0;
			Flag_SF = (Result_32 >> 15) & 1;
			Flag_OF = 0;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << (int)*ptr_r16[byte2 & 7];

		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[(Instruction_Pointer + 2 + additional_IPs + *CS * 16) & 0xFFFFF];
			*ptr_Src_H = memory_2[(Instruction_Pointer + 3 + additional_IPs + *CS * 16) & 0xFFFFF];
			if (log_to_console) cout << "IMM(" << (int)*ptr_Src << ") XOR ";
			Result_32 = (memory_2[New_Addr_32] + memory_2[New_Addr_32 + 1] * 256) ^ *ptr_Src;
			memory_2[New_Addr_32] = Result_32 & 255;
			memory_2[New_Addr_32 + 1] = Result_32 >> 8;
			Flag_CF = 0;
			Flag_SF = (Result_32 >> 15) & 1;
			Flag_OF = 0;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << " M" << OPCODE_comment << " = " << (int)(Result_32 & 0xFFFF);
		}
		break;

	case 7:   //CMP IMM -> R/M 16 bit

		if ((byte2 >> 6) == 3)
		{
			// mod 11 источник - регистр
			*ptr_Src_L = memory_2[(Instruction_Pointer + 2 + *CS * 16) & 0xFFFFF];
			*ptr_Src_H = memory_2[(Instruction_Pointer + 3 + *CS * 16) & 0xFFFFF];
			if (log_to_console) cout << "CMP " << reg16_name[byte2 & 7] << "(" << (int)*ptr_r16[byte2 & 7] << ") - IMM(" << (int)*ptr_Src << ") = ";
			Flag_AF = (((*ptr_r16[byte2 & 7] & 15) - (*ptr_Src & 15)) >> 4) & 1;
			OF_Carry = ((*ptr_r16[byte2 & 7] & 0x7FFF) - (*ptr_Src & 0x7FFF)) >> 15;
			Result_32 = *ptr_r16[byte2 & 7] - *ptr_Src;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << (int)*ptr_r16[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			*ptr_Src_L = memory_2[(Instruction_Pointer + 2 + additional_IPs + *CS * 16) & 0xFFFFF];
			*ptr_Src_H = memory_2[(Instruction_Pointer + 3 + additional_IPs + *CS * 16) & 0xFFFFF];
			if (log_to_console) cout << "CMP M" << OPCODE_comment << " - IMM(" << (int)*ptr_Src << ") = ";
			Flag_AF = (((memory_2[New_Addr_32] & 15) - (*ptr_Src & 15)) >> 4) & 1;
			OF_Carry = (((memory_2[New_Addr_32] + memory_2[New_Addr_32 + 1] * 256) & 0x7FFF) - (*ptr_Src & 0x7FFF)) >> 15;
			Result_32 = memory_2[New_Addr_32] + memory_2[New_Addr_32 + 1] * 256 - *ptr_Src;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << (int)(Result_32 & 0xFFFF);
		}
		break;
	}
	Instruction_Pointer += 4 + additional_IPs;
}

void XOR_IMM_ACC_8()   //XOR IMM to ACC 8bit
{
	uint8 Result = 0;

	//непосредственный операнд
	uint8 imm = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];

	if (log_to_console) cout << "XOR IMM(" << (int)imm << ") ^ ";

	//определяем объект назначения и результат операции
	if (log_to_console) cout << " AL(" << (int)(*ptr_AL) << ") = ";
	*ptr_AL = *ptr_AL ^ imm;
	if (log_to_console) cout << (int)*ptr_AL;
	Flag_CF = 0;
	Flag_OF = 0;
	Flag_SF = ((*ptr_AL >> 7) & 1);
	if (*ptr_AL) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[*ptr_AL];
	Instruction_Pointer += 2;
}
void XOR_IMM_ACC_16()  //XOR IMM to ACC 16bit
{
	uint16 Result = 0;

	//непосредственный операнд
	uint16 imm = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory.read_2(Instruction_Pointer + 2 + *CS * 16) * 256;

	if (log_to_console) cout << "XOR IMM(" << (int)imm << ") ^ AX(" << (int)AX << ") = ";

	//определяем объект назначения и результат операции
	AX = AX ^ imm;
	if (log_to_console) cout << (int)AX;
	Flag_CF = 0;
	Flag_OF = 0;
	Flag_SF = ((AX >> 15) & 1);
	if (AX) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[AX & 255];
	Instruction_Pointer += 3;
}

//============String Manipulation=========================

void REPNE()		//REP = Repeat while ZF=0 and CX > 0   [F2]  для CMPS, SCAS
{
	uint8 OP = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	uint16 Result_16 = 0;
	uint32 Result_32 = 0;
	
	if (log_to_console) cout << "REPNE[F2] ";
	switch (OP)
	{
	case 0b10100100:  //MOVS 8

		switch (Flag_segment_override)
		{
		case 0:
			//ничего не меняем
			operand_RM_seg = *DS;
			break;
		case 1:
			//меняем на ES
			operand_RM_seg = *ES;
			break;
		case 2:
			//меняем на CS
			operand_RM_seg = *CS;
			break;
		case 3:
			//меняем на SS
			operand_RM_seg = *SS;
			break;
		case 4:
			//ничего не меняем (уже DS)
			operand_RM_seg = *DS;
			break;
		}
		if (log_to_console) cout << "MOVES_B (" << (int)CX << " bytes) from [" << (int)operand_RM_seg << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
		while (CX)
		{
			memory_2[(Destination_Index + *ES * 16) & 0xFFFFF] = memory_2[(Source_Index + operand_RM_seg * 16) & 0xFFFFF];
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

		switch (Flag_segment_override)
		{
		case 0:
			//ничего не меняем
			operand_RM_seg = *DS;
			break;
		case 1:
			//меняем на ES
			operand_RM_seg = *ES;
			break;
		case 2:
			//меняем на CS
			operand_RM_seg = *CS;
			break;
		case 3:
			//меняем на SS
			operand_RM_seg = *SS;
			break;
		case 4:
			//ничего не меняем (уже DS)
			operand_RM_seg = *DS;
			break;
		}
		if (log_to_console) cout << "MOVES_W (" << (int)CX << " words) from [" << (int)operand_RM_seg << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
		while (CX)
		{
			memory_2[(Destination_Index + *ES * 16) & 0xFFFFF] = memory_2[(Source_Index + operand_RM_seg * 16) & 0xFFFFF];
			Destination_Index++;
			Source_Index++;
			memory_2[(Destination_Index + *ES * 16) & 0xFFFFF] = memory_2[(Source_Index + operand_RM_seg * 16) & 0xFFFFF];

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

		switch (Flag_segment_override)
		{
		case 0:
			//ничего не меняем
			operand_RM_seg = *DS;
			break;
		case 1:
			//меняем на ES
			operand_RM_seg = *ES;
			break;
		case 2:
			//меняем на CS
			operand_RM_seg = *CS;
			break;
		case 3:
			//меняем на SS
			operand_RM_seg = *SS;
			break;
		case 4:
			//ничего не меняем (уже DS)
			operand_RM_seg = *DS;
			break;
		}
		if (log_to_console) cout << "CMPS_B until == 0 (" << (int)CX << " bytes) from [" << (int)operand_RM_seg << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]  ";
		if (log_to_console)
		{
			cout << "[";
			for (int i = 0; i < CX; i++) cout << memory_2[(Source_Index + operand_RM_seg * 16) & 0xFFFFF];
			cout << "][";
			for (int i = 0; i < CX; i++) cout << memory_2[(Destination_Index + *ES * 16) & 0xFFFFF];
			cout << "]" << endl;;
		}
		
		do
		{
			if (!CX) break;
			Result_16 = memory_2[(Source_Index + operand_RM_seg * 16) & 0xFFFFF] - memory_2[(Destination_Index + *ES * 16) & 0xFFFFF];
			OF_Carry = ((memory_2[(Source_Index + operand_RM_seg * 16) & 0xFFFFF] & 0x7F) - (memory_2[(Destination_Index + *ES * 16) & 0xFFFFF] & 0x7F)) >> 7;
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			Flag_AF = (((memory_2[(Source_Index + operand_RM_seg * 16) & 0xFFFFF] & 0x0F) - (memory_2[(Destination_Index + *ES * 16) & 0xFFFFF] & 0x0F)) >> 4) & 1;
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

		switch (Flag_segment_override)
		{
		case 0:
			//ничего не меняем
			operand_RM_seg = *DS;
			break;
		case 1:
			//меняем на ES
			operand_RM_seg = *ES;
			break;
		case 2:
			//меняем на CS
			operand_RM_seg = *CS;
			break;
		case 3:
			//меняем на SS
			operand_RM_seg = *SS;
			break;
		case 4:
			//ничего не меняем (уже DS)
			operand_RM_seg = *DS;
			break;
		}
		if (log_to_console) cout << "CMPS_W (" << (int)CX << " word) from [" << (int)operand_RM_seg << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]" << endl;
		do
		{
			if (!CX) break;
			Src = memory_2[(Source_Index + operand_RM_seg * 16) & 0xFFFFF];
			Dst = memory_2[(Destination_Index + *ES * 16) & 0xFFFFF];
			Destination_Index++;
			Source_Index++;
			Src += memory_2[(Source_Index + operand_RM_seg * 16) & 0xFFFFF] * 256;
			Dst += memory_2[(Destination_Index + *ES * 16) & 0xFFFFF] * 256;

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
		if (!CX) break;
		do
		{
			
			uint16 Result = *ptr_AL - memory_2[(Destination_Index + *ES * 16) & 0xFFFFF];
			OF_Carry = ((*ptr_AL & 0x7F) - (memory_2[(Destination_Index + *ES * 16) & 0xFFFFF] & 0x7F)) >> 7;
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = (((*ptr_AL & 15) - (memory_2[(Destination_Index + *ES * 16) & 0xFFFFF] & 15)) >> 4) & 1;
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
		
		switch (Flag_segment_override)
		{
		case 0:
			//ничего не меняем
			operand_RM_seg = *DS;
			break;
		case 1:
			//меняем на ES
			operand_RM_seg = *ES;
			break;
		case 2:
			//меняем на CS
			operand_RM_seg = *CS;
			break;
		case 3:
			//меняем на SS
			operand_RM_seg = *SS;
			break;
		case 4:
			//ничего не меняем (уже DS)
			operand_RM_seg = *DS;
			break;
		}
		if (log_to_console) cout << "LODS_B (" << (int)CX << " bytes) from [" << (int)operand_RM_seg << "]:[" << (int)Source_Index << "]";
		while (CX)
		{
			*ptr_AL = memory_2[(Source_Index + operand_RM_seg * 16) & 0xFFFFF];
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
		
		switch (Flag_segment_override)
		{
		case 0:
			//ничего не меняем
			operand_RM_seg = *DS;
			break;
		case 1:
			//меняем на ES
			operand_RM_seg = *ES;
			break;
		case 2:
			//меняем на CS
			operand_RM_seg = *CS;
			break;
		case 3:
			//меняем на SS
			operand_RM_seg = *SS;
			break;
		case 4:
			//ничего не меняем (уже DS)
			operand_RM_seg = *DS;
			break;
		}
		if (log_to_console) cout << "LODS_B (" << (int)CX << " words) from [" << (int)operand_RM_seg << "]:[" << (int)Source_Index << "]";
		while (CX)
		{
			*ptr_AL = memory_2[(Source_Index + operand_RM_seg * 16) & 0xFFFFF];
			Source_Index++;
			*ptr_AH = memory_2[(Source_Index + operand_RM_seg * 16) & 0xFFFFF];
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
			memory_2[(Destination_Index + *ES * 16) & 0xFFFFF] = *ptr_AL;
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
			memory_2[(Destination_Index + *ES * 16) & 0xFFFFF] = *ptr_AL;
			Destination_Index++;
			memory_2[(Destination_Index + *ES * 16) & 0xFFFFF] = *ptr_AH;
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
	
	if (log_to_console) cout << "REP[F3] ";
	switch (OP)
	{
	case 0b10100100:  //MOVS 8
		switch (Flag_segment_override)
		{
		case 0:
			//ничего не меняем
			operand_RM_seg = *DS;
			break;
		case 1:
			//меняем на ES
			operand_RM_seg = *ES;
			break;
		case 2:
			//меняем на CS
			operand_RM_seg = *CS;
			break;
		case 3:
			//меняем на SS
			operand_RM_seg = *SS;
			break;
		case 4:
			//ничего не меняем (уже DS)
			operand_RM_seg = *DS;
			break;
		}
		if (log_to_console) cout << "MOVES_B (" << (int)CX << " bytes) from [" << (int)operand_RM_seg << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
		while (CX)
		{
			memory.write_2(Destination_Index + *ES * 16, memory.read_2(Source_Index + operand_RM_seg * 16));
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

		switch (Flag_segment_override)
		{
		case 0:
			//ничего не меняем
			operand_RM_seg = *DS;
			break;
		case 1:
			//меняем на ES
			operand_RM_seg = *ES;
			break;
		case 2:
			//меняем на CS
			operand_RM_seg = *CS;
			break;
		case 3:
			//меняем на SS
			operand_RM_seg = *SS;
			break;
		case 4:
			//ничего не меняем (уже DS)
			operand_RM_seg = *DS;
			break;
		}
		if (log_to_console) cout << "MOVES_W (" << (int)CX << " words) from [" << (int)operand_RM_seg << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
		while (CX)
		{
			memory.write_2(Destination_Index + *ES * 16, memory.read_2(Source_Index + operand_RM_seg * 16));
			Destination_Index ++;
			Source_Index ++;
			memory.write_2(Destination_Index + *ES * 16, memory.read_2(Source_Index + operand_RM_seg * 16));
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
		
		switch (Flag_segment_override)
		{
		case 0:
			//ничего не меняем
			operand_RM_seg = *DS;
			break;
		case 1:
			//меняем на ES
			operand_RM_seg = *ES;
			break;
		case 2:
			//меняем на CS
			operand_RM_seg = *CS;
			break;
		case 3:
			//меняем на SS
			operand_RM_seg = *SS;
			break;
		case 4:
			//ничего не меняем (уже DS)
			operand_RM_seg = *DS;
			break;
		}
		if (log_to_console) cout << "CMPS_B while == 0 (" << (int)CX << " bytes) from [" << (int)operand_RM_seg << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]  ";
		if (log_to_console)
		{
			cout << "[";
			for (int i = 0; i < CX; i = i + (1 + Flag_DF * -2)) cout << memory.read_2(Source_Index + i + operand_RM_seg * 16);
			cout << "][";
			for (int i = 0; i < CX; i = i + (1 + Flag_DF * -2)) cout << memory.read_2(Destination_Index + i + *ES * 16);
			cout << "]" << endl;
		}
				
		do
		{
			if (!CX) break;
			Result_16 = memory.read_2(Source_Index + operand_RM_seg * 16) - memory.read_2(Destination_Index + *ES * 16);
			OF_Carry = ((memory.read_2(Source_Index + operand_RM_seg * 16) & 0x7F) - (memory.read_2(Destination_Index + *ES * 16) & 0x7F)) >> 7;
			//if (log_to_console || 1) cout << " (" << memory.read_2(Source_Index + *DS * 16) << "-" << memory.read_2(Destination_Index + *ES * 16) << ") ";
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			//cout << (int)Flag_ZF << endl;
			Flag_PF = parity_check[Result_16 & 255];
			Flag_AF = (((memory.read_2(Source_Index + operand_RM_seg * 16) & 0x0F) - (memory.read_2(Destination_Index + *ES * 16) & 0x0F)) >> 4) & 1;
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

		switch (Flag_segment_override)
		{
		case 0:
			//ничего не меняем
			operand_RM_seg = *DS;
			break;
		case 1:
			//меняем на ES
			operand_RM_seg = *ES;
			break;
		case 2:
			//меняем на CS
			operand_RM_seg = *CS;
			break;
		case 3:
			//меняем на SS
			operand_RM_seg = *SS;
			break;
		case 4:
			//ничего не меняем (уже DS)
			operand_RM_seg = *DS;
			break;
		}
		if (log_to_console) cout << "CMPS_W (" << (int)CX << " word) from [" << (int)operand_RM_seg << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
		do
		{
			if (!CX) break;
			*ptr_Src_L = memory_2[(Source_Index + operand_RM_seg * 16) & 0xFFFFF];
			*ptr_Dst_L = memory_2[(Destination_Index + *ES * 16) & 0xFFFFF];
			Destination_Index++;
			Source_Index++;
			*ptr_Src_H = memory_2[(Source_Index + operand_RM_seg * 16) & 0xFFFFF];
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
		switch (Flag_segment_override)
		{
		case 0:
			//ничего не меняем
			operand_RM_seg = *DS;
			break;
		case 1:
			//меняем на ES
			operand_RM_seg = *ES;
			break;
		case 2:
			//меняем на CS
			operand_RM_seg = *CS;
			break;
		case 3:
			//меняем на SS
			operand_RM_seg = *SS;
			break;
		case 4:
			//ничего не меняем (уже DS)
			operand_RM_seg = *DS;
			break;
		}
		if (log_to_console) cout << "LODS_B (" << (int)CX << " bytes) from [" << (int)operand_RM_seg << "]:[" << (int)Source_Index << "]";
		while (CX)
		{
			*ptr_AL = memory.read_2(Source_Index + operand_RM_seg * 16);
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
		switch (Flag_segment_override)
		{
		case 0:
			//ничего не меняем
			operand_RM_seg = *DS;
			break;
		case 1:
			//меняем на ES
			operand_RM_seg = *ES;
			break;
		case 2:
			//меняем на CS
			operand_RM_seg = *CS;
			break;
		case 3:
			//меняем на SS
			operand_RM_seg = *SS;
			break;
		case 4:
			//ничего не меняем (уже DS)
			operand_RM_seg = *DS;
			break;
		}
		if (log_to_console) cout << "LODS_B (" << (int)CX << " words) from [" << (int)operand_RM_seg << "]:[" << (int)Source_Index << "]";
		while (CX)
		{
			*ptr_AL = memory.read_2(Source_Index + operand_RM_seg * 16);
			Source_Index++;
			*ptr_AH = memory.read_2(Source_Index + operand_RM_seg * 16);
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
			memory.write_2(Destination_Index + *ES * 16, *ptr_AL);
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
			memory.write_2(Destination_Index + *ES * 16, *ptr_AL);
			Destination_Index++;
			memory.write_2(Destination_Index + *ES * 16, *ptr_AH);
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
	switch (Flag_segment_override)
	{
	case 0:
		//ничего не меняем
		operand_RM_seg = *DS;
		break;
	case 1:
		//меняем на ES
		operand_RM_seg = *ES;
		break;
	case 2:
		//меняем на CS
		operand_RM_seg = *CS;
		break;
	case 3:
		//меняем на SS
		operand_RM_seg = *SS;
		break;
	case 4:
		//ничего не меняем (уже DS)
		operand_RM_seg = *DS;
		break;
	}
	
	if (log_to_console) cout << "MOVES_B("<< (int)memory.read_2(Source_Index + operand_RM_seg * 16) << ") from [" << (int)operand_RM_seg << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
	memory_2[(Destination_Index + *ES * 16) & 0xFFFFF] = memory_2[(Source_Index + operand_RM_seg * 16) & 0xFFFFF];
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
	switch (Flag_segment_override)
	{
	case 0:
		//ничего не меняем
		operand_RM_seg = *DS;
		break;
	case 1:
		//меняем на ES
		operand_RM_seg = *ES;
		break;
	case 2:
		//меняем на CS
		operand_RM_seg = *CS;
		break;
	case 3:
		//меняем на SS
		operand_RM_seg = *SS;
		break;
	case 4:
		//ничего не меняем (уже DS)
		operand_RM_seg = *DS;
		break;
	}
	if (log_to_console) cout << "MOVES_W ("<< (int)memory.read_2(Source_Index + operand_RM_seg * 16) << (int)memory.read_2(Source_Index + 1 + operand_RM_seg * 16) << ") from [" << (int)operand_RM_seg << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
	memory.write_2(Destination_Index + *ES * 16, memory.read_2(Source_Index + operand_RM_seg * 16));
	Destination_Index++;
	Source_Index++;
	memory.write_2(Destination_Index + *ES * 16, memory.read_2(Source_Index + operand_RM_seg * 16));
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
	switch (Flag_segment_override)
	{
	case 0:
		//ничего не меняем
		operand_RM_seg = *DS;
		break;
	case 1:
		//меняем на ES
		operand_RM_seg = *ES;
		break;
	case 2:
		//меняем на CS
		operand_RM_seg = *CS;
		break;
	case 3:
		//меняем на SS
		operand_RM_seg = *SS;
		break;
	case 4:
		//ничего не меняем (уже DS)
		operand_RM_seg = *DS;
		break;
	}
	if (log_to_console) cout << "CMPS_8 SOLO (" << memory.read_2(Source_Index + operand_RM_seg * 16) << ") from [" << (int)operand_RM_seg << "]:[" << Source_Index << "] to [" << (int)*ES << "]:[" << Destination_Index << "]";
	uint8 Src = memory.read_2(Source_Index + operand_RM_seg * 16);
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
	switch (Flag_segment_override)
	{
	case 0:
		//ничего не меняем
		operand_RM_seg = *DS;
		break;
	case 1:
		//меняем на ES
		operand_RM_seg = *ES;
		break;
	case 2:
		//меняем на CS
		operand_RM_seg = *CS;
		break;
	case 3:
		//меняем на SS
		operand_RM_seg = *SS;
		break;
	case 4:
		//ничего не меняем (уже DS)
		operand_RM_seg = *DS;
		break;
	}
	if (log_to_console) cout << "CMPS_16 SOLO (" << (int)(memory.read_2(Source_Index + operand_RM_seg * 16) + memory.read_2(Source_Index + operand_RM_seg * 16)*256) << ") from [" << (int)operand_RM_seg << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
	*ptr_Src_L = memory_2[(Source_Index + operand_RM_seg * 16) & 0xFFFFF];
	Source_Index++;
	*ptr_Src_H = memory_2[(Source_Index + operand_RM_seg * 16) & 0xFFFFF];
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
	switch (Flag_segment_override)
	{
	case 0:
		//ничего не меняем
		operand_RM_seg = *DS;
		break;
	case 1:
		//меняем на ES
		operand_RM_seg = *ES;
		break;
	case 2:
		//меняем на CS
		operand_RM_seg = *CS;
		break;
	case 3:
		//меняем на SS
		operand_RM_seg = *SS;
		break;
	case 4:
		//ничего не меняем (уже DS)
		operand_RM_seg = *DS;
		break;
	}
	if (log_to_console) cout << "LODS_8 SOLO from [" << (int)operand_RM_seg << "]:[" << Source_Index << "] to AL";
	*ptr_AL = memory_2[(Source_Index + operand_RM_seg * 16) & 0xFFFFF];
	if (log_to_console) cout << "(" << (int)*ptr_AL << ")";
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
	switch (Flag_segment_override)
	{
	case 0:
		//ничего не меняем
		operand_RM_seg = *DS;
		break;
	case 1:
		//меняем на ES
		operand_RM_seg = *ES;
		break;
	case 2:
		//меняем на CS
		operand_RM_seg = *CS;
		break;
	case 3:
		//меняем на SS
		operand_RM_seg = *SS;
		break;
	case 4:
		//ничего не меняем (уже DS)
		operand_RM_seg = *DS;
		break;
	}
	if (log_to_console) cout << "LODS_16 SOLO from [" << (int)operand_RM_seg << "]:[" << Source_Index << "] to AX";
	*ptr_AL = memory_2[(Source_Index + operand_RM_seg * 16) & 0xFFFFF];
	Source_Index++;
	*ptr_AH = memory_2[(Source_Index + operand_RM_seg * 16) & 0xFFFFF];
	if (log_to_console) cout << "("  << (int)AX << ")";
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
	if (log_to_console) cout << "STOS_8 SOLO string from AL(" << (int)*ptr_AL  << ") to [" << (int)*ES << "]:[" << Destination_Index << "]";
	memory_2[(Destination_Index + *ES * 16) & 0xFFFFF] = *ptr_AL;
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
	if (log_to_console) cout << "STOS_16 SOLO string_w from AX (" << (int)AX << ") to [" << (int)*ES << "]:[" << Destination_Index << "]";
	memory_2[(Destination_Index + *ES * 16) & 0xFFFFF] = *ptr_AL;
	Destination_Index++;
	memory_2[(Destination_Index + *ES * 16) & 0xFFFFF] = *ptr_AH;
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
	if (log_to_console)SetConsoleTextAttribute(hConsole, 13);
	if (log_to_console) cout << "Direct Call within Segment. Ret to " << (int)*CS << ":" << Instruction_Pointer + 3;
	//else cout << "Direct Call within Segment. Ret to " << (int)*CS << ":" << Instruction_Pointer + 3 << endl;
	if (log_to_console)SetConsoleTextAttribute(hConsole, 7);
	Stack_Pointer--;
	memory.write_2(SS_data * 16 + Stack_Pointer, (Instruction_Pointer + 3) >> 8);
	Stack_Pointer--;
	memory.write_2(SS_data * 16 + Stack_Pointer, (Instruction_Pointer + 3) & 255);

	Instruction_Pointer += DispCalc16(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory.read_2(Instruction_Pointer + 2 + *CS * 16) * 256) + 3;
}
// INC/DEC/CALL/JUMP/PUSH
void Call_Jump_Push()				//Indirect (4 operations)
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];//второй байт
	uint8 OP = (byte2 >> 3) & 7;
	uint16 disp = 0;		//смещение
	additional_IPs = 0;
	uint16 old_IP = 0;
	uint16 new_IP = 0;
	uint16 new_CS = 0;
	uint32 stack_addr = 0;


	switch (OP)
	{
	case 0:  //INC R/M 16bit
	
		if ((byte2 >> 6) == 3)
		{
			// mod 11 - адрес в регистре
			
			Flag_AF = (((*ptr_r16[byte2 & 7] & 0x0F) + 1) >> 4) & 1;
			Flag_OF = (((*ptr_r16[byte2 & 7] & 0x7FFF) + 1) >> 15) & 1;
			++(*ptr_r16[byte2 & 7]);
			if (*ptr_r16[byte2 & 7]) Flag_ZF = 0;
			else Flag_ZF = 1;
			Flag_SF = (*ptr_r16[byte2 & 7] >> 15) & 1;
			Flag_PF = parity_check[*ptr_r16[byte2 & 7] & 255];
			
			if (log_to_console) cout << "INC " << reg16_name[byte2 & 7]  << " = " << (int)*ptr_r16[byte2 & 7];
			
			Instruction_Pointer += 2;
		}
		else
		{
			mod_RM_3(byte2);
			*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			operand_RM_offset++;
			*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			Flag_AF = (((*ptr_Src & 0x0F) + 1) >> 4) & 1;
			Flag_OF = (((*ptr_Src & 0x7FFF) + 1) >> 15) & 1;
			++(*ptr_Src);
			memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = *ptr_Src_H;
			operand_RM_offset--;
			memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = *ptr_Src_L;
			if (*ptr_Src) Flag_ZF = 0;
			else Flag_ZF = 1;
			Flag_SF = (*ptr_Src >> 15) & 1;
			Flag_PF = parity_check[*ptr_Src & 255];
			if (log_to_console) cout << "INC M" << OPCODE_comment << " = " << (int)*ptr_Src;

			Instruction_Pointer += 2 + additional_IPs;
		}
		
		break;

	case 1:  //DEC R/M 16bit

		if ((byte2 >> 6) == 3)
		{
			// mod 11 - адрес в регистре

			Flag_AF = (((*ptr_r16[byte2 & 7] & 0x0F) - 1) >> 4) & 1;
			Flag_OF = (((*ptr_r16[byte2 & 7] & 0x7FFF) - 1) >> 15) & 1;
			--(*ptr_r16[byte2 & 7]);
			if (*ptr_r16[byte2 & 7]) Flag_ZF = 0;
			else Flag_ZF = 1;
			Flag_SF = (*ptr_r16[byte2 & 7] >> 15) & 1;
			Flag_PF = parity_check[*ptr_r16[byte2 & 7] & 255];

			if (log_to_console) cout << "DEC " << reg16_name[byte2 & 7] << " = " << (int)*ptr_r16[byte2 & 7];

			Instruction_Pointer += 2;
		}
		else
		{
			mod_RM_3(byte2);
			*ptr_Src_L = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			operand_RM_offset++;
			*ptr_Src_H = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
			Flag_AF = (((*ptr_Src & 0x0F) - 1) >> 4) & 1;
			Flag_OF = (((*ptr_Src & 0x7FFF) - 1) >> 15) & 1;
			--(*ptr_Src);
			memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = *ptr_Src_H;
			operand_RM_offset--;
			memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] = *ptr_Src_L;
			if (*ptr_Src) Flag_ZF = 0;
			else Flag_ZF = 1;
			Flag_SF = (*ptr_Src >> 15) & 1;
			Flag_PF = parity_check[*ptr_Src & 255];
			if (log_to_console) cout << "DEC M" << OPCODE_comment << " = " << (int)*ptr_Src;

			Instruction_Pointer += 2 + additional_IPs;
		}

		break;

	case 2:  //Indirect Call within Segment

		//рассчитываем адрес где находится адрес перехода
		if ((byte2 >> 6) == 3)
		{
			// mod 11 - адрес в регистре
			new_IP = *ptr_r16[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			new_IP = memory_2[New_Addr_32] + memory_2[New_Addr_32 + 1] * 256;
		}

		old_IP = Instruction_Pointer;
		stack_addr = SS_data * 16 + Stack_Pointer - 1;
		Stack_Pointer--;
		memory.write_2(SS_data * 16 + Stack_Pointer, (Instruction_Pointer + 2 + additional_IPs) >> 8);
		Stack_Pointer--;
		memory.write_2(SS_data * 16 + Stack_Pointer, (Instruction_Pointer + 2 + additional_IPs) & 255);
		
		Instruction_Pointer = new_IP;
		if (log_to_console) SetConsoleTextAttribute(hConsole, 13);
		if (log_to_console) cout << "Near Indirect Call to " << (int)*CS << ":" << (int)Instruction_Pointer << " (ret to " << (int)*CS << ":" << (int)(old_IP + 2 + additional_IPs) << ")";
		//else cout << "Near Indirect Call to " << (int)*CS << ":" << (int)Instruction_Pointer << " (ret to " << (int)*CS << ":" << (int)(old_IP + 2 + additional_IPs) << ")";
		if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
		break;

	case 3:  //Indirect Intersegment Call
			//рассчитываем адрес, где хранится адрес перехода (IP + CS)
		
		if ((byte2 >> 6) == 3)
		{
			operand_RM_seg = DS_data * 16;
			operand_RM_offset = *ptr_r16[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
		}
		
		if (log_to_console) New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
		
		new_IP = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		new_IP = new_IP + memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] * 256;
		operand_RM_offset++;
		new_CS = memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF];
		operand_RM_offset++;
		new_CS = new_CS + memory_2[(operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF] * 256;

		Stack_Pointer--;
		memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] = CS_data >> 8;
		Stack_Pointer--;
		memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] = CS_data & 255;
		Stack_Pointer--;
		memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] =  (Instruction_Pointer + 2 + additional_IPs) >> 8;
		Stack_Pointer--;
		memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] =  (Instruction_Pointer + 2 + additional_IPs) & 255;
		
		Instruction_Pointer = new_IP;
		*CS = new_CS;

		if (log_to_console) SetConsoleTextAttribute(hConsole, 13);
		if (log_to_console) cout << "Indirect Intersegment Call to " << (int)*CS << ":" << (int)Instruction_Pointer;
		if (log_to_console) cout << " DEBUG addr = " << (int)New_Addr_32;
		if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
		break;

	case 4:  //Indirect Jump within Segment  (mod 100 rm)
			//рассчитываем новый адрес в текущем сегменте
		if ((byte2 >> 6) == 3)
		{
			Instruction_Pointer = *ptr_r16[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			Instruction_Pointer = memory_2[New_Addr_32] + memory_2[New_Addr_32 + 1] * 256;
		}
		
		if (log_to_console) SetConsoleTextAttribute(hConsole, 13);
		if (log_to_console) cout << "Indirect Jump within Segment to " << (int)*CS << ":" << (int)Instruction_Pointer;
		if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
		break;

	case 5:  //Indirect Intersegment Jump
			//рассчитываем адрес, где хранится адрес перехода (IP + CS)
		if ((byte2 >> 6) == 3)
		{
			New_Addr_32 = *ptr_r16[byte2 & 7];
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
		}

		Instruction_Pointer = memory_2[New_Addr_32] + memory_2[New_Addr_32 + 1] * 256;
		*CS = memory_2[New_Addr_32 + 2] + memory_2[New_Addr_32 + 3] * 256;
		if (log_to_console) SetConsoleTextAttribute(hConsole, 13);
		if (log_to_console) cout << "Indirect Intersegment Jump to " << (int)*CS << ":" << (int)Instruction_Pointer;
		if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
		break;

	case 6:  //PUSH Register/Memory 16bit
	case 7:  //ALIAS
			 //определяем источник данных
		if ((byte2 >> 6) == 3)
		{
			Src = *ptr_r16[byte2 & 7];
			if ((byte2 & 7) == 4) Src -= 2; //если кладем SP, он должен быть уже уменьшен
			if (log_to_console) cout << "PUSH " << reg16_name[byte2 & 7] << "(" << (int)Src << ")";
		}
		else
		{
			mod_RM_3(byte2);
			New_Addr_32 = (operand_RM_seg * 16 + operand_RM_offset) & 0xFFFFF;
			Src = memory_2[New_Addr_32] + memory_2[New_Addr_32 + 1] * 256;
			if (log_to_console) cout << "PUSH M" << OPCODE_comment;
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
	if (log_to_console) SetConsoleTextAttribute(hConsole, 13);
	if (log_to_console) cout << "Direct Intersegment Call to " << (int)*CS << ":" << (int)Instruction_Pointer;
	//else cout << "Direct Intersegment Call to " << (int)*CS << ":" << (int)Instruction_Pointer << endl;
	if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
}
void Jump_Near_8()				//Direct jump within Segment-Short
{
	Instruction_Pointer += DispCalc8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]) + 2;
	if (log_to_console) SetConsoleTextAttribute(hConsole, 15);
	if (log_to_console) cout << "Direct jump(8) within Segment-Short to " << (int)*CS << ":" << (int)Instruction_Pointer;
	if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
}
void Jump_Near_16()				//Direct jump within Segment-Short
{
	Instruction_Pointer += DispCalc16(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory.read_2(Instruction_Pointer + 2 + *CS * 16) * 256) + 3;
	if (log_to_console) SetConsoleTextAttribute(hConsole, 15);
	if (log_to_console) cout << "Direct jump(16) within Segment-Short to " << (int)*CS << ":" << (int)Instruction_Pointer;
	if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
}
void Jump_Far()					//Direct Intersegment Jump
{
	uint16 new_IP = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory.read_2(Instruction_Pointer + 2 + *CS * 16) * 256;
	*CS = memory.read_2(Instruction_Pointer + 3 + *CS * 16) + memory.read_2(Instruction_Pointer + 4 + *CS * 16) * 256;
	Instruction_Pointer = new_IP;
	if (log_to_console) SetConsoleTextAttribute(hConsole, 15);
	if (log_to_console) cout << "Direct Intersegment Jump to " << (int)*CS << ":" << (int)Instruction_Pointer;
	if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
}

// Conditional Jumps
void JE_JZ()			// JE/JZ = Jump on Equal/Zero
{
	if (Flag_ZF)
	{
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]) + 2;
		//if (log_to_console) SetConsoleTextAttribute(hConsole, 15);
		if (log_to_console) cout << "Jump on Equal / Zero (JE/JZ) to " << (int)*CS << ":" << (int)Instruction_Pointer;
		//if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
	}
	else
	{
		Instruction_Pointer += 2;
		//if (log_to_console) SetConsoleTextAttribute(hConsole, 15);
		if (log_to_console) cout << "NOT Jump on Equal / Zero (JE/JZ)";
		//if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
	}
}
void JL_JNGE()			// JL/JNGE = Jump on Less/Not Greater, or Equal
{
	if (Flag_SF ^ Flag_OF)
	{
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]) + 2;
		//if (log_to_console) SetConsoleTextAttribute(hConsole, 15);
		if (log_to_console) cout << "Jump on Less/Not Greater, or Equal (JL/JNGE) to " << (int)*CS << ":" << (int)Instruction_Pointer;
		//if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
	}
	else
	{
		Instruction_Pointer += 2;
		//if (log_to_console) SetConsoleTextAttribute(hConsole, 15);
		if (log_to_console) cout << "NOT Jump on Less/Not Greater, or Equal (JL/JNGE)";
		//if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
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
	if (log_to_console) SetConsoleTextAttribute(hConsole, 13);
	if (log_to_console) cout << "Near RET to " << (int)*CS << ":" << (int)Instruction_Pointer;
	//else cout << "Near RET to " << (int)*CS << ":" << (int)Instruction_Pointer << endl;
	if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
}
void RET_Segment_IMM_SP()			//Return Within Segment Adding Immediate to SP
{
	uint16 pop_bytes = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] + memory.read_2(Instruction_Pointer + 2 + *CS * 16) * 256;
	uint32 stack_addr = SS_data * 16 + Stack_Pointer;
	Instruction_Pointer = memory.read_2(stack_addr) + memory.read_2(stack_addr + 1) * 256;
	Stack_Pointer += 2 + pop_bytes;
	if (log_to_console) SetConsoleTextAttribute(hConsole, 13);
	if (log_to_console) cout << "Near RET to " << (int)*CS << ":" << (int)Instruction_Pointer << " + " << pop_bytes << " bytes popped";
	//else  cout << "Near RET to " << (int)*CS << ":" << (int)Instruction_Pointer << " + " << pop_bytes << " bytes popped" << endl;
	if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
}
void RET_Inter_Segment()			//Return Intersegment
{
	uint32 stack_addr = SS_data * 16 + Stack_Pointer;
	Instruction_Pointer = memory.read_2(stack_addr) + memory.read_2(stack_addr + 1) * 256;
	Stack_Pointer += 2;
	*CS = memory.read_2(Stack_Pointer + SS_data * 16) + memory.read_2(Stack_Pointer + 1 + SS_data * 16) * 256;
	Stack_Pointer += 2;
	if (log_to_console) SetConsoleTextAttribute(hConsole, 13);
	if (log_to_console) cout << "Far RET to " << (int)*CS << ":" << (int)Instruction_Pointer;
	//else cout << "Far RET to " << (int)*CS << ":" << (int)Instruction_Pointer << endl;
	if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
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
	if (log_to_console) SetConsoleTextAttribute(hConsole, 13);
	if (log_to_console || last_INT == 0x21) cout << "Far RET to " << (int)*CS << ":" << (int)Instruction_Pointer << " + " << pop_bytes << " bytes popped " << " AX(" << (int)AX << ") CF=" << (int)Flag_CF;
	if (!log_to_console && last_INT == 0x21) cout << endl;
	if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
}

//========Conditional Transfer-===============

void INT_N()			//INT = Interrupt
{
	//перехват системных прерываний
	//syscall(memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]);
	//Instruction_Pointer += 2;
	//return;

	uint8 int_type = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF];

	last_INT = int_type;

	//if (int_type == 0) cout << "int0!";

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
	if (log_to_console) SetConsoleTextAttribute(hConsole, 10);
	//if (int_type != 0x10) cout << "INT " << (int)int_type << " (AX=" << (int)AX << ") -> " << (int)new_CS << ":" << (int)Instruction_Pointer << " call from " << old << endl;
	if (log_to_console) cout << "INT " << (int)int_type << " (AX=" << (int)AX << ") -> " << (int)new_CS << ":" << (int)Instruction_Pointer << " call from " << old;
	if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
	if ((int_type == 0x13) && !test_mode)
	{
		//step_mode = 1;
		//if (*ptr_DL > 4) HDD_monitor.log("INT13(HDD) drv=" + int_to_hex(DX & 255, 2) + " AX = " + int_to_hex(AX, 4));
		//добавляем инфу по вызову
		if (log_to_console_FDD) cout << "INT13 DRV=" << int_to_hex(DX & 255, 2) << " AH=" << (int)*ptr_AH << " ";
		if (log_to_console_FDD) switch (*ptr_AH)
		{
		case 0x0:
			cout << "RESET DISKETTE SYSTEM";
			break;
		case 0x1:
			cout << "READ THE STATUS OF THE SYSTEM INTO (AH)";
			break;
		case 0x2:
			cout << "READ THE DESIRED SECTORS INTO MEMORY";
			break;
		case 0x3:
			cout << "WRITE THE DESIRED SECTORS FROM MEMORY";
			break;
		case 0x4:
			cout << "VERIFY THE DESIRED SECTORS";
			break;
		case 0x5:
			cout << "FORMAT THE DESIRED TRACK";
			break;
		case 0x8:
			cout << "READ DRIVE PARAMETERS";
			break;
		case 0x15:
			cout << "READ DASD TYPE";
			break;
		case 0x16:
			cout << "DISK CHANGE LINE STATUS";
			break;
		case 0x17:
			cout << "SET DASD TYPE FOR FORMAT";
			break;
		case 0x18:
			cout << "SET MEDIA TYPE FOR FORMAT (tracks=" << dec << (int)(*ptr_CH + (*ptr_CL >> 6) * 256) << " sectors=" << (int)(*ptr_CL & 0b00111111) << " drv=" << (int)*ptr_DL;
			break;
		}
		if (log_to_console_FDD) cout << endl;


#ifdef DEBUG
		if (log_to_console_FDD && (AX >> 8) == 2 && (*ptr_DL < 4)) FDD_monitor.log("INT13(READ) drv=" + int_to_hex(DX & 255, 2) + " head=" + int_to_hex(DX >> 8, 2) + " track=" + int_to_hex(CX >> 8, 2) + " sector_beg=" + int_to_hex(CX & 255, 2) + " sector_num=" + int_to_hex(AX & 255, 2) + " buff = [" + int_to_hex(*ES, 4) + ":" + int_to_hex(BX, 4) + "]");
		if (log_to_console_HDD && (AX >> 8) == 2 && (*ptr_DL > 4)) HDD_monitor.log("INT13(HDD READ) drv=" + int_to_hex(DX & 255, 2) + " head=" + int_to_hex(DX >> 8, 2) + " track=" + int_to_hex(CX >> 8, 2) + " sector_beg=" + int_to_hex(CX & 255, 2) + " sector_num=" + int_to_hex(AX & 255, 2) + " buff = [" + int_to_hex(*ES, 4) + ":" + int_to_hex(BX, 4) + "]");
		if (log_to_console_HDD && (AX >> 8) == 0xA && (*ptr_DL > 4)) HDD_monitor.log("INT13(HDD READLONG) drv=" + int_to_hex(DX & 255,2) + " head=" + int_to_hex(DX >> 8, 2) + " track=" + int_to_hex(CX >> 8, 2) + " sector_beg=" + int_to_hex(CX & 255, 2) + " sector_num=" + int_to_hex(AX & 255,2) + " buff = [" + int_to_hex(*ES,4) + ":" + int_to_hex(BX,4) + "]");
		if (log_to_console_FDD && (AX >> 8) == 3 && (*ptr_DL < 4)) FDD_monitor.log("INT13(WRITE) drv=" + int_to_hex(DX & 255, 2) + " head=" + int_to_hex(DX >> 8, 2) + " track=" + int_to_hex(CX >> 8, 2) + " sector_beg=" + int_to_hex(CX & 255, 2) + " sector_num=" + int_to_hex(AX & 255, 2) + " buff = [" + int_to_hex(*ES, 4) + ":" + int_to_hex(BX, 4) + "]");
		if (log_to_console_HDD && (AX >> 8) == 3 && (*ptr_DL > 4)) HDD_monitor.log("INT13(HDD WRITE) drv=" + int_to_hex(DX & 255, 2) + " head=" + int_to_hex(DX >> 8, 2) + " track=" + int_to_hex(CX >> 8, 2) + " sector_beg=" + int_to_hex(CX & 255, 2) + " sector_num=" + int_to_hex(AX & 255, 2) + " buff = [" + int_to_hex(*ES, 4) + ":" + int_to_hex(BX, 4) + "]");
		if (log_to_console_HDD && (AX >> 8) == 0xB && (*ptr_DL > 4)) HDD_monitor.log("INT13(HDD WRITE LONG) drv=" + int_to_hex(DX & 255, 2) + " head=" + int_to_hex(DX >> 8, 2) + " track=" + int_to_hex(CX >> 8, 2) + " sector_beg=" + int_to_hex(CX & 255, 2) + " sector_num=" + int_to_hex(AX & 255, 2) + " buff = [" + int_to_hex(*ES, 4) + ":" + int_to_hex(BX, 4) + "]");
		if (log_to_console_HDD && (AX >> 8) == 0x10 && (*ptr_DL > 4)) HDD_monitor.log("INT13(HDD TEST DISK READY) drv=" + int_to_hex(DX & 255, 2));
		if (log_to_console_HDD && (AX >> 8) == 0x11 && (*ptr_DL > 4)) HDD_monitor.log("INT13(HDD RECALIBRATE) drv=" + int_to_hex(DX & 255, 2));
		if (log_to_console_HDD && (AX >> 8) == 0x12 && (*ptr_DL > 4)) HDD_monitor.log("INT13(HDD CTRL RAM DIAG) drv=" + int_to_hex(DX & 255, 2));
		if (log_to_console_HDD && (AX >> 8) == 0x13 && (*ptr_DL > 4)) HDD_monitor.log("INT13(HDD CTRL DRIVE DIAG) drv=" + int_to_hex(DX & 255, 2));
		if (log_to_console_HDD && (AX >> 8) == 0x14 && (*ptr_DL > 4)) HDD_monitor.log("INT13(HDD CTRL INTNL DIAG) drv=" + int_to_hex(DX & 255, 2));
#endif

	}
	//прерывания DOS
	if ((int_type == 0x21) && !test_mode && *ptr_AH != 0x2c)
	{
		if (log_to_console_DOS) cout << "INT 21H from [" << (int)*CS << ":" << (int)Instruction_Pointer << "]" << " (AX=" << (int)AX << ") ";
		if (*ptr_AH == 0x40)
		{
			if (log_to_console_DOS) SetConsoleTextAttribute(hConsole, 14); //выделение функции 40
			//if (*ptr_AL == 0) log_to_console = 1;
		}
		if (log_to_console_DOS) cout << get_int21_data() << endl;
		if (log_to_console_DOS) SetConsoleTextAttribute(hConsole, 7);
		//if (*ptr_AH == 0x25) step_mode = 1;
		//if (!log_to_console) cout << endl;
	}
	
	if (int_type == 0x10 && !test_mode && log_to_console_DOS)
	{
		if (*ptr_AH == 0 || *ptr_AH == 0xb) cout << "INT 10H from [" << (int)*CS << ":" << (int)Instruction_Pointer << "]" << " (AX=" << (int)AX << ") -> ";
		if (*ptr_AH == 0 || *ptr_AH == 0xb) cout << get_int10_data() << endl;
	}
		
	if (int_type == 0x19)
	{
		HDD_monitor.log("INT19 - boot");
		//step_mode = 1;
	}
}
void INT_3()			//INT = Interrupt Type 3
{
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
	if (log_to_console)cout << "INT_3 " << (int)int_type << " -> " << (int)new_CS << ":" << (int)Instruction_Pointer;
}
void INT_O()			//INTO = Interrupt on Overflow
{
	if (Flag_OF)
	{
		uint8 int_type = 4;
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
		if (log_to_console) cout << "INT_0F " << " -> " << (int)new_CS << ":" << (int)Instruction_Pointer;
	}
	else
	{
		Instruction_Pointer++;
	}
}
void I_RET()			//Interrupt Return
{
	//POP IP
	Instruction_Pointer = memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF];
	Stack_Pointer++;
	Instruction_Pointer += memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] * 256;
	Stack_Pointer++;

	//POP CS
	*CS = memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF];
	Stack_Pointer++;
	*CS += memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] * 256;
	Stack_Pointer++;

	//POP Flags
	uint16 Flags = memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF];
	Stack_Pointer++;
	Flags += memory_2[(SS_data * 16 + Stack_Pointer) & 0xFFFFF] * 256;
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

	//if (Flag_TF) step_mode = 1;

	if (log_to_console) SetConsoleTextAttribute(hConsole, 10);
	if (log_to_console) cout << "I_RET to " << *CS << ":" << Instruction_Pointer << " AX(" << (int)AX << ") CF=" << (int)Flag_CF;
	if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
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
	int_ctrl.set_timeout(1); //ставим задержку неактивности иначе возможны ошибки
	Instruction_Pointer++;
}
void HLT()					//Halt
{
	halt_cpu = true;
	Instruction_Pointer++;
	if (log_to_console) cout << "Halt! IP=" << (int)Instruction_Pointer;
}
void Wait()					//Wait
{
	cout << "Wait 8087 execute command (NOP)";
	if (!log_to_console) cout << endl;
	Instruction_Pointer++;
	step_mode = 1;
}
void Lock()					//Bus lock prefix
{
	if (log_to_console) cout << "Lock command";
	//bus_lock = 2;  //флаг блокировки шины
	//log_to_console = 1;
	//step_mode = 1;
	Instruction_Pointer++;
}
void Esc_8087()				//Call 8087
{
	//Декодируем инструкцию 8087
	uint8 f_opcode = ((memory.read_2(Instruction_Pointer + *CS * 16) & 7) << 3) | ((memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] >> 3) & 7);
	op_code_table_8087[f_opcode]();
}

// ================ UNDOC ==============

void CALC()
{
	if (Flag_CF) *ptr_AL = 0xFF;
	else *ptr_AL = 0;
	if (log_to_console) cout << "CALC AL = " << (int)(AX & 255);
	Instruction_Pointer++;
}

//=================== 8087 ========================

void Esc_8087_001_000_Load()
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		//LOAD STi to ST0
		if(log_to_console_8087) cout << "8087 (LOAD STi to ST0)" << endl;
		Instruction_Pointer += 2;

		//FFREE = Free ST(i)  ???
	}
	else
	{
	
		//Load 32bit real to ST0
		if (log_to_console_8087) cout << "8087 (Load 32bit real to ST0)" << endl;
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
	}
}

void Esc_8087_011_000_Load()
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		if (log_to_console_8087) cout << "8087 illegal op" << endl;
		Instruction_Pointer += 2;
	}
	else
	{
		//Load 32bit integer to ST0
		if (log_to_console_8087) cout << "8087 (Load 32bit integer to ST0)" << endl;
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
	}
}

void Esc_8087_101_000_Load()
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		if (log_to_console_8087) cout << "8087 illegal op" << endl;
		Instruction_Pointer += 2;
	}
	else
	{
		//Load 64bit real to ST0
		if (log_to_console_8087) cout << "8087 (64bit real to ST0)" << endl;
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
	}
}

void Esc_8087_111_000_Load()
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		if (log_to_console_8087) cout << "8087 illegal op" << endl;
		Instruction_Pointer += 2;
	}
	else
	{
		
		// Load 64bit integer to ST0
		if (log_to_console_8087) cout << "8087 (64bit integer to ST0)" << endl;
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
	}
}

void Esc_8087_111_101_Load()  //LOAD long INT to ST0
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		if (log_to_console_8087) cout << "8087 illegal op" << endl;
		Instruction_Pointer += 2;
	}
	else
	{
		if (log_to_console_8087) cout << "8087 (LOAD long INT to ST0)" << endl;
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
	}
}
void Esc_8087_011_101_Load()	//LOAD temp real to ST0
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		if (log_to_console_8087) cout << "8087 illegal op" << endl;
		Instruction_Pointer += 2;
	}
	else
	{
		if (log_to_console_8087) cout << "8087 (LOAD temp real to ST0)" << endl;
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
	}
}
void Esc_8087_111_100_Load()	//LOAD BCD to ST0
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		if (log_to_console_8087) cout << "8087 illegal op" << endl;
		Instruction_Pointer += 2;
	}
	else
	{
		if (log_to_console_8087) cout << "8087 (LOAD BCD to ST0)" << endl;
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
	}
}

void Esc_8087_001_010_Store()	//STORE ST0 to int/real
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		//FNOP = No Operation
		if (log_to_console_8087) cout << "FNOP" << endl;
		Instruction_Pointer += 2;
	}
	else
	{
		//STORE 32bit real
		if (log_to_console_8087) cout << "8087 (STORE ST0 to int/real)" << endl;
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
	}
}

void Esc_8087_011_010_Store()	//STORE ST0 to int/real
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		if (log_to_console_8087) cout << "8087 illegal op" << endl;
		Instruction_Pointer += 2;
	}
	else
	{
		
		//STORE 32bit integer
		if (log_to_console_8087) cout << "8087 (STORE ST0 to int/real)" << endl;
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
	}
}
void Esc_8087_101_010_Store()	//STORE ST0 to int/real
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		//LOAD ST0 to STi
		if (log_to_console_8087) cout << "8087 (LOAD ST0 to STi)" << endl;
		Instruction_Pointer += 2;
	}
	else
	{
		//Store 64bit real ST0 to memory
		if (log_to_console_8087) cout << "8087 (Store 64bit real ST0 to memory)" << endl;
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
	}
}
void Esc_8087_111_010_Store()	//STORE ST0 to int/real
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		if (log_to_console_8087) cout << "8087 illegal op" << endl;
		Instruction_Pointer += 2;
	}
	else
	{
		//STORE 64bit integer to MEM
		if (log_to_console_8087) cout << "8087 (STORE 64bit integer to MEM)" << endl;
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
	}
}

void Esc_8087_001_011_StorePop() //FSTP = Store and Pop
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		if (log_to_console_8087) cout << "8087 illegal" << endl;
		Instruction_Pointer += 2;
	}
	else
	{
		//STOREPOP 32bit real
		if (log_to_console_8087) cout << "8087 (STOREPOP 32bit real)" << endl;
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
	}
}
void Esc_8087_011_011_StorePop() //FSTP = Store and Pop
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		if (log_to_console_8087) cout << "8087 illegal" << endl;
		Instruction_Pointer += 2;
	}
	else
	{
		//STOREPOP 32bit integer
		if (log_to_console_8087) cout << "8087 (STOREPOP 32bit integer)" << endl;
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
	}
}
void Esc_8087_101_011_StorePop() //FSTP = Store and Pop + ST0 to STi 
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		//StorePop ST0 to STi
		if (log_to_console_8087) cout << "8087 (StorePop ST0 to STi)" << endl;
		Instruction_Pointer += 2;
	}
	else
	{
		//StorePop 64bit real
		if (log_to_console_8087) cout << "8087 (StorePop 64bit real)" << endl;
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
	}

}
void Esc_8087_111_011_StorePop() //FSTP = Store and Pop
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		if (log_to_console_8087) cout << "8087 illegal" << endl;
		Instruction_Pointer += 2;
	}
	else
	{
		//StorePop 64bit integer
		if (log_to_console_8087) cout << "8087 (StorePop 64bit integer)" << endl;
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
	}
}

void Esc_8087_111_111_StorePop() //StorePop ST0 Long Int to MEM
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		if (log_to_console_8087) cout << "8087 illegal" << endl;
		Instruction_Pointer += 2;
	}
	else
	{
		//StorePop long integer to MEM
		if (log_to_console_8087) cout << "8087 (StorePop long integer)" << endl;
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
	}
}
void Esc_8087_011_111_StorePop() //StorePop ST0 to TMP real mem
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		if (log_to_console_8087) cout << "8087 illegal" << endl;
		Instruction_Pointer += 2;
	}
	else
	{
		//StorePop ST0 to temp real MEM
		if (log_to_console_8087) cout << "8087 (StorePop to temp real)" << endl;
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
	}
}
void Esc_8087_111_110_StorePop() //StorePop ST0 to BCD mem
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		if (log_to_console_8087) cout << "8087 illegal" << endl;
		Instruction_Pointer += 2;
	}
	else
	{
		//StorePop ST0 to BCD
		if (log_to_console_8087) cout << "8087 (StorePop ST0 to BCD)" << endl;
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
	}
}

void Esc_8087_001_001_FXCH()	//EXchange ST0 - STi
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		if (log_to_console_8087) cout << "8087 (EXchange ST0 - STi)" << endl;
		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		if (log_to_console_8087) cout << "8087 illegal" << endl;
		Instruction_Pointer += 2 + additional_IPs;;
	}
}

void Esc_8087_000_010_FCOM()		//FCOM = Compare + STi to ST0
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		//StorePop ST0 to STi
		if (log_to_console_8087) cout << "8087 (StorePop ST0 to STi)" << endl;
		Instruction_Pointer += 2;
	}
	else
	{
		//StorePop 64bit real
		if (log_to_console_8087) cout << "8087 (StorePop 64bit real)" << endl;
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;

	}
}
void Esc_8087_010_010_FCOM()		//FCOM = Compare
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 illegal op" << endl;
	}
	else
	{
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FCOM = Compare" << endl;
	}
}
void Esc_8087_100_010_FCOM()		//FCOM = Compare
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		if (log_to_console_8087) cout << "8087 illegal" << endl;
		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FCOM = Compare" << endl;
	}
}
void Esc_8087_110_010_FCOM()		//FCOM = Compare
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		if (log_to_console_8087) cout << "8087 illegal" << endl;
		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FCOM = Compare" << endl;
	}
}
void Esc_8087_000_011_FCOM()		//FcomPop + STi to ST0
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		//STi to ST0
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 STi to ST0" << endl;
	}
	else
	{
		//FCOMP
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FCOMP" << endl;
	}
}
void Esc_8087_010_011_FCOM()		//FcomPop
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		Instruction_Pointer += 2;
	}
	else
	{
		//FCOMP
		byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FCOMP" << endl;
	}
}
void Esc_8087_100_011_FCOM()		//FcomPop
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		Instruction_Pointer += 2;
	}
	else
	{
		//FCOMP
		byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FCOMP" << endl;
	}
}
void Esc_8087_110_011_FCOM()		//FcomPop + FCOMPP
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		//FCOMPP
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 FCOMPP" << endl;
	}
	else
	{
		//FCOMP
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FCOMP" << endl;

	}
}

void Esc_8087_001_100_TEST()		//FTST/FXAM/FABS/FCHS
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		if ((byte2 & 0b11000111) == 0b11000000)
		{
			//FCHS = Change Sign of ST(O)
			if (log_to_console_8087) cout << "8087 FCHS = Change Sign of ST(O)" << endl;
			Instruction_Pointer += 2;
			return;
		}

		if ((byte2 & 0b11000111) == 0b11000001)
		{
			//FADS = Absolute Value of ST(O)
			if (log_to_console_8087) cout << "8087 FADS = Absolute Value of ST(O)" << endl;
			Instruction_Pointer += 2;
			return;
		}

		if ((byte2 & 0b11000111) == 0b11000100)
		{
			//FTST = Test ST(O)
			if (log_to_console_8087) cout << "8087 FTST = Test ST(O)" << endl;
			Instruction_Pointer += 2;
			return;
		}

		if ((byte2 & 0b11000111) == 0b11000101)
		{
			//FXAM = Examine ST(O)
			if (log_to_console_8087) cout << "8087 FXAM = Examine ST(O)" << endl;
			Instruction_Pointer += 2;
			return;
		}

		//отсутствующие инструкции
		if (log_to_console_8087) cout << "8087 Illegal instruction" << endl;
		Instruction_Pointer += 2;

	}
	else
	{
		//mod r/m
		//FLDENV = Load Environment
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FLDENV = Load Environment" << endl;
	}

}

void Esc_8087_000_000_FADD()		//FADD
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	
	if ((byte2 >> 6) == 3)
	{
		//dP = 00
		//STi to ST0
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 FADD" << endl;
	}
	else
	{
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FADD" << endl;
	}
}
void Esc_8087_010_000_FADD()		//FADD
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		//dP = 01
		//STi to ST0
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 FADD" << endl;
	}
	else
	{
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FADD" << endl;
	}
}
void Esc_8087_100_000_FADD()		//FADD
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		//dP = 10
		//STi to ST0
		if (log_to_console_8087) cout << "8087 FADD" << endl;
		Instruction_Pointer += 2;
	}
	else
	{
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FADD" << endl;
	}
}
void Esc_8087_110_000_FADD()		//FADD
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		//dP = 11
		//STi to ST0
		if (log_to_console_8087) cout << "8087 FADD" << endl;
		Instruction_Pointer += 2;
	}
	else
	{
		//mod R/M
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FADD" << endl;
	}
}

void Esc_8087_000_100_FSUB()		//FSUB R = 0
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		//dP = 00
		//STi to ST0
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 FSUB R = 0" << endl;
	}
	else
	{
		//mod R/M
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FSUB R = 0" << endl;
	}
}
void Esc_8087_000_101_FSUB()		//FSUB R = 1
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		//dP = 00
		//STi to ST0
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 FSUB R = 1" << endl;
	}
	else
	{
		//mod R/M
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FSUB R = 1" << endl;
	}
}
void Esc_8087_010_100_FSUB()		//FSUB R = 0
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		//dP = 01
		//STi to ST0
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 FSUB R = 0" << endl;
	}
	else
	{
		//mod R/M
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FSUB R = 0" << endl;
	}
}
void Esc_8087_010_101_FSUB()		//FSUB R = 1
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		//dP = 01
		//STi to ST0
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 FSUB R = 1" << endl;
	}
	else
	{
		//mod R/M
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FSUB R = 1" << endl;
	}
}
void Esc_8087_100_100_FSUB()		//FSUB R = 0
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		//dP = 10
		//STi to ST0
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 FSUB R = 0" << endl;
	}
	else
	{
		//mod R/M
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FSUB R = 0" << endl;
	}
}
void Esc_8087_100_101_FSUB()		//FSUB R = 1
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		//dP = 10
		//STi to ST0
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 FSUB R = 1" << endl;
	}
	else
	{
		//mod R/M
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FSUB R = 1" << endl;
	}
}
void Esc_8087_110_100_FSUB()		//FSUB R = 0
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		//dP = 11
		//STi to ST0
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 FSUB R = 0" << endl;
	}
	else
	{
		//mod R/M
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FSUB R = 0" << endl;
	}
}
void Esc_8087_110_101_FSUB()		//FSUB R = 1
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		//dP = 11
		//STi to ST0
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 FSUB R = 1" << endl;
	}
	else
	{
		//mod R/M
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FSUB R = 1" << endl;
	}
}

void Esc_8087_000_001_FMUL()		//FMUL
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		//dP = 00
		//STi to ST0
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 FMUL" << endl;
	}
	else
	{
		//mod R/M
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FMUL" << endl;
	}
}
void Esc_8087_010_001_FMUL()		//FMUL
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		//dP = 01
		//STi to ST0
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 FMUL" << endl;
	}
	else
	{
		//mod R/M
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FMUL" << endl;
	}
}
void Esc_8087_100_001_FMUL()		//FMUL
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		//dP = 10
		//STi to ST0
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 FMUL" << endl;
	}
	else
	{
		//mod R/M
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FMUL" << endl;
	}
}
void Esc_8087_110_001_FMUL()		//FMUL
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		//dP = 11
		//STi to ST0
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 FMUL" << endl;
	}
	else
	{
		//mod R/M
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FMUL" << endl;
	}
}

void Esc_8087_000_110_FDIV()		//FDIV R = 0
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		//dP = 00
		//STi to ST0
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 FDIV R = 0" << endl;
	}
	else
	{
		//mod R/M
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FDIV R = 0" << endl;
	}
}
void Esc_8087_000_111_FDIV()		//FDIV R = 1
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		//dP = 00
		//STi to ST0
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 FDIV R = 1" << endl;
	}
	else
	{
		//mod R/M
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FDIV R = 1" << endl;
	}
}
void Esc_8087_010_110_FDIV()		//FDIV R = 0
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		//dP = 01
		//STi to ST0
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 FDIV R = 0" << endl;
	}
	else
	{
		//mod R/M
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FDIV R = 0" << endl;
	}
}
void Esc_8087_010_111_FDIV()		//FDIV R = 1
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		//dP = 01
		//STi to ST0
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 FDIV R = 1" << endl;
	}
	else
	{
		//mod R/M
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FDIV R = 1" << endl;
	}
}
void Esc_8087_100_110_FDIV()		//FDIV R = 0
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		//dP = 10
		//STi to ST0
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 FDIV R = 0" << endl;
	}
	else
	{
		//mod R/M
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FDIV R = 0" << endl;
	}
}
void Esc_8087_100_111_FDIV()		//FDIV R = 1
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		//dP = 10
		//STi to ST0
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 FDIV R = 1" << endl;
	}
	else
	{
		//mod R/M
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FDIV R = 1" << endl;
	}
}
void Esc_8087_110_110_FDIV()		//FDIV R = 0
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		//dP = 11
		//STi to ST0
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 FDIV R = 0" << endl;
	}
	else
	{
		//mod R/M
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FDIV R = 0" << endl;
	}
}
void Esc_8087_110_111_FDIV()		//FDIV R = 1
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		//dP = 11
		//STi to ST0
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "8087 FDIV R = 1" << endl;
	}
	else
	{
		//mod R/M
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FDIV R = 1" << endl;
	}
}

void Esc_8087_001_111_FSQRT()		//FSQRT/FSCALE/FPREM/FRNDINIT/FYL2XP1/FSTCW
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		switch(byte2 & 7)
		{ 
		case 0: //FPREM	ST0/ST1	

			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 FPREM = Partial Remainder of ST(O) -;- ST(1)" << endl;
			break;

		case 1: //FYL2XP1

			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 FYL2XP1 = ST(1) X Log 2 [ST(O) + 1]" << endl;
			break;

		case 2: //FSQRT	of ST0	
	
			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 FSQRT = Square Root of ST(O)" << endl;
			break;

		case 4: //FRNDINIT ST0 to INT
			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 FRNDINT = Round ST(O) to Integer" << endl;
			break;

		case 5: //FSCALE ST0 by ST1
			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 FSCALE = Scale ST(O) by ST(1)" << endl;
			break;
		
		default:
			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 illegal op" << endl;
			break;

		}
	}
	else
	{
		//mod r/m
		//FSTCW
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FSTCW" << endl;
	}
}

void Esc_8087_001_110_FXTRACT()	//FXTRACT/FPTAN/F2XM1/FPATAN/FYL2X
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		switch (byte2 & 0b11000111)
		{
		case 0b11000000:

			//F2XM1
			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 F2XM1 = 2 s T(O) -1" << endl;
			break;

		case 0b11000001:

			//FYL2X
			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 FYL2X = ST(1) X Log 2 [ST(O)]" << endl;
			break;

		case 0b11000010:

			//FPTAN = Partial Tangent of ST(O)
			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 FPTAN = Partial Tangent of ST(O)" << endl;
			break;

		case 0b11000011:

			//FPATAN = Partial Arctangent of ST(O) / ST(1)
			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 FPATAN = Partial Arctangent of ST(O) / ST(1)" << endl;
			break;

		case 0b11000100:

			//FXTRACT = Extract Components of ST(O)
			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 FXTRACT = Extract Components of ST(O)" << endl;
			break;

		case 0b11000101:

			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 illegal_OP 001_110" << endl;
			break;

		case 0b11000110:

			//FDECSTP = Decrement Stack Pointer
			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 FDECSTP = Decrement Stack Pointer" << endl;
			break;

		case 0b11000111:

			//FINCSTP = Increment Stack Pointer
			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 FINCSTP = Increment Stack Pointer" << endl;
			break;
		}
	}
	else
	{
		//mod r/m
		//FSTENV = Store Environment
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FSTENV = Store Environment" << endl;
	}
}

void Esc_8087_001_101_FLDZ()		//FLDZ/FLD1/FLOPI/FLOL2T/FLOCW
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm

	if ((byte2 >> 6) == 3)
	{
		switch (byte2 & 0b11000111)
		{

		case 0b11000000:
			//FLD1
			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 FLD1 = Load + 1.0 into ST(O)" << endl;
			break;

		case 0b11000001:
			//FLOL2T
			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 FLOL2T" << endl;
			break;

		case 0b11000011:
			//FLOPI
			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 FLOPI" << endl;
			break;

		case 0b11000100:
			//FLOLG2
			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 FLOLG2 = Load Log lO 2 into ST(O)" << endl;
			break;

		case 0b11000101:
			//FLDLN2
			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 FLDLN2 = Load Loge 2 into ST(O)" << endl;
			break;

		case 0b11000110:
			//FLDZ
			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 FLDZ = Load + 0.0 into ST(O)" << endl;
			break;

		default:
			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 illegal op" << endl;
			break;

		}
	}
	else
	{
		//mod r/m
		//FLOCW
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FLOCW" << endl;
	}
}

void Esc_8087_011_100_FINIT()		//FINIT/FENI/FDISI
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		switch (byte2 & 0b11000111)
		{

		case 0b11000000:
			//FENI = Enable Interrupts
			//Flag_IF = 1;
			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 FENI = Enable Interrupts" << endl;
			break;

		case 0b11000001:
			//FDISI = Disable Interrupts
			//Flag_IF = 0;
			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 FDISI = Disable Interrupts" << endl;
			break;

		case 0b11000010:
			//FCLEX = Clear Exceptions
			Flag_IF = 0;
			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 FCLEX = Clear Exceptions" << endl;
			break;

		case 0b11000011:
			//FINIT = Initialize NDP
			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 FINIT = Initialize NDP" << endl;
			break;
		default:
			Instruction_Pointer += 2;
			if (log_to_console_8087) cout << "8087 illegal op 011_100" << endl;
			break;

		
		}
	}
	else
	{
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 illegal op" << endl;
	}
}

void Esc_8087_101_111_FSTSW()		//FSTSW
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "ollegal op" << endl;
	}
	else
	{
		//FSTSW = Store Status Word
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FSTSW = Store Status Word" << endl;
	}
}

void Esc_8087_101_110_FSAVE()		//FSAVE
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "ollegal op" << endl;
	}
	else
	{
		//FSAVE = Save State
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FSAVE = Save State" << endl;
	}
}

void Esc_8087_101_100_FRSTOR()	//FRSTOR
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "ollegal op" << endl;
	}
	else
	{
		//FRSTOR = Restore State
		mod_RM_3(byte2);
		Instruction_Pointer += 2 + additional_IPs;
		if (log_to_console_8087) cout << "8087 FRSTOR = Restore State" << endl;
	}
}

void op_code_8087_unknown()
{
	byte2 = memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF]; //mod / reg / rm
	if ((byte2 >> 6) == 3)
	{
		Instruction_Pointer += 2;
		if (log_to_console_8087) cout << "Unknown 8087 operation IP = " << Instruction_Pointer << " OPCODE = " << (bitset<8>)memory.read_2(Instruction_Pointer + *CS * 16) << " + " << (bitset<8>)memory.read_2(Instruction_Pointer + *CS * 16) << endl;
	}
	else
	{
		mod_RM_3(byte2);
		if (log_to_console_8087) cout << "Unknown 8087 operation IP = " << Instruction_Pointer << " OPCODE = " << (bitset<8>)memory.read_2(Instruction_Pointer + *CS * 16) << " + " << (bitset<8>)memory.read_2(Instruction_Pointer + *CS * 16) << endl;
		Instruction_Pointer += 2 + additional_IPs;
	}
}