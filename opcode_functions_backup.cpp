#include "opcode_functions.h"
#include "custom_classes.h"

#include <Windows.h>
#include <iostream>
#include <string>
#include <conio.h>
#include <bitset>
#include <sstream>

#define DEBUG
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

extern bool Interrupts_enabled;//разрешение прерываний

extern bool cont_exec;

//временные регистры
extern uint16 temp_ACC_16;
extern uint8 temp_ACC_8;
extern uint16 temp_Addr;

//вспомогательные переменные для рассчета адресов операндов
uint8 additional_IPs;

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
extern Video_device monitor;

extern void (*op_code_table[256])();

extern bool test_mode;
extern bool repeat_test_op;
extern bool negate_IDIV;

//=============
__int16 DispCalc16(uint16 data); //декларация
__int8 DispCalc8(uint8 data);
//==============Service functions===============

void op_code_unknown()		// Unknown operation
{
	if (log_to_console) cout << "Unknown operation IP = " << Instruction_Pointer << " OPCODE = " << (bitset<8>)memory.read(Instruction_Pointer, CS);
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
	uint8 Seg = (memory.read(Instruction_Pointer, CS) >> 3) & 3;

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
		//cout << int_to_hex(memory.read(0x441, 0), 2) << "  " << int_to_hex(memory.read(0x442, 0), 2) << " ";
		cout << *CS << ":" << std::setfill('0') << std::setw(4) << Instruction_Pointer << "  " <<
			std::setfill('0') << std::setw(2) << (int)memory.read(Instruction_Pointer, CS) << "  " <<
			std::setfill('0') << std::setw(2) << (int)memory.read(Instruction_Pointer + 1, CS) << "  " <<
			std::setfill('0') << std::setw(2) << (int)memory.read(Instruction_Pointer + 2, CS) << "  " <<
			std::setfill('0') << std::setw(2) << (int)memory.read(Instruction_Pointer + 3, CS) << "  " <<
			std::setfill('0') << std::setw(2) << (int)memory.read(Instruction_Pointer + 4, CS) << "  " <<
			std::setfill('0') << std::setw(2) << (int)memory.read(Instruction_Pointer + 5, CS) << "\t";
	}
#endif
	op_code_table[memory.read(Instruction_Pointer, CS)]();
	//если установлен флаг negate_IDIV - выполняем еще одну команду
	if (negate_IDIV)
	{
		if (log_to_console) cout << endl;
		goto cmd_rep;
	}
	DS = &DS_data; //возвращаем назад сегмент
	SS = &SS_data;
}
inline uint16 mod_RM(uint8 byte2)		//расчет адреса операнда по биту 2
{
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
			return memory.read(Instruction_Pointer + 2, CS) + memory.read(Instruction_Pointer + 3, CS) * 256;
		case 7:
			return BX;
		}

	}
	if ((byte2 >> 6) == 1) // 8-bit displacement
	{
		//грузим смещение
		__int8 d8 = memory.read(Instruction_Pointer + 2, CS);

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
		__int16 d16 = memory.read(Instruction_Pointer + 2, CS) + memory.read(Instruction_Pointer + 3, CS) * 256;
		
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
inline uint32 mod_RM_2(uint8 byte2)		//расчет адреса операнда по биту 2
{
	if ((byte2 >> 6) == 0) //no displacement
	{
		additional_IPs = 0;
		switch (byte2 & 7)
		{
		case 0:
			return ((BX + Source_Index) & 0xFFFF) + *DS * 16;
		case 1:
			return ((BX + Destination_Index) & 0xFFFF) + *DS * 16;
		case 2:
			return ((Base_Pointer + Source_Index) & 0xFFFF) + *SS * 16;
		case 3:
			return ((Base_Pointer + Destination_Index) & 0xFFFF) + *SS * 16;
		case 4:
			return ((Source_Index) & 0xFFFF) + *DS * 16;
		case 5:
			return ((Destination_Index) & 0xFFFF) + *DS * 16;
		case 6:
			additional_IPs = 2;
			return memory.read(Instruction_Pointer + 2, CS) + memory.read(Instruction_Pointer + 3, CS) * 256 + *DS * 16;
		case 7:
			return BX + *DS * 16;
		}

	}
	if ((byte2 >> 6) == 1) // 8-bit displacement
	{
		additional_IPs = 1;
		
		//грузим смещение
		__int8 d8 = memory.read(Instruction_Pointer + 2, CS);

		switch (byte2 & 7)
		{
		case 0:
			return ((BX + Source_Index + d8) & 0xFFFF) + *DS * 16;
		case 1:
			return ((BX + Destination_Index + d8) & 0xFFFF) + *DS * 16;
		case 2:
			return ((Base_Pointer + Source_Index + d8) & 0xFFFF) + *SS * 16;
		case 3:
			return ((Base_Pointer + Destination_Index + d8) & 0xFFFF) + *SS * 16;
		case 4:
			return ((Source_Index + d8) & 0xFFFF) + *DS * 16;
		case 5:
			return ((Destination_Index + d8) & 0xFFFF) + *DS * 16;
		case 6:
			return ((Base_Pointer + d8) & 0xFFFF) + *SS * 16;
		case 7:
			return ((BX + d8) & 0xFFFF) + *DS * 16;
		}

	}
	if ((byte2 >> 6) == 2) // 16-bit displacement
	{
		additional_IPs = 2;
		
		//грузим два байта смещения
		__int16 d16 = memory.read(Instruction_Pointer + 2, CS) + memory.read(Instruction_Pointer + 3, CS) * 256;

		switch (byte2 & 7)
		{
		case 0:
			return ((BX + Source_Index + d16) & 0xFFFF) + *DS * 16;
		case 1:
			return ((BX + Destination_Index + d16) & 0xFFFF) + *DS * 16;
		case 2:
			return ((Base_Pointer + Source_Index + d16) & 0xFFFF) + *SS * 16;
		case 3:
			return ((Base_Pointer + Destination_Index + d16) & 0xFFFF) + *SS * 16;
		case 4:
			return ((Source_Index + d16) & 0xFFFF) + *DS * 16;
		case 5:
			return ((Destination_Index + d16) & 0xFFFF) + *DS * 16;
		case 6:
			return ((Base_Pointer + d16) & 0xFFFF) + *SS * 16;
		case 7:
			return ((BX + d16) & 0xFFFF) + *DS * 16;
		}
	}
	return 0;
}
inline __int8 DispCalc8(uint8 data)
{
	__int8 disp;
	//превращаем беззнаковое смещение в число со знаком
	if (data >> 7) disp = -127 + ((data - 1) & 127);
	else disp = data;
	if (data == 0x80) disp = -128;
	//if (log_to_console) cout << " disp = " << (int)data << "	";
	return disp;
}
inline __int16 DispCalc16(uint16 data)
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
			
			while (memory.read(DX, DS) != 36)
			{
				monitor.teletype(memory.read(DX, DS));
				DX++;
			}
			monitor.sync(1);
		}

	}
}

//============== Data Transfer Group ===========

void mov_R_to_RM_8() //Move 8 bit R->R/M
{
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint8 additional_IPs = 0;

#ifdef DEBUG
	if (log_to_console) cout << "MOV ";
#endif
	//выбираем источник данных
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Src = AX & 255;
		if (log_to_console) cout << "AL("<<(int)(AX & 255)<<")  to ";
		break;
	case 1:
		Src = CX & 255;
		if (log_to_console) cout << "CL(" << (int)(CX & 255) << ") to ";
		break;
	case 2:
		Src = DX & 255;
		if (log_to_console) cout << "DL(" << (int)(DX & 255) << ") to ";
		break;
	case 3:
		Src = BX & 255;
		if (log_to_console) cout << "BL(" << (int)(BX & 255) << ") to ";
		break;
	case 4:
		Src = AX >> 8;
		if (log_to_console) cout << "AH(" << (int)((AX >> 8) & 255) << ") to ";
		break;
	case 5:
		Src = CX >> 8;
		if (log_to_console) cout << "CH(" << (int)((CX >> 8) & 255) << ") to ";
		break;
	case 6:
		Src = DX >> 8;
		if (log_to_console) cout << "DH(" << (int)((DX >> 8) & 255) << ") to ";
		break;
	case 7:
		Src = BX >> 8;
		if (log_to_console) cout << "BH(" << (int)((BX >> 8) & 255) << ") to ";
		break;
	}

	//определяем получателя
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		memory.write(New_Addr, DS, Src & 255);
#ifdef DEBUG
		if (log_to_console) cout << "M[" << (int)*DS << ":" << (int)New_Addr << "]";
#endif
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		memory.write(New_Addr, DS, Src & 255);
#ifdef DEBUG
		if (log_to_console) cout << "M[" << (int)*DS << ":" << (int)New_Addr << "]";
#endif
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		memory.write(New_Addr, DS, Src & 255);
#ifdef DEBUG
		if (log_to_console) cout << "M[" << (int)*DS << ":" << (int)New_Addr << "]";
#endif
		break;
	case 3:
		// mod 11 получатель - регистр
		switch (byte2 & 7)
		{
		case 0:
			AX = (AX & 0xFF00) | Src;
#ifdef DEBUG
			if (log_to_console) cout << "AL(" << (int)Src << ")";
#endif
			break;
		case 1:
			CX = (CX & 0xFF00) | Src;
#ifdef DEBUG
			if (log_to_console) cout << "CL(" << (int)Src << ")";
#endif
			break;
		case 2:
			DX = (DX & 0xFF00) | Src;
#ifdef DEBUG
			if (log_to_console) cout << "DL(" << (int)Src << ")";
#endif
			break;
		case 3:
			BX = (BX & 0xFF00) | Src;
#ifdef DEBUG
			if (log_to_console) cout << "BL(" << (int)Src << ")";
#endif
			break;
		case 4:
			AX = (AX & 0x00FF) | (Src * 256);
#ifdef DEBUG
			if (log_to_console) cout << "AH(" << (int)Src << ")";
#endif
			break;
		case 5:
			CX = (CX & 0x00FF) | (Src * 256);
#ifdef DEBUG
			if (log_to_console) cout << "CH(" << (int)Src << ")";
#endif
			break;
		case 6:
			DX = (DX & 0x00FF) | (Src * 256);
#ifdef DEBUG
			if (log_to_console) cout << "DH(" << (int)Src << ")";
#endif
			break;
		case 7:
			BX = (BX & 0x00FF) | (Src * 256);
#ifdef DEBUG
			if (log_to_console) cout << "BH(" << (int)Src << ")";
#endif
			break;
		}
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void mov_R_to_RM_16() //Move 16 bit R->R/M
{
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint8 additional_IPs = 0;

#ifdef DEBUG
	if (log_to_console) cout << "move ";
#endif

	//выбираем источник данных
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Src = AX;
		if (log_to_console) cout << "AX(" << (int)(AX) << ") to ";
		break;
	case 1:
		Src = CX;
		if (log_to_console) cout << "CX(" << (int)(CX) << ") to ";
		break;
	case 2:
		Src = DX;
		if (log_to_console) cout << "DX(" << (int)(DX) << ") to ";
		break;
	case 3:
		Src = BX;
		if (log_to_console) cout << "BX(" << (int)(BX) << ") to ";
		break;
	case 4:
		Src = Stack_Pointer;
		if (log_to_console) cout << "SP(" << (int)(Stack_Pointer) << ") to ";
		break;
	case 5:
		Src = Base_Pointer;
		if (log_to_console) cout << "BP(" << (int)(Base_Pointer) << ") to ";
		break;
	case 6:
		Src = Source_Index;
		if (log_to_console) cout << "SI(" << (int)(Source_Index) << ") to ";
		break;
	case 7:
		Src = Destination_Index;
		if (log_to_console) cout << "DI(" << (int)(Destination_Index) << ") to ";
		break;
	}

	//определяем получателя
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		memory.write(New_Addr, DS, Src & 255);
		memory.write(New_Addr + 1, DS, Src >> 8);
		if (log_to_console) cout << "M[" << (int)*DS << ":" << (int)New_Addr << "]";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		memory.write(New_Addr, DS, Src & 255);
		memory.write(New_Addr + 1, DS, Src >> 8);
		if (log_to_console) cout << "M[" << (int)*DS << ":" << (int)New_Addr << "]";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		memory.write(New_Addr, DS, Src & 255);
		memory.write(New_Addr + 1, DS, Src >> 8);
		if (log_to_console) cout << "M[" << (int)*DS << ":" << (int)New_Addr << "]";
		break;
	case 3:
		// mod 11 получатель - регистр
		switch (byte2 & 7)
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
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void mov_RM_to_R_8() //Move 8 bit R/M->R
{
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint8 additional_IPs = 0;

#ifdef DEBUG
	if (log_to_console) cout << "MOV ";
#endif

	//определяем источник
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] to ";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] to ";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] to ";
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Src = AX & 255;
			if (log_to_console) cout << "AL to ";
			break;
		case 1:
			Src = CX & 255;
			if (log_to_console) cout << "CL to ";
			break;
		case 2:
			Src = DX & 255;
			if (log_to_console) cout << "DL to ";
			break;
		case 3:
			Src = BX & 255;
			if (log_to_console) cout << "BL to ";
			break;
		case 4:
			Src = AX >> 8;
			if (log_to_console) cout << "AH to ";
			break;
		case 5:
			Src = CX >> 8;
			if (log_to_console) cout << "CH to ";
			break;
		case 6:
			Src = DX >> 8;
			if (log_to_console) cout << "DH to ";
			break;
		case 7:
			Src = BX >> 8;
			if (log_to_console) cout << "BH to ";
			break;
		}
		break;
	}

	//выбираем приёмник данных
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		AX = (AX & 0xFF00) | (Src & 255);
		if (log_to_console) cout << "AL(" << (int)Src << ")";
		break;
	case 1:
		CX = (CX & 0xFF00) | (Src & 255);
		if (log_to_console) cout << "CL(" << (int)Src << ")";
		break;
	case 2:
		DX = (DX & 0xFF00) | (Src & 255);
		if (log_to_console) cout << "DL(" << (int)Src << ")";
		break;
	case 3:
		BX = (BX & 0xFF00) | (Src & 255);
		if (log_to_console) cout << "BL(" << (int)Src << ")";
		break;
	case 4:
		AX = (AX & 0x00FF) | (Src * 256);
		if (log_to_console) cout << "AH(" << (int)Src << ")";
		break;
	case 5:
		CX = (CX & 0x00FF) | (Src * 256);
		if (log_to_console) cout << "CH(" << (int)Src << ")";
		break;
	case 6:
		DX = (DX & 0x00FF) | (Src * 256);
		if (log_to_console) cout << "DH(" << (int)Src << ")";
		break;
	case 7:
		BX = (BX & 0x00FF) | (Src * 256);
		if (log_to_console) cout << "BH(" << (int)Src << ")";
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void mov_RM_to_R_16() //Move 16 bit R/M->R
{
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint8 additional_IPs = 0;
#ifdef DEBUG
	if (log_to_console) cout << "move ";
#endif

	//определяем источник
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] to ";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] to ";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] to ";
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Src = AX;
			if (log_to_console) cout << "AX to ";
			break;
		case 1:
			Src = CX;
			if (log_to_console) cout << "CX to ";
			break;
		case 2:
			Src = DX;
			if (log_to_console) cout << "DX to ";
			break;
		case 3:
			Src = BX;
			if (log_to_console) cout << "BX to ";
			break;
		case 4:
			Src = Stack_Pointer;
			if (log_to_console) cout << "SP to ";
			break;
		case 5:
			Src = Base_Pointer;
			if (log_to_console) cout << "BP to ";
			break;
		case 6:
			Src = Source_Index;
			if (log_to_console) cout << "SI to ";
			break;
		case 7:
			Src = Destination_Index;
			if (log_to_console) cout << "DI to ";
			break;
		}
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
	Instruction_Pointer += 2 + additional_IPs;
}
void IMM_8_to_RM()		//IMM_8 to R/M
{
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint8 additional_IPs = 0;
#ifdef DEBUG
	if (log_to_console) cout << "MOV ";
#endif
	//определяем получателя
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		memory.write(New_Addr, DS, memory.read(Instruction_Pointer + 2 + additional_IPs, CS));
		if (log_to_console) cout << "IMM (" << (int)memory.read(Instruction_Pointer + 2 + additional_IPs, CS) << ") to M[" << (int)*DS << ":" << (int)New_Addr << "]";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		memory.write(New_Addr, DS, memory.read(Instruction_Pointer + 2 + additional_IPs, CS));
		if (log_to_console) cout << "IMM (" << (int)memory.read(Instruction_Pointer + 2 + additional_IPs, CS) << ") to M[" << (int)*DS << ":" << (int)New_Addr << "]";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		memory.write(New_Addr, DS, memory.read(Instruction_Pointer + 2 + additional_IPs, CS));
		if (log_to_console) cout << "IMM (" << (int)memory.read(Instruction_Pointer + 2 + additional_IPs, CS) << ") to M[" << (int)*DS << ":" << (int)New_Addr << "]";
		break;
	case 3:
		// mod 11 получатель - регистр
		switch (byte2 & 7)
		{
		case 0:
			AX = (AX & 0xFF00) | memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "IMM(" << (int)memory.read(Instruction_Pointer + 2 + additional_IPs, CS) << ") to AL";
			break;
		case 1:
			CX = (CX & 0xFF00) | memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "IMM(" << (int)memory.read(Instruction_Pointer + 2 + additional_IPs, CS) << ") to CL";
			break;
		case 2:
			DX = (DX & 0xFF00) | memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "IMM(" << (int)memory.read(Instruction_Pointer + 2 + additional_IPs, CS) << ") to DL";
			break;
		case 3:
			BX = (BX & 0xFF00) | memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "IMM(" << (int)memory.read(Instruction_Pointer + 2 + additional_IPs, CS) << ") to BL";
			break;
		case 4:
			AX = (AX & 0x00FF) | (memory.read(Instruction_Pointer + 2 + additional_IPs, CS) * 256);
			if (log_to_console) cout << "IMM(" << (int)memory.read(Instruction_Pointer + 2 + additional_IPs, CS) << ") to AH";
			break;
		case 5:
			CX = (CX & 0x00FF) | (memory.read(Instruction_Pointer + 2 + additional_IPs, CS) * 256);
			if (log_to_console) cout << "IMM(" << (int)memory.read(Instruction_Pointer + 2 + additional_IPs, CS) << ") to CH";
			break;
		case 6:
			DX = (DX & 0x00FF) | (memory.read(Instruction_Pointer + 2 + additional_IPs, CS) * 256);
			if (log_to_console) cout << "IMM(" << (int)memory.read(Instruction_Pointer + 2 + additional_IPs, CS) << ") to DH";
			break;
		case 7:
			BX = (BX & 0x00FF) | (memory.read(Instruction_Pointer + 2 + additional_IPs, CS) * 256);
			if (log_to_console) cout << "IMM(" << (int)memory.read(Instruction_Pointer + 2 + additional_IPs, CS) << ") to BH";
			break;
		}
		break;
	}
	Instruction_Pointer += 3 + additional_IPs;
}
void IMM_16_to_RM()	//IMM_16 to R/M
{
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint8 additional_IPs = 0;
#ifdef DEBUG
	if (log_to_console) cout << "MOV ";
#endif
	//определяем получателя
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		memory.write(New_Addr, DS, memory.read(Instruction_Pointer + 2 + additional_IPs, CS));
		memory.write(New_Addr + 1, DS, memory.read(Instruction_Pointer + 3 + additional_IPs, CS));
		if (log_to_console) cout << "IMM (" << (int)(memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256) << ") to M[" << (int)*DS << ":" << (int)New_Addr << "]";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		memory.write(New_Addr, DS, memory.read(Instruction_Pointer + 2 + additional_IPs, CS));
		memory.write(New_Addr + 1, DS, memory.read(Instruction_Pointer + 3 + additional_IPs, CS));
		if (log_to_console) cout << "IMM (" << (int)(memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256) << ") to M[" << (int)*DS << ":" << (int)New_Addr << "]";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		memory.write(New_Addr, DS, memory.read(Instruction_Pointer + 2 + additional_IPs, CS));
		memory.write(New_Addr + 1, DS, memory.read(Instruction_Pointer + 3 + additional_IPs, CS));
		if (log_to_console) cout << "IMM (" << (int)(memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256) << ") to M[" << (int)*DS << ":" << (int)New_Addr << "]";
		break;
	case 3:
		// mod 11 получатель - регистр
		switch (byte2 & 7)
		{
		case 0:
			AX = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "IMM(" << (int)AX << ") to AX";
			break;
		case 1:
			CX = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "IMM(" << (int)CX << ") to CX";
			break;
		case 2:
			DX = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "IMM(" << (int)DX << ") to DX";
			break;
		case 3:
			BX = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "IMM(" << (int)BX << ") to BX";
			break;
		case 4:
			Stack_Pointer = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "IMM(" << (int)Stack_Pointer << ") to SP";
			break;
		case 5:
			Base_Pointer = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "IMM(" << (int)Base_Pointer << ") to BP";
			break;
		case 6:
			Source_Index = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "IMM(" << (int)Source_Index << ") to SI";
			break;
		case 7:
			Destination_Index = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "IMM(" << (int)Destination_Index << ") to DI";
			break;
		}
		break;
	}
	Instruction_Pointer += 4 + additional_IPs;
}

