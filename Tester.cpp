#include "opcode_functions.h"
#include "custom_classes.h"
#include "video.h"
#include "audio.h"
#include <Windows.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <conio.h>
#include <chrono>
#include <bitset>
#include <sstream>
#include <thread>
#include <cmath>

//#define DEBUG
typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;

//Доп функции
template< typename T >
extern std::string int_to_hex(T i, int w);

// переменные из основного файла программы
using namespace std;
extern HANDLE hConsole;

extern uint8 exeption;

//регистры процессора
extern uint16 AX; // AX
extern uint16 BX; // BX
extern uint16 CX; // CX
extern uint16 DX; // DX

extern unsigned __int16 Status_Flags;	//Flags register
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

extern uint16 CS_data;			//Code Segment
extern uint16 DS_data;			//Data Segment
extern uint16 SS_data;			//Stack Segment
extern uint16 ES_data;			//Extra Segment

extern bool Interrupts_enabled;//разрешение прерываний

extern bool cont_exec;

//временные регистры
extern uint16 temp_ACC_16;
extern uint8 temp_ACC_8;
extern uint16 temp_Addr;

//префикс замены сегмента
extern uint8 Flag_segment_override;

//флаг аппаратных прерываний
extern bool Flag_hardware_INT;

extern Mem_Ctrl memory;
extern SoundMaker speaker;
extern IO_Ctrl IO_device;
extern IC8259 int_ctrl;


extern uint16 Stack_Pointer;

extern uint16 registers[8];
extern bool parity_check[256];

extern bool log_to_console;
extern bool step_mode;

extern string regnames[8];
extern string pairnames[4];
extern Monitor monitor;
extern Dev_mon_device debug_monitor;
extern void (*op_code_table[256])();


//=============
extern __int16 DispCalc16(uint16 data); //декларация
extern __int8 DispCalc8(uint8 data);
//==============Service functions===============



