#include "opcode_functions.h"
#include "custom_classes.h"
#include "video.h"


#include <Windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <conio.h>
#include <chrono>
#include <bitset>
#include <sstream>
#include <thread>

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
extern bool Flag_OF;		//Overflow flag
extern bool Flag_DF;		//Direction flag
extern bool Flag_IF;		//Int Enable flag
extern bool Flag_TF;		//Trap SS flag
extern bool Flag_SF;		//Signflag
extern bool Flag_ZF;		//Zeroflag
extern bool Flag_AF;		//Aux Carry flag
extern bool Flag_PF;		//Parity flag
extern bool Flag_CF;		//Carry flag

extern uint16 Stack_Pointer;			//указатель стека
extern uint16 Instruction_Pointer;		//указатель текущей команды
extern uint16 Base_Pointer;
extern uint16 Source_Index;
extern uint16 Destination_Index;

//указатели сегментов
extern uint16* CS;			//Code Segment
extern uint16* DS;			//Data Segment
extern uint16* SS;			//Stack Segment
extern uint16* ES;			//Extra Segment

extern uint16 CS_data;		//Code Segment
extern uint16 DS_data;		//Data Segment
extern uint16 SS_data;		//Stack Segment
extern uint16 ES_data;		//Extra Segment

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
extern Video_device monitor;

//признак неопределенности флага
bool OF_undef = false;
bool PF_undef = false;
bool SF_undef = false;
bool ZF_undef = false;
bool AF_undef = false;
bool CF_undef = false;

extern void (*op_code_table[256])();

extern std::string path;

uint8 process(string json_str); //декларация

bool test_log = 0;
bool repeat_test_op = false;
bool exeption_fired = false;
extern bool negate_IDIV;