void IMM_8_to_R()		//IMM_8 to Register
{
	uint8 IMM8 = memory.read(Instruction_Pointer + 1, CS);
	switch (memory.read(Instruction_Pointer, CS) & 7)
	{
	case 0: //AL
		AX = (AX & 0xFF00) | IMM8;
		if (log_to_console) cout << "IMM(" << (int)IMM8 << ") to AL";
		break;
	case 1: //CL
		CX = (CX & 0xFF00) | IMM8;
		if (log_to_console) cout << "IMM(" << (int)IMM8 << ") to CL";
		break;
	case 2: //DL
		DX = (DX & 0xFF00) | IMM8;
		if (log_to_console) cout << "IMM(" << (int)IMM8 << ") to DL";
		break;
	case 3: //BL
		BX = (BX & 0xFF00) | IMM8;
		if (log_to_console) cout << "IMM(" << (int)IMM8 << ") to BL";
		break;
	case 4: //AH
		AX = (AX & 0x00FF) | (IMM8 * 256);
		if (log_to_console) cout << "IMM(" << (int)IMM8 << ") to AH";
		break;
	case 5: //CH
		CX = (CX & 0x00FF) | (IMM8 * 256);
		if (log_to_console) cout << "IMM(" << (int)IMM8 << ") to CH";
		break;
	case 6: //DH
		DX = (DX & 0x00FF) | (IMM8 * 256);
		if (log_to_console) cout << "IMM(" << (int)IMM8 << ") to DH";
		break;
	case 7: //BH
		BX = (BX & 0x00FF) | (IMM8 * 256);
		if (log_to_console) cout << "IMM(" << (int)IMM8 << ") to BH";
	}
	Instruction_Pointer += 2;
}
void IMM_16_to_R()		//IMM_16 to Register
{
	uint16 IMM16 = memory.read(Instruction_Pointer + 1, CS) + memory.read(Instruction_Pointer + 2, CS) * 256;
	switch (memory.read(Instruction_Pointer, CS) & 7)
	{
	case 0: //AX
		AX = IMM16;
		if (log_to_console) cout << "IMM(" << (int)(IMM16) << ") to AX";
		break;
	case 1: //CX
		CX = IMM16;
		if (log_to_console) cout << "IMM(" << (int)(IMM16) << ") to CX";
		break;
	case 2: //DX
		DX = IMM16;
		if (log_to_console) cout << "IMM(" << (int)(IMM16) << ") to DX";
		break;
	case 3: //BX
		BX = IMM16;
		if (log_to_console) cout << "IMM(" << (int)(IMM16) << ") to BX";
		break;
	case 4: //SP
		Stack_Pointer = IMM16;
		if (log_to_console) cout << "IMM(" << (int)(IMM16) << ") to SP";
		break;
	case 5: //BP
		Base_Pointer = IMM16;
		if (log_to_console) cout << "IMM(" << (int)(IMM16) << ") to BP";
		break;
	case 6: //SI
		Source_Index = IMM16;
		if (log_to_console) cout << "IMM(" << (int)(IMM16) << ") to SI";
		break;
	case 7: //DI
		Destination_Index = IMM16;
		if (log_to_console) cout << "IMM(" << (int)(IMM16) << ") to DI";
	}
	Instruction_Pointer += 3;
}

void M_8_to_ACC()		//Memory to Accumulator 8
{
	uint16 Addr = memory.read(Instruction_Pointer + 1, CS) + memory.read(Instruction_Pointer + 2, CS) * 256;
	AX = (AX & 0xFF00) | memory.read(Addr, DS);
	Instruction_Pointer += 3;
	if (log_to_console) cout << "Memory[" << (int)Addr << "] to AL(" << (int)(AX & 255) << ")";
}
void M_16_to_ACC()		//Memory to Accumulator 16
{
	uint16 Addr = memory.read(Instruction_Pointer + 1, CS) + memory.read(Instruction_Pointer + 2, CS) * 256;
	AX = memory.read(Addr, DS) + memory.read(Addr + 1, DS) * 256;
	Instruction_Pointer += 3;
	if (log_to_console) cout << "Memory[" << (int)Addr << "] to AX(" << (int)(AX) << ")";
}
void ACC_8_to_M()		//Accumulator to Memory 8
{
	uint16 Addr = memory.read(Instruction_Pointer + 1, CS) + memory.read(Instruction_Pointer + 2, CS) * 256;
	memory.write(Addr, DS, AX & 255);
	Instruction_Pointer += 3;
	if (log_to_console) cout << "AL(" << (int)(AX & 255) << ") to Memory[" << (int)Addr << "]";
}
void ACC_16_to_M()		//Accumulator to Memory 16
{
	uint16 Addr = memory.read(Instruction_Pointer + 1, CS) + memory.read(Instruction_Pointer + 2, CS) * 256;
	memory.write(Addr, DS, AX & 255);
	memory.write(Addr + 1, DS, AX >> 8);
	Instruction_Pointer += 3;
	if (log_to_console) cout << "AX(" << (int)AX << ") to Memory[" << (int)Addr << "]";
}

void RM_to_Segment_Reg()	//Register/Memory to Segment Register
{
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint8 additional_IPs = 0;

	//определяем источник
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] to ";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] to ";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] to ";
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Src = AX;
			if (log_to_console) cout << "AX(" << (int)Src << ") to ";
			break;
		case 1:
			Src = CX;
			if (log_to_console) cout << "CX(" << (int)Src << ") to ";
			break;
		case 2:
			Src = DX;
			if (log_to_console) cout << "DX(" << (int)Src << ") to ";
			break;
		case 3:
			Src = BX;
			if (log_to_console) cout << "BX(" << (int)Src << ") to ";
			break;
		case 4:
			Src = Stack_Pointer;
			if (log_to_console) cout << "SP(" << (int)Src << ") to ";
			break;
		case 5:
			Src = Base_Pointer;
			if (log_to_console) cout << "BP(" << (int)Src << ") to ";
			break;
		case 6:
			Src = Source_Index;
			if (log_to_console) cout << "SI(" << (int)Src << ") to ";
			break;
		case 7:
			Src = Destination_Index;
			if (log_to_console) cout << "DI(" << (int)Src << ") to ";
			break;
		}
		break;
	}

	//выбираем приёмник данных
	switch ((byte2 >> 3) & 3)
	{
	case 0: //ES
		ES_data = Src;
		if (log_to_console) cout << "ES";
		break;
	case 1: //CS
		CS_data = Src;
		if (log_to_console) cout << "CS[NOP]";
		break;
	case 2: //SS
		SS_data = Src;
		if (log_to_console) cout << "SS";
		break;
	case 3: //DS
		DS_data = Src;
		if (log_to_console) cout << "DS";
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void Segment_Reg_to_RM()	//Segment Register to Register/Memory
{
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint8 additional_IPs = 0;

	//определяем источник
	switch ((byte2 >> 3) & 3)
	{
	case 0: //ES
		Src = ES_data;
		if (log_to_console) cout << "ES(" << *ES << ") to ";
		break;
	case 1: //CS
		Src = CS_data;
		if (log_to_console) cout << "CS(" << *CS << ") to ";
		break;
	case 2: //SS
		Src = SS_data;
		if (log_to_console) cout << "SS(" << *SS << ") to ";
		break;
	case 3: //DS
		Src = DS_data;
		if (log_to_console) cout << "DS(" << *DS << ") to ";
	}

	//выбираем приёмник данных
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		memory.write(New_Addr, DS, Src & 255);
		memory.write(New_Addr + 1, DS, Src >> 8);
		if (log_to_console) cout << "M[" << (int)New_Addr << "]";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		memory.write(New_Addr, DS, Src & 255);
		memory.write(New_Addr + 1, DS, Src >> 8);
		if (log_to_console) cout << "M[" << (int)New_Addr << "]";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		memory.write(New_Addr, DS, Src & 255);
		memory.write(New_Addr + 1, DS, Src >> 8);
		if (log_to_console) cout << "M[" << (int)New_Addr << "]";
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			AX = Src;
			if (log_to_console) cout << "AX";
			break;
		case 1:
			CX = Src;
			if (log_to_console) cout << "CX";
			break;
		case 2:
			DX = Src;
			if (log_to_console) cout << "DX";
			break;
		case 3:
			BX = Src;
			if (log_to_console) cout << "BX";
			break;
		case 4:
			Stack_Pointer = Src;
			if (log_to_console) cout << "SP";
			break;
		case 5:
			Base_Pointer = Src;
			if (log_to_console) cout << "BP";
			break;
		case 6:
			Source_Index = Src;
			if (log_to_console) cout << "SI";
			break;
		case 7:
			Destination_Index = Src;
			if (log_to_console) cout << "DI";
			break;
		}
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}

void Push_R()		//PUSH Register
{
	uint8 reg = memory.read(Instruction_Pointer, CS) & 7;
	uint16 Src = 0;
	switch (reg)
	{
	case 0: //AX
		Src = AX;
		if (log_to_console) cout << "PUSH AX(" << (int)AX << ")";
		break;
	case 1: //CX
		Src = CX;
		if (log_to_console) cout << "PUSH CX(" << (int)CX << ")";
		break;
	case 2: //DX
		Src = DX;
		if (log_to_console) cout << "PUSH DX(" << (int)DX << ")";
		break;
	case 3: //BX
		Src = BX;
		if (log_to_console) cout << "PUSH BX(" << (int)BX << ")";
		break;
	case 4: //SP
		Src = Stack_Pointer - 2;
		if (log_to_console) cout << "PUSH SP(" << (int)Stack_Pointer << ")";
		break;
	case 5: //BP
		Src = Base_Pointer;
		if (log_to_console) cout << "PUSH BP(" << (int)Base_Pointer << ")";
		break;
	case 6: //SI
		Src = Source_Index;
		if (log_to_console) cout << "PUSH SI(" << (int)Source_Index << ")";
		break;
	case 7: //DI
		Src = Destination_Index;
		if (log_to_console) cout << "PUSH DI(" << (int)Destination_Index << ")";
		break;
	}
	//пушим число
	uint32 stack_addr = SS_data * 16 + Stack_Pointer - 1;
	memory.write_2(stack_addr, Src >> 8);
	memory.write_2(stack_addr - 1, Src & 255);
	Stack_Pointer -= 2;
	Instruction_Pointer++;
}
void Push_SegReg()	//PUSH Segment Register
{
	uint8 reg = (memory.read(Instruction_Pointer, CS) >> 3) & 3;
	uint16 Src = 0;
	switch (reg)
	{
	case 0: //ES
		Src = ES_data;
		if (log_to_console) cout << "PUSH ES(" << (int)Src << ")";
		break;
	case 1: //CS
		Src = CS_data;
		if (log_to_console) cout << "PUSH CS(" << (int)Src << ")";
		break;
	case 2: //SS
		Src = SS_data;
		if (log_to_console) cout << "PUSH SS(" << (int)Src << ")";
		break;
	case 3: //DS
		Src = DS_data;
		if (log_to_console) cout << "PUSH DS(" << (int)Src << ")";
		break;
	}
	//пушим число
	uint32 stack_addr = SS_data * 16 + Stack_Pointer - 1;
	memory.write_2(stack_addr, Src >> 8);
	memory.write_2(stack_addr - 1, Src & 255);
	Stack_Pointer -= 2;
	Instruction_Pointer++;
}

void Pop_RM()			//POP Register/Memory
{
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS);//второй байт
	uint8 OP = (byte2 >> 3) & 7;
	uint16 Src = 0;
	uint8 additional_IPs = 0;
	uint16 Addr;

	if (OP == 0)
	{
		//делаем POP
		uint32 stack_addr = SS_data * 16 + Stack_Pointer;
		Src = memory.read_2(stack_addr) + memory.read_2(stack_addr + 1) * 256;
		Stack_Pointer += 2;

		switch (byte2 >> 6)
		{
		case 0:
			if ((byte2 & 7) == 6) additional_IPs = 2;
			Addr = mod_RM(byte2);
			memory.write(Addr, DS, Src & 255);
			memory.write(Addr + 1, DS, Src >> 8);
			if (log_to_console) cout << "POP M[" << (int)*DS << ":" << (int)Addr << "]";
			break;
		case 1:
			additional_IPs = 1;
			Addr = mod_RM(byte2);
			memory.write(Addr, DS, Src & 255);
			memory.write(Addr + 1, DS, Src >> 8);
			if (log_to_console) cout << "POP M[" << (int)*DS << ":" << (int)Addr << "]";
			break;
		case 2:
			additional_IPs = 2;
			Addr = mod_RM(byte2);
			memory.write(Addr, DS, Src & 255);
			memory.write(Addr + 1, DS, Src >> 8);
			if (log_to_console) cout << "POP M[" << (int)*DS << ":" << (int)Addr << "]";
			break;
		case 3:
			// mod 11 - адрес в регистре
			switch (byte2 & 7)
			{
			case 0:
				AX = Src;
				if (log_to_console) cout << "POP AX(" << (int)Src << ")";
				break;
			case 1:
				CX = Src;
				if (log_to_console) cout << "POP CX(" << (int)Src << ")";
				break;
			case 2:
				DX = Src;
				if (log_to_console) cout << "POP DX(" << (int)Src << ")";
				break;
			case 3:
				BX = Src;
				if (log_to_console) cout << "POP BX(" << (int)Src << ")";
				break;
			case 4:
				Stack_Pointer = Src;
				if (log_to_console) cout << "POP SP(" << (int)Src << ")";
				break;
			case 5:
				Base_Pointer = Src;
				if (log_to_console) cout << "POP BP(" << (int)Src << ")";
				break;
			case 6:
				Source_Index = Src;
				if (log_to_console) cout << "POP SI(" << (int)Src << ")";
				break;
			case 7:
				Destination_Index = Src;
				if (log_to_console) cout << "POP DI(" << (int)Src << ")";
				break;
			}
		}
		Instruction_Pointer += 2 + additional_IPs;
	}
	else
	{
		cout << "Error: POP RM with REG!=0. IP= " << (int)Instruction_Pointer;
		log_to_console = 1;
		step_mode = 1;
		//Instruction_Pointer += 2 + additional_IPs;
	}
}
void Pop_R()			//POP Register
{
	uint8 reg = memory.read(Instruction_Pointer, CS) & 7;
	uint16 Src = 0;

	//извлекаем число
	uint32 stack_addr = SS_data * 16 + Stack_Pointer;
	Src = memory.read_2(stack_addr) + memory.read_2(stack_addr + 1) * 256;
	Stack_Pointer += 2;

	switch (reg)
	{
	case 0: //AX
		AX = Src;
		if (log_to_console) cout << "POP AX(" << (int)Src <<  ")";
		break;
	case 1: //CX
		CX = Src;
		if (log_to_console) cout << "POP CX(" << (int)Src << ")";
		break;
	case 2: //DX
		DX = Src;
		if (log_to_console) cout << "POP DX(" << (int)Src << ")";
		break;
	case 3: //BX
		BX = Src;
		if (log_to_console) cout << "POP BX(" << (int)Src << ")";
		break;
	case 4: //SP
		Stack_Pointer = Src;
		if (log_to_console) cout << "POP SP(" << (int)Src << ")";
		break;
	case 5: //BP
		Base_Pointer = Src;
		if (log_to_console) cout << "POP BP(" << (int)Src << ")";
		break;
	case 6: //SI
		Source_Index = Src;
		if (log_to_console) cout << "POP SI(" << (int)Src << ")";
		break;
	case 7: //DI
		Destination_Index = Src;
		if (log_to_console) cout << "POP DI(" << (int)Src << ")";
		break;
	}
	Instruction_Pointer++;
}
void Pop_SegReg()		//POP Segment Register
{
	uint8 reg = (memory.read(Instruction_Pointer, CS) >> 3) & 3;
	uint16 Src = 0;

	//извлекаем число
	uint32 stack_addr = SS_data * 16 + Stack_Pointer;
	Src = memory.read_2(stack_addr) + memory.read_2(stack_addr + 1) * 256;
	Stack_Pointer += 2;

	switch (reg)
	{
	case 0: //ES
		ES_data = Src;
		if (log_to_console) cout << "POP ES(" << (int)Src << ")";
		break;
	case 1: //CS
		CS_data = Src;
		if (log_to_console) cout << "POP CS(" << (int)Src << ")";
		break;
	case 2: //SS
		SS_data = Src;
		if (log_to_console) cout << "POP SS(" << (int)Src << ")";
		break;
	case 3: //DS
		DS_data = Src;
		if (log_to_console) cout << "POP DS(" << (int)Src << ")";
		break;
	}
	Instruction_Pointer++;
}

void XCHG_8()			//Exchange Register/Memory with Register 8bit
{
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint8 Src = 0;
	uint8 Dest = 0;
	uint8 additional_IPs = 0;

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
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Dest = memory.read(New_Addr, DS);
		memory.write(New_Addr, DS, Src);
		if (log_to_console) cout << " M[" << (int)New_Addr << "]";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Dest = memory.read(New_Addr, DS);
		memory.write(New_Addr, DS, Src);
		if (log_to_console) cout << " M[" << (int)New_Addr << "]";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Dest = memory.read(New_Addr, DS);
		memory.write(New_Addr, DS, Src);
		if (log_to_console) cout << " M[" << (int)New_Addr << "]";
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint16 Dest = 0;
	uint8 additional_IPs = 0;

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
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Dest = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		memory.write(New_Addr, DS, Src & 255);
		memory.write(New_Addr + 1, DS, Src >> 8);
		if (log_to_console) cout << " M[" << (int)New_Addr << "]";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Dest = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		memory.write(New_Addr, DS, Src & 255);
		memory.write(New_Addr + 1, DS, Src >> 8);
		if (log_to_console) cout << " M[" << (int)New_Addr << "]";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Dest = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		memory.write(New_Addr, DS, Src & 255);
		memory.write(New_Addr + 1, DS, Src >> 8);
		if (log_to_console) cout << " M[" << (int)New_Addr << "]";
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
	uint8 reg = memory.read(Instruction_Pointer, CS) & 7; //reg
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
	AX = (AX & 0xFF00) | IO_device.input_from_port_8(memory.read(Instruction_Pointer + 1, CS)); //пишем в AL байт из порта
	SetConsoleTextAttribute(hConsole, 10);
	if (log_to_console) cout << "read (" << (int)(AX & 255) << ") from port " << (int)memory.read(Instruction_Pointer + 1, CS);
	SetConsoleTextAttribute(hConsole, 7);
	Instruction_Pointer += 2;
}
void In_16_to_ACC_from_port()    //Input 16 to AL/AX AX from fixed PORT
{
	AX = IO_device.input_from_port_16(memory.read(Instruction_Pointer + 1, CS)); //пишем в AX 2 байта из порта
	SetConsoleTextAttribute(hConsole, 10);
	if (log_to_console) cout << "read (" << (int)AX << ") from port " << (int)memory.read(Instruction_Pointer + 1, CS);
	SetConsoleTextAttribute(hConsole, 7);
	Instruction_Pointer += 2;
}
void Out_8_from_ACC_to_port()    //Output 8 from AL/AX AX from fixed PORT
{
	IO_device.output_to_port_8(memory.read(Instruction_Pointer + 1, CS), AX & 255);//выводим в порт байт AL
	SetConsoleTextAttribute(hConsole, 10);
	if (log_to_console) cout << "write AL(" << (int)(AX & 255) << ") to port " << (int)memory.read(Instruction_Pointer + 1, CS);
	SetConsoleTextAttribute(hConsole, 7);
	Instruction_Pointer += 2;
}
void Out_16_from_ACC_to_port()   //Output 16 from AL/AX AX from fixed PORT
{
	IO_device.output_to_port_16(memory.read(Instruction_Pointer + 1, CS), AX);//выводим в порт 2 байта AX
	SetConsoleTextAttribute(hConsole, 10);
	if (log_to_console) cout << "write AX(" << (int)(AX) << ") to port " << (int)memory.read(Instruction_Pointer + 1, CS);
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
	AX = (AX & 0xFF00) | memory.read(Addr, DS);
	if (log_to_console) cout << (int)(AX & 255);
	Instruction_Pointer++;
}
void LEA()			//Load EA to Register
{
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint8 additional_IPs = 0;

	//определяем смещение
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		if (log_to_console) cout << "LEA M[] = " << (int)New_Addr;
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		if (log_to_console) cout << "LEA M[] = " << (int)New_Addr;
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		if (log_to_console) cout << "LEA M[] = " << (int)New_Addr;
		break;
	case 3:
		New_Addr = 0; //заглушка
		break;
	}

	//выбираем приёмник данных
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		AX = New_Addr;
		if (log_to_console) cout << " to AX";
		break;
	case 1:
		CX = New_Addr;
		if (log_to_console) cout << " to CX";
		break;
	case 2:
		DX = New_Addr;
		if (log_to_console) cout << " to DX";
		break;
	case 3:
		BX = New_Addr;
		if (log_to_console) cout << " to BX";
		break;
	case 4:
		Stack_Pointer = New_Addr;
		if (log_to_console) cout << " to SP";
		break;
	case 5:
		Base_Pointer = New_Addr;
		if (log_to_console) cout << " to BP";
		break;
	case 6:
		Source_Index = New_Addr;
		if (log_to_console) cout << " to SI";
		break;
	case 7:
		Destination_Index = New_Addr;
		if (log_to_console) cout << " to DI";
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void LDS()			//Load Pointer to DS
{
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint16 Src_2 = 0;
	uint8 additional_IPs = 0;
#ifdef DEBUG
	if (log_to_console) cout << "LDS ";
#endif

	//определяем источник
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		Src_2 = memory.read(New_Addr + 2, DS) + memory.read(New_Addr + 3, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] to ";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		Src_2 = memory.read(New_Addr + 2, DS) + memory.read(New_Addr + 3, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] to ";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		Src_2 = memory.read(New_Addr + 2, DS) + memory.read(New_Addr + 3, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] to ";
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
	if (log_to_console) cout << " + DS(" << (int)*DS << ")";

	Instruction_Pointer += 2 + additional_IPs;
}
void LES()			//Load Pointer to ES
{
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint16 Src_2 = 0;
	uint8 additional_IPs = 0;
#ifdef DEBUG
	if (log_to_console) cout << "LES ";
#endif

	//определяем источник
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		Src_2 = memory.read(New_Addr + 2, DS) + memory.read(New_Addr + 3, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] to ";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		Src_2 = memory.read(New_Addr + 2, DS) + memory.read(New_Addr + 3, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] to ";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		Src_2 = memory.read(New_Addr + 2, DS) + memory.read(New_Addr + 3, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] to ";
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

	*ES = Src_2;
	if (log_to_console) cout << " + ES(" << (int)*ES << ")";

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
	uint32 stack_addr = SS_data * 16 + Stack_Pointer - 1;
	memory.write_2(stack_addr, 0xF0 | (Flag_OF * 8) | (Flag_DF * 4) | (Flag_IF * 2) | Flag_TF);
	memory.write_2(stack_addr - 1, 0x2 | (Flag_SF * 128) | (Flag_ZF * 64) | (Flag_AF * 16) | (Flag_PF * 4) | (Flag_CF));
	Stack_Pointer -= 2;
	if (log_to_console) cout << "Push Flags";
	Instruction_Pointer++;
}
void POPF()			// Pop Flags
{
	uint32 stack_addr = SS_data * 16 + Stack_Pointer;
	int Flags = memory.read_2(stack_addr) + memory.read_2(stack_addr + 1) * 256;
	Flag_OF = (Flags >> 11) & 1;
	Flag_DF = (Flags >> 10) & 1;
	Flag_IF = (Flags >> 9) & 1;
	Flag_TF = (Flags >> 8) & 1;
	Flag_SF = (Flags >> 7) & 1;
	Flag_ZF = (Flags >> 6) & 1;
	Flag_AF = (Flags >> 4) & 1;
	Flag_PF = (Flags >> 2) & 1;
	Flag_CF = (Flags) & 1;

	Stack_Pointer += 2;
	if (log_to_console) cout << "Pop Flags";
	Instruction_Pointer++;
}