void tester()
{

	cout << "Testing start" << endl;

	uint8 test_Src_8;
	uint8 test_Dest_8;
	
	uint16 test_Src_16;
	uint16 test_Dest_16;

	uint8 result_8;
	uint16 result_16;
	uint32 result_32;
	
	uint8 test_command;
	uint16 calc_IP;
	uint8 total_op, success_op, success_ip, success_flags;
	bool result_CF;

	bool details = 0;

	AX = 0x0101;
	BX = 0x1010;
	CX = 0x1118;
	DX = 0x0410;
	Stack_Pointer =		0x200;
	Base_Pointer =		0x100;
	Source_Index =		0x300;
	Destination_Index = 0x400;

	SS_data = 400;
	DS_data = 600;
	ES_data = 200;
	CS_data = 500;

	Instruction_Pointer = 0;
	
	step_mode = 0; log_to_console = 0;

	memory.write(*CS * 16 + Instruction_Pointer, 0x3C);
	memory.write(*CS * 16 + uint16(Instruction_Pointer + 1), 0xFF);

	cout << "Start test" << endl;
	
	//кол-во операций
	int cycles = 10000000;

	auto start = std::chrono::system_clock::now();

	for (int i = 0;i < 256;++i)
	{
		AX = i;
		op_code_table[memory.read(Instruction_Pointer + *CS * 16)]();
		Instruction_Pointer = 0;
		cout << "AX = " << (int)AX << " cmp " << (int)i << " FZ = " << Flag_ZF << endl;
	}
	
	auto end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end - start;
	int op_sec = cycles / elapsed_seconds.count();
	
	cout << setw(25) << left << "MOV AX, BX" << (int)cycles << " opers " << fixed << elapsed_seconds.count() << " sec - " << (int)op_sec << " op/sec" << endl;

	//================================================================================

	memory.write(*CS * 16 + Instruction_Pointer, 0x89);
	memory.write(*CS * 16 + Instruction_Pointer + 1, 0xD8);

	start = std::chrono::system_clock::now();

	for (int i = 0; i < cycles; ++i)
	{
		op_code_table[memory.read(Instruction_Pointer + *CS * 16)]();
		Instruction_Pointer = 0;
	}

	end = std::chrono::system_clock::now();
	elapsed_seconds = end - start;
	op_sec = cycles / elapsed_seconds.count();

	cout << setw(25) << left << "MOV AL, CL" << (int)cycles << " opers " << elapsed_seconds.count() << " sec - " << (int)op_sec << " op/sec" << endl;
	//================================================================================

	memory.write(*CS * 16 + Instruction_Pointer, 0x00);
	memory.write(*CS * 16 + uint16(Instruction_Pointer + 1), 0xCC);

	start = std::chrono::system_clock::now();

	for (int i = 0; i < cycles; ++i)
	{
		op_code_table[memory.read(Instruction_Pointer + *CS * 16)]();
		Instruction_Pointer = 0;
	}

	end = std::chrono::system_clock::now();
	elapsed_seconds = end - start;
	op_sec = cycles / elapsed_seconds.count();

	cout << setw(25) << left << "ADD AH, CL" << (int)cycles << " opers " << elapsed_seconds.count() << " sec - " << (int)op_sec << " op/sec" << endl;
	//================================================================================

	memory.write(*CS * 16 + Instruction_Pointer, 0x88);
	memory.write(*CS * 16 + Instruction_Pointer + 1, 0x6F);
	memory.write(*CS * 16 + Instruction_Pointer + 2, 0x04);

	start = std::chrono::system_clock::now();

	for (int i = 0; i < cycles; ++i)
	{
		op_code_table[memory.read(Instruction_Pointer + *CS * 16)]();
		Instruction_Pointer = 0;
	}

	end = std::chrono::system_clock::now();
	elapsed_seconds = end - start;
	op_sec = cycles / elapsed_seconds.count();

	cout << setw(25) << left << "MOV [BX+4], CH" << (int)cycles << " opers " << elapsed_seconds.count() << " sec - " << (int)op_sec << " op/sec" << endl;

	//================================================================================

	memory.write(*CS * 16 + Instruction_Pointer, 0x0);
	memory.write(*CS * 16 + Instruction_Pointer + 1, 0xAC);
	memory.write(*CS * 16 + Instruction_Pointer + 2, 0x45);
	memory.write(*CS * 16 + Instruction_Pointer + 3, 0x12);

	start = std::chrono::system_clock::now();

	for (int i = 0; i < cycles; ++i)
	{
		op_code_table[memory.read(Instruction_Pointer + *CS * 16)]();
		Instruction_Pointer = 0;
	}

	end = std::chrono::system_clock::now();
	elapsed_seconds = end - start;
	op_sec = cycles / elapsed_seconds.count();

	cout << setw(25) << left << "ADD [SI+0x1245], CH" << (int)cycles << " opers " << elapsed_seconds.count() << " sec - " << (int)op_sec << " op/sec" << endl;

	//================================================================================

	memory.write(*CS * 16 + Instruction_Pointer, 0x28);
	memory.write(*CS * 16 + Instruction_Pointer + 1, 0x9f);
	memory.write(*CS * 16 + Instruction_Pointer + 2, 0x45);
	memory.write(*CS * 16 + Instruction_Pointer + 3, 0x12);

	start = std::chrono::system_clock::now();

	for (int i = 0; i < cycles; ++i)
	{
		op_code_table[memory.read(Instruction_Pointer + *CS * 16)]();
		Instruction_Pointer = 0;
	}

	end = std::chrono::system_clock::now();
	elapsed_seconds = end - start;
	op_sec = cycles / elapsed_seconds.count();

	cout << setw(25) << left << "sub [BX + 0x1245], bl" << (int)cycles << " opers " << elapsed_seconds.count() << " sec - " << (int)op_sec << " op/sec" << endl;


	//================================================================================

	//log_to_console = 1;

	memory.write(*CS * 16 + Instruction_Pointer, 0xFF);
	memory.write(*CS * 16 + Instruction_Pointer + 1, 0x87);
	memory.write(*CS * 16 + Instruction_Pointer + 2, 0x45);
	memory.write(*CS * 16 + Instruction_Pointer + 3, 0x12);

	start = std::chrono::system_clock::now();

	for (int i = 0; i < cycles; ++i)
	{
		op_code_table[memory.read(Instruction_Pointer + *CS * 16)]();
		Instruction_Pointer = 0;
	}

	end = std::chrono::system_clock::now();
	elapsed_seconds = end - start;
	op_sec = cycles / elapsed_seconds.count();

	cout << setw(25) << left << "INC [BX + 0x1245]" << (int)cycles << " opers " << elapsed_seconds.count() << " sec - " << (int)op_sec << " op/sec" << endl;
	//================================================================================

	//log_to_console = 1;

	memory.write(*CS * 16 + Instruction_Pointer, 0x43);
	memory.write(*CS * 16 + Instruction_Pointer + 1, 0x87);
	memory.write(*CS * 16 + Instruction_Pointer + 2, 0x45);
	memory.write(*CS * 16 + Instruction_Pointer + 3, 0x12);

	start = std::chrono::system_clock::now();

	for (int i = 0; i < cycles; ++i)
	{
		op_code_table[memory.read(Instruction_Pointer + *CS * 16)]();
		Instruction_Pointer = 0;
	}

	end = std::chrono::system_clock::now();
	elapsed_seconds = end - start;
	op_sec = cycles / elapsed_seconds.count();

	cout << setw(25) << left << "INC BX" << (int)cycles << " opers " << elapsed_seconds.count() << " sec - " << (int)op_sec << " op/sec" << endl;


	//================================================================================

	//log_to_console = 1;

	memory.write(*CS * 16 + Instruction_Pointer, 0x88);
	memory.write(*CS * 16 + Instruction_Pointer + 1, 0x3F);
	memory.write(*CS * 16 + Instruction_Pointer + 2, 0x45);
	memory.write(*CS * 16 + Instruction_Pointer + 3, 0x12);

	start = std::chrono::system_clock::now();

	for (int i = 0; i < cycles; ++i)
	{
		op_code_table[memory.read(Instruction_Pointer + *CS * 16)]();
		Instruction_Pointer = 0;
	}

	end = std::chrono::system_clock::now();
	elapsed_seconds = end - start;
	op_sec = cycles / elapsed_seconds.count();

	cout << setw(25) << left <<  "MOV [BX], BH" << (int)cycles << " opers " << elapsed_seconds.count() << " sec - " << (int)op_sec << " op/sec" << endl;

	AX = 0x1234;
	
	cout << "AX " << hex << (int)AX << endl;
	uint16* ptr_AX = &AX;
	cout << "ptr_AX " << hex << (int)*ptr_AX << endl;
	uint8* ptr_AL = (uint8*)ptr_AX;
	cout << "ptr_AL " << hex << (int)*ptr_AL << endl;
	uint8* ptr_AH = ptr_AL + 1;
	cout << "ptr_AH " << hex << (int)*ptr_AH << endl;
	
	*ptr_AL = 0x88;
	*ptr_AH = 0x33;
	
	cout << "AX " << hex << (int)AX << endl;
	
	// тест графики
	// переключаем адаптер
	
	//monitor.set_CGA_mode(4); // 6 - 640*200

	for (int y = 0; y < 64; ++y)
	{
		//memory.write_2(0xB8000 + y, 55);
		memory.write(0xB8000 + y * 80, (y + 0) * 2);
		memory.write(0xB8000 + y * 80 + 8192, (y + 0) * 2 + 1);
	}
	
	while (1) monitor.update(0);
}