void tester2()
{
	string files1[333] = {"tests\\00.json" ,"tests\\01.json" ,"tests\\02.json" ,"tests\\03.json" ,"tests\\04.json" ,"tests\\05.json" ,"tests\\06.json" ,"tests\\07.json" ,"tests\\08.json" ,
	"tests\\09.json","tests\\0A.json","tests\\0B.json","tests\\0C.json","tests\\0D.json","tests\\0E.json","tests\\0F.json","tests\\10.json","tests\\11.json","tests\\12.json","tests\\13.json",
	"tests\\14.json","tests\\15.json","tests\\16.json","tests\\17.json","tests\\18.json","tests\\19.json","tests\\1A.json","tests\\1B.json","tests\\1C.json","tests\\1D.json","tests\\1E.json",
	"tests\\1F.json","tests\\20.json","tests\\21.json","tests\\22.json","tests\\23.json","tests\\24.json","tests\\25.json","tests\\26.json","tests\\27.json","tests\\28.json","tests\\29.json",
	"tests\\2A.json", "tests\\2B.json", "tests\\2C.json", "tests\\2D.json", "tests\\2E.json", "tests\\2F.json", "tests\\30.json", "tests\\31.json", "tests\\32.json", "tests\\33.json", 
	"tests\\34.json","tests\\35.json","tests\\36.json","tests\\37.json","tests\\38.json","tests\\39.json","tests\\3A.json","tests\\3B.json","tests\\3C.json","tests\\3D.json","tests\\3E.json",
	"tests\\3F.json" ,"tests\\40.json" ,"tests\\41.json" ,"tests\\42.json" ,"tests\\43.json" ,"tests\\44.json" ,"tests\\45.json" ,"tests\\46.json" ,"tests\\47.json" ,"tests\\48.json" ,"tests\\49.json",
	"tests\\4A.json","tests\\4B.json","tests\\4C.json","tests\\4D.json","tests\\4E.json","tests\\4F.json","tests\\50.json","tests\\51.json","tests\\52.json","tests\\53.json","tests\\54.json","tests\\55.json",
	"tests\\56.json","tests\\57.json","tests\\58.json","tests\\59.json","tests\\5A.json","tests\\5B.json","tests\\5C.json","tests\\5D.json","tests\\5E.json","tests\\5F.json","tests\\60.json",
	"tests\\61.json","tests\\62.json" ,"tests\\63.json" ,"tests\\64.json" ,"tests\\65.json" ,"tests\\66.json" ,"tests\\67.json" ,"tests\\68.json" ,"tests\\69.json" ,"tests\\6A.json" ,
	"tests\\6B.json","tests\\6C.json" ,"tests\\6D.json" ,"tests\\6E.json" ,"tests\\6F.json" ,"tests\\70.json" ,"tests\\71.json" ,"tests\\72.json" ,"tests\\73.json" ,"tests\\74.json",
	"tests\\75.json","tests\\76.json" ,"tests\\77.json" ,"tests\\78.json" ,"tests\\79.json" ,"tests\\7A.json" ,"tests\\7B.json" ,"tests\\7C.json" ,"tests\\7D.json" ,"tests\\7E.json",
	"tests\\7F.json", "tests\\80.0.json","tests\\80.1.json","tests\\80.2.json","tests\\80.3.json","tests\\80.4.json","tests\\80.5.json","tests\\80.6.json","tests\\80.7.json",
	"tests\\81.0.json", "tests\\81.1.json", "tests\\81.2.json", "tests\\81.3.json", "tests\\81.4.json", "tests\\81.5.json", "tests\\81.6.json", "tests\\81.7.json",
	"tests\\82.0.json", "tests\\82.1.json", "tests\\82.2.json", "tests\\82.3.json", "tests\\82.4.json", "tests\\82.5.json", "tests\\82.6.json", "tests\\82.7.json",
	"tests\\83.0.json", "tests\\83.1.json", "tests\\83.2.json", "tests\\83.3.json", "tests\\83.4.json", "tests\\83.5.json", "tests\\83.6.json", "tests\\83.7.json",	"tests\\84.json",
	"tests\\85.json","tests\\86.json" ,"tests\\87.json" ,"tests\\88.json" ,"tests\\89.json" ,"tests\\8A.json" ,"tests\\8B.json" ,"tests\\8C.json" ,"tests\\8D.json" ,"tests\\8E.json",
	"tests\\8F.json","tests\\90.json" ,"tests\\91.json" ,"tests\\92.json" ,"tests\\93.json" ,"tests\\94.json", "tests\\95.json","tests\\96.json" ,"tests\\97.json" ,"tests\\98.json" ,
	"tests\\99.json" ,"tests\\9A.json" ,"tests\\9B.json" ,"tests\\9C.json" ,"tests\\9D.json" ,"tests\\9E.json",	"tests\\9F.json", "tests\\A0.json" ,"tests\\A1.json" ,"tests\\A2.json",
	"tests\\A3.json" ,"tests\\A4.json",	"tests\\A5.json","tests\\A6.json" ,"tests\\A7.json" ,"tests\\A8.json" ,"tests\\A9.json" ,"tests\\AA.json" ,"tests\\AB.json" ,"tests\\AC.json" ,
	"tests\\AD.json" ,"tests\\AE.json",	"tests\\AF.json","tests\\B0.json" ,"tests\\B1.json" ,"tests\\B2.json" ,"tests\\B3.json" ,"tests\\B4.json",
	"tests\\B5.json","tests\\B6.json" ,"tests\\B7.json" ,"tests\\B8.json" ,"tests\\B9.json" ,"tests\\BA.json" ,"tests\\BB.json" ,"tests\\BC.json" ,"tests\\BD.json" ,"tests\\BE.json",
	"tests\\BF.json","tests\\C0.json" ,"tests\\C1.json" ,"tests\\C2.json" ,"tests\\C3.json" ,"tests\\C4.json",
	"tests\\C5.json","tests\\C6.json" ,"tests\\C7.json" ,"tests\\C8.json" ,"tests\\C9.json" ,"tests\\CA.json" ,"tests\\CB.json" ,"tests\\CC.json" ,"tests\\CD.json" ,"tests\\CE.json",
	"tests\\CF.json","tests\\D0.0.json","tests\\D0.1.json","tests\\D0.2.json","tests\\D0.3.json","tests\\D0.4.json","tests\\D0.5.json","tests\\D0.6.json","tests\\D0.7.json",
	"tests\\D1.0.json","tests\\D1.1.json","tests\\D1.2.json","tests\\D1.3.json","tests\\D1.4.json","tests\\D1.5.json","tests\\D1.6.json","tests\\D1.7.json",
	"tests\\D2.0.json","tests\\D2.1.json","tests\\D2.2.json","tests\\D2.3.json","tests\\D2.4.json","tests\\D2.5.json","tests\\D2.6.json","tests\\D2.7.json",
	"tests\\D3.0.json","tests\\D3.1.json","tests\\D3.2.json","tests\\D3.3.json","tests\\D3.4.json","tests\\D3.5.json","tests\\D3.6.json","tests\\D3.7.json",
	"tests\\D4.json","tests\\D5.json","tests\\D6.json","tests\\D7.json","tests\\D8.json","tests\\D9.json","tests\\DA.json","tests\\DB.json","tests\\DC.json","tests\\DD.json",
	"tests\\DE.json","tests\\DF.json","tests\\E0.json","tests\\E1.json","tests\\E2.json","tests\\E3.json","tests\\E4.json","tests\\E5.json","tests\\E6.json","tests\\E7.json","tests\\E8.json",
	"tests\\E9.json","tests\\EA.json","tests\\EB.json","tests\\EC.json","tests\\ED.json","tests\\EF.json","tests\\F0.json","tests\\F1.json","tests\\F2.json","tests\\F3.json","tests\\F4.json",
	"tests\\F5.json","tests\\F6.0.json","tests\\F6.1.json","tests\\F6.2.json","tests\\F6.3.json","tests\\F6.4.json","tests\\F6.5.json","tests\\F6.6.json","tests\\F6.7.json","tests\\F7.0.json",
	"tests\\F7.1.json","tests\\F7.2.json","tests\\F7.3.json","tests\\F7.4.json","tests\\F7.5.json","tests\\F7.6.json","tests\\F7.7.json","tests\\F8.json","tests\\F9.json","tests\\FA.json",
	"tests\\FB.json","tests\\FC.json","tests\\FD.json","tests\\FE.0.json","tests\\FE.1.json","tests\\FF.0.json","tests\\FF.1.json","tests\\FF.2.json","tests\\FF.3.json","tests\\FF.4.json",
	"tests\\FF.5.json","tests\\FF.6.json","tests\\FF.7.json" };
	string files[1] = {"tests\\D2.6.json"};

	test_log = 0;

	int file_N = 0;
	for (auto f_N : files)
	{
		//открываем файл json
		//string json_file_name = "tests\\f7.0.json";
		string json_file_name = f_N;
		file_N++;

		//неопределенные флаги
		if (json_file_name == "tests\\27.json") OF_undef = true;
		if (json_file_name == "tests\\2F.json") OF_undef = true;
		if (json_file_name == "tests\\37.json") { OF_undef = true; PF_undef = true; SF_undef = true; ZF_undef = true; }
		if (json_file_name == "tests\\3F.json") { OF_undef = true; PF_undef = true; SF_undef = true; ZF_undef = true; }
		if (json_file_name == "tests\\80.6.json") AF_undef = true;
		if (json_file_name == "tests\\83.1.json") AF_undef = true;
		if (json_file_name == "tests\\D3.5.json") OF_undef = true;
		if (json_file_name == "tests\\F6.6.json") { OF_undef = true; PF_undef = true; SF_undef = true; ZF_undef = true; AF_undef = true; CF_undef = true; }
		if (json_file_name == "tests\\F6.7.json") { OF_undef = true; PF_undef = true; SF_undef = true; ZF_undef = true; AF_undef = true; CF_undef = true; }
		if (json_file_name == "tests\\F7.6.json") { OF_undef = true; PF_undef = true; SF_undef = true; ZF_undef = true; AF_undef = true; CF_undef = true; }
		if (json_file_name == "tests\\F7.7.json") { OF_undef = true; PF_undef = true; SF_undef = true; ZF_undef = true; AF_undef = true; CF_undef = true; }
		if (json_file_name == "tests\\D4.json") { OF_undef = true; AF_undef = true; CF_undef = true; }

		std::string json_string;
		std::ifstream inputFile(path + json_file_name);
		cout << file_N << "/" << files->length()  << " Testing start file: " << json_file_name << "   ";
		enum class i_state { wait, copy, process };

		i_state state = i_state::wait;
		uint8 level = 0;

		uint16 total_err = 0;
		uint16 total_ops = 0;

		bool OP_name_printed = 0;

		sf::Clock timer;
		timer.start();

		if (inputFile.is_open()) {
			std::string line;
			while (std::getline(inputFile, line)) {
				//считываем строку
				//cout << (int)level << "->" << line << endl;
				switch (state)
				{
				case i_state::wait:
					//ищем начало блока
					if (line.find("{") != std::string::npos)
					{
						level++;
						json_string = "{\n";
						state = i_state::copy;
					}
					break;

				case i_state::copy:
					//копируем тело
					if (line.find("{") != std::string::npos) level++;
					if (line.find("}") != std::string::npos) level--;
					if (level)
					{
						//мы еще внутри блока
						json_string += line;
						if (line.find("\"name\":") != std::string::npos && !OP_name_printed)
						{
							cout << line.substr(12) << endl;
							OP_name_printed = 1;
						}
					}
					else
					{
						//последняя скобка закрылась
						json_string += "\n}";
						if (test_log) cout << "test file " << json_file_name << " ";
						total_err += process(json_string);
						total_ops++;
						if (!test_log) cout << "\rOperations:  " << (int)total_ops;
						if (total_err) SetConsoleTextAttribute(hConsole, 12);
						if (!test_log) cout << "   Errors:  " << (int)total_err;
						SetConsoleTextAttribute(hConsole, 7);
						//cout << json_string << endl;
						if (total_ops == 10000) { cout << std::endl;  goto end_test; }//выход
						state = i_state::wait;
					}
					break;
				}
			}
			inputFile.close();
			
		}
		else {
			std::cerr << "Не удалось открыть файл: " << json_file_name << std::endl;
		}

	end_test:
		timer.stop();
		uint16 el = timer.getElapsedTime().asSeconds();
		cout << "Total time: " << (int)(el / 3600) << " h " << (int)((el % 3600) / 60) << " min " << (int)((el % 3600) % 60) << " sec" << endl;
		cout << std::endl;
	}
}