//============Arithmetic===================================

//ADD

void ADD_R_to_RM_8()		// INC R/M -> R/M 8bit
{
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint32 New_Addr_2 = 0;
	uint8 Src = 0;
	bool OF_Carry = false;
	uint16 Result = 0;
	uint16 Result_old = 0;
	uint8 additional_IPs = 0;

	if (log_to_console) cout << "ADD ";

	//определяем 1-й операнд
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Src = AX & 255;
		if (log_to_console) cout << "AL("<<(int)(Src)<<") ";
		break;
	case 1:
		Src = CX & 255;
		if (log_to_console) cout << "CL(" << (int)(Src) << ") ";
		break;
	case 2:
		Src = DX & 255;
		if (log_to_console) cout << "DL(" << (int)(Src) << ") ";
		break;
	case 3:
		Src = BX & 255;
		if (log_to_console) cout << "BL(" << (int)(Src) << ") ";
		break;
	case 4:
		Src = AX >> 8;
		if (log_to_console) cout << "AH(" << (int)(Src) << ") ";
		break;
	case 5:
		Src = CX >> 8;
		if (log_to_console) cout << "CH(" << (int)(Src) << ") ";
		break;
	case 6:
		Src = DX >> 8;
		if (log_to_console) cout << "DH(" << (int)(Src) << ") ";
		break;
	case 7:
		Src = BX >> 8;
		if (log_to_console) cout << "BH(" << (int)(Src) << ") ";
		break;
	}

	//определяем объект назначения и результат операции ADD
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		//New_Addr = mod_RM(byte2);
		New_Addr_2 = mod_RM_2(byte2);
		//Result = memory.read(New_Addr, DS) + Src;
		Result = memory.read_2(New_Addr_2) + Src;
		Flag_AF = (((memory.read_2(New_Addr_2) & 15) + (Src & 15)) >> 4) & 1;
		Flag_CF = Result >> 8;
		OF_Carry = ((memory.read_2(New_Addr_2) & 0x7F) + (Src & 0x7F)) >> 7;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		memory.write_2(New_Addr_2, Result & 255);
		if (log_to_console) cout << " Add M[" << (int)*DS << ":" << (int)New_Addr << "] = " << (int)(Result & 255);
		break;
	case 1:
		additional_IPs = 1;
		New_Addr_2 = mod_RM_2(byte2);
		//Result = memory.read(New_Addr, DS) + Src;
		Result = memory.read_2(New_Addr_2) + Src;
		Flag_AF = (((memory.read_2(New_Addr_2) & 15) + (Src & 15)) >> 4) & 1;
		Flag_CF = Result >> 8;
		OF_Carry = ((memory.read_2(New_Addr_2) & 0x7F) + (Src & 0x7F)) >> 7;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		memory.write_2(New_Addr_2, Result & 255);
		if (log_to_console) cout << " Add M[" << (int)*DS << ":" << (int)New_Addr << "] = " << (int)(Result & 255);
		break;
	case 2:
		additional_IPs = 2;
		New_Addr_2 = mod_RM_2(byte2);
		//Result = memory.read(New_Addr, DS) + Src;
		Result = memory.read_2(New_Addr_2) + Src;
		Flag_AF = (((memory.read_2(New_Addr_2) & 15) + (Src & 15)) >> 4) & 1;
		Flag_CF = Result >> 8;
		OF_Carry = ((memory.read_2(New_Addr_2) & 0x7F) + (Src & 0x7F)) >> 7;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		memory.write_2(New_Addr_2, Result & 255);
		if (log_to_console) cout << " Add M[" << (int)*DS << ":" << (int)New_Addr << "] = " << (int)(Result & 255);
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Result = (AX & 255) + Src;
			if (log_to_console) cout << "ADD AL(" << (int)(AX & 255) << ") = " << (int)(Result & 255);
			Flag_AF = (((AX & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = ((AX & 0x7F) + (Src & 0x7F)) >> 7;
			AX = (AX & 0xFF00) | (Result & 255);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 1:
			Result = (CX & 255) + Src;
			if (log_to_console) cout << "ADD CL(" << (int)(CX & 255) << ") = " << (int)(Result & 255);
			Flag_AF = (((CX & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = ((CX & 0x7F) + (Src & 0x7F)) >> 7;
			CX = (CX & 0xFF00) | (Result & 255);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 2:
			Result = (DX & 255) + Src;
			if (log_to_console) cout << "ADD DL(" << (int)(DX & 255) << ") = " << (int)(Result & 255);
			Flag_AF = (((DX & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = ((DX & 0x7F) + (Src & 0x7F)) >> 7;
			DX = (DX & 0xFF00) | (Result & 255);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 3:
			Result = (BX & 255) + Src;
			if (log_to_console) cout << "ADD BL(" << (int)(BX & 255) << ") = " << (int)(Result & 255);
			Flag_AF = (((BX & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = ((BX & 0x7F) + (Src & 0x7F)) >> 7;
			BX = (BX & 0xFF00) | (Result & 255);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 4:
			Result = (AX >> 8) + Src;
			if (log_to_console) cout << "ADD AH(" << (int)(AX >> 8) << ") = " << (int)(Result & 255);
			Flag_AF = ((((AX >> 8) & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = (((AX >> 8) & 0x7F) + (Src & 0x7F)) >> 7;
			AX = (AX & 0x00FF) | ((Result & 255) << 8);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 5:
			Result = (CX >> 8) + Src;
			if (log_to_console) cout << "ADD CH(" << (int)(CX >> 8) << ") = " << (int)(Result & 255);
			Flag_AF = ((((CX >> 8) & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = (((CX >> 8) & 0x7F) + (Src & 0x7F)) >> 7;
			CX = (CX & 0x00FF) | ((Result & 255) << 8);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 6:
			Result = (DX >> 8) + Src;
			if (log_to_console) cout << "ADD DH(" << (int)(DX >> 8) << ") = " << (int)(Result & 255);
			Flag_AF = ((((DX >> 8) & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = (((DX >> 8) & 0x7F) + (Src & 0x7F)) >> 7;
			DX = (DX & 0x00FF) | ((Result & 255) << 8);
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 7:
			Result = (BX >> 8) + Src;
			if (log_to_console) cout << "ADD BH(" << (int)(BX >> 8) << ") = " << (int)(Result & 255);
			Flag_AF = ((((BX >> 8) & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = (((BX >> 8) & 0x7F) + (Src & 0x7F)) >> 7;
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
void ADD_R_to_RM_16()		// INC R/M -> R/M 16bit
{
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	bool OF_Carry = false;
	uint32 Result = 0;
	uint8 additional_IPs = 0;

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
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Result = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256 + Src;
		Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15)) >> 4) & 1;
		OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) + (Src & 0x7FFF)) >> 15;
		memory.write(New_Addr, DS, Result & 255);
		memory.write(New_Addr + 1, DS, (Result >> 8) & 255);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " Add M[" << (int)New_Addr << "]=" << (int)(Result & 0xFFFF);
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Result = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256 + Src;
		Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15)) >> 4) & 1;
		OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) + (Src & 0x7FFF)) >> 15;
		memory.write(New_Addr, DS, Result & 255);
		memory.write(New_Addr + 1, DS, (Result >> 8) & 255);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " Add M[" << (int)New_Addr << "]=" << (int)(Result & 0xFFFF);
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Result = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256 + Src;
		Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15)) >> 4) & 1;
		OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) + (Src & 0x7FFF)) >> 15;
		memory.write(New_Addr, DS, Result & 255);
		memory.write(New_Addr + 1, DS, (Result >> 8) & 255);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " Add M[" << (int)New_Addr << "]=" << (int)(Result & 0xFFFF);
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint8 Src = 0;
	uint16 Result = 0;
	uint8 additional_IPs = 0;
	bool OF_Carry = false;
	
	if (log_to_console) cout << "ADD ";
	//определяем 1-й операнд
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] + ";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] + ";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] + ";
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint32 New_Addr = 0;
	uint16 Src = 0;
	uint32 Result = 0;
	uint8 additional_IPs = 0;
	bool OF_Carry = false;

	if (log_to_console) cout << "ADD ";
	//определяем 1-й операнд
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM_2(byte2);
		Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] + ";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM_2(byte2);
		Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] + ";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM_2(byte2);
		Src = memory.read_2(New_Addr) + memory.read_2(New_Addr + 1) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] + ";
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint8 OP = (byte2 >> 3) & 7;
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint32 Result_32 = 0;
	uint16 Result = 0;
	uint8 additional_IPs = 0;
	bool OF_Carry = false;

	switch (OP)
	{
	case 0: //  ADD  mod 000 r/m
			//определяем объект назначения и результат операции
		switch (byte2 >> 6)
		{
		case 0:
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "ADD IMMs(" << (int)Src << ") + ";
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) + Src;
			Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) + (Src & 0x7FFF)) >> 15;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;

		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "ADD IMMs(" << (int)Src << ") + ";
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) + Src;
			Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) + (Src & 0x7FFF)) >> 15;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "ADD IMMs(" << (int)Src << ") + ";
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) + Src;
			Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) + (Src & 0x7FFF)) >> 15;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
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
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)Src << ") OR ";
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) | Src;
			memory.write(New_Addr, DS, Result & 255);
			memory.write(New_Addr + 1, DS, Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)Src << ") OR ";
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) | Src;
			memory.write(New_Addr, DS, Result & 255);
			memory.write(New_Addr + 1, DS, Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)Src << ") OR ";
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) | Src;
			memory.write(New_Addr, DS, Result & 255);
			memory.write(New_Addr + 1, DS, Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
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
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "ADC IMMs(" << (int)Src << ") + CF(" << Flag_CF << ") + ";
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) + Src + Flag_CF;
			Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) + (Src & 0x7FFF)) >> 15;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "ADC IMMs(" << (int)Src << ") + CF(" << Flag_CF << ") + ";
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) + Src + Flag_CF;
			Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) + (Src & 0x7FFF)) >> 15;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "ADC IMMs(" << (int)Src << ") + CF(" << Flag_CF << ") + ";
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) + Src + Flag_CF;
			Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) + (Src & 0x7FFF)) >> 15;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
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
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if ((Src >> 7) & 1) Src = Src | 0b1111111100000000; //продолжаем знак на старший байт
			if (log_to_console) cout << "SBB IMMs(" << (int)Src << ") + ";
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) - Src - Flag_CF;
			Flag_AF = (((memory.read(New_Addr, DS) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if ((Src >> 7) & 1) Src = Src | 0b1111111100000000; //продолжаем знак на старший байт
			if (log_to_console) cout << "SBB IMMs(" << (int)Src << ") + ";
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) - Src - Flag_CF;
			Flag_AF = (((memory.read(New_Addr, DS) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if ((Src >> 7) & 1) Src = Src | 0b1111111100000000; //продолжаем знак на старший байт
			if (log_to_console) cout << "SBB IMMs(" << (int)Src << ") + ";
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) - Src - Flag_CF;
			Flag_AF = (((memory.read(New_Addr, DS) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
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
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)Src << ") AND ";
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & Src;
			memory.write(New_Addr, DS, Result & 255);
			memory.write(New_Addr + 1, DS, Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)Src << ") AND ";
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & Src;
			memory.write(New_Addr, DS, Result & 255);
			memory.write(New_Addr + 1, DS, Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)Src << ") AND ";
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & Src;
			memory.write(New_Addr, DS, Result & 255);
			memory.write(New_Addr + 1, DS, Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
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
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if ((Src >> 7) & 1) Src = Src | 0b1111111100000000; //продолжаем знак на старший байт
			if (log_to_console) cout << "SUB IMMs(" << (int)Src << ") + ";
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) - Src;
			Flag_AF = (((memory.read(New_Addr, DS) & 15) - (Src & 15)) >> 4) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if ((Src >> 7) & 1) Src = Src | 0b1111111100000000; //продолжаем знак на старший байт
			if (log_to_console) cout << "SUB IMMs(" << (int)Src << ") + ";
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) - Src;
			Flag_AF = (((memory.read(New_Addr, DS) & 15) - (Src & 15)) >> 4) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if ((Src >> 7) & 1) Src = Src | 0b1111111100000000; //продолжаем знак на старший байт
			if (log_to_console) cout << "SUB IMMs(" << (int)Src << ") + ";
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) - Src;
			Flag_AF = (((memory.read(New_Addr, DS) & 15) - (Src & 15)) >> 4) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
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
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)Src << ") AND ";
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) ^ Src;
			memory.write(New_Addr, DS, Result & 255);
			memory.write(New_Addr + 1, DS, Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)Src << ") AND ";
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) ^ Src;
			memory.write(New_Addr, DS, Result & 255);
			memory.write(New_Addr + 1, DS, Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "IMMs(" << (int)Src << ") AND ";
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) ^ Src;
			memory.write(New_Addr, DS, Result & 255);
			memory.write(New_Addr + 1, DS, Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
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
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "CMP IMMs(" << (int)Src << ") -> ";
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) - Src;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_AF = (((memory.read(New_Addr, DS) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			Flag_PF = parity_check[Result_32 & 255];
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "CMP IMMs(" << (int)Src << ") -> ";
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) - Src;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_AF = (((memory.read(New_Addr, DS) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			Flag_PF = parity_check[Result_32 & 255];
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if ((Src >> 7) & 1) Src = Src | 0xFF00; //продолжаем знак на старший байт
			if (log_to_console) cout << "CMP IMMs(" << (int)Src << ") -> ";
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) - Src;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_AF = (((memory.read(New_Addr, DS) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			Flag_PF = parity_check[Result_32 & 255];
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
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
	uint8 imm = memory.read(Instruction_Pointer + 1, CS);
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
	uint16 imm = memory.read(Instruction_Pointer + 1, CS) + memory.read(Instruction_Pointer + 2, CS) * 256;
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint8 Src = 0;
	bool OF_Carry = false;
	uint16 Result = 0;
	uint8 additional_IPs = 0;

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
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Result = memory.read(New_Addr, DS) + Src + Flag_CF;
		Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
		OF_Carry = ((memory.read(New_Addr, DS) & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
		Flag_CF = Result >> 8;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		memory.write(New_Addr, DS, Result & 255);
		if (log_to_console) cout << " + [" << (int)New_Addr << "] = " << (int)(Result & 255);
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Result = memory.read(New_Addr, DS) + Src + Flag_CF;
		Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
		OF_Carry = ((memory.read(New_Addr, DS) & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
		Flag_CF = Result >> 8;
		Flag_SF = (Result >> 7) & 1;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		memory.write(New_Addr, DS, Result & 255);
		if (log_to_console) cout << " + M[" << (int)New_Addr << "] = " << (int)(Result & 255);
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Result = memory.read(New_Addr, DS) + Src + Flag_CF;
		Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
		OF_Carry = ((memory.read(New_Addr, DS) & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
		Flag_CF = Result >> 8;
		Flag_SF = (Result >> 7) & 1;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		memory.write(New_Addr, DS, Result & 255);
		if (log_to_console) cout << " + M[" << (int)New_Addr << "] = " << (int)(Result & 255);
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	bool OF_Carry = false;
	uint32 Result = 0;
	uint8 additional_IPs = 0;

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
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Result = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256 + Src + Flag_CF;
		Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
		OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
		memory.write(New_Addr, DS, Result & 255);
		memory.write(New_Addr + 1, DS, (Result >> 8) & 255);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " Add M[" << (int)New_Addr << "]=" << (int)(Result & 0xFFFF);
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Result = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256 + Src + Flag_CF;
		Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
		OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
		memory.write(New_Addr, DS, Result & 255);
		memory.write(New_Addr + 1, DS, (Result >> 8) & 255);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " Add M[" << (int)New_Addr << "]=" << (int)(Result & 0xFFFF);
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Result = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256 + Src + Flag_CF;
		Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
		OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
		memory.write(New_Addr, DS, Result & 255);
		memory.write(New_Addr + 1, DS, (Result >> 8) & 255);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " Add M[" << (int)New_Addr << "]=" << (int)(Result & 0xFFFF);
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint8 Src = 0;
	uint16 Result = 0;
	uint8 additional_IPs = 0;
	bool OF_Carry = false;

	if (log_to_console) cout << "ADC ";
	//определяем 1-й операнд
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] + ";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] + ";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] + ";
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint32 Result = 0;
	uint8 additional_IPs = 0;
	bool OF_Carry = false;

	if (log_to_console) cout << "ADC ";
	//определяем 1-й операнд
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] + CF("<<(int)Flag_CF<<")";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] + CF(" << (int)Flag_CF << ")";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] + CF(" << (int)Flag_CF << ")";
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
	uint8 imm = memory.read(Instruction_Pointer + 1, CS);
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
	uint16 imm = memory.read(Instruction_Pointer + 1, CS) + memory.read(Instruction_Pointer + 2, CS) * 256;
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS);//второй байт
	uint8 Src = 0;		//источник данных
	uint16 New_Addr = 0;	//адрес перехода
	uint8 additional_IPs = 0;

	switch ((byte2 >> 3) & 7)
	{
	case 0: //INC RM
		//находим цель инкремента
		switch (byte2 >> 6)
		{
		case 0:
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			Flag_AF = (((Src & 0x0F) + 1) >> 4) & 1;
			Flag_OF = ((Src & 0x7F) + 1) >> 7;
			Src++;
			memory.write(New_Addr, DS, Src);
			if (Src) Flag_ZF = 0;
			else Flag_ZF = 1;
			Flag_SF = (Src >> 7) & 1;
			Flag_PF = parity_check[Src];
			if (log_to_console) cout << "INC M[" << (int)New_Addr << "] = " << (int)Src;
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			Flag_AF = (((Src & 0x0F) + 1) >> 4) & 1;
			Flag_OF = ((Src & 0x7F) + 1) >> 7;
			Src++;
			memory.write(New_Addr, DS, Src);
			if (Src) Flag_ZF = 0;
			else Flag_ZF = 1;
			Flag_SF = (Src >> 7) & 1;
			Flag_PF = parity_check[Src];
			if (log_to_console) cout << "INC M[" << (int)New_Addr << "] = " << (int)Src;
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			Flag_AF = (((Src & 0x0F) + 1) >> 4) & 1;
			Flag_OF = ((Src & 0x7F) + 1) >> 7;
			Src++;
			memory.write(New_Addr, DS, Src);
			if (Src) Flag_ZF = 0;
			else Flag_ZF = 1;
			Flag_SF = (Src >> 7) & 1;
			Flag_PF = parity_check[Src];
			if (log_to_console) cout << "INC M[" << (int)New_Addr << "] = " << (int)Src;
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
				else Flag_ZF = 1;
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
				else Flag_ZF = 1;
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
				else Flag_ZF = 1;
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
				else Flag_ZF = 1;
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
				else Flag_ZF = 1;
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
				else Flag_ZF = 1;
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
				else Flag_ZF = 1;
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
				else Flag_ZF = 1;
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
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			Flag_AF = (((Src & 0x0F) - 1) >> 4) & 1;
			Flag_OF = (((Src & 0x7F) - 1) >> 7) & !(Src == 0);
			Src--;
			memory.write(New_Addr, DS, Src);
			if (Src) Flag_ZF = 0;
			else Flag_ZF = 1;
			Flag_SF = (Src >> 7) & 1;
			Flag_PF = parity_check[Src];
			if (log_to_console) cout << "DEC M[" << (int)New_Addr << "] = " << (int)Src;
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			Flag_AF = (((Src & 0x0F) - 1) >> 4) & 1;
			Flag_OF = (((Src & 0x7F) - 1) >> 7) & !(Src == 0);
			Src--;
			memory.write(New_Addr, DS, Src);
			if (Src) Flag_ZF = 0;
			else Flag_ZF = 1;
			Flag_SF = (Src >> 7) & 1;
			Flag_PF = parity_check[Src];
			if (log_to_console) cout << "DEC M[" << (int)New_Addr << "] = " << (int)Src;
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			Flag_AF = (((Src & 0x0F) - 1) >> 4) & 1;
			Flag_OF = (((Src & 0x7F) - 1) >> 7) & !(Src == 0);
			Src--;
			memory.write(New_Addr, DS, Src);
			if (Src) Flag_ZF = 0;
			else Flag_ZF = 1;
			Flag_SF = (Src >> 7) & 1;
			Flag_PF = parity_check[Src];
			if (log_to_console) cout << "DEC M[" << (int)New_Addr << "] = " << (int)Src;
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
	uint8 byte1 = memory.read(Instruction_Pointer, CS);//второй байт
	uint16 Src = 0;		//источник данных

switch (byte1 & 7)
{
case 0:
	Flag_AF = (((AX & 0x0F) + 1) >> 4) & 1;
	Flag_OF = (((AX & 0x7FFF) + 1) >> 15);
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
	Flag_OF = (((CX & 0x7FFF) + 1) >> 15);
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
	Flag_OF = (((DX & 0x7FFF) + 1) >> 15);
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
	Flag_OF = (((BX & 0x7FFF) + 1) >> 15);
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
	Flag_OF = (((Stack_Pointer & 0x7FFF) + 1) >> 15);
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
	Flag_OF = (((Base_Pointer & 0x7FFF) + 1) >> 15);
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
	Flag_OF = (((Source_Index & 0x7FFF) + 1) >> 15);
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
	Flag_OF = (((Destination_Index & 0x7FFF) + 1) >> 15);
	Destination_Index++;
	Src = Destination_Index;
	if (Src) Flag_ZF = 0;
	else Flag_ZF = 1;
	Flag_SF = (Src >> 15) & 1;
	Flag_PF = parity_check[Src & 255];
	if (log_to_console) cout << "INC DI = " << (int)Src;
	break;
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint8 Src = 0;
	uint8 Dest = 0;
	uint16 Result = 0;
	uint8 additional_IPs = 0;
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
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Dest = memory.read(New_Addr, DS);
		Result = Dest - Src;
		memory.write(New_Addr, DS, Result & 255);
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = (Result >> 7) & 1;
		OF_Carry = ((Dest & 0x7F) - (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((Dest & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << "from M[" << New_Addr << "]=" << (int)(Result & 255);
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Dest = memory.read(New_Addr, DS);
		Result = Dest - Src;
		memory.write(New_Addr, DS, Result & 255);
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = (Result >> 7) & 1;
		OF_Carry = ((Dest & 0x7F) - (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((Dest & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << "from M[" << New_Addr << "]=" << (int)(Result & 255);
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Dest = memory.read(New_Addr, DS);
		Result = Dest - Src;
		memory.write(New_Addr, DS, Result & 255);
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = (Result >> 7) & 1;
		OF_Carry = ((Dest & 0x7F) - (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((Dest & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << "from M[" << New_Addr << "]=" << (int)(Result & 255);
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint32 Result = 0;
	uint8 additional_IPs = 0;
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
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Result = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256 - Src;
		OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
		Flag_AF = ((((memory.read(New_Addr, DS)) & 15) - (Src & 15)) >> 4) & 1;
		memory.write(New_Addr, DS, Result & 255);
		memory.write(New_Addr + 1, DS, (Result >> 8) & 255);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " from M[" << (int)New_Addr << "]=" << (int)(Result & 0xFFFF);
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Result = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256 - Src;
		OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
		Flag_AF = ((((memory.read(New_Addr, DS)) & 15) - (Src & 15)) >> 4) & 1;
		memory.write(New_Addr, DS, Result & 255);
		memory.write(New_Addr + 1, DS, (Result >> 8) & 255);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " from M[" << (int)New_Addr << "]=" << (int)(Result & 0xFFFF);
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Result = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256 - Src;
		OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
		Flag_AF = ((((memory.read(New_Addr, DS)) & 15) - (Src & 15)) >> 4) & 1;
		memory.write(New_Addr, DS, Result & 255);
		memory.write(New_Addr + 1, DS, (Result >> 8) & 255);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " from M[" << (int)New_Addr << "]=" << (int)(Result & 0xFFFF);
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint8 Src = 0;
	uint8 Dest = 0;
	uint16 Result = 0;
	uint8 additional_IPs = 0;
	bool OF_Carry = false;

	if (log_to_console) cout << "SUB ";
	//определяем 1-й операнд
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] + ";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] + ";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] + ";
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint32 Result = 0;
	uint8 additional_IPs = 0;
	bool OF_Carry = false;

	if (log_to_console) cout << "SUB ";
	//определяем 1-й операнд
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] ";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] ";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] ";
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
	uint8 imm = memory.read(Instruction_Pointer + 1, CS);
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
	uint16 imm = memory.read(Instruction_Pointer + 1, CS) + memory.read(Instruction_Pointer + 2, CS) * 256;
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint8 Src = 0;
	uint8 Dest = 0;
	uint16 Result = 0;
	uint8 additional_IPs = 0;
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
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Dest = memory.read(New_Addr, DS);
		Result = Dest - Src - Flag_CF;
		OF_Carry = ((Dest & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
		Flag_AF = (((Dest & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
		memory.write(New_Addr, DS, Result & 255);
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = (Result >> 7) & 1;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "from M[" << New_Addr << "]=" << (int)(Result & 255);
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Dest = memory.read(New_Addr, DS);
		Result = Dest - Src - Flag_CF;
		OF_Carry = ((Dest & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
		Flag_AF = (((Dest & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
		memory.write(New_Addr, DS, Result & 255);
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = (Result >> 7) & 1;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "from M[" << New_Addr << "]=" << (int)(Result & 255);
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Dest = memory.read(New_Addr, DS);
		Result = Dest - Src - Flag_CF;
		OF_Carry = ((Dest & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
		Flag_AF = (((Dest & 0x0F) - (Src & 0x0F) - Flag_CF) >> 4) & 1;
		memory.write(New_Addr, DS, Result & 255);
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = (Result >> 7) & 1;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "from M[" << New_Addr << "]=" << (int)(Result & 255);
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint32 Result = 0;
	uint8 additional_IPs = 0;
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
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Result = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256 - Src - Flag_CF;
		OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
		Flag_AF = ((((memory.read(New_Addr, DS)) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
		memory.write(New_Addr, DS, Result & 255);
		memory.write(New_Addr + 1, DS, (Result >> 8) & 255);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " from M[" << (int)New_Addr << "]=" << (int)(Result & 0xFFFF);
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Result = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256 - Src - Flag_CF;
		OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
		Flag_AF = ((((memory.read(New_Addr, DS)) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
		memory.write(New_Addr, DS, Result & 255);
		memory.write(New_Addr + 1, DS, (Result >> 8) & 255);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " from M[" << (int)New_Addr << "]=" << (int)(Result & 0xFFFF);
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Result = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256 - Src - Flag_CF;
		OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
		Flag_AF = ((((memory.read(New_Addr, DS)) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
		memory.write(New_Addr, DS, Result & 255);
		memory.write(New_Addr + 1, DS, (Result >> 8) & 255);
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = ((Result >> 15) & 1);
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " from M[" << (int)New_Addr << "]=" << (int)(Result & 0xFFFF);
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint8 Src = 0;
	uint8 Dest = 0;
	uint16 Result = 0;
	uint8 additional_IPs = 0;
	bool OF_Carry = false;

	if (log_to_console) cout << "SBB ";
	//определяем 1-й операнд
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] - ";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] - ";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] - ";
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint32 Result = 0;
	uint8 additional_IPs = 0;
	bool OF_Carry = false;

	if (log_to_console) cout << "SBB ";
	//определяем 1-й операнд
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] ";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] ";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] ";
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
	uint8 imm = memory.read(Instruction_Pointer + 1, CS);
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
	uint16 imm = memory.read(Instruction_Pointer + 1, CS) + memory.read(Instruction_Pointer + 2, CS) * 256;
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
	uint8 byte1 = memory.read(Instruction_Pointer, CS);//второй байт
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint8 Src = 0;
	uint16 Result = 0;
	uint8 additional_IPs = 0;
	bool OF_Carry = false;
	
	//в данном случае Src(вычитаемое) - регистр, Destination - память
	//определяем 1-й операнд
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Src = AX & 255;
		if (log_to_console) cout << "CMP AL(" << (int)Src << ") ";
		break;
	case 1:
		Src = CX & 255;
		if (log_to_console) cout << "CMP CL(" << (int)Src << ") ";
		break;
	case 2:
		Src = DX & 255;
		if (log_to_console) cout << "CMP DL(" << (int)Src << ") ";
		break;
	case 3:
		Src = BX & 255;
		if (log_to_console) cout << "CMP BL(" << (int)Src << ") ";
		break;
	case 4:
		Src = AX >> 8;
		if (log_to_console) cout << "CMP AH(" << (int)Src << ") ";
		break;
	case 5:
		Src = CX >> 8;
		if (log_to_console) cout << "CMP CH(" << (int)Src << ") ";
		break;
	case 6:
		Src = DX >> 8;
		if (log_to_console) cout << "CMP DH(" << (int)Src << ") ";
		break;
	case 7:
		Src = BX >> 8;
		if (log_to_console) cout << "CMP BH(" << (int)Src << ") ";
		break;
	}

	//определяем объект назначения и результат операции SUB
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Result = memory.read(New_Addr, DS) - Src;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = ((memory.read(New_Addr, DS) & 0x7F) - (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((memory.read(New_Addr, DS) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << "with M[" << (int)New_Addr << "]";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Result = memory.read(New_Addr, DS) - Src;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = ((memory.read(New_Addr, DS) & 0x7F) - (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((memory.read(New_Addr, DS) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << "with M[" << (int)New_Addr << "]";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Result = memory.read(New_Addr, DS) - Src;
		Flag_CF = (Result >> 8) & 1;
		Flag_SF = ((Result >> 7) & 1);
		OF_Carry = ((memory.read(New_Addr, DS) & 0x7F) - (Src & 0x7F)) >> 7;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 255) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = (((memory.read(New_Addr, DS) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << "with M[" << (int)New_Addr << "]";
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Result = (AX & 255) - Src;
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			OF_Carry = (((AX & 255) & 0x7F) - (Src & 0x7F)) >> 7;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = (((AX & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "with AL(" << (int)(AX & 255) << ") = " << (int)(Result & 0x1FF);
			break;
		case 1:
			Result = (CX & 255) - Src;
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			OF_Carry = (((CX & 255) & 0x7F) - (Src & 0x7F)) >> 7;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = (((CX & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "with CL(" << (int)(CX & 255) << ") = " << (int)(Result & 0x1FF);
			break;
		case 2:
			Result = (DX & 255) - Src;
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			OF_Carry = (((DX & 255) & 0x7F) - (Src & 0x7F)) >> 7;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = (((DX & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "with DL(" << (int)(DX & 255) << ") = " << (int)(Result & 0x1FF);
			break;
		case 3:
			Result = (BX & 255) - Src;
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			OF_Carry = (((BX & 255) & 0x7F) - (Src & 0x7F)) >> 7;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = (((BX & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "with BL(" << (int)(BX & 255) << ") = " << (int)(Result & 0x1FF);
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
			if (log_to_console) cout << "with AH(" << (int)(AX >> 8) << ") = " << (int)(Result & 0x1FF);
			break;
		case 5:
			Result = (CX >> 8) - Src;
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			OF_Carry = ((((CX >> 8)) & 0x7F) - (Src & 0x7F)) >> 7;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = ((((CX >> 8) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "with CH(" << (int)(CX >> 8) << ") = " << (int)(Result & 0x1FF);
			break;
		case 6:
			Result = (DX >> 8) - Src;
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			OF_Carry = ((((DX >> 8)) & 0x7F) - (Src & 0x7F)) >> 7;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = ((((DX >> 8) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "with DH(" << (int)(DX >> 8) << ") = " << (int)(Result & 0x1FF);
			break;
		case 7:
			Result = (BX >> 8) - Src;
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			OF_Carry = ((((BX >> 8)) & 0x7F) - (Src & 0x7F)) >> 7;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = ((((BX >> 8) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "with BH(" << (int)(BX >> 8) << ") = " << (int)(Result & 0x1FF);
			break;
		}
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void CMP_Reg_RM_16()	//  CMP Reg with R/M 16 bit
{
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint32 Result = 0;
	uint8 additional_IPs = 0;
	bool OF_Carry = false;

	if (log_to_console) cout << "CMP ";

	//определяем SRC, это вычитаемое
	switch ((byte2 >> 3) & 7)
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

	//определяем объект назначения и результат операции SUB
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Result = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256 - Src;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = (Result >> 15) & 1;
		OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << " with M[" << (int)New_Addr << "]";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Result = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256 - Src;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = (Result >> 15) & 1;
		OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << " with M[" << (int)New_Addr << "]";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Result = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256 - Src;
		Flag_CF = (Result >> 16) & 1;
		Flag_SF = (Result >> 15) & 1;
		OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
		Flag_OF = Flag_CF ^ OF_Carry;
		if (Result & 0xFFFF) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		Flag_AF = ((((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
		if (log_to_console) cout << " with M[" << (int)New_Addr << "]";
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Result = AX - Src;
			//AX = Result & 0xFFFF;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			OF_Carry = (((AX) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = (parity_check[Result & 255]);
			Flag_AF = ((((AX) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "with AX = " << (int)(Result);
			break;
		case 1:
			Result = CX - Src;
			//CX = Result & 0xFFFF;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			OF_Carry = (((CX) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = (parity_check[Result & 255]);
			Flag_AF = ((((CX) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "with CX = " << (int)(Result);
			break;
		case 2:
			Result = DX - Src;
			//DX = Result & 0xFFFF;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			OF_Carry = (((DX) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = (parity_check[Result & 255]);
			Flag_AF = ((((DX) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "with DX = " << (int)(Result);
			break;
		case 3:
			Result = BX - Src;
			//BX = Result & 0xFFFF;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			OF_Carry = (((BX) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = (parity_check[Result & 255]);
			Flag_AF = ((((BX) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			break;
		case 4:
			Result = Stack_Pointer - Src;
			//Stack_Pointer = Result & 0xFFFF;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			OF_Carry = (((Stack_Pointer) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = (parity_check[Result & 255]);
			Flag_AF = ((((Stack_Pointer) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			break;
		case 5:
			Result = Base_Pointer - Src;
			//Base_Pointer = Result & 0xFFFF;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			OF_Carry = (((Base_Pointer) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = (parity_check[Result & 255]);
			Flag_AF = ((((Base_Pointer) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "with BP = " << (int)(Result);
			break;
		case 6:
			Result = Source_Index - Src;
			//Source_Index = Result & 0xFFFF;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			OF_Carry = (((Source_Index) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = (parity_check[Result & 255]);
			Flag_AF = ((((Source_Index) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "with SI = " << (int)(Result);
			break;
		case 7:
			Result = Destination_Index - Src;
			//Destination_Index = Result & 0xFFFF;
			Flag_CF = (Result >> 16) & 1;
			Flag_SF = ((Result >> 15) & 1);
			OF_Carry = (((Destination_Index) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = (parity_check[Result & 255]);
			Flag_AF = ((((Destination_Index) & 0x0F) - (Src & 0x0F)) >> 4) & 1;
			if (log_to_console) cout << "with DI = " << (int)(Result);
			break;
		}
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void CMP_RM_Reg_8()		//  CMP R/M with Reg 8 bit
{
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint8 Src = 0;
	uint16 Result = 0;
	uint8 additional_IPs = 0;
	bool OF_Carry = false;

	if (log_to_console) cout << "CMP ";
	//определяем операнд Src - вычитаемое
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] with ";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] with ";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] with ";
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint32 Result = 0;
	uint8 additional_IPs = 0;
	bool OF_Carry = false;

	if (log_to_console) cout << "CMP ";
	//определяем операнд Src - вычитаемое
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "](" << (int)Src << ") ";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "](" << (int)Src << ") ";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "](" << (int)Src << ") ";
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
	uint8 imm = memory.read(Instruction_Pointer + 1, CS);
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
	uint16 imm = memory.read(Instruction_Pointer + 1, CS) + memory.read(Instruction_Pointer + 2, CS) * 256;
	
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
	uint8 base = memory.read(Instruction_Pointer + 1, CS);

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
	uint8 base = memory.read(Instruction_Pointer + 1, CS);
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS);//второй байт
	uint8 OP = (byte2 >> 3) & 7;
	uint16 disp = 0;		//смещение
	uint16 Src = 0;		//источник данных
	uint16 New_Addr = 0;	//адрес перехода
	uint8 Result = 0;
	uint16 Result_16 = 0;
	uint8 additional_IPs = 0;
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS); //IMM
			if (log_to_console) cout << "TEST IMM(" << (int)Src << ") AND ";
			Result = memory.read(New_Addr, DS) & Src;
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)Result;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result];
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);//IMM
			if (log_to_console) cout << "TEST IMM(" << (int)Src << ") AND ";
			Result = memory.read(New_Addr, DS) & Src;
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)Result;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);//IMM
			if (log_to_console) cout << "TEST IMM(" << (int)Src << ") AND ";
			Result = memory.read(New_Addr, DS) & Src;
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)Result;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 3:
			// mod 11 источник - регистр
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = ~memory.read(New_Addr, DS);
			memory.write(New_Addr, DS, Src & 255);
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = ~memory.read(New_Addr, DS);
			memory.write(New_Addr, DS, Src & 255);
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = ~memory.read(New_Addr, DS);
			memory.write(New_Addr, DS, Src & 255);
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			if (memory.read(New_Addr, DS)) Flag_CF = 1;
			else Flag_CF = 0;
			if (memory.read(New_Addr, DS)==0x80) Flag_OF = 1;
			else Flag_OF = 0;
			//Flag_AF = (((~memory.read(New_Addr, DS) & 15) + 1) >> 4) & 1;
			//OF_Carry = ((~memory.read(New_Addr, DS) & 0x7F) + 1) >> 7;
			Src = ~memory.read(New_Addr, DS) + 1;
			//Flag_CF = (Src >> 8) & 1;
			Flag_SF = (Src >> 7) & 1;
			//Flag_OF = Flag_CF ^ OF_Carry;
			if (Src & 0xFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (Src & 15) Flag_AF = 1;
			else Flag_AF = 0;
			memory.write(New_Addr, DS, Src & 255);
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)(Src & 255);
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			if (memory.read(New_Addr, DS)) Flag_CF = 1;
			else Flag_CF = 0;
			if (memory.read(New_Addr, DS) == 0x80) Flag_OF = 1;
			else Flag_OF = 0;
			//Flag_AF = (((~memory.read(New_Addr, DS) & 15) + 1) >> 4) & 1;
			//OF_Carry = ((~memory.read(New_Addr, DS) & 0x7F) + 1) >> 7;
			Src = ~memory.read(New_Addr, DS) + 1;
			//Flag_CF = (Src >> 8) & 1;
			Flag_SF = ((Src >> 7) & 1);
			//Flag_OF = Flag_CF ^ OF_Carry;
			if (Src & 0xFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (Src & 15) Flag_AF = 1;
			else Flag_AF = 0;
			memory.write(New_Addr, DS, Src & 255);
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)(Src & 255);
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			if (memory.read(New_Addr, DS)) Flag_CF = 1;
			else Flag_CF = 0;
			if (memory.read(New_Addr, DS) == 0x80) Flag_OF = 1;
			else Flag_OF = 0;
			//Flag_AF = (((~memory.read(New_Addr, DS) & 15) + 1) >> 4) & 1;
			//OF_Carry = ((~memory.read(New_Addr, DS) & 0x7F) + 1) >> 7;
			Src = ~memory.read(New_Addr, DS) + 1;
			//Flag_CF = (Src >> 8) & 1;
			Flag_SF = ((Src >> 7) & 1);
			//Flag_OF = Flag_CF ^ OF_Carry;
			if (Src & 0xFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (Src & 15) Flag_AF = 1;
			else Flag_AF = 0;
			memory.write(New_Addr, DS, Src & 255);
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)(Src & 255);
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			if (log_to_console) cout << "M[" << int(New_Addr) << "]=";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			if (log_to_console) cout << "M[" << int(New_Addr) << "]=";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			if (log_to_console) cout << "M[" << int(New_Addr) << "]=";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			if (log_to_console) cout << "M[" << int(New_Addr) << "] = ";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			if (log_to_console) cout << "M[" << int(New_Addr) << "] = ";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			if (log_to_console) cout << "M[" << int(New_Addr) << "] = ";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			if (log_to_console) cout << "M[" << (int)(New_Addr) << "]("<<(int)Src <<") = ";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			if (log_to_console) cout << "M[" << (int)(New_Addr) << "](" << (int)Src << ") = ";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			if (log_to_console) cout << "M[" << setw(4) << (int)New_Addr << "]("<<(int)Src <<") = ";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			if (log_to_console) cout << "M[" << setw(4) << (int)New_Addr << "](" << (int)Src << ") = ";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			if (log_to_console) cout << "M[" << setw(4) << (int)New_Addr << "](" << (int)Src << ") = ";
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS);//второй байт
	uint8 OP = (byte2 >> 3) & 7;
	uint16 disp = 0;		//смещение
	uint16 Src = 0;			//источник данных
	uint16 New_Addr = 0;	//адрес перехода
	uint16 Result = 0;
	uint32 Result_32 = 0;
	uint8 additional_IPs = 0;
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "TEST IMM(" << (int)Src << ") AND ";
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & Src;
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)Result;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "TEST IMM(" << (int)Src << ") AND ";
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & Src;
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)Result;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "TEST IMM[" << (int)Src << "] AND ";
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & Src;
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)Result;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			break;
		case 3:
			// mod 11 источник - регистр
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = ~memory.read(New_Addr, DS);
			memory.write(New_Addr, DS, Src & 255);
			Src = ~memory.read(New_Addr + 1, DS);
			memory.write(New_Addr + 1, DS, Src & 255);
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = ~memory.read(New_Addr, DS);
			memory.write(New_Addr, DS, Src & 255);
			Src = ~memory.read(New_Addr + 1, DS);
			memory.write(New_Addr + 1, DS, Src & 255);
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = ~memory.read(New_Addr, DS);
			memory.write(New_Addr, DS, Src & 255);
			Src = ~memory.read(New_Addr + 1, DS);
			memory.write(New_Addr + 1, DS, Src & 255);
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
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
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)(Src & 255);
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
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
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)(Src & 255);
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
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
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)(Src & 255);
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			if (log_to_console) cout << "M[" << int(New_Addr) << "]=";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;;
			if (log_to_console) cout << "M[" << int(New_Addr) << "]=";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			if (log_to_console) cout << "M[" << int(New_Addr) << "]=";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			if (log_to_console) cout << "M[" << int(New_Addr) << "]=";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;;
			if (log_to_console) cout << "M[" << int(New_Addr) << "]=";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			if (log_to_console) cout << "M[" << int(New_Addr) << "]=";
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
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			if (log_to_console) cout << "M[" << int(New_Addr) << "] = ";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;;
			if (log_to_console) cout << "M[" << int(New_Addr) << "] = ";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			if (log_to_console) cout << "M[" << int(New_Addr) << "] = ";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			if (log_to_console) cout << "M[" << int(New_Addr) << "]("<<(int)Src <<") = ";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;;
			if (log_to_console) cout << "M[" << int(New_Addr) << "](" << (int)Src << ") = ";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			if (log_to_console) cout << "M[" << int(New_Addr) << "](" << (int)Src << ") = ";
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint8 Result = 0;
	uint16 Src = 0;
	uint8 MSB = 0;
	uint8 additional_IPs = 0;

	switch ((byte2 >> 3) & 7)
	{
	case 0:  //ROL Rotate Left

		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			Flag_CF = (Src >> 7) & 1;
			Src = (Src << 1) | Flag_CF;
			Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
			memory.write(New_Addr, DS, Src & 255);
			if (log_to_console) cout << "ROL M[" << (int)New_Addr << "]";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			Flag_CF = (Src >> 7) & 1;
			Src = (Src << 1) | Flag_CF;
			Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
			memory.write(New_Addr, DS, Src & 255);
			if (log_to_console) cout << "ROL M[" << (int)New_Addr << "]";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			Flag_CF = (Src >> 7) & 1;
			Src = (Src << 1) | Flag_CF;
			Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
			memory.write(New_Addr, DS, Src & 255);
			if (log_to_console) cout << "ROL M[" << (int)New_Addr << "]";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Flag_CF = (memory.read(New_Addr, DS)) & 1;
			Src = (Flag_CF * 0x80) | (memory.read(New_Addr, DS) >> 1);
			memory.write(New_Addr, DS, Src & 255);
			Flag_OF = !parity_check[Src & 0b11000000];
			if (log_to_console) cout << "ROR M[" << (int)New_Addr << "]";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Flag_CF = (memory.read(New_Addr, DS)) & 1;
			Src = (Flag_CF * 0x80) | (memory.read(New_Addr, DS) >> 1);
			memory.write(New_Addr, DS, Src & 255);
			Flag_OF = !parity_check[Src & 0b11000000];
			if (log_to_console) cout << "ROR M[" << (int)New_Addr << "]";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Flag_CF = (memory.read(New_Addr, DS)) & 1;
			Src = (Flag_CF * 0x80) | (memory.read(New_Addr, DS) >> 1);
			memory.write(New_Addr, DS, Src & 255);
			Flag_OF = !parity_check[Src & 0b11000000];
			if (log_to_console) cout << "ROR M[" << (int)New_Addr << "]";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = (memory.read(New_Addr, DS) << 1) | Flag_CF;
			Flag_CF = (Src >> 8) & 1;
			memory.write(New_Addr, DS, Src & 255);
			Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
			if (log_to_console) cout << "RCL M[" << (int)New_Addr << "]";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = (memory.read(New_Addr, DS) << 1) | Flag_CF;
			Flag_CF = (Src >> 8) & 1;
			memory.write(New_Addr, DS, Src & 255);
			Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
			if (log_to_console) cout << "RCL M[" << (int)New_Addr << "]";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = (memory.read(New_Addr, DS) << 1) | Flag_CF;
			Flag_CF = (Src >> 8) & 1;
			memory.write(New_Addr, DS, Src & 255);
			Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
			if (log_to_console) cout << "RCL M[" << (int)New_Addr << "]";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = (Flag_CF << 7) | (memory.read(New_Addr, DS) >> 1);
			Flag_CF = (memory.read(New_Addr, DS)) & 1;
			memory.write(New_Addr, DS, Src & 255);
			Flag_OF = !parity_check[(Src ) & 0b11000000];
			if (log_to_console) cout << "RCR M[" << (int)New_Addr << "]";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = (Flag_CF << 7) | (memory.read(New_Addr, DS) >> 1);
			Flag_CF = (memory.read(New_Addr, DS)) & 1;
			memory.write(New_Addr, DS, Src & 255);
			Flag_OF = !parity_check[(Src ) & 0b11000000];
			if (log_to_console) cout << "RCR M[" << (int)New_Addr << "]";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = (Flag_CF << 7) | (memory.read(New_Addr, DS) >> 1);
			Flag_CF = (memory.read(New_Addr, DS)) & 1;
			memory.write(New_Addr, DS, Src & 255);
			Flag_OF = !parity_check[(Src ) & 0b11000000];
			if (log_to_console) cout << "RCR M[" << (int)New_Addr << "]";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) << 1;
			memory.write(New_Addr, DS, Src & 255);
			Flag_CF = (Src >> 8) & 1;
			Flag_SF = (Src >> 7) & 1;
			Flag_OF = !parity_check[(Src >> 7) & 3];
			if (Src & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift left M[" << (int)New_Addr << "]";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) << 1;
			memory.write(New_Addr, DS, Src & 255);
			Flag_CF = (Src >> 8) & 1;
			Flag_SF = (Src >> 7) & 1;
			Flag_OF = !parity_check[(Src >> 7) & 3];
			if (Src & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift left M[" << (int)New_Addr << "]";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) << 1;
			memory.write(New_Addr, DS, Src & 255);
			Flag_CF = (Src >> 8) & 1;
			Flag_SF = (Src >> 7) & 1;
			Flag_OF = !parity_check[(Src >> 7) & 3];
			if (Src & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift left M[" << (int)New_Addr << "]";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Flag_CF = (memory.read(New_Addr, DS)) & 1;
			Src = memory.read(New_Addr, DS) >> 1;
			memory.write(New_Addr, DS, Src & 255);
			Flag_SF = ((Src >> 7) & 1);
			Flag_OF = !parity_check[Src & 0b11000000];
			if (Src & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift(SHR) right M[" << New_Addr << "]";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Flag_CF = (memory.read(New_Addr, DS)) & 1;
			Src = memory.read(New_Addr, DS) >> 1;
			memory.write(New_Addr, DS, Src & 255);
			Flag_SF = ((Src >> 7) & 1);
			Flag_OF = !parity_check[Src & 0b11000000];
			if (Src & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift(SHR) right M[" << New_Addr << "]";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Flag_CF = (memory.read(New_Addr, DS)) & 1;
			Src = memory.read(New_Addr, DS) >> 1;
			memory.write(New_Addr, DS, Src & 255);
			Flag_SF = ((Src >> 7) & 1);
			Flag_OF = !parity_check[Src & 0b11000000];
			if (Src & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift(SHR) right M[" << New_Addr << "]";
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
		if (log_to_console) cout << "SETMO ";
		switch (byte2 >> 6)
		{
		case 0:
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Result = memory.read(New_Addr, DS) | Src;
			memory.write(New_Addr, DS, Result);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = true;
			Flag_ZF = false;
			Flag_PF = true;
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Result = memory.read(New_Addr, DS) | Src;
			memory.write(New_Addr, DS, Result);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = true;
			Flag_ZF = false;
			Flag_PF = true;
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Result = memory.read(New_Addr, DS) | Src;
			memory.write(New_Addr, DS, Result);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = true;
			Flag_ZF = false;
			Flag_PF = true;
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)Result;
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Flag_CF = (memory.read(New_Addr, DS)) & 1;
			MSB = (memory.read(New_Addr, DS)) & 128;
			Src = (memory.read(New_Addr, DS) >> 1) | MSB;
			memory.write(New_Addr, DS, Src & 255);
			Flag_SF = ((Src >> 7) & 1);
			Flag_OF = !parity_check[Src & 0b11000000];
			if (Src & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift(SAR) right M[" << (int)New_Addr << "]";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Flag_CF = (memory.read(New_Addr, DS)) & 1;
			MSB = (memory.read(New_Addr, DS)) & 128;
			Src = (memory.read(New_Addr, DS) >> 1) | MSB;
			memory.write(New_Addr, DS, Src & 255);
			Flag_SF = ((Src >> 7) & 1);
			Flag_OF = !parity_check[Src & 0b11000000];
			if (Src & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift(SAR) right M[" << (int)New_Addr << "]";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Flag_CF = (memory.read(New_Addr, DS)) & 1;
			MSB = (memory.read(New_Addr, DS)) & 128;
			Src = (memory.read(New_Addr, DS) >> 1) | MSB;
			memory.write(New_Addr, DS, Src & 255);
			Flag_SF = ((Src >> 7) & 1);
			Flag_OF = !parity_check[Src & 0b11000000];
			if (Src & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift(SAR) right M[" << (int)New_Addr << "]";
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
		
	default:
		cout << "unknown OP = " << (int)memory.read(Instruction_Pointer, CS);
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void SHL_ROT_16()			// Shift Logical / Arithmetic Left / 16bit / once
{
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint32 Src = 0;
	uint16 MSB = 0;
	uint8 additional_IPs = 0;
	uint16 Result = 0;

	switch ((byte2 >> 3) & 7)
	{

	case 0:  //ROL Rotate Left
	
			//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			Flag_CF = (Src >> 15) & 1;
			Flag_OF = !parity_check[(Src >> 8) & 0b11000000];
			Src = (Src << 1) | Flag_CF;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			if (log_to_console) cout << "ROL M[" << (int)New_Addr << "]";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			Flag_CF = (Src >> 15) & 1;
			Flag_OF = !parity_check[(Src >> 8) & 0b11000000];
			Src = (Src << 1) | Flag_CF;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			if (log_to_console) cout << "ROL M[" << (int)New_Addr << "]";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			Flag_CF = (Src >> 15) & 1;
			Flag_OF = !parity_check[(Src >> 8) & 0b11000000];
			Src = (Src << 1) | Flag_CF;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			if (log_to_console) cout << "ROL M[" << (int)New_Addr << "]";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Flag_CF = memory.read(New_Addr, DS) & 1;
			Src = (Flag_CF * 0x8000) | ((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) >> 1);
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, Src >> 8);
			Flag_OF = !parity_check[(Src >> 14) & 3];
			if (log_to_console) cout << "ROR M[" << New_Addr << "]";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Flag_CF = memory.read(New_Addr, DS) & 1;
			Src = (Flag_CF * 0x8000) | ((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) >> 1);
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, Src >> 8);
			Flag_OF = !parity_check[(Src >> 14) & 3];
			if (log_to_console) cout << "ROR M[" << New_Addr << "]";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Flag_CF = memory.read(New_Addr, DS) & 1;
			Src = (Flag_CF * 0x8000) | ((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) >> 1);
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, Src >> 8);
			Flag_OF = !parity_check[(Src >> 14) & 3];
			if (log_to_console) cout << "ROR M[" << New_Addr << "]";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = ((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) << 1) | Flag_CF;
			Flag_CF = (Src >> 16) & 1;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			Flag_OF = !parity_check[(Src >> 15) & 3];
			if (log_to_console) cout << "RCL M[" << (int)New_Addr << "]";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = ((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) << 1) | Flag_CF;
			Flag_CF = (Src >> 16) & 1;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			Flag_OF = !parity_check[(Src >> 15) & 3];
			if (log_to_console) cout << "RCL M[" << (int)New_Addr << "]";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = ((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) << 1) | Flag_CF;
			Flag_CF = (Src >> 16) & 1;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			Flag_OF = !parity_check[(Src >> 15) & 3];
			if (log_to_console) cout << "RCL M[" << (int)New_Addr << "]";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Flag_OF = memory.read(New_Addr, DS) & 1; //tmp
			Src = (Flag_CF << 15) | ((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) >> 1);
			Flag_CF = Flag_OF;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			Flag_OF = !parity_check[(Src >> 14) & 3];
			if (log_to_console) cout << "RCR M[" << New_Addr << "]";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Flag_OF = memory.read(New_Addr, DS) & 1; //tmp
			Src = (Flag_CF << 15) | ((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) >> 1);
			Flag_CF = Flag_OF;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			Flag_OF = !parity_check[(Src >> 14) & 3];
			if (log_to_console) cout << "RCR M[" << New_Addr << "]";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Flag_OF = memory.read(New_Addr, DS) & 1; //tmp
			Src = (Flag_CF << 15) | ((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) >> 1);
			Flag_CF = Flag_OF;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			Flag_OF = !parity_check[(Src >> 14) & 3];
			if (log_to_console) cout << "RCR M[" << New_Addr << "]";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) << 1;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			Flag_CF = (Src >> 16) & 1;
			Flag_SF = (Src >> 15) & 1;
			Flag_OF = !parity_check[(Src >> 15) & 3];
			if (Src & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift left M[" << (int)New_Addr << "]";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) << 1;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			Flag_CF = (Src >> 16) & 1;
			Flag_SF = (Src >> 15) & 1;
			Flag_OF = !parity_check[(Src >> 15) & 3];
			if (Src & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift left M[" << (int)New_Addr << "]";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) << 1;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			Flag_CF = (Src >> 16) & 1;
			Flag_SF = (Src >> 15) & 1;
			Flag_OF = !parity_check[(Src >> 15) & 3];
			if (Src & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift left M[" << (int)New_Addr << "]";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Flag_CF = memory.read(New_Addr, DS) & 1;
			Src = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) >> 1;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			Flag_SF = false;
			Flag_OF = (Src >> 14) & 1;
			if (Src & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift(SHR) right M[" << (int)New_Addr << "]";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Flag_CF = memory.read(New_Addr, DS) & 1;
			Src = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) >> 1;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			Flag_SF = false;
			Flag_OF = (Src >> 14) & 1;
			if (Src & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift(SHR) right M[" << (int)New_Addr << "]";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Flag_CF = memory.read(New_Addr, DS) & 1;
			Src = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) >> 1;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			Flag_SF = false;
			Flag_OF = (Src >> 14) & 1;
			if (Src & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift(SHR) right M[" << (int)New_Addr << "]";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) | Src;
			memory.write(New_Addr, DS, Result & 255);
			memory.write(New_Addr + 1, DS, (Result >> 8) & 255);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = true;
			Flag_ZF = false;
			Flag_PF = true;
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) | Src;
			memory.write(New_Addr, DS, Result & 255);
			memory.write(New_Addr + 1, DS, (Result >> 8) & 255);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = true;
			Flag_ZF = false;
			Flag_PF = true;
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) | Src;
			memory.write(New_Addr, DS, Result & 255);
			memory.write(New_Addr + 1, DS, (Result >> 8) & 255);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = true;
			Flag_ZF = false;
			Flag_PF = true;
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)Result;
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Flag_CF = memory.read(New_Addr, DS) & 1;
			Src = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256);
			MSB = Src & 0x8000;
			Src = (Src >> 1) | MSB;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			Flag_SF = ((Src >> 15) & 1);
			Flag_OF = 0;
			if (Src & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift(SAR) right M[" << New_Addr << "]";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Flag_CF = memory.read(New_Addr, DS) & 1;
			Src = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256);
			MSB = Src & 0x8000;
			Src = (Src >> 1) | MSB;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			Flag_SF = ((Src >> 15) & 1);
			Flag_OF = 0;
			if (Src & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift(SAR) right M[" << New_Addr << "]";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Flag_CF = memory.read(New_Addr, DS) & 1;
			Src = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256);
			MSB = Src & 0x8000;
			Src = (Src >> 1) | MSB;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			Flag_SF = ((Src >> 15) & 1);
			Flag_OF = 0;
			if (Src & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift(SAR) right M[" << New_Addr << "]";
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
		cout << "unknown OP = " << memory.read(Instruction_Pointer, CS);
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void SHL_ROT_8_mult()		// Shift Logical / Arithmetic Left / 8bit / CL
{
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint16 Result = 0;
	uint8 repeats = CX & 255;
	uint8 MSB = 0;
	uint8 additional_IPs = 0;
	uint8 oldCF;

	switch ((byte2 >> 3) & 7)
	{
	case 0:  //ROL Rotate Left

		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = (Src >> 7) & 1;
				Src = (Src << 1) | Flag_CF;
			}
			memory.write(New_Addr, DS, Src & 255);
			Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
			if (log_to_console) cout << "ROL M[" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = (Src >> 7) & 1;
				Src = (Src << 1) | Flag_CF;
			}
			memory.write(New_Addr, DS, Src & 255);
			Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
			if (log_to_console) cout << "ROL M[" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = (Src >> 7) & 1;
				Src = (Src << 1) | Flag_CF;
			}
			memory.write(New_Addr, DS, Src & 255);
			Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
			if (log_to_console) cout << "ROL M[" << (int)New_Addr << "] " << (int)repeats << " times";
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
				Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
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
				Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
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
				Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
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
				Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
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
				Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
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
				Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
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
				Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
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
				Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = Src & 1;
				Src = (Src >> 1) | (Flag_CF * 0x80);
			}
			memory.write(New_Addr, DS, Src & 255);
			if (log_to_console) cout << "ROR M[" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = Src & 1;
				Src = (Src >> 1) | (Flag_CF * 0x80);
			}
			memory.write(New_Addr, DS, Src & 255);
			if (log_to_console) cout << "ROR M[" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = Src & 1;
				Src = (Src >> 1) | (Flag_CF * 0x80);
			}
			memory.write(New_Addr, DS, Src & 255);
			if (log_to_console) cout << "ROR M[" << (int)New_Addr << "] " << (int)repeats << " times";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			for (int i = 0; i < repeats; i++)
			{
				Src = (Src << 1) | Flag_CF;
				Flag_CF = (Src >> 8) & 1;
			}
			memory.write(New_Addr, DS, Src & 255);
			if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
			if (log_to_console) cout << "RCL M[" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			for (int i = 0; i < repeats; i++)
			{
				Src = (Src << 1) | Flag_CF;
				Flag_CF = (Src >> 8) & 1;
			}
			memory.write(New_Addr, DS, Src & 255);
			if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
			if (log_to_console) cout << "RCL M[" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			for (int i = 0; i < repeats; i++)
			{
				Src = (Src << 1) | Flag_CF;
				Flag_CF = (Src >> 8) & 1;
			}
			memory.write(New_Addr, DS, Src & 255);
			if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
			if (log_to_console) cout << "RCL M[" << (int)New_Addr << "] " << (int)repeats << " times";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			for (int i = 0; i < repeats; i++)
			{
				oldCF = Src & 1; // tmp
				Src = (Src >> 1) | (Flag_CF << 7);
				Flag_CF = oldCF;
			}
			memory.write(New_Addr, DS, Src & 255);
			if (repeats == 1) Flag_OF = !parity_check[Src & 0b11000000];
			if (log_to_console) cout << "RCR M[" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			for (int i = 0; i < repeats; i++)
			{
				oldCF = Src & 1; // tmp
				Src = (Src >> 1) | (Flag_CF << 7);
				Flag_CF = oldCF;
			}
			memory.write(New_Addr, DS, Src & 255);
			if (repeats == 1) Flag_OF = !parity_check[Src & 0b11000000];
			if (log_to_console) cout << "RCR M[" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			for (int i = 0; i < repeats; i++)
			{
				oldCF = Src & 1; // tmp
				Src = (Src >> 1) | (Flag_CF << 7);
				Flag_CF = oldCF;
			}
			memory.write(New_Addr, DS, Src & 255);
			if (repeats == 1) Flag_OF = !parity_check[Src & 0b11000000];
			if (log_to_console) cout << "RCR M[" << (int)New_Addr << "] " << (int)repeats << " times";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) << repeats;
			memory.write(New_Addr, DS, Src & 255);
			Flag_CF = (Src >> 8) & 1;
			Flag_SF = (Src >> 7) & 1;
			if (Src & 255) Flag_ZF = false;
			else Flag_ZF = true;
			if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift left M[" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 1:
			additional_IPs = 1;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) << repeats;
			memory.write(New_Addr, DS, Src & 255);
			Flag_CF = (Src >> 8) & 1;
			Flag_SF = (Src >> 7) & 1;
			if (Src & 255) Flag_ZF = false;
			else Flag_ZF = true;
			if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift left M[" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 2:
			additional_IPs = 2;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) << repeats;
			memory.write(New_Addr, DS, Src & 255);
			Flag_CF = (Src >> 8) & 1;
			Flag_SF = (Src >> 7) & 1;
			if (Src & 255) Flag_ZF = false;
			else Flag_ZF = true;
			if (repeats == 1) Flag_OF = !parity_check[(Src >> 1) & 0b11000000];
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift left M[" << (int)New_Addr << "] " << (int)repeats << " times";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Flag_CF = (memory.read(New_Addr, DS) >> (repeats - 1)) & 1;
			Src = memory.read(New_Addr, DS) >> repeats;
			memory.write(New_Addr, DS, Src & 255);
			Flag_SF = ((Src >> 7) & 1);
			if (Src & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (repeats == 1) Flag_OF = !parity_check[(Src) & 0b11000000];
			if (log_to_console) cout << "Shift(SHR) right M[" << New_Addr << "] " << (int)repeats << " times";
			break;
		case 1:
			additional_IPs = 1;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Flag_CF = (memory.read(New_Addr, DS) >> (repeats - 1)) & 1;
			Src = memory.read(New_Addr, DS) >> repeats;
			memory.write(New_Addr, DS, Src & 255);
			Flag_SF = ((Src >> 7) & 1);
			if (Src & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (repeats == 1) Flag_OF = !parity_check[(Src) & 0b11000000];
			if (log_to_console) cout << "Shift(SHR) right M[" << New_Addr << "] " << (int)repeats << " times";
			break;
		case 2:
			additional_IPs = 2;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Flag_CF = (memory.read(New_Addr, DS) >> (repeats - 1)) & 1;
			Src = memory.read(New_Addr, DS) >> repeats;
			memory.write(New_Addr, DS, Src & 255);
			Flag_SF = ((Src >> 7) & 1);
			if (Src & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (repeats == 1) Flag_OF = !parity_check[(Src) & 0b11000000];
			if (log_to_console) cout << "Shift(SHR) right M[" << New_Addr << "] " << (int)repeats << " times";
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
		if (log_to_console) cout << "SETMO ";
		switch (byte2 >> 6)
		{
		case 0:
			if ((byte2 & 7) == 6) additional_IPs = 2;
			if (!(CX & 255)) break;
			New_Addr = mod_RM(byte2);
			Result = memory.read(New_Addr, DS) | Src;
			memory.write(New_Addr, DS, Result);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = true;
			Flag_ZF = false;
			Flag_PF = true;
			Flag_AF = 0;
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 1:
			additional_IPs = 1;
			if (!(CX & 255)) break;
			New_Addr = mod_RM(byte2);
			Result = memory.read(New_Addr, DS) | Src;
			memory.write(New_Addr, DS, Result);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = true;
			Flag_ZF = false;
			Flag_PF = true;
			Flag_AF = 0;
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 2:
			additional_IPs = 2;
			if (!(CX & 255)) break;
			New_Addr = mod_RM(byte2);
			Result = memory.read(New_Addr, DS) | Src;
			memory.write(New_Addr, DS, Result);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = true;
			Flag_ZF = false;
			Flag_PF = true;
			Flag_AF = 0;
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)Result;
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			if (!repeats) break;
			Src = memory.read(New_Addr, DS);
			for (int v = 0; v < repeats; v++)
			{
				Flag_CF = Src & 1;
				Flag_SF = MSB = Src & 0b10000000;
				Src = (Src >> 1) | MSB;
			}
			memory.write(New_Addr, DS, Src & 255);
			if (Src & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (repeats == 1) Flag_OF = false;
			if (log_to_console) cout << "Shift(SAR) right M[" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 1:
			additional_IPs = 1;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			for (int v = 0; v < repeats; v++)
			{
				Flag_CF = Src & 1;
				Flag_SF = MSB = Src & 0b10000000;
				Src = (Src >> 1) | MSB;
			}
			memory.write(New_Addr, DS, Src & 255);
			if (Src & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (repeats == 1) Flag_OF = false;
			if (log_to_console) cout << "Shift(SAR) right M[" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 2:
			additional_IPs = 2;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS);
			for (int v = 0; v < repeats; v++)
			{
				Flag_CF = Src & 1;
				Flag_SF = MSB = Src & 0b10000000;
				Src = (Src >> 1) | MSB;
			}
			memory.write(New_Addr, DS, Src & 255);
			if (Src & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (repeats == 1) Flag_OF = false;
			if (log_to_console) cout << "Shift(SAR) right M[" << (int)New_Addr << "] " << (int)repeats << " times";
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
		cout << "unknown OP = " << memory.read(Instruction_Pointer, CS);
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void SHL_ROT_16_mult()		// Shift Logical / Arithmetic Left / 16bit / CL
{
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint32 Src = 0;
	uint32 Result = 0;
	uint8 repeats = CX & 255;
	uint16 MSB = 0;
	uint8 additional_IPs = 0;
	uint16 oldCF;

	switch ((byte2 >> 3) & 7)
	{
	case 0:  //ROL Rotate Left

		//while (repeats > 16) repeats -= 16;
		//определяем объект
		switch (byte2 >> 6)
		{
		case 0:
			if ((byte2 & 7) == 6) additional_IPs = 2;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = (Src >> 15) & 1;
				Src = Src << 1;
				Src = Src | Flag_CF;
			}
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			if (log_to_console) cout << "ROL M[" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 1:
			additional_IPs = 1;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = (Src >> 15) & 1;
				Src = Src << 1;
				Src = Src | Flag_CF;
			}
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			if (log_to_console) cout << "ROL M[" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 2:
			additional_IPs = 2;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = (Src >> 15) & 1;
				Src = Src << 1;
				Src = Src | Flag_CF;
			}
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			if (log_to_console) cout << "ROL M[" << (int)New_Addr << "] " << (int)repeats << " times";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = Src & 1;
				Src = (Src >> 1) | (Flag_CF * 0x8000);
			}
			if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			if (log_to_console) cout << "RCR M[" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 1:
			additional_IPs = 1;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = Src & 1;
				Src = (Src >> 1) | (Flag_CF * 0x8000);
			}
			if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			if (log_to_console) cout << "ROR M[" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 2:
			additional_IPs = 2;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			for (int i = 0; i < repeats; i++)
			{
				Flag_CF = Src & 1;
				Src = (Src >> 1) | (Flag_CF * 0x8000);
			}
			if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			if (log_to_console) cout << "ROR M[" << (int)New_Addr << "] " << (int)repeats << " times";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			for (int i = 0; i < repeats; i++)
			{
				Src = (Src << 1) | Flag_CF;
				Flag_CF = (Src >> 16) & 1;
			}
			if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			if (log_to_console) cout << "RCL M[" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 1:
			additional_IPs = 1;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			for (int i = 0; i < repeats; i++)
			{
				Src = (Src << 1) | Flag_CF;
				Flag_CF = (Src >> 16) & 1;
			}
			if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			if (log_to_console) cout << "RCL M[" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 2:
			additional_IPs = 2;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			for (int i = 0; i < repeats; i++)
			{
				Src = (Src << 1) | Flag_CF;
				Flag_CF = (Src >> 16) & 1;
			}
			if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			if (log_to_console) cout << "RCL M[" << (int)New_Addr << "] " << (int)repeats << " times";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			for (int i = 0; i < repeats; i++)
			{
				oldCF = Flag_CF;
				Flag_CF = Src & 1;
				Src = (Src >> 1) | (oldCF << 15);
			}
			if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			if (log_to_console) cout << "RCR M[" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 1:
			additional_IPs = 1;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			for (int i = 0; i < repeats; i++)
			{
				oldCF = Flag_CF;
				Flag_CF = Src & 1;
				Src = (Src >> 1) | (oldCF << 15);
			}
			if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			if (log_to_console) cout << "RCR M[" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 2:
			additional_IPs = 2;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			for (int i = 0; i < repeats; i++)
			{
				oldCF = Flag_CF;
				Flag_CF = Src & 1;
				Src = (Src >> 1) | (oldCF << 15);
			}
			if (repeats == 1) Flag_OF = !parity_check[Src >> 14];
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			if (log_to_console) cout << "RCR M[" << (int)New_Addr << "] " << (int)repeats << " times";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) << repeats;
			if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			Flag_CF = (Src >> 16) & 1;
			Flag_SF = ((Src >> 15) & 1);
			if (Src & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift left M[" << (int)*DS << ":" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 1:
			additional_IPs = 1;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) << repeats;
			if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			Flag_CF = (Src >> 16) & 1;
			Flag_SF = ((Src >> 15) & 1);
			if (Src & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift left M[" << (int)*DS << ":" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 2:
			additional_IPs = 2;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) << repeats;
			if (repeats == 1) Flag_OF = !parity_check[Src >> 15];
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			Flag_CF = (Src >> 16) & 1;
			Flag_SF = ((Src >> 15) & 1);
			if (Src & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift left M[" << (int)*DS << ":" << (int)New_Addr << "] " << (int)repeats << " times";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256);
			Flag_CF = (Src >> (repeats - 1)) & 1;
			Src = Src >> repeats;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			Flag_SF = ((Src >> 15) & 1);
			if (Src & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			if (repeats == 1) Flag_OF = Flag_SF;
			else Flag_OF = 0;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift(SHR) right M[" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 1:
			additional_IPs = 1;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256);
			Flag_CF = (Src >> (repeats - 1)) & 1;
			Src = Src >> repeats;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			Flag_SF = ((Src >> 15) & 1);
			if (Src & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			if (repeats == 1) Flag_OF = Flag_SF;
			else Flag_OF = 0;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift(SHR) right M[" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 2:
			additional_IPs = 2;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256);
			Flag_CF = (Src >> (repeats - 1)) & 1;
			Src = Src >> repeats;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			Flag_SF = ((Src >> 15) & 1);
			if (Src & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			if (repeats == 1) Flag_OF = Flag_SF;
			else Flag_OF = 0;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift(SHR) right M[" << (int)New_Addr << "] " << (int)repeats << " times";
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			if (!(CX & 255)) break;
			New_Addr = mod_RM(byte2);
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) | Src;
			memory.write(New_Addr, DS, Result & 255);
			memory.write(New_Addr + 1, DS, (Result >> 8) & 255);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = true;
			Flag_ZF = false;
			Flag_PF = true;
			Flag_AF = 0;
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 1:
			additional_IPs = 1;
			if (!(CX & 255)) break;
			New_Addr = mod_RM(byte2);
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) | Src;
			memory.write(New_Addr, DS, Result & 255);
			memory.write(New_Addr + 1, DS, (Result >> 8) & 255);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = true;
			Flag_ZF = false;
			Flag_PF = true;
			Flag_AF = 0;
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 2:
			additional_IPs = 2;
			if (!(CX & 255)) break;
			New_Addr = mod_RM(byte2);
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) | Src;
			memory.write(New_Addr, DS, Result & 255);
			memory.write(New_Addr + 1, DS, (Result >> 8) & 255);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = true;
			Flag_ZF = false;
			Flag_PF = true;
			Flag_AF = 0;
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)Result;
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256);
			if (Src & 0x8000) MSB = 0b1111111111111111 << (16 - repeats);
			Flag_CF = (Src >> (repeats - 1)) & 1;
			Src = (Src >> repeats) | MSB;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			Flag_SF = ((Src >> 15) & 1);
			if (Src & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			if (repeats == 1) Flag_OF = 0;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift(SAR) right M[" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 1:
			additional_IPs = 1;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256);
			if (Src & 0x8000) MSB = 0b1111111111111111 << (16 - repeats);
			Flag_CF = (Src >> (repeats - 1)) & 1;
			Src = (Src >> repeats) | MSB;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			Flag_SF = ((Src >> 15) & 1);
			if (Src & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			if (repeats == 1) Flag_OF = 0;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift(SAR) right M[" << (int)New_Addr << "] " << (int)repeats << " times";
			break;
		case 2:
			additional_IPs = 2;
			if (!repeats) break;
			New_Addr = mod_RM(byte2);
			Src = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256);
			if (Src & 0x8000) MSB = 0b1111111111111111 << (16 - repeats);
			Flag_CF = (Src >> (repeats - 1)) & 1;
			Src = (Src >> repeats) | MSB;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, (Src >> 8) & 255);
			Flag_SF = ((Src >> 15) & 1);
			if (Src & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			if (repeats == 1) Flag_OF = 0;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "Shift(SAR) right M[" << (int)New_Addr << "] " << (int)repeats << " times";
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
		cout << "unknown OP = " << memory.read(Instruction_Pointer, CS);
	}
	Instruction_Pointer += 2 + additional_IPs;
}

//AND
void AND_RM_8()				// AND R + R/M -> R/M 8bit
{
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint8 Src = 0;
	uint8 Result = 0;
	uint8 additional_IPs = 0;

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
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		if (log_to_console) cout << "AND M[" << (int)New_Addr << "] = ";
		Result = memory.read(New_Addr, DS) & Src;
		memory.write(New_Addr, DS, Result);
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 7) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		if (log_to_console) cout << "AND M[" << (int)New_Addr << "] = ";
		Result = memory.read(New_Addr, DS) & Src;
		memory.write(New_Addr, DS, Result);
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 7) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		if (log_to_console) cout << "AND M[" << (int)New_Addr << "] = ";
		Result = memory.read(New_Addr, DS) & Src;
		memory.write(New_Addr, DS, Result);
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint16 Result = 0;
	uint8 additional_IPs = 0;

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
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		if (log_to_console) cout << "AND M[" << (int)New_Addr << "] = ";
		Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & Src;
		memory.write(New_Addr, DS, Result & 255);
		memory.write(New_Addr + 1, DS, Result >> 8);
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 15) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		if (log_to_console) cout << "AND M[" << (int)New_Addr << "] = ";
		Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & Src;
		memory.write(New_Addr, DS, Result & 255);
		memory.write(New_Addr + 1, DS, Result >> 8);
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 15) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		if (log_to_console) cout << "AND M[" << (int)New_Addr << "] = ";
		Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & Src;
		memory.write(New_Addr, DS, Result & 255);
		memory.write(New_Addr + 1, DS, Result >> 8);
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint8 Src = 0;
	uint8 Result = 0;
	uint8 additional_IPs = 0;

	//определяем источник
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] AND ";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] AND ";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] AND ";
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint16 Result = 0;
	uint8 additional_IPs = 0;

	//определяем источник
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "M[" << (int)New_Addr << "] ";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "[" << (int)New_Addr << "] ";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "[" << (int)New_Addr << "] ";
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
	Src = memory.read(Instruction_Pointer + 1, CS);

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
	Src = memory.read(Instruction_Pointer + 1, CS) + memory.read(Instruction_Pointer + 2, CS) * 256;

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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint8 Src = 0;
	uint8 Result = 0;
	uint8 additional_IPs = 0;

	//определяем источник
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "TEST M[" << (int)New_Addr << "] AND ";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "TEST M[" << (int)New_Addr << "] AND ";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "TEST M[" << (int)New_Addr << "] AND ";
		break;
	case 3:
		// mod 11 источник - регистр
		switch (byte2 & 7)
		{
		case 0:
			Src = AX & 255;
			if (log_to_console) cout << "TEST AL(" << (int)Src << ") AND ";
			break;
		case 1:
			Src = CX & 255;
			if (log_to_console) cout << "TEST CL(" << (int)Src << ") AND ";
			break;
		case 2:
			Src = DX & 255;
			if (log_to_console) cout << "TEST DL(" << (int)Src << ") AND ";
			break;
		case 3:
			Src = BX & 255;
			if (log_to_console) cout << "TEST BL(" << (int)Src << ") AND ";
			break;
		case 4:
			Src = AX >> 8;
			if (log_to_console) cout << "TEST AH(" << (int)Src << ") AND ";
			break;
		case 5:
			Src = CX >> 8;
			if (log_to_console) cout << "TEST CH(" << (int)Src << ") AND ";
			break;
		case 6:
			Src = DX >> 8;
			if (log_to_console) cout << "TEST DH(" << (int)Src << ") AND ";
			break;
		case 7:
			Src = BX >> 8;
			if (log_to_console) cout << "TEST BH(" << (int)Src << ") AND ";
			break;
		}
		break;
	}

	//определяем регистр - получатель
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Result = Src & (AX & 255);
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result];
		if (log_to_console) cout << "AL(" << (int)(AX & 255) << ") = " << (int)Result;
		break;
	case 1:
		Result = Src & (CX & 255);
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result];
		if (log_to_console) cout << "CL(" << (int)(CX & 255) << ") = " << (int)Result;
		break;
	case 2:
		Result = Src & (DX & 255);
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result];
		if (log_to_console) cout << "DL(" << (int)(DX & 255) << ") = " << (int)Result;
		break;
	case 3:
		Result = Src & (BX & 255);
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result];
		if (log_to_console) cout << "BL(" << (int)(BX & 255) << ") = " << (int)Result;
		break;
	case 4:
		Result = Src & (AX >> 8);
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result];
		if (log_to_console) cout << "AH(" << (int)(AX >> 8) << ") = " << (int)Result;
		break;
	case 5:
		Result = Src & (CX >> 8);
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result];
		if (log_to_console) cout << "CH(" << (int)(CX >> 8) << ") = " << (int)Result;
		break;
	case 6:
		Result = Src & (DX >> 8);
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result];
		if (log_to_console) cout << "DH(" << (int)(DX >> 8) << ") = " << (int)Result;
		break;
	case 7:
		Result = Src & (BX >> 8);
		Flag_CF = 0;
		Flag_SF = ((Result >> 7) & 1);
		Flag_OF = 0;
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result];
		if (log_to_console) cout << "BH(" << (int)(BX >> 8) << ") = " << (int)Result;
		break;
	}
	Instruction_Pointer += 2 + additional_IPs;
}
void TEST_16()       //TEST = AND Function to Flags
{
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint16 Result = 0;
	uint8 additional_IPs = 0;

	//определяем регистр - источник
	switch ((byte2 >> 3) & 7)
	{
	case 0:
		Src = AX;
		if (log_to_console) cout << "TEST AX(" << (int)Src << ") ";
		break;
	case 1:
		Src = CX;
		if (log_to_console) cout << "TEST CX(" << (int)Src << ") ";
		break;
	case 2:
		Src = DX;
		if (log_to_console) cout << "TEST DX(" << (int)Src << ") ";
		break;
	case 3:
		Src = BX;
		if (log_to_console) cout << "TEST BX(" << (int)Src << ") ";
		break;
	case 4:
		Src = Stack_Pointer;
		if (log_to_console) cout << "TEST SP(" << (int)Src << ") ";
		break;
	case 5:
		Src = Base_Pointer;
		if (log_to_console) cout << "TEST BP(" << (int)Src << ") ";
		break;
	case 6:
		Src = Source_Index;
		if (log_to_console) cout << "TEST SI(" << (int)Src << ") ";
		break;
	case 7:
		Src = Destination_Index;
		if (log_to_console) cout << "TEST DI(" << (int)Src << ") ";
		break;
	}

	//определяем объект назначения и результат операции
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		if (log_to_console) cout << "AND M[" << (int)New_Addr << "] = ";
		Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & Src;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 15) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)Result << "{" << (int)Flag_PF << "}";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		if (log_to_console) cout << "AND M[" << (int)New_Addr << "] = ";
		Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & Src;
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 15) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		if (log_to_console) cout << "AND M[" << (int)New_Addr << "] = ";
		Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & Src;
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
			Result = AX & Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << (int)Result;
			break;
		case 1:
			if (log_to_console) cout << "AND CX(" << (int)CX << ") = ";
			Result = CX & Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << (int)Result;
			break;
		case 2:
			if (log_to_console) cout << "AND DX(" << (int)DX << ") = ";
			Result = DX & Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << (int)Result;
			break;
		case 3:
			if (log_to_console) cout << "AND BX(" << (int)BX << ") = ";
			Result = BX & Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << (int)Result;
			break;
		case 4:
			if (log_to_console) cout << "AND SP(" << (int)Stack_Pointer << ") = ";
			Result = Stack_Pointer & Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << (int)Result;
			break;
		case 5:
			if (log_to_console) cout << "AND BP(" << (int)Base_Pointer << ") = ";
			Result = Base_Pointer & Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << (int)Result;
			break;
		case 6:
			if (log_to_console) cout << "AND SI(" << (int)Source_Index << ") = ";
			Result = Source_Index & Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << (int)Result;
			break;
		case 7:
			if (log_to_console) cout << "AND DI(" << (int)Destination_Index << ") = ";
			Result = Destination_Index & Src;
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
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
void TEST_IMM_ACC_8()		//TEST = AND Function to Flags
{
	uint8 Src = 0;
	uint8 Result = 0;

	//непосредственный операнд
	Src = memory.read(Instruction_Pointer + 1, CS);

	if (log_to_console) cout << "TEST IMM[" << (int)Src << "] AND AL(" << (int)(AX & 255) << ") = "; 

	//определяем объект назначения и результат операции
	Result = (AX & 255) & Src;
	//AX = (AX & 0xFF00) | Result;
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
	uint16 Src = 0;
	uint16 Result = 0;

	//непосредственный операнд
	Src = memory.read(Instruction_Pointer + 1, CS) + memory.read(Instruction_Pointer + 2, CS) * 256;

	if (log_to_console) cout << "TEST IMM[" << (int)Src << "] AND AX(" << (int)AX << ") = ";

	//определяем объект назначения и результат операции
	Result = AX & Src;
	//AX = Result;
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint8 Src = 0;
	uint8 Result = 0;
	uint8 additional_IPs = 0;

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
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		if (log_to_console) cout << "OR M[" << (int)New_Addr << "] = ";
		Result = memory.read(New_Addr, DS) | Src;
		memory.write(New_Addr, DS, Result);
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 7) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		if (log_to_console) cout << "OR M[" << (int)New_Addr << "] = ";
		Result = memory.read(New_Addr, DS) | Src;
		memory.write(New_Addr, DS, Result);
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 7) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		if (log_to_console) cout << "OR M[" << (int)New_Addr << "] = ";
		Result = memory.read(New_Addr, DS) | Src;
		memory.write(New_Addr, DS, Result);
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint16 Result = 0;
	uint8 additional_IPs = 0;

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
		
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		if (log_to_console) cout << "OR M[" << (int)New_Addr << "] = ";
		Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) | Src;
		memory.write(New_Addr, DS, Result & 255);
		memory.write(New_Addr + 1, DS, Result >> 8);
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 15) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 1:
		
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		if (log_to_console) cout << "OR M[" << (int)New_Addr << "] = ";
		Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) | Src;
		memory.write(New_Addr, DS, Result & 255);
		memory.write(New_Addr + 1, DS, Result >> 8);
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 15) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << (int)Result;
		break;
	case 2:
		
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		if (log_to_console) cout << "OR M[" << (int)New_Addr << "] = ";
		Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) | Src;
		memory.write(New_Addr, DS, Result & 255);
		memory.write(New_Addr + 1, DS, Result >> 8);
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint8 Src = 0;
	uint8 Result = 0;
	uint8 additional_IPs = 0;

	//определяем источник
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] OR ";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] OR ";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] OR ";
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint16 Result = 0;
	uint8 additional_IPs = 0;

	//определяем источник
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "[" << (int)New_Addr << "] ";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "[" << (int)New_Addr << "] ";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "[" << (int)New_Addr << "] ";
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
	Src = memory.read(Instruction_Pointer + 1, CS);

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
	Src = memory.read(Instruction_Pointer + 1, CS) + memory.read(Instruction_Pointer + 2, CS) * 256;

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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint8 Src = 0;
	uint8 Result = 0;
	uint8 additional_IPs = 0;

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
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Result = memory.read(New_Addr, DS) ^ Src;
		memory.write(New_Addr, DS, Result);
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 7) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "XOR M[" << (int)New_Addr << "]=" << (int)Result;
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Result = memory.read(New_Addr, DS) ^ Src;
		memory.write(New_Addr, DS, Result);
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 7) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "XOR M[" << (int)New_Addr << "]=" << (int)Result;
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Result = memory.read(New_Addr, DS) ^ Src;
		memory.write(New_Addr, DS, Result);
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 7) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << "XOR M[" << (int)New_Addr << "]=" << (int)Result;
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint16 Result = 0;
	uint8 additional_IPs = 0;

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
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) ^ Src;
		memory.write(New_Addr, DS, Result & 255);
		memory.write(New_Addr + 1, DS, Result >> 8);
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 15) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " M[" << (int)New_Addr << "]=" << (int)Result;
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) ^ Src;
		memory.write(New_Addr, DS, Result & 255);
		memory.write(New_Addr + 1, DS, Result >> 8);
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 15) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " M[" << (int)New_Addr << "]=" << (int)Result;
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) ^ Src;
		memory.write(New_Addr, DS, Result & 255);
		memory.write(New_Addr + 1, DS, Result >> 8);
		Flag_CF = 0;
		Flag_OF = 0;
		Flag_SF = ((Result >> 15) & 1);
		if (Result) Flag_ZF = false;
		else Flag_ZF = true;
		Flag_PF = parity_check[Result & 255];
		if (log_to_console) cout << " M[" << (int)New_Addr << "]=" << (int)Result;
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint8 Src = 0;
	uint8 Result = 0;
	uint8 additional_IPs = 0;

	//определяем источник
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] XOR ";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] XOR ";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS);
		if (log_to_console) cout << "M[" << (int)New_Addr << "] XOR ";
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint16 Result = 0;
	uint8 additional_IPs = 0;

	//определяем источник
	switch (byte2 >> 6)
	{
	case 0:
		if ((byte2 & 7) == 6) additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "XOR M[" << (int)New_Addr << "] ^ ";
		break;
	case 1:
		additional_IPs = 1;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "XOR M[" << (int)New_Addr << "] ^ ";
		break;
	case 2:
		additional_IPs = 2;
		New_Addr = mod_RM(byte2);
		Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		if (log_to_console) cout << "XOR M[" << (int)New_Addr << "] ^ ";
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint8 OP = (byte2 >> 3) & 7;
	uint16 New_Addr = 0;
	uint8 Src = 0;
	uint8 Result = 0;
	uint16 Result_16 = 0;
	uint8 additional_IPs = 0;
	bool OF_Carry = false;
	
	switch (OP)
	{
	case 0:   //ADD IMM -> R/M 8 bit

		//определяем объект назначения и результат операции
		switch (byte2 >> 6)
		{
		case 0:
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "ADD IMM[" << (int)Src << "] + ";
			Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = ((memory.read(New_Addr, DS) & 0x7F) + (Src & 0x7F)) >> 7;
			Result_16 = memory.read(New_Addr, DS) + Src;
			memory.write(New_Addr, DS, Result_16 & 255);
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)(Result & 255);
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "ADD IMM[" << (int)Src << "] + ";
			Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = ((memory.read(New_Addr, DS) & 0x7F) + (Src & 0x7F)) >> 7;
			Result_16 = memory.read(New_Addr, DS) + Src;
			memory.write(New_Addr, DS, Result_16 & 255);
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)(Result & 255);
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "ADD IMM[" << (int)Src << "] + ";
			Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = ((memory.read(New_Addr, DS) & 0x7F) + (Src & 0x7F)) >> 7;
			Result_16 = memory.read(New_Addr, DS) + Src;
			memory.write(New_Addr, DS, Result_16 & 255);
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)(Result & 255);
			break;
		case 3:
			// mod 11 источник - регистр
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "IMM(" << (int)Src << ") OR ";
			Result = memory.read(New_Addr, DS) | Src;
			memory.write(New_Addr, DS, Result);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result];
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "IMM(" << (int)Src << ") OR ";
			Result = memory.read(New_Addr, DS) | Src;
			memory.write(New_Addr, DS, Result);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result];
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "IMM(" << (int)Src << ") OR ";
			Result = memory.read(New_Addr, DS) | Src;
			memory.write(New_Addr, DS, Result);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result];
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "ADC IMM(" << (int)Src << ") + ";
			Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = ((memory.read(New_Addr, DS) & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
			Result_16 = memory.read(New_Addr, DS) + Src + Flag_CF;
			memory.write(New_Addr, DS, Result_16 & 255);
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)(Result & 255);
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "ADC IMM(" << (int)Src << ") + ";
			Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = ((memory.read(New_Addr, DS) & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
			Result_16 = memory.read(New_Addr, DS) + Src + Flag_CF;
			memory.write(New_Addr, DS, Result_16 & 255);
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)(Result & 255);
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "ADC IMM(" << (int)Src << ") + ";
			Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = ((memory.read(New_Addr, DS) & 0x7F) + (Src & 0x7F) + Flag_CF) >> 7;
			Result_16 = memory.read(New_Addr, DS) + Src + Flag_CF;
			memory.write(New_Addr, DS, Result_16 & 255);
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)(Result & 255);
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "SBB IMM(" << (int)Src << ") ";
			Flag_AF = (((memory.read(New_Addr, DS) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
			OF_Carry = ((memory.read(New_Addr, DS) & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
			Result_16 = memory.read(New_Addr, DS) - Src - Flag_CF;
			memory.write(New_Addr, DS, Result_16 & 255);
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			if (log_to_console) cout << "from M[" << (int)New_Addr << "] = " << (int)(Result & 255);
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "SBB IMM[" << (int)Src << "] ";
			Flag_AF = (((memory.read(New_Addr, DS) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
			OF_Carry = ((memory.read(New_Addr, DS) & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
			Result_16 = memory.read(New_Addr, DS) - Src - Flag_CF;
			memory.write(New_Addr, DS, Result_16 & 255);
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			if (log_to_console) cout << "from M[" << (int)New_Addr << "] = " << (int)(Result & 255);
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "SBB IMM[" << (int)Src << "] ";
			Flag_AF = (((memory.read(New_Addr, DS) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
			OF_Carry = ((memory.read(New_Addr, DS) & 0x7F) - (Src & 0x7F) - Flag_CF) >> 7;
			Result_16 = memory.read(New_Addr, DS) - Src - Flag_CF;
			memory.write(New_Addr, DS, Result_16 & 255);
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			if (log_to_console) cout << "from M[" << (int)New_Addr << "] = " << (int)(Result & 255);
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "IMM(" << (int)Src << ") AND ";
			Result = memory.read(New_Addr, DS) & Src;
			memory.write(New_Addr, DS, Result);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "IMM(" << (int)Src << ") AND ";
			Result = memory.read(New_Addr, DS) & Src;
			memory.write(New_Addr, DS, Result);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "IMM(" << (int)Src << ") AND ";
			Result = memory.read(New_Addr, DS) & Src;
			memory.write(New_Addr, DS, Result);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "SUB IMM(" << (int)Src << ") ";
			Flag_AF = (((memory.read(New_Addr, DS) & 15) - (Src & 15)) >> 4) & 1;
			OF_Carry = ((memory.read(New_Addr, DS) & 0x7F) - (Src & 0x7F)) >> 7;
			Result_16 = memory.read(New_Addr, DS) - Src;
			memory.write(New_Addr, DS, Result_16 & 255);
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			if (log_to_console) cout << "from M[" << (int)New_Addr << "] = " << (int)(Result & 255);
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "SUB IMM(" << (int)Src << ") ";
			Flag_AF = (((memory.read(New_Addr, DS) & 15) - (Src & 15)) >> 4) & 1;
			OF_Carry = ((memory.read(New_Addr, DS) & 0x7F) - (Src & 0x7F)) >> 7;
			Result_16 = memory.read(New_Addr, DS) - Src;
			memory.write(New_Addr, DS, Result_16 & 255);
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			if (log_to_console) cout << "from M[" << (int)New_Addr << "] = " << (int)(Result & 255);
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "SUB IMM(" << (int)Src << ") ";
			Flag_AF = (((memory.read(New_Addr, DS) & 15) - (Src & 15)) >> 4) & 1;
			OF_Carry = ((memory.read(New_Addr, DS) & 0x7F) - (Src & 0x7F)) >> 7;
			Result_16 = memory.read(New_Addr, DS) - Src;
			memory.write(New_Addr, DS, Result_16 & 255);
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			if (log_to_console) cout << "from M[" << (int)New_Addr << "] = " << (int)(Result & 255);
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "XOR IMM(" << (int)Src << ") ^ ";
			Result = memory.read(New_Addr, DS) ^ Src;
			memory.write(New_Addr, DS, Result);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "XOR IMM[" << (int)Src << "] ^ ";
			Result = memory.read(New_Addr, DS) ^ Src;
			memory.write(New_Addr, DS, Result);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "XOR IMM[" << (int)Src << "] ^ ";
			Result = memory.read(New_Addr, DS) ^ Src;
			memory.write(New_Addr, DS, Result);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 7) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << "M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд - вычитаемое
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "CMP IMM(" << (int)(Src) << ") ";
			Result_16 = memory.read(New_Addr, DS) - Src;
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = (Result_16 >> 7) & 1;
			OF_Carry = ((memory.read(New_Addr, DS) & 0x7F) - (Src & 0x7F)) >> 7;
			Flag_OF = Flag_CF ^ OF_Carry;
			Flag_PF = parity_check[Result_16 & 255];
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_AF = (((memory.read(New_Addr, DS) & 15) - (Src & 15)) >> 4) & 1;
			
			if (log_to_console) cout << "with M[" << (int)New_Addr << "]("<<(int)memory.read(New_Addr, DS) <<") ";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд - вычитаемое
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "CMP IMM(" << (int)(Src) << ") ";
			Result_16 = memory.read(New_Addr, DS) - Src;

			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = (Result_16 >> 7) & 1;
			OF_Carry = ((memory.read(New_Addr, DS) & 0x7F) - (Src & 0x7F)) >> 7;
			Flag_OF = Flag_CF ^ OF_Carry;
			Flag_PF = parity_check[Result_16 & 255];
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_AF = (((memory.read(New_Addr, DS) & 15) - (Src & 15)) >> 4) & 1;

			if (log_to_console) cout << "with M[" << (int)New_Addr << "](" << (int)memory.read(New_Addr, DS) << ") ";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд - вычитаемое
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
			if (log_to_console) cout << "CMP IMM(" << (int)(Src) << ") ";
			Result_16 = memory.read(New_Addr, DS) - Src;

			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = (Result_16 >> 7) & 1;
			OF_Carry = ((memory.read(New_Addr, DS) & 0x7F) - (Src & 0x7F)) >> 7;
			Flag_OF = Flag_CF ^ OF_Carry;
			Flag_PF = parity_check[Result_16 & 255];
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_AF = (((memory.read(New_Addr, DS) & 15) - (Src & 15)) >> 4) & 1;

			if (log_to_console) cout << "with M[" << (int)New_Addr << "](" << (int)memory.read(New_Addr, DS) << ") ";
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд - вычитаемое
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS);
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
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint8 OP = (byte2 >> 3) & 7;
	uint16 New_Addr = 0;
	uint16 Src = 0;
	uint16 Result = 0;
	uint32 Result_32 = 0;
	uint8 additional_IPs = 0;
	bool OF_Carry = false;

	switch (OP)
	{
	case 0:   //ADD IMM -> R/M 16 bit
		//определяем объект назначения и результат операции
		switch (byte2 >> 6)
		{
		case 0:
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "IMM(" << (int)Src << ") ADD ";
			Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) + (Src & 0x7FFF)) >> 15;
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) + Src;
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "IMM(" << (int)Src << ") ADD ";
			Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) + (Src & 0x7FFF)) >> 15;
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) + Src;
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "IMM(" << (int)Src << ") ADD ";
			Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15)) >> 4) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) + (Src & 0x7FFF)) >> 15;
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) + Src;
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "IMM(" << (int)Src << ") OR ";
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) | Src;
			memory.write(New_Addr, DS, Result & 255);
			memory.write(New_Addr + 1, DS, Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "IMM(" << (int)Src << ") OR ";
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) | Src;
			memory.write(New_Addr, DS, Result & 255);
			memory.write(New_Addr + 1, DS, Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "IMM(" << (int)Src << ") OR ";
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) | Src;
			memory.write(New_Addr, DS, Result & 255);
			memory.write(New_Addr + 1, DS, Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "ADC IMM(" << (int)Src << ") + ";
			Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) + Src + Flag_CF;
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "ADC IMM(" << (int)Src << ") + ";
			Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) + Src + Flag_CF;
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "ADC IMM(" << (int)Src << ") + ";
			Flag_AF = (((memory.read(New_Addr, DS) & 15) + (Src & 15) + Flag_CF) >> 4) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) + (Src & 0x7FFF) + Flag_CF) >> 15;
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) + Src + Flag_CF;
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "SBB IMM[" << (int)Src << "] ";
			Flag_AF = (((memory.read(New_Addr, DS) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) - Src - Flag_CF;
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << "from M[" << (int)New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "SBB IMM[" << (int)Src << "] ";
			Flag_AF = (((memory.read(New_Addr, DS) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) - Src - Flag_CF;
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << "from M[" << (int)New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "SBB IMM[" << (int)Src << "] ";
			Flag_AF = (((memory.read(New_Addr, DS) & 15) - (Src & 15) - Flag_CF) >> 4) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF) - Flag_CF) >> 15;
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) - Src - Flag_CF;
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << "from M[" << (int)New_Addr << "] = " << (int)(Result_32 & 0xFFFF) - Flag_CF;
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "IMM[" << (int)Src << "] AND ";
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & Src;
			memory.write(New_Addr, DS, Result & 255);
			memory.write(New_Addr + 1, DS, Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "IMM[" << (int)Src << "] AND ";
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & Src;
			memory.write(New_Addr, DS, Result & 255);
			memory.write(New_Addr + 1, DS, Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "IMM[" << (int)Src << "] AND ";
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & Src;
			memory.write(New_Addr, DS, Result & 255);
			memory.write(New_Addr + 1, DS, Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "SUB IMM[" << (int)Src << "] ";
			Flag_AF = (((memory.read(New_Addr, DS) & 15) - (Src & 15)) >> 4) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) - Src;
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << "from M[" << New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "SUB IMM[" << (int)Src << "] ";
			Flag_AF = (((memory.read(New_Addr, DS) & 15) - (Src & 15)) >> 4) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) - Src;
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << "from M[" << New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "SUB IMM[" << (int)Src << "] ";
			Flag_AF = (((memory.read(New_Addr, DS) & 15) - (Src & 15)) >> 4) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) - Src;
			memory.write(New_Addr, DS, Result_32 & 255);
			memory.write(New_Addr + 1, DS, (Result_32 >> 8) & 255);
			Flag_CF = ((Result_32 >> 16) & 1);
			Flag_SF = ((Result_32 >> 15) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			if (log_to_console) cout << "from M[" << New_Addr << "] = " << (int)(Result_32 & 0xFFFF);
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "XOR IMM(" << (int)Src << ") ";
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) ^ Src;
			memory.write(New_Addr, DS, Result & 255);
			memory.write(New_Addr + 1, DS, Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "XOR IMM(" << (int)Src << ") ";
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) ^ Src;
			memory.write(New_Addr, DS, Result & 255);
			memory.write(New_Addr + 1, DS, Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "XOR IMM(" << (int)Src << ") ";
			Result = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) ^ Src;
			memory.write(New_Addr, DS, Result & 255);
			memory.write(New_Addr + 1, DS, Result >> 8);
			Flag_CF = 0;
			Flag_OF = 0;
			Flag_SF = ((Result >> 15) & 1);
			if (Result) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			if (log_to_console) cout << " M[" << (int)New_Addr << "] = " << (int)Result;
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "CMP IMM(" << (int)Src << ") ";
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) - Src;
			Flag_CF = (Result_32 >> 16) & 1;
			Flag_SF = (Result_32 >> 15) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			Flag_AF = ((((memory.read(New_Addr, DS)) & 15) - (Src & 15)) >> 4) & 1;
			if (log_to_console) cout << "with M[" << (int)New_Addr << "] ";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "CMP IMM(" << (int)Src << ") ";
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) - Src;
			Flag_CF = (Result_32 >> 16) & 1;
			Flag_SF = (Result_32 >> 15) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			Flag_AF = ((((memory.read(New_Addr, DS)) & 15) - (Src & 15)) >> 4) & 1;
			if (log_to_console) cout << "with M[" << (int)New_Addr << "]";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
			if (log_to_console) cout << "CMP IMM(" << (int)Src << ") ";
			Result_32 = (memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) - Src;
			Flag_CF = (Result_32 >> 16) & 1;
			Flag_SF = (Result_32 >> 15) & 1;
			OF_Carry = (((memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256) & 0x7FFF) - (Src & 0x7FFF)) >> 15;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32 & 0xFFFF) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			Flag_AF = ((((memory.read(New_Addr, DS)) & 15) - (Src & 15)) >> 4) & 1;
			if (log_to_console) cout << "with M[" << (int)New_Addr << "]";
			break;
		case 3:
			// mod 11 источник - регистр
			//непосредственный операнд - вычитаемое
			Src = memory.read(Instruction_Pointer + 2 + additional_IPs, CS) + memory.read(Instruction_Pointer + 3 + additional_IPs, CS) * 256;
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
	Src = memory.read(Instruction_Pointer + 1, CS);

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
	Src = memory.read(Instruction_Pointer + 1, CS) + memory.read(Instruction_Pointer + 2, CS) * 256;

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
	uint8 OP = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 Src = 0;
	uint16 Dst = 0;
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
			memory.write(Destination_Index, ES, memory.read(Source_Index, DS));
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
			memory.write(Destination_Index, ES, memory.read(Source_Index, DS));
			memory.write(Destination_Index + 1, ES, memory.read(Source_Index + 1, DS));
			if (Flag_DF)
			{
				Destination_Index -= 2;
				Source_Index -= 2;
			}
			else
			{
				Destination_Index += 2;
				Source_Index += 2;
			}
			CX--;
		}
		break;

	case 0b10100110:  //CMPS 8

		if (log_to_console) cout << "CMPS_B (" << (int)CX << " bytes) from [" << (int)*DS << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
		do
		{
			if (!CX) break;
			Result_16 = memory.read(Source_Index, DS) - memory.read(Destination_Index, ES);
			OF_Carry = ((memory.read(Source_Index, DS) & 0x7F) - (memory.read(Destination_Index, ES) & 0x7F)) >> 7;
			//cout << "CMP " << (int)memory.read(Source_Index, DS) << " with " << (int)memory.read(Destination_Index, ES) << " ZF = ";
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			//cout << (int)Flag_ZF << endl;
			Flag_PF = parity_check[Result_16 & 255];
			Flag_AF = (((memory.read(Source_Index, DS) & 0x0F) - (memory.read(Destination_Index, ES) & 0x0F)) >> 4) & 1;
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
			Src = memory.read(Source_Index, DS) + memory.read(Source_Index + 1, DS) * 256;
			Dst = memory.read(Destination_Index, ES) + memory.read(Destination_Index + 1, ES) * 256;
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
				Destination_Index -= 2;
				Source_Index -= 2;
			}
			else
			{
				Destination_Index += 2;
				Source_Index += 2;
			}
			CX--;
		} while (CX && !Flag_ZF);
		break;

	case 0b10101110:  //SCAS 8
		if (log_to_console) cout << "SCAS_B (" << (int)CX << " bytes) from [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
		do
		{
			if (!CX) break;
			uint16 Result = (AX & 255) - memory.read(Destination_Index, ES);
			OF_Carry = ((AX & 0x7F) - (memory.read(Destination_Index, ES) & 0x7F)) >> 7;
			Flag_CF = (Result >> 8) & 1;
			Flag_SF = ((Result >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result & 255];
			Flag_AF = (((AX & 15) - (memory.read(Destination_Index, ES) & 15)) >> 4) & 1;
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
		} while (CX && !Flag_ZF);
		break;

	case 0b10101111:  //SCAS 16

		if (log_to_console) cout << "SCAS_W (" << (int)CX << " word) from [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
		do
		{
			if (!CX) break;
			Dst = memory.read(Destination_Index, ES) + memory.read(Destination_Index + 1, ES) * 256;
			Result_32 = AX - Dst;
			OF_Carry = ((AX & 0x7FFF) - (Dst & 0x7FFF)) >> 15;
			Flag_CF = (Result_32 >> 16) & 1;
			Flag_SF = (Result_32 >> 15) & 1;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_32) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_32 & 255];
			Flag_AF = (((AX & 15) - (memory.read(Destination_Index, ES) & 15)) >> 4) & 1;
			if (Flag_DF)
			{
				Destination_Index -= 2;
				//Source_Index -= 2;
			}
			else
			{
				Destination_Index += 2;
				//Source_Index += 2;
			}
			CX--;
		} while (CX && !Flag_ZF);
		break;

	case 0b10101100:  //LODS 8
		if (log_to_console) cout << "LODS_B (" << (int)CX << " bytes) from [" << (int)*DS << "]:[" << (int)Source_Index << "]";
		while (CX)
		{
			AX = (AX & 0xFF00) | memory.read(Source_Index, DS);
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
			AX = memory.read(Source_Index, DS) + memory.read(Source_Index + 1, DS) * 256;
			if (Flag_DF)
			{
				Source_Index -= 2;
			}
			else
			{
				Source_Index += 2;
			}
			CX--;
		}
		break;

	case 0b10101010:  //STOS 8

		if (log_to_console) cout << "STOS_B (" << (int)CX << " bytes) to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
		while (CX)
		{
			memory.write(Destination_Index, ES, AX & 255);
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
			memory.write(Destination_Index, ES, AX & 255);
			memory.write(Destination_Index + 1, ES, AX >> 8);
			if (Flag_DF)
			{
				Destination_Index -= 2;
			}
			else
			{
				Destination_Index += 2;
			}
			CX--;
		}
		break;

	default:
		Instruction_Pointer ++;
		if (test_mode) repeat_test_op = 1;
		//проверяем следующую команду F6 или F7 + mod 111 r/m
		if (memory.read(Instruction_Pointer, CS) == 0xF6 || memory.read(Instruction_Pointer, CS) == 0xF7)
		{
			if ((memory.read(Instruction_Pointer + 1, CS) & 0b00111000) == 0b00111000) 
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
	uint8 OP = memory.read(Instruction_Pointer + 1, CS); //mod / reg / rm
	uint16 Src = 0;
	uint16 Dst = 0;
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
			memory.write(Destination_Index, ES, memory.read(Source_Index, DS));
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
			memory.write(Destination_Index, ES, memory.read(Source_Index, DS));
			memory.write(Destination_Index + 1, ES, memory.read(Source_Index + 1, DS));
			if (Flag_DF)
			{
				Destination_Index -= 2;
				Source_Index -= 2;
			}
			else
			{
				Destination_Index += 2;
				Source_Index += 2;
			}
			CX--;
		}
		break;

	case 0b10100110:  //CMPS 8
		
		if (log_to_console) cout << "CMPS_B (" << (int)CX << " bytes) from [" << (int)*DS << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
		do
		{
			if (!CX) break;
			Result_16 = memory.read(Source_Index, DS) - memory.read(Destination_Index, ES);
			OF_Carry = ((memory.read(Source_Index, DS) & 0x7F) - (memory.read(Destination_Index, ES) & 0x7F)) >> 7;
			//cout << "CMP " << (int)memory.read(Source_Index, DS) << " with " << (int)memory.read(Destination_Index, ES) << " ZF = ";
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = ((Result_16 >> 7) & 1);
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			//cout << (int)Flag_ZF << endl;
			Flag_PF = parity_check[Result_16 & 255];
			Flag_AF = (((memory.read(Source_Index, DS) & 0x0F) - (memory.read(Destination_Index, ES) & 0x0F)) >> 4) & 1;
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
			Src = memory.read(Source_Index, DS) + memory.read(Source_Index + 1, DS) * 256;
			Dst = memory.read(Destination_Index, ES) + memory.read(Destination_Index + 1, ES) * 256;
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
				Destination_Index -= 2;
				Source_Index -= 2;
			}
			else
			{
				Destination_Index += 2;
				Source_Index += 2;
			}
			CX--;
		} while (CX && Flag_ZF);
		break;

	case 0b10101110:  //SCAS 8
		if (log_to_console) cout << "SCAS_B (" << (int)CX << " bytes) from [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
		do
		{
			if (!CX) break;
			Result_16 = (AX & 255) - memory.read(Destination_Index, ES);
			OF_Carry = (((AX & 0x7F) - (memory.read(Destination_Index, ES) & 0x7F))) >> 7;
			Flag_CF = (Result_16 >> 8) & 1;
			Flag_SF = (Result_16 >> 7) & 1;
			Flag_OF = Flag_CF ^ OF_Carry;
			if (Result_16 & 255) Flag_ZF = false;
			else Flag_ZF = true;
			Flag_PF = parity_check[Result_16 & 255];
			Flag_AF = (((AX & 15) - (memory.read(Destination_Index, ES) & 15)) >> 4) & 1;
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
			Dst = memory.read(Destination_Index, ES) + memory.read(Destination_Index + 1, ES) * 256;
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
				Destination_Index -= 2;
				//Source_Index -= 2;
			}
			else
			{
				Destination_Index += 2;
				//Source_Index += 2;
			}
			CX--;
		} while (CX && Flag_ZF);
		break;

	case 0b10101100:  //LODS 8
		if (log_to_console) cout << "LODS_B (" << (int)CX << " bytes) from [" << (int)*DS << "]:[" << (int)Source_Index << "]";
		while (CX)
		{
			AX = (AX & 0xFF00) | memory.read(Source_Index, DS);
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
			AX = memory.read(Source_Index, DS) + memory.read(Source_Index + 1, DS) * 256;
			if (Flag_DF)
			{
				Source_Index -= 2;
			}
			else
			{
				Source_Index += 2;
			}
			CX--;
		}
		break;
	
	case 0b10101010:  //STOS 8

		if (log_to_console) cout << "STOS_B (" << (int)CX << " bytes) to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
		while (CX)
		{
			memory.write(Destination_Index, ES, AX & 255);
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
			memory.write(Destination_Index, ES, AX & 255);
			memory.write(Destination_Index + 1, ES, AX >> 8);
			if (Flag_DF)
			{
				Destination_Index -= 2;
			}
			else
			{
				Destination_Index += 2;
			}
			CX--;
		}
		break;
	
	default:
		Instruction_Pointer++;
		if (test_mode) repeat_test_op = 1;
		//проверяем следующую команду F6 или F7 + mod 111 r/m
		if (memory.read(Instruction_Pointer, CS) == 0xF6 || memory.read(Instruction_Pointer, CS) == 0xF7)
		{
			if ((memory.read(Instruction_Pointer + 1, CS) & 0b00111000) == 0b00111000)
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
	if (log_to_console) cout << "MOVES_B("<< (int)memory.read(Source_Index, DS) << ") from [" << (int)*DS << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
	memory.write(Destination_Index, ES, memory.read(Source_Index, DS));
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
	if (log_to_console) cout << "MOVES_W ("<< (int)memory.read(Source_Index, DS) << (int)memory.read(Source_Index + 1, DS) << ") from [" << (int)*DS << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
	memory.write(Destination_Index, ES, memory.read(Source_Index, DS));
	memory.write(Destination_Index + 1, ES, memory.read(Source_Index + 1, DS));
	if (Flag_DF)
	{
		Destination_Index -= 2;
		Source_Index-=2;
	}
	else
	{
		Destination_Index+=2;
		Source_Index+=2;
	}
	Instruction_Pointer ++;
}
void CMPS_8()     //CMPS = Compare String
{
	if (log_to_console) cout << "CMPS_8 SOLO (" << memory.read(Source_Index, DS) << ") from [" << (int)*DS << "]:[" << Source_Index << "] to [" << (int)*ES << "]:[" << Destination_Index << "]";
	uint8 Src = memory.read(Source_Index, DS);
	uint8 Dst = memory.read(Destination_Index, ES);
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
	if (log_to_console) cout << "CMPS_16 SOLO (" << (int)(memory.read(Source_Index, DS) + memory.read(Source_Index, DS)*256) << ") from [" << (int)*DS << "]:[" << (int)Source_Index << "] to [" << (int)*ES << "]:[" << (int)Destination_Index << "]";
	uint16 Src = memory.read(Source_Index, DS) + memory.read(Source_Index + 1, DS) * 256;
	uint16 Dst = memory.read(Destination_Index, ES) + memory.read(Destination_Index + 1, ES) * 256;
	uint32 Result_32 = Src - Dst;
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
		Destination_Index-=2;
		Source_Index-=2;
	}
	else
	{
		Destination_Index+=2;
		Source_Index+=2;
	}
	Instruction_Pointer++;
}
void SCAS_8()     //SCAS = Scan String
{
	if (log_to_console) cout << "SCAS 8 SOLO AL with string at [" << (int)*ES << "]:[" << Destination_Index << "]";
	uint16 Result = (AX & 255) - memory.read(Destination_Index, ES);
	bool OF_Carry = ((AX & 0x7F) - (memory.read(Destination_Index, ES) & 0x7F)) >> 7;
	Flag_CF = (Result >> 8) & 1;
	Flag_SF = ((Result >> 7) & 1);
	Flag_OF = Flag_CF ^ OF_Carry;
	if (Result & 255) Flag_ZF = false;
	else Flag_ZF = true;
	Flag_PF = parity_check[Result & 255];
	Flag_AF = (((AX & 0x0F) - (memory.read(Destination_Index, ES) & 0x0F)) >> 4) & 1;
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
	uint16 Dst = memory.read(Destination_Index, ES) + memory.read(Destination_Index + 1, ES) * 256;
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
		Destination_Index -= 2;
		//Source_Index -= 2;
	}
	else
	{
		Destination_Index += 2;
		//Source_Index += 2;
	}
	Instruction_Pointer++;
}
void LODS_8()     //LODS = Load String
{
	if (log_to_console) cout << "move string from [" << (int)*DS << "]:[" << Source_Index << "] to AL";
	AX = (AX & 0xFF00)|memory.read(Source_Index, DS);
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
	AX = memory.read(Source_Index, DS) + memory.read(Source_Index + 1, DS) * 256;
	if (Flag_DF)
	{
		Source_Index-=2;
	}
	else
	{
		Source_Index+=2;
	}
	Instruction_Pointer++;
}
void STOS_8()     //STOS = Store String
{
	if (log_to_console && memory.read(Instruction_Pointer, CS) != 0xF2 && memory.read(Instruction_Pointer, CS) != 0xF3) cout << "move string from AL to [" << (int)*ES << "]:[" << Destination_Index << "]";
	memory.write(Destination_Index, ES, AX & 255);
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
	if (log_to_console && memory.read(Instruction_Pointer, CS) != 0xF2 && memory.read(Instruction_Pointer, CS) != 0xF3) cout << "move string_w from AX to [" << (int)*ES << "]:[" << Destination_Index << "]";
	memory.write(Destination_Index, ES, AX & 255);
	memory.write(Destination_Index + 1, ES, AX >> 8);
	if (Flag_DF)
	{
		Destination_Index -= 2;
	}
	else
	{
		Destination_Index += 2;
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
	uint32 stack_addr = SS_data * 16 + Stack_Pointer - 1;
	memory.write_2(stack_addr, (Instruction_Pointer + 3) >> 8);
	memory.write_2(stack_addr - 1, (Instruction_Pointer + 3) & 255);
	Stack_Pointer -= 2;
	Instruction_Pointer += DispCalc16(memory.read(Instruction_Pointer + 1, CS) + memory.read(Instruction_Pointer + 2, CS) * 256) + 3;
}
// INC/DEC/CALL/JUMP/PUSH
void Call_Jump_Push()				//Indirect (4 operations)
{
	uint8 byte2 = memory.read(Instruction_Pointer + 1, CS);//второй байт
	uint8 OP = (byte2 >> 3) & 7;
	uint16 disp = 0;		//смещение
	uint16 Src = 0;			//источник данных
	uint16 New_Addr = 0;	//адрес перехода
	uint8 additional_IPs = 0;
	uint8 tmp;
	uint16 old_IP;
	uint16 a;
	uint32 stack_addr = 0;

	switch (OP)
	{
	case 0:  //INC R/M 16bit
	
		//находим цель инкремента
		switch (byte2 >> 6)
		{
		case 0:
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			Flag_AF = (((Src & 0x0F) + 1) >> 4) & 1;
			Flag_OF = (((Src & 0x7FFF) + 1) >> 15) & 1;
			Src++;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, Src >> 8);
			if (Src) Flag_ZF = 0;
			else Flag_ZF = 1;
			Flag_SF = (Src >> 15) & 1;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "INC M[" << (int)New_Addr << "] = " << (int)Src;
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			Flag_AF = (((Src & 0x0F) + 1) >> 4) & 1;
			Flag_OF = (((Src & 0x7FFF) + 1) >> 15) & 1;
			Src++;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, Src >> 8);
			if (Src) Flag_ZF = 0;
			else Flag_ZF = 1;
			Flag_SF = (Src >> 15) & 1;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "INC M[" << (int)New_Addr << "] = " << (int)Src;
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			Flag_AF = (((Src & 0x0F) + 1) >> 4) & 1;
			Flag_OF = (((Src & 0x7FFF) + 1) >> 15) & 1;
			Src++;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, Src >> 8);
			if (Src) Flag_ZF = 0;
			else Flag_ZF = 1;
			Flag_SF = (Src >> 15) & 1;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "INC M[" << (int)New_Addr << "] = " << (int)Src;
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			Flag_AF = (((Src & 0x0F) - 1) >> 4) & 1;
			Flag_OF = (((Src & 0x7FFF) - 1) >> 15) & 1;
			Src--;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, Src >> 8);
			if (Src) Flag_ZF = 0;
			else Flag_ZF = 1;
			Flag_SF = (Src >> 15) & 1;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "DEC M[" << (int)New_Addr << "] = " << (int)Src;
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			Flag_AF = (((Src & 0x0F) - 1) >> 4) & 1;
			Flag_OF = (((Src & 0x7FFF) - 1) >> 15) & 1;
			Src--;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, Src >> 8);
			if (Src) Flag_ZF = 0;
			else Flag_ZF = 1;
			Flag_SF = (Src >> 15) & 1;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "DEC M[" << (int)New_Addr << "] = " << (int)Src;
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			Flag_AF = (((Src & 0x0F) - 1) >> 4) & 1;
			Flag_OF = (((Src & 0x7FFF) - 1) >> 15) & 1;
			Src--;
			memory.write(New_Addr, DS, Src & 255);
			memory.write(New_Addr + 1, DS, Src >> 8);
			if (Src) Flag_ZF = 0;
			else Flag_ZF = 1;
			Flag_SF = (Src >> 15) & 1;
			Flag_PF = parity_check[Src & 255];
			if (log_to_console) cout << "DEC M[" << (int)New_Addr << "] = " << (int)Src;
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			a = mod_RM(byte2);
			New_Addr = memory.read(a, DS) + memory.read(a + 1, DS) * 256;
			break;
		case 1:
			additional_IPs = 1;
			a = mod_RM(byte2);
			New_Addr = memory.read(a, DS) + memory.read(a + 1, DS) * 256;
			break;
		case 2:
			additional_IPs = 2;
			a = mod_RM(byte2);
			New_Addr = memory.read(a, DS) + memory.read(a + 1, DS) * 256;
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
		old_IP = Instruction_Pointer;
		stack_addr = SS_data * 16 + Stack_Pointer - 1;
		memory.write_2(stack_addr, (Instruction_Pointer + 2 + additional_IPs) >> 8);
		memory.write_2(stack_addr - 1, (Instruction_Pointer + 2 + additional_IPs) & 255);
		Stack_Pointer -= 2;
		
		Instruction_Pointer = New_Addr;
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
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
		stack_addr = SS_data * 16 + Stack_Pointer - 1;
		memory.write_2(stack_addr, (*CS) >> 8);
		memory.write_2(stack_addr - 1, (*CS) & 255);
		memory.write_2(stack_addr - 2, (Instruction_Pointer + 2 + additional_IPs)  >> 8);
		memory.write_2(stack_addr - 3, (Instruction_Pointer + 2 + additional_IPs) & 255);
		Stack_Pointer -= 4;
		
		//пробую менять местами
		Instruction_Pointer = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		*CS = memory.read(New_Addr + 2, DS) + memory.read(New_Addr + 3, DS) * 256;
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Instruction_Pointer = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Instruction_Pointer = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Instruction_Pointer = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
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

		Instruction_Pointer = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
		*CS = memory.read(New_Addr + 2, DS) + memory.read(New_Addr + 3, DS) * 256;
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
			if ((byte2 & 7) == 6) additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			if (log_to_console) cout << "PUSH M[" << (int)*DS << ":" << (int)New_Addr << "]";
			break;
		case 1:
			additional_IPs = 1;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			if (log_to_console) cout << "PUSH M[" << (int)*DS << ":" << (int)New_Addr << "]";
			break;
		case 2:
			additional_IPs = 2;
			New_Addr = mod_RM(byte2);
			Src = memory.read(New_Addr, DS) + memory.read(New_Addr + 1, DS) * 256;
			if (log_to_console) cout << "PUSH M[" << (int)*DS << ":" << (int)New_Addr << "]";
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
		stack_addr = SS_data * 16 + Stack_Pointer - 1;
		memory.write_2(stack_addr, Src >> 8);
		memory.write_2(stack_addr - 1, Src & 255);
		Stack_Pointer -= 2;
		Instruction_Pointer += 2 + additional_IPs;
		break;

	}
}
void Call_dir_interseg()		//Direct Intersegment Call
{
	uint32 stack_addr = SS_data * 16 + Stack_Pointer - 1;
	memory.write_2(stack_addr, *CS >> 8);
	memory.write_2(stack_addr - 1, (*CS) & 255);
	memory.write_2(stack_addr - 2, (Instruction_Pointer + 5) >> 8);
	memory.write_2(stack_addr - 3, (Instruction_Pointer + 5) & 255);
	Stack_Pointer -= 4;
	uint16 new_IP = memory.read(Instruction_Pointer + 1, CS) + memory.read(Instruction_Pointer + 2, CS) * 256;
	*CS = memory.read(Instruction_Pointer + 3, CS) + memory.read(Instruction_Pointer + 4, CS) * 256;
	Instruction_Pointer = new_IP;
	SetConsoleTextAttribute(hConsole, 13);
	if (log_to_console) cout << "Direct Intersegment Call to " << (int)*CS << ":" << (int)Instruction_Pointer;
	//else cout << "Direct Intersegment Call to " << (int)*CS << ":" << (int)Instruction_Pointer << endl;
	SetConsoleTextAttribute(hConsole, 7);
}
void Jump_Near_8()				//Direct jump within Segment-Short
{
	Instruction_Pointer += DispCalc8(memory.read(Instruction_Pointer + 1, CS)) + 2;
	SetConsoleTextAttribute(hConsole, 15);
	if (log_to_console) cout << "Direct jump(8) within Segment-Short to " << (int)*CS << ":" << (int)Instruction_Pointer;
	SetConsoleTextAttribute(hConsole, 7);
}
void Jump_Near_16()				//Direct jump within Segment-Short
{
	Instruction_Pointer += DispCalc16(memory.read(Instruction_Pointer + 1, CS) + memory.read(Instruction_Pointer + 2, CS) * 256) + 3;
	SetConsoleTextAttribute(hConsole, 15);
	if (log_to_console) cout << "Direct jump(16) within Segment-Short to " << (int)*CS << ":" << (int)Instruction_Pointer;
	SetConsoleTextAttribute(hConsole, 7);
}
void Jump_Far()					//Direct Intersegment Jump
{
	uint16 new_IP = memory.read(Instruction_Pointer + 1, CS) + memory.read(Instruction_Pointer + 2, CS) * 256;
	*CS = memory.read(Instruction_Pointer + 3, CS) + memory.read(Instruction_Pointer + 4, CS) * 256;
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
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory.read(Instruction_Pointer + 1, CS)) + 2;
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
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory.read(Instruction_Pointer + 1, CS)) + 2;
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
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory.read(Instruction_Pointer + 1, CS)) + 2;
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
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory.read(Instruction_Pointer + 1, CS)) + 2;
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
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory.read(Instruction_Pointer + 1, CS)) + 2;
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
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory.read(Instruction_Pointer + 1, CS)) + 2;
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
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory.read(Instruction_Pointer + 1, CS)) + 2;
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
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory.read(Instruction_Pointer + 1, CS)) + 2;
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
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory.read(Instruction_Pointer + 1, CS)) + 2;
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
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory.read(Instruction_Pointer + 1, CS)) + 2;
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
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory.read(Instruction_Pointer + 1, CS)) + 2;
		if (log_to_console) cout << "Jump on Not Less, or Equal/Greater (JNLE/JG) to " << (int)*CS << ":" << (int)Instruction_Pointer;
	}
}
void JNB_JAE()			// JNB/JAE = Jump on Not Below/Above, or Equal
{
	if (!Flag_CF)
	{
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory.read(Instruction_Pointer + 1, CS)) + 2;
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
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory.read(Instruction_Pointer + 1, CS)) + 2;
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
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory.read(Instruction_Pointer + 1, CS)) + 2;
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
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory.read(Instruction_Pointer + 1, CS)) + 2;
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
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory.read(Instruction_Pointer + 1, CS)) + 2;
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
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory.read(Instruction_Pointer + 1, CS)) + 2;
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
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory.read(Instruction_Pointer + 1, CS)) + 2;
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
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory.read(Instruction_Pointer + 1, CS)) + 2;
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
		Instruction_Pointer = Instruction_Pointer + DispCalc8(memory.read(Instruction_Pointer + 1, CS)) + 2;
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
	uint16 pop_bytes = memory.read(Instruction_Pointer + 1, CS) + memory.read(Instruction_Pointer + 2, CS) * 256;
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
	*CS = memory.read(Stack_Pointer, SS) + memory.read(Stack_Pointer + 1, SS) * 256;
	Stack_Pointer += 2;
	SetConsoleTextAttribute(hConsole, 13);
	if (log_to_console) cout << "Far RET to " << (int)*CS << ":" << (int)Instruction_Pointer;
	//else cout << "Far RET to " << (int)*CS << ":" << (int)Instruction_Pointer << endl;
	SetConsoleTextAttribute(hConsole, 7);
}
void RET_Inter_Segment_IMM_SP()		//Return Intersegment Adding Immediate to SP
{
	uint16 pop_bytes = memory.read(Instruction_Pointer + 1, CS) + memory.read(Instruction_Pointer + 2, CS) * 256;
	uint32 stack_addr = SS_data * 16 + Stack_Pointer;
	Instruction_Pointer = memory.read_2(stack_addr) + memory.read_2(stack_addr + 1) * 256;
	Stack_Pointer += 2;
	*CS = memory.read(Stack_Pointer, SS) + memory.read(Stack_Pointer + 1, SS) * 256;
	Stack_Pointer += 2 + pop_bytes;
	SetConsoleTextAttribute(hConsole, 13);
	if (log_to_console) cout << "Far RET to " << (int)*CS << ":" << (int)Instruction_Pointer << " + " << pop_bytes << " bytes popped";
	SetConsoleTextAttribute(hConsole, 7);
}

//========Conditional Transfer-===============

void INT_N()			//INT = Interrupt
{
	//перехват системных прерываний
	//syscall(memory.read(Instruction_Pointer + 1, CS));
	//Instruction_Pointer += 2;
	//return;

	uint8 int_type = memory.read(Instruction_Pointer + 1, CS);

	if (!Flag_IF && !test_mode)
	{
		if (log_to_console) cout << "INT("<<(int)int_type<<") disabled!";
		Instruction_Pointer += 2;
		return;
	}
	
	//определяем новый IP и CS
	uint16 new_IP = memory.read(int_type * 4, 0) + memory.read(int_type * 4 + 1, 0) * 256;
	uint16 new_CS = memory.read(int_type * 4 + 2, 0) + memory.read(int_type * 4 + 3, 0) * 256;
	
	//помещаем в стек флаги
	memory.write(Stack_Pointer - 1, SS, 0xF0 | (Flag_OF * 8) | (Flag_DF * 4) | (Flag_IF * 2) | Flag_TF);
	memory.write(Stack_Pointer - 2, SS, 0x2 | (Flag_SF * 128) | (Flag_ZF * 64) | (Flag_AF * 16) | (Flag_PF * 4) | (Flag_CF));
	Stack_Pointer -= 2;

	//помещаем в стек сегмент
	memory.write(Stack_Pointer - 1, SS, *CS >> 8);
	memory.write(Stack_Pointer - 2, SS, (*CS) & 255);
	Stack_Pointer -= 2;
	//помещаем в стек IP

	memory.write(Stack_Pointer - 1, SS, (Instruction_Pointer + 2) >> 8);
	memory.write(Stack_Pointer - 2, SS, (Instruction_Pointer + 2) & 255);
	Stack_Pointer -= 2;
	//передаем управление
	Flag_IF = false;//запрет внешних прерываний
	Flag_TF = false;
	*CS = new_CS;
	uint16 old = Instruction_Pointer;
	Instruction_Pointer = new_IP;
	SetConsoleTextAttribute(hConsole, 10);
	//if (int_type != 0x10) cout << "INT " << (int)int_type << " (AX=" << (int)AX << ") -> " << (int)new_CS << ":" << (int)Instruction_Pointer << " call from " << old << endl;
	if (log_to_console) cout << "INT " << (int)int_type << " (AX=" << (int)AX << ") -> " << (int)new_CS << ":" << (int)Instruction_Pointer << " call from " << old;
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
	uint16 new_IP = memory.read(int_type * 4, 0) + memory.read(int_type * 4 + 1, 0) * 256;
	uint16 new_CS = memory.read(int_type * 4 + 2, 0) + memory.read(int_type * 4 + 3, 0) * 256;

	//помещаем в стек флаги
	memory.write(Stack_Pointer - 1, SS, 0xF0 | (Flag_OF * 8) | (Flag_DF * 4) | (Flag_IF * 2) | Flag_TF);
	memory.write(Stack_Pointer - 2, SS, 0x2 | (Flag_SF * 128) | (Flag_ZF * 64) | (Flag_AF * 16) | (Flag_PF * 4) | (Flag_CF));
	Stack_Pointer -= 2;
	//помещаем в стек сегмент
	memory.write(Stack_Pointer - 1, SS, *CS >> 8);
	memory.write(Stack_Pointer - 2, SS, (*CS) & 255);
	Stack_Pointer -= 2;
	//помещаем в стек IP
	memory.write(Stack_Pointer - 1, SS, (Instruction_Pointer + 1) >> 8);
	memory.write(Stack_Pointer - 2, SS, (Instruction_Pointer + 1) & 255);
	Stack_Pointer -= 2;
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
		uint16 new_IP = memory.read(0x10, 0) + memory.read(0x11, 0) * 256;
		uint16 new_CS = memory.read(0x12, 0) + memory.read(0x13, 0) * 256;

		//помещаем в стек флаги
		memory.write(Stack_Pointer - 1, SS, 0xF0 | (Flag_OF * 8) | (Flag_DF * 4) | (Flag_IF * 2) | Flag_TF);
		memory.write(Stack_Pointer - 2, SS, 0x2 | (Flag_SF * 128) | (Flag_ZF * 64) | (Flag_AF * 16) | (Flag_PF * 4) | (Flag_CF));
		Stack_Pointer -= 2;
		//помещаем в стек сегмент
		memory.write(Stack_Pointer - 1, SS, (*CS) >> 8);
		memory.write(Stack_Pointer - 2, SS, (*CS) & 255);
		Stack_Pointer -= 2;
		//помещаем в стек IP
		memory.write(Stack_Pointer - 1, SS, (Instruction_Pointer + 1) >> 8);
		memory.write(Stack_Pointer - 2, SS, (Instruction_Pointer + 1) & 255);
		Stack_Pointer -= 2;
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
	uint32 stack_addr = SS_data * 16 + Stack_Pointer;
	Instruction_Pointer = memory.read_2(stack_addr) + memory.read_2(stack_addr + 1) * 256;
	Stack_Pointer += 2;
	stack_addr += 2;
	//POP CS
	*CS = memory.read_2(stack_addr) + memory.read_2(stack_addr + 1) * 256;
	Stack_Pointer += 2;
	stack_addr += 2;
	//POP Flags
	int Flags = memory.read_2(stack_addr) + memory.read_2(stack_addr + 1) * 256;
	Stack_Pointer += 2;	
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
	if (log_to_console) cout << "I_RET to " << *CS << ":" << Instruction_Pointer;
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
	uint8 f_opcode = ((memory.read(Instruction_Pointer, CS) & 7) << 3) | ((memory.read(Instruction_Pointer + 1, CS) >> 3) & 7);
	uint8 rm_code = ((memory.read(Instruction_Pointer + 1, CS) >> 3) & 0b11000) | (memory.read(Instruction_Pointer + 1, CS) & 7);
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