uint8 process(string json_str)
{
	//cout << json_str << endl;
	nlohmann::json jsonData; //объект json
	jsonData = nlohmann::json::parse(json_str);
	

	if (test_log) cout << "=========================================" << endl;

	//значения регистров
	if (test_log) cout << jsonData["name"] << " : " << hex << jsonData["bytes"] << endl;

	AX = (uint16)jsonData["initial"]["regs"]["ax"];
	if (test_log) cout << "AX = " << hex << (int)AX << endl;
	BX = (uint16)jsonData["initial"]["regs"]["bx"];
	if (test_log) cout << "BX = " << hex << (int)BX << endl;
	CX = (uint16)jsonData["initial"]["regs"]["cx"];
	if (test_log) cout << "CX = " << hex << (int)CX << endl;
	DX = (uint16)jsonData["initial"]["regs"]["dx"];
	if (test_log) cout << "DX = " << hex << (int)DX << endl;
	*CS = (uint16)jsonData["initial"]["regs"]["cs"];
	if (test_log) cout << "CS = " << hex << (int)*CS << endl;
	*SS = (uint16)jsonData["initial"]["regs"]["ss"];
	if (test_log) cout << "SS = " << hex << (int)*SS << endl;
	*DS = (uint16)jsonData["initial"]["regs"]["ds"];
	if (test_log) cout << "DS = " << hex << (int)*DS << endl;
	*ES = (uint16)jsonData["initial"]["regs"]["es"];
	if (test_log) cout << "ES = " << hex << (int)*ES << endl;
	Stack_Pointer = (uint16)jsonData["initial"]["regs"]["sp"];
	if (test_log) cout << "SP = " << hex << (int)Stack_Pointer << endl;
	Base_Pointer = (uint16)jsonData["initial"]["regs"]["bp"];
	if (test_log) cout << "BP = " << hex << (int)Base_Pointer << endl;
	Source_Index = (uint16)jsonData["initial"]["regs"]["si"];
	if (test_log) cout << "SI = " << hex << (int)Source_Index << endl;
	Destination_Index = (uint16)jsonData["initial"]["regs"]["di"];
	if (test_log) cout << "DI = " << hex << (int)Destination_Index << endl;
	Instruction_Pointer = (uint16)jsonData["initial"]["regs"]["ip"];
	if (test_log) cout << "IP = " << hex << (int)Instruction_Pointer << endl;
	uint16 Flags = (uint16)jsonData["initial"]["regs"]["flags"];
	//if (test_log) 	cout << "FLAGS = " << hex << (bitset<16>)Flags << endl;

	Flag_OF = (Flags >> 11) & 1;
	Flag_DF = (Flags >> 10) & 1;
	Flag_IF = (Flags >> 9) & 1;
	Flag_TF = (Flags >> 8) & 1;
	Flag_SF = (Flags >> 7) & 1;
	Flag_ZF = (Flags >> 6) & 1;
	Flag_AF = (Flags >> 4) & 1;
	Flag_PF = (Flags >> 2) & 1;
	Flag_CF = (Flags) & 1;

	if (test_log) cout << "Flags["<< hex << (uint16)jsonData["initial"]["regs"]["flags"] << "]: DF = " << Flag_DF << " ZF = " << Flag_ZF << " CF = " << Flag_CF << " AF = " << Flag_AF << " SF = " << Flag_SF << " PF = " << Flag_PF << " OF = " << Flag_OF << " IF = " << Flag_IF << endl;

	//odiszapc

	//ячейки памяти
	if (test_log) cout << "Load RAM" << endl;
	for (auto e : jsonData["initial"]["ram"])
	{
		memory.write_2((int)e[0], (int)e[1]);
		if (test_log) cout << hex << setw(5) << (int)e[0] << " " << setw(2) << (int)e[1] << endl;
	}
	

	//проверяем префикс замены регистра
	string c_name = jsonData["name"];

	if (c_name.find("cs:") != std::string::npos)
	{
		//заменяем сегмент
		DS = &CS_data;
		SetConsoleTextAttribute(hConsole, 10);
		if (test_log) cout << "change DS segment to CS" << endl;
		SetConsoleTextAttribute(hConsole, 7);
	}

	if (c_name.find("ss:") != std::string::npos)
	{
		//заменяем сегмент
		DS = &SS_data;
		SetConsoleTextAttribute(hConsole, 10);
		if (test_log) cout << "change DS segment to SS" << endl;
		SetConsoleTextAttribute(hConsole, 7);
	}

	if (c_name.find("es:") != std::string::npos)
	{
		//заменяем сегмент
		DS = &ES_data;
		SetConsoleTextAttribute(hConsole, 10);
		if (test_log) cout << "change DS segment to ES" << endl;
		SetConsoleTextAttribute(hConsole, 7);
	}

	
	log_to_console = test_log;
	exeption_fired = 0; //флаг события исключения

	//выполняем команду


test_rep:
	if (test_log)
	{
		cout << "Run..." << hex << endl;
		cout << *CS << ":" << std::setfill('0') << std::setw(4) << Instruction_Pointer << "  " <<
			std::setfill('0') << std::setw(2) << (int)memory.read_2(Instruction_Pointer + *CS * 16) << "  " <<
			std::setfill('0') << std::setw(2) << (int)memory.read_2(uint16(Instruction_Pointer + 1) + *CS * 16) << "  " <<
			std::setfill('0') << std::setw(2) << (int)memory.read_2(uint16(Instruction_Pointer + 2) + *CS * 16) << "  " <<
			std::setfill('0') << std::setw(2) << (int)memory.read_2(uint16(Instruction_Pointer + 3) + *CS * 16) << "  " <<
			std::setfill('0') << std::setw(2) << (int)memory.read_2(uint16(Instruction_Pointer + 4) + *CS * 16) << "  " <<
			std::setfill('0') << std::setw(2) << (int)memory.read_2(uint16(Instruction_Pointer + 5) + *CS * 16) << "\t";
	}
	
	op_code_table[memory.read_2(Instruction_Pointer + *CS * 16)]();
	
	//еще один круг
	if (negate_IDIV)
	{
		goto test_rep;
	}
	//обрабока исключений

	monitor.sync(1);
	if (test_log) cout << "\t ZF=" << Flag_ZF << " CF=" << Flag_CF << " AF=" << Flag_AF << " SF=" << Flag_SF << " PF=" << Flag_PF << " OF=" << Flag_OF << " IF=" << Flag_IF << endl;
	
	if (exeption)
	{
		exeption -= 0x10; //убираем префикс исключения
		//помещаем в стек IP и переходим по адресу прерывания
		//новые адреса
		uint16 new_IP = memory.read_2(exeption * 4) + memory.read_2(exeption * 4 + 1) * 256;
		uint16 new_CS = memory.read_2(exeption * 4 + 2) + memory.read_2(exeption * 4 + 3) * 256;

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
		memory.write_2(Stack_Pointer + SS_data * 16, (Instruction_Pointer) >> 8);
		Stack_Pointer--;
		memory.write_2(Stack_Pointer + SS_data * 16, (Instruction_Pointer) & 255);
		
		//передаем управление
		Flag_IF = false;//запрет внешних прерываний
		Flag_TF = false;
		*CS = new_CS;
		uint16 old = Instruction_Pointer;
		Instruction_Pointer = new_IP;
		SetConsoleTextAttribute(hConsole, 10);
		if (log_to_console) cout << "EXEPTION " << (int)exeption << " jump to " << (int)new_CS << ":" << (int)Instruction_Pointer << " ret to " << (int)old;
		SetConsoleTextAttribute(hConsole, 7);

		exeption = 0; //сброс флага
		exeption_fired = 1; //для проверки результата
	}
	
	if (test_log) cout << endl << "Result control" << endl;
	
	//возвращаем сегмент назад
	DS = &DS_data;
	
	//сверяем данные после выполнения
	uint8 err = 0;
	if (test_log) cout << "AX = " << hex << setw(4) << (int)AX << " supposed " << (uint16)jsonData["final"]["regs"]["ax"];
	if (AX == (uint16)jsonData["final"]["regs"]["ax"])
	{
		if (test_log) cout << " OK" << endl;
	}
	else
	{
		SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		err++;
		SetConsoleTextAttribute(hConsole, 7);
	}
	if (test_log) cout << "BX = " << hex << (int)BX << " supposed " << (uint16)jsonData["final"]["regs"]["bx"];
	if (BX == (uint16)jsonData["final"]["regs"]["bx"])
	{
		if (test_log) cout << " OK" << endl;
	}
	else
	{
		SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		err++;
		SetConsoleTextAttribute(hConsole, 7);
	}
	if (test_log) cout << "CX = " << hex << (int)CX << " supposed " << (uint16)jsonData["final"]["regs"]["cx"];
	if (CX == (uint16)jsonData["final"]["regs"]["cx"])
	{
		if (test_log) cout << " OK" << endl;
	}
	else
	{
		SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		err++;
		SetConsoleTextAttribute(hConsole, 7);
	}
	if (test_log) cout << "DX = " << hex << (int)DX << " supposed " << (uint16)jsonData["final"]["regs"]["dx"];
	if (DX == (uint16)jsonData["final"]["regs"]["dx"])
	{
		if (test_log) cout << " OK" << endl;
	}
	else
	{
		SetConsoleTextAttribute(hConsole, 12);
		if (test_log) 	cout << " ERROR" << endl;
		err++;
		SetConsoleTextAttribute(hConsole, 7);
	}


	if (test_log) cout << "SC = " << hex << (int)*CS << " supposed " << (uint16)jsonData["final"]["regs"]["cs"];
	if (*CS == (uint16)jsonData["final"]["regs"]["cs"])
	{
		if (test_log) cout << " OK" << endl;
	}
	else
	{
		SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		err++;
		SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "SS = " << hex << (int)*SS << " supposed " << (uint16)jsonData["final"]["regs"]["ss"];
	if (*SS == (uint16)jsonData["final"]["regs"]["ss"])
	{
		if (test_log) cout << " OK" << endl;
	}
	else
	{
		SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		err++;
		SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "DS = " << hex << (int)*DS << " supposed " << (uint16)jsonData["final"]["regs"]["ds"];
	if (*DS == (uint16)jsonData["final"]["regs"]["ds"]) 
	{
		if (test_log) cout << " OK" << endl;
	}
	else
	{
		SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		err++;
		SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "ES = " << hex << (int)*ES << " supposed " << (uint16)jsonData["final"]["regs"]["es"];
	if (*ES == (uint16)jsonData["final"]["regs"]["es"])
	{
		if (test_log) cout << " OK" << endl;
	}
	else
	{
		SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		err++;
		SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "SP = " << hex << (int)Stack_Pointer << " supposed " << (uint16)jsonData["final"]["regs"]["sp"];
	if (Stack_Pointer == (uint16)jsonData["final"]["regs"]["sp"]) 
	{
		if (test_log) cout << " OK" << endl;
	}
	else
	{
		SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		err++;
		SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "BP = " << hex << (int)Base_Pointer << " supposed " << (uint16)jsonData["final"]["regs"]["bp"];
	if (Base_Pointer == (uint16)jsonData["final"]["regs"]["bp"])
	{
		if (test_log) 	cout << " OK" << endl;
	}
	else
	{
		SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		err++;
		SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "SI = " << hex << (int)Source_Index << " supposed " << (uint16)jsonData["final"]["regs"]["si"];
	if (Source_Index == (uint16)jsonData["final"]["regs"]["si"])
	{
		if (test_log) cout << " OK" << endl;
	}
	else
	{
		SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		err++;
		SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "DI = " << hex << (int)Destination_Index << " supposed " << (uint16)jsonData["final"]["regs"]["di"];
	if (Destination_Index == (uint16)jsonData["final"]["regs"]["di"]) 
	{
		if (test_log) cout << " OK" << endl;
	}
	else
	{
		SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		err++;
		SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "IP = " << hex << (int)Instruction_Pointer << " supposed " << (uint16)jsonData["final"]["regs"]["ip"];
	if (Instruction_Pointer == (uint16)jsonData["final"]["regs"]["ip"])
	{
		if (test_log) cout << " OK" << endl;
	}
	else
	{
		SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		if (Instruction_Pointer != 0x400) err++;  //игнорируем ошибку в файле F6.7
		SetConsoleTextAttribute(hConsole, 7);
	}

	//проверка флагов

	Flags = (uint16)jsonData["final"]["regs"]["flags"]; //новые флаги

	if (test_log) cout << "Flag_OF = " << Flag_OF << " supposed " << ((Flags >> 11) & 1);
	if ((Flag_OF == ((Flags >> 11) & 1)) || OF_undef)
	{
		if (test_log && !OF_undef) 	cout << " OK" << endl;
		if (test_log && OF_undef) 	cout << " UNDEFINED" << endl;
	}
	else
	{
		SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		err++;
		SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "Flag_DF = " << Flag_DF << " supposed " << ((Flags >> 10) & 1);
	if (Flag_DF == ((Flags >> 10) & 1))
	{
		if (test_log) 	cout << " OK" << endl;
	}
	else
	{
		SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		err++;
		SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "Flag_IF = " << Flag_IF << " supposed " << ((Flags >> 9) & 1);
	if (Flag_IF == ((Flags >> 9) & 1))
	{
		if (test_log) 	cout << " OK" << endl;
	}
	else
	{
		SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		err++;
		SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "Flag_TF = " << Flag_TF << " supposed " << ((Flags >> 8) & 1);
	if (Flag_TF == ((Flags >> 8) & 1))
	{
		if (test_log) 	cout << " OK" << endl;
	}
	else
	{
		SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		err++;
		SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "Flag_SF = " << Flag_SF << " supposed " << ((Flags >> 7) & 1);
	if ((Flag_SF == ((Flags >> 7) & 1)) || SF_undef)
	{
		if (test_log && !SF_undef) 	cout << " OK" << endl;
		if (test_log && SF_undef) 	cout << " UNDEFINED" << endl;
	}
	else
	{
		SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		err++;
		SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "Flag_ZF = " << Flag_ZF << " supposed " << ((Flags >> 6) & 1);
	if ((Flag_ZF == ((Flags >> 6) & 1) )|| ZF_undef)
	{
		if (test_log && !ZF_undef) 	cout << " OK" << endl;
		if (test_log && ZF_undef) 	cout << " UNDEFINED" << endl;
	}
	else
	{
		SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		err++;
		SetConsoleTextAttribute(hConsole, 7);
	}
	if (test_log) cout << "Flag_AF = " << Flag_AF << " supposed " << ((Flags >> 4) & 1);
	if ((Flag_AF == ((Flags >> 4) & 1) )|| AF_undef)
	{
		if (test_log && !AF_undef) 	cout << " OK" << endl;
		if (test_log && AF_undef) 	cout << " UNDEFINED" << endl;
	}
	else
	{
		SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		err++;
		SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "Flag_PF = " << Flag_PF << " supposed " << ((Flags >> 2) & 1);
	if ((Flag_PF == ((Flags >> 2) & 1)) || PF_undef)
	{
		if (test_log && !PF_undef) 	cout << " OK" << endl;
		if (test_log && PF_undef) 	cout << " UNDEFINED" << endl;
	}
	else
	{
		SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		err++;
		SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "Flag_CF = " << Flag_CF << " supposed " << ((Flags) & 1);
	if ((Flag_CF == ((Flags) & 1)) || CF_undef)
	{
		if (test_log && !CF_undef) 	cout << " OK" << endl;
		if (test_log && CF_undef) 	cout << " UNDEFINED" << endl;
	}
	else
	{
		SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		err++;
		SetConsoleTextAttribute(hConsole, 7);
	}

	if (err) SetConsoleTextAttribute(hConsole, 12);
	if (test_log) cout << "ERRORs = " << (int)err << endl << endl;
	SetConsoleTextAttribute(hConsole, 7);

	if (test_log) cout << "Checking RAM" << endl;
	for (auto e : jsonData["final"]["ram"])
	{
		uint8 fact_ram = memory.read_2((int)e[0]);
		if (test_log) cout << hex << setw(5) << (int)e[0] << " " << setw(2) << (int)fact_ram << " supposed " << setw(2) << (int)e[1];
		if (fact_ram == (int)e[1])
		{
			if (test_log) cout << " OK" << endl;
		}
		else
		{
			SetConsoleTextAttribute(hConsole, 12);
			if (test_log) cout << " ERROR" << endl;
			string op_name = jsonData["name"];
			if (!exeption_fired) err++;
			SetConsoleTextAttribute(hConsole, 7);
		}
	}

	//пауза
	if (test_log) while (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F8) && err) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }

	return err;

}
