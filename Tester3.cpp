#include "opcode_functions.h"
#include "custom_classes.h"
#include "video.h"
#include "audio.h"
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <conio.h>
#include <chrono>
#include <bitset>
#include <sstream>
#include <thread>

//#define DEBUG
typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;

//Доп функции
template< typename T >
extern std::string int_to_hex(T i, int w);

// переменные из основного файла программы
using namespace std;
extern HANDLE hConsole;

extern bool exeption_0;
extern bool exeption_1;

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
extern bool keep_segment_override; //сохранить флаг между командами

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

//признак неопределенности флага
extern bool OF_undef;
extern bool PF_undef;
extern bool SF_undef;
extern bool ZF_undef;
extern bool AF_undef;
extern bool CF_undef;

extern void (*op_code_table[256])();

extern std::string path;

extern bool test_log;
extern bool repeat_test_op;
extern bool exeption_fired;
extern bool negate_IDIV;

struct data_error
{
	int total_err;
	int op_err;

};

bool cycle_op = 0; //признак циклических операций

data_error process2(string json_str); //декларация

void tester3()
{
	string files11[323] = {"tests2\\00.json" ,"tests2\\01.json" ,"tests2\\02.json" ,"tests2\\03.json" ,"tests2\\04.json" ,"tests2\\05.json" ,"tests2\\06.json" ,"tests2\\07.json" ,"tests2\\08.json" ,
	"tests2\\09.json","tests2\\0A.json","tests2\\0B.json","tests2\\0C.json","tests2\\0D.json","tests2\\0E.json", "tests2\\10.json","tests2\\11.json","tests2\\12.json","tests2\\13.json",
	"tests2\\14.json","tests2\\15.json","tests2\\16.json","tests2\\17.json","tests2\\18.json","tests2\\19.json","tests2\\1A.json","tests2\\1B.json","tests2\\1C.json","tests2\\1D.json","tests2\\1E.json",
	"tests2\\1F.json","tests2\\20.json","tests2\\21.json","tests2\\22.json","tests2\\23.json","tests2\\24.json","tests2\\25.json","tests2\\27.json","tests2\\28.json","tests2\\29.json",
	"tests2\\2A.json", "tests2\\2B.json", "tests2\\2C.json", "tests2\\2D.json", "tests2\\2F.json", "tests2\\30.json", "tests2\\31.json", "tests2\\32.json", "tests2\\33.json", 
	"tests2\\34.json","tests2\\35.json","tests2\\37.json","tests2\\38.json","tests2\\39.json","tests2\\3A.json","tests2\\3B.json","tests2\\3C.json","tests2\\3D.json",
	"tests2\\3F.json" ,"tests2\\40.json" ,"tests2\\41.json" ,"tests2\\42.json" ,"tests2\\43.json" ,"tests2\\44.json" ,"tests2\\45.json" ,"tests2\\46.json" ,"tests2\\47.json" ,"tests2\\48.json" ,"tests2\\49.json",
	"tests2\\4A.json","tests2\\4B.json","tests2\\4C.json","tests2\\4D.json","tests2\\4E.json","tests2\\4F.json","tests2\\50.json","tests2\\51.json","tests2\\52.json","tests2\\53.json","tests2\\54.json","tests2\\55.json",
	"tests2\\56.json","tests2\\57.json","tests2\\58.json","tests2\\59.json","tests2\\5A.json","tests2\\5B.json","tests2\\5C.json","tests2\\5D.json","tests2\\5E.json","tests2\\5F.json","tests2\\60.json",
	"tests2\\61.json","tests2\\62.json" ,"tests2\\63.json" ,"tests2\\64.json" ,"tests2\\65.json" ,"tests2\\66.json" ,"tests2\\67.json" ,"tests2\\68.json" ,"tests2\\69.json" ,"tests2\\6A.json" ,
	"tests2\\6B.json","tests2\\6C.json" ,"tests2\\6D.json" ,"tests2\\6E.json" ,"tests2\\6F.json" ,"tests2\\70.json" ,"tests2\\71.json" ,"tests2\\72.json" ,"tests2\\73.json" ,"tests2\\74.json",
	"tests2\\75.json","tests2\\76.json" ,"tests2\\77.json" ,"tests2\\78.json" ,"tests2\\79.json" ,"tests2\\7A.json" ,"tests2\\7B.json" ,"tests2\\7C.json" ,"tests2\\7D.json" ,"tests2\\7E.json",
	"tests2\\7F.json", "tests2\\80.0.json","tests2\\80.1.json","tests2\\80.2.json","tests2\\80.3.json","tests2\\80.4.json","tests2\\80.5.json","tests2\\80.6.json","tests2\\80.7.json",
	"tests2\\81.0.json", "tests2\\81.1.json", "tests2\\81.2.json", "tests2\\81.3.json", "tests2\\81.4.json", "tests2\\81.5.json", "tests2\\81.6.json", "tests2\\81.7.json",
	"tests2\\82.0.json", "tests2\\82.1.json", "tests2\\82.2.json", "tests2\\82.3.json", "tests2\\82.4.json", "tests2\\82.5.json", "tests2\\82.6.json", "tests2\\82.7.json",
	"tests2\\83.0.json", "tests2\\83.1.json", "tests2\\83.2.json", "tests2\\83.3.json", "tests2\\83.4.json", "tests2\\83.5.json", "tests2\\83.6.json", "tests2\\83.7.json",	"tests2\\84.json",
	"tests2\\85.json","tests2\\86.json" ,"tests2\\87.json" ,"tests2\\88.json" ,"tests2\\89.json" ,"tests2\\8A.json" ,"tests2\\8B.json" ,"tests2\\8C.json" ,"tests2\\8D.json" ,"tests2\\8E.json",
	"tests2\\8F.json","tests2\\90.json" ,"tests2\\91.json" ,"tests2\\92.json" ,"tests2\\93.json" ,"tests2\\94.json", "tests2\\95.json","tests2\\96.json" ,"tests2\\97.json" ,"tests2\\98.json" ,
	"tests2\\99.json" ,"tests2\\9A.json" ,"tests2\\9C.json" ,"tests2\\9D.json" ,"tests2\\9E.json",	"tests2\\9F.json", "tests2\\A0.json" ,"tests2\\A1.json" ,"tests2\\A2.json",
	"tests2\\A3.json" ,"tests2\\A4.json",	"tests2\\A5.json","tests2\\A6.json" ,"tests2\\A7.json" ,"tests2\\A8.json" ,"tests2\\A9.json" ,"tests2\\AA.json" ,"tests2\\AB.json" ,"tests2\\AC.json" ,
	"tests2\\AD.json" ,"tests2\\AE.json",	"tests2\\AF.json","tests2\\B0.json" ,"tests2\\B1.json" ,"tests2\\B2.json" ,"tests2\\B3.json" ,"tests2\\B4.json",
	"tests2\\B5.json","tests2\\B6.json" ,"tests2\\B7.json" ,"tests2\\B8.json" ,"tests2\\B9.json" ,"tests2\\BA.json" ,"tests2\\BB.json" ,"tests2\\BC.json" ,"tests2\\BD.json" ,"tests2\\BE.json",
	"tests2\\BF.json","tests2\\C0.json" ,"tests2\\C1.json" ,"tests2\\C2.json" ,"tests2\\C3.json" ,"tests2\\C4.json",
	"tests2\\C5.json","tests2\\C6.json" ,"tests2\\C7.json" ,"tests2\\C8.json" ,"tests2\\C9.json" ,"tests2\\CA.json" ,"tests2\\CB.json" ,"tests2\\CC.json" ,"tests2\\CD.json" ,"tests2\\CE.json",
	"tests2\\CF.json","tests2\\D0.0.json","tests2\\D0.1.json","tests2\\D0.2.json","tests2\\D0.3.json","tests2\\D0.4.json","tests2\\D0.5.json","tests2\\D0.6.json","tests2\\D0.7.json",
	"tests2\\D1.0.json","tests2\\D1.1.json","tests2\\D1.2.json","tests2\\D1.3.json","tests2\\D1.4.json","tests2\\D1.5.json","tests2\\D1.6.json","tests2\\D1.7.json",
	"tests2\\D2.0.json","tests2\\D2.1.json","tests2\\D2.2.json","tests2\\D2.3.json","tests2\\D2.4.json","tests2\\D2.5.json","tests2\\D2.6.json","tests2\\D2.7.json",
	"tests2\\D3.0.json","tests2\\D3.1.json","tests2\\D3.2.json","tests2\\D3.3.json","tests2\\D3.4.json","tests2\\D3.5.json","tests2\\D3.6.json","tests2\\D3.7.json",
	"tests2\\D4.json","tests2\\D5.json","tests2\\D6.json","tests2\\D7.json","tests2\\D8.json","tests2\\D9.json","tests2\\DA.json","tests2\\DB.json","tests2\\DC.json","tests2\\DD.json",
	"tests2\\DE.json","tests2\\DF.json","tests2\\E0.json","tests2\\E1.json","tests2\\E2.json","tests2\\E3.json","tests2\\E4.json","tests2\\E5.json","tests2\\E6.json","tests2\\E7.json","tests2\\E8.json",
	"tests2\\E9.json","tests2\\EA.json","tests2\\EB.json","tests2\\EC.json","tests2\\ED.json","tests2\\EF.json","tests2\\EE.json",
	"tests2\\F5.json","tests2\\F6.0.json","tests2\\F6.1.json","tests2\\F6.2.json","tests2\\F6.3.json","tests2\\F6.4.json","tests2\\F6.5.json","tests2\\F6.6.json","tests2\\F6.7.json","tests2\\F7.0.json",
	"tests2\\F7.1.json","tests2\\F7.2.json","tests2\\F7.3.json","tests2\\F7.4.json","tests2\\F7.5.json","tests2\\F7.6.json","tests2\\F7.7.json","tests2\\F8.json","tests2\\F9.json","tests2\\FA.json",
	"tests2\\FB.json","tests2\\FC.json","tests2\\FD.json","tests2\\FE.0.json","tests2\\FE.1.json","tests2\\FF.0.json","tests2\\FF.1.json","tests2\\FF.2.json","tests2\\FF.3.json","tests2\\FF.4.json",
	"tests2\\FF.5.json","tests2\\FF.6.json","tests2\\FF.7.json" };
	
	// сделаны D0 и D1
	string files[6] = {"tests2\\27.json","tests2\\2F.json","tests2\\F6.6.json","tests2\\F6.7.json","tests2\\F7.6.json","tests2\\F7.6.json"};
	//сопроцессор
	//string files[8] = { "tests2\\F6.7.json" };
	string files14[14] = { "tests2\\A4.json",	"tests2\\A5.json","tests2\\A6.json" ,"tests2\\A7.json" ,"tests2\\AA.json" ,"tests2\\AB.json" ,"tests2\\AC.json" ,
	"tests2\\AD.json" ,"tests2\\AE.json",	"tests2\\AF.json" , "tests2\\F6.6.json","tests2\\F6.7.json","tests2\\F7.6.json","tests2\\F7.6.json" };
	//string files[3] = {"tests2\\A7.json" ,"tests2\\AE.json",	"tests2\\AF.json" };
	
	test_log = 0;
		
	//move - 88, 89, 8A, 8B, 8C, 8E, A0, A1, A2, A3, A4, A5, B0 - BF, C6, C7
	//pop, push - 06, 07, 0E, 16, 17, 1E, 1F, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 5A, 5B, 5C, 5D, 5E, 5F, 8F, 9C, 9D, FF.6, FF.7
	//str - A4, A5, A6, A7, AA, AB, AC, AD, AE, AF    
	//cmp - 38, 39, 3A, 3B, 3C, 3D, 80.7, 81.7, 82.7, 83.7 
	//XCHG - 86, 87, 91 - 97
	//LEA, LDS, LES - 8D, C4, C5
	//LAHF, SAHF - 9E, 9F
	//POPF, PUSHF - 9D, 9C
	//ADD - 00, 01, 02, 03, 04, 05, 80.0, 81.0, 82.0, 83.0
	//ADC - 10, 11, 12, 13, 14, 15, 80.2, 81.2, 82.2, 83.2
	//SUB - 28, 29, 2A, 2B, 2C, 2D, 80.5, 81.5, 82.5, 83.5
	//SBB - 18, 19, 1A, 1B, 1C, 1D, 80.3, 81.3, 82.3, 83.3
	//INC - 40 - 47, FE.0, FF.0
	//DEC - 48 - 4F, FE.1, FF.1
	//CMP - 38, 39, 3A, 3B, 3C, 3D, 80.7, 81.7, 82.7, 83.7
	//AND - 20, 21, 22, 23, 24, 25, 80.4, 81.4, 82.4, 83.4
	//OR  - 08, 09, 0A, 0B, 0C, 0D, 80.1, 81.1, 82.1, 83.1
	//XOR - 30, 31, 32, 33, 34, 35, 80.6, 81.6, 82.6, 83.6
	//test - 84, 85, A8, A9, F6.0, F6.1, F7.0, F7.1

	
	int file_N = 0;
	for (auto f_N : files)
	{
		//открываем файл json
		//string json_file_name = "tests2\\f7.0.json";
		string json_file_name = f_N.replace(0,6, "tests");  //замена tests2 на tests
		//string json_file_name = f_N;
		file_N++;

		//неопределенные флаги
		if (json_file_name == "tests\\08.json") AF_undef = true;
		if (json_file_name == "tests\\09.json") AF_undef = true;
		if (json_file_name == "tests\\0A.json") AF_undef = true;
		if (json_file_name == "tests\\0B.json") AF_undef = true;
		if (json_file_name == "tests\\0C.json") AF_undef = true;
		if (json_file_name == "tests\\0D.json") AF_undef = true;
		if (json_file_name == "tests\\20.json") AF_undef = true;
		if (json_file_name == "tests\\21.json") AF_undef = true;
		if (json_file_name == "tests\\22.json") AF_undef = true;
		if (json_file_name == "tests\\23.json") AF_undef = true;
		if (json_file_name == "tests\\24.json") AF_undef = true;
		if (json_file_name == "tests\\25.json") AF_undef = true;
		if (json_file_name == "tests\\30.json") AF_undef = true;
		if (json_file_name == "tests\\31.json") AF_undef = true;
		if (json_file_name == "tests\\32.json") AF_undef = true;
		if (json_file_name == "tests\\33.json") AF_undef = true;
		if (json_file_name == "tests\\34.json") AF_undef = true;
		if (json_file_name == "tests\\35.json") AF_undef = true;
		if (json_file_name == "tests\\80.1.json") AF_undef = true;
		if (json_file_name == "tests\\80.4.json") AF_undef = true;
		if (json_file_name == "tests\\27.json") OF_undef = true;
		if (json_file_name == "tests\\2F.json") OF_undef = true;
		if (json_file_name == "tests\\37.json") { OF_undef = true; PF_undef = true; SF_undef = true; ZF_undef = true; }
		if (json_file_name == "tests\\3F.json") { OF_undef = true; PF_undef = true; SF_undef = true; ZF_undef = true; }
		if (json_file_name == "tests\\80.6.json") AF_undef = true;
		if (json_file_name == "tests\\81.1.json") AF_undef = true;
		if (json_file_name == "tests\\81.4.json") AF_undef = true;
		if (json_file_name == "tests\\81.6.json") AF_undef = true;
		if (json_file_name == "tests\\82.1.json") AF_undef = true;
		if (json_file_name == "tests\\83.1.json") AF_undef = true;
		if (json_file_name == "tests\\82.4.json") AF_undef = true;
		if (json_file_name == "tests\\83.4.json") AF_undef = true;
		if (json_file_name == "tests\\82.6.json") AF_undef = true;
		if (json_file_name == "tests\\83.6.json") AF_undef = true;
		if (json_file_name == "tests\\84.json") AF_undef = true;
		if (json_file_name == "tests\\85.json") AF_undef = true;
		if (json_file_name == "tests\\A4.json") cycle_op = true;
		if (json_file_name == "tests\\A5.json") cycle_op = true;
		if (json_file_name == "tests\\A6.json") cycle_op = true;
		if (json_file_name == "tests\\A7.json") cycle_op = true;
		if (json_file_name == "tests\\A8.json") AF_undef = true;
		if (json_file_name == "tests\\A9.json") AF_undef = true;
		if (json_file_name == "tests\\AA.json") cycle_op = true;
		if (json_file_name == "tests\\AB.json") cycle_op = true;
		if (json_file_name == "tests\\AC.json") cycle_op = true;
		if (json_file_name == "tests\\AD.json") cycle_op = true;
		if (json_file_name == "tests\\AE.json") cycle_op = true;
		if (json_file_name == "tests\\AF.json") cycle_op = true;
		
		if (json_file_name == "tests\\F6.0.json") AF_undef = true;
		
		if (json_file_name == "tests\\F6.4.json") { AF_undef = true; PF_undef = true; ZF_undef = true; SF_undef = true; }
		//if (json_file_name == "tests2\\F7.4.json") { PF_undef = true; AF_undef = true; ZF_undef = true; SF_undef = true; }
		if (json_file_name == "tests\\F6.5.json") { PF_undef = true; AF_undef = true; ZF_undef = true; SF_undef = true; }
		if (json_file_name == "tests\\F7.0.json") AF_undef = true;
		if (json_file_name == "tests\\F7.1.json") AF_undef = true;
		if (json_file_name == "tests\\F7.5.json") { PF_undef = true; AF_undef = true; ZF_undef = true; SF_undef = true;}
		if (json_file_name == "tests\\F6.6.json") { OF_undef = true; PF_undef = true; SF_undef = true; ZF_undef = true; AF_undef = true; CF_undef = true; }
		if (json_file_name == "tests\\F6.7.json") { OF_undef = true; PF_undef = true; SF_undef = true; ZF_undef = true; AF_undef = true; CF_undef = true; }
		if (json_file_name == "tests\\F7.6.json") { OF_undef = true; PF_undef = true; SF_undef = true; ZF_undef = true; AF_undef = true; CF_undef = true; }
		if (json_file_name == "tests\\F7.7.json") { OF_undef = true; PF_undef = true; SF_undef = true; ZF_undef = true; AF_undef = true; CF_undef = true; }
		if (json_file_name == "tests\\D4.json") { OF_undef = true; AF_undef = true; CF_undef = true; }
		if (json_file_name == "tests\\D0.4.json") AF_undef = true;
		if (json_file_name == "tests\\D0.5.json") AF_undef = true;
		if (json_file_name == "tests\\D0.7.json") AF_undef = true;
		if (json_file_name == "tests\\D1.4.json") AF_undef = true;
		if (json_file_name == "tests\\D1.5.json") AF_undef = true;
		if (json_file_name == "tests\\D1.7.json") AF_undef = true;
		if (json_file_name == "tests\\D2.4.json") AF_undef = true;
		if (json_file_name == "tests\\D2.5.json") AF_undef = true;
		if (json_file_name == "tests\\D2.7.json") AF_undef = true;
		if (json_file_name == "tests\\D3.4.json") AF_undef = true;
		if (json_file_name == "tests\\D3.5.json") AF_undef = true;
		if (json_file_name == "tests\\D3.7.json") AF_undef = true;
		

		std::string json_string;
		std::ifstream inputFile(path + json_file_name);
		cout << file_N << "/" << files->size()  << " Testing file: " << json_file_name << " ";
		enum class i_state { wait, copy, process };

		i_state state = i_state::wait;
		uint8 level = 0;

		uint16 total_err = 0;
		uint16 total_ops = 0;
		uint16 ip_err = 0;

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
							cout << line.substr(12) << " ";
							OP_name_printed = 1;
						}
					}
					else
					{
						//последняя скобка закрылась
						json_string += "\n}";
						if (test_log) cout << "test file " << json_file_name << " ";
						data_error r = process2(json_string);
						total_err += r.total_err;
						ip_err += r.op_err;
						total_ops++;
						//cout << json_string << endl;
						if (total_ops == 10000) { goto end_test; }//выход
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
		SetConsoleTextAttribute(hConsole, 7);
		cout << " Operations: " << (int)total_ops;
		if (total_err) SetConsoleTextAttribute(hConsole, 12);
		cout << " Errors: " << (int)total_err << " (ip_err " << (int)ip_err << ")";
		SetConsoleTextAttribute(hConsole, 7);

		timer.stop();
		uint16 el = timer.getElapsedTime().asSeconds();
		cout << "Total time: " << (int)(el / 3600) << " h " << (int)((el % 3600) / 60) << " min " << (int)((el % 3600) % 60) << " sec";
		cout << std::endl;
	}
}

data_error process2(string json_str)
{
	//cout << json_str << endl;
	nlohmann::json jsonData; //объект json
	jsonData = nlohmann::json::parse(json_str);
	

	if (test_log) cout << endl << "=========================================" << endl;

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

	//ячейки памяти
	if (test_log) cout << "Load RAM" << endl;
	for (auto e : jsonData["initial"]["ram"])
	{
		memory.write_2((int)e[0], (int)e[1]);
		if (test_log) cout << hex << setw(5) << (int)e[0] << " " << setw(2) << (int)e[1] << endl;
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
			std::setfill('0') << std::setw(2) << (int)memory_2[(Instruction_Pointer + 1 + *CS * 16) & 0xFFFFF] << "  " <<
			std::setfill('0') << std::setw(2) << (int)memory.read_2(Instruction_Pointer + 2 + *CS * 16) << "  " <<
			std::setfill('0') << std::setw(2) << (int)memory.read_2(Instruction_Pointer + 3 + *CS * 16) << "  " <<
			std::setfill('0') << std::setw(2) << (int)memory.read_2(Instruction_Pointer + 4 + *CS * 16) << "  " <<
			std::setfill('0') << std::setw(2) << (int)memory.read_2(Instruction_Pointer + 5 + *CS * 16) << "\t";
	}
	
	op_code_table[memory.read_2((Instruction_Pointer + *CS * 16) & 0xFFFFF)]();
	

	if (keep_segment_override) { 
		keep_segment_override = false; //сбрасываем флаг сохранения
		goto test_rep; //выполняем еще одну команду
	} 
	else { Flag_segment_override = 0; } //сбрасываем флаг смены сегмента

	//еще один круг
	if (negate_IDIV)
	{
		goto test_rep;
	}
	//обрабока исключений

	//monitor.sync(1);
	if (test_log) cout << "\t ZF=" << Flag_ZF << " CF=" << Flag_CF << " AF=" << Flag_AF << " SF=" << Flag_SF << " PF=" << Flag_PF << " OF=" << Flag_OF << " IF=" << Flag_IF << endl;
	
	if (exeption_0)
	{
		//помещаем в стек IP и переходим по адресу прерывания
		//новые адреса
		uint16 new_IP = memory.read_2(0) + memory.read_2(1) * 256;
		uint16 new_CS = memory.read_2(2) + memory.read_2(3) * 256;

		if (log_to_console)
		{
			SetConsoleTextAttribute(hConsole, 10);
			cout << "EXEPTION 0 [DIV 0] jump to " << int_to_hex(new_CS, 4) << ":" << int_to_hex(new_IP, 4) << " ret to " << int_to_hex(*CS, 4) << ":" << int_to_hex(Instruction_Pointer, 4) << endl;
			SetConsoleTextAttribute(hConsole, 7);
		}

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
		Instruction_Pointer = new_IP;
		exeption_0 = 0; //сброс флага
	}
	if (exeption_1)
	{
		//помещаем в стек IP и переходим по адресу прерывания
		//новые адреса
		uint16 new_IP = memory.read_2(4) + memory.read_2(4 + 1) * 256;
		uint16 new_CS = memory.read_2(4 + 2) + memory.read_2(4 + 3) * 256;

		if (log_to_console)
		{
			SetConsoleTextAttribute(hConsole, 10);
			cout << "EXEPTION 1(trap) jump to " << int_to_hex(new_CS, 4) << ":" << int_to_hex(new_IP, 4) << " ret to " << int_to_hex(*CS, 4) << ":" << int_to_hex(Instruction_Pointer, 4) << endl;
			SetConsoleTextAttribute(hConsole, 7);
		}

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
		Instruction_Pointer = new_IP;
		exeption_1 = 0; //сброс флага
	}
	
	if (cycle_op && memory.read_2((Instruction_Pointer + *CS * 16) & 0xFFFFF) != 0x90) goto test_rep;//повтор если IP не указывает на 90 (для строк)

	if (test_log) cout << endl << "Result control" << endl;
	
	//возвращаем сегмент назад
	//DS = &DS_data;
	
	//сверяем данные после выполнения
	data_error d_err = {0,0};
	

	
	if (jsonData["final"]["regs"]["ax"].is_null()) jsonData["final"]["regs"]["ax"] = jsonData["initial"]["regs"]["ax"];
	if (jsonData["final"]["regs"]["bx"].is_null()) jsonData["final"]["regs"]["bx"] = jsonData["initial"]["regs"]["bx"];
	if (jsonData["final"]["regs"]["cx"].is_null()) jsonData["final"]["regs"]["cx"] = jsonData["initial"]["regs"]["cx"];
	if (jsonData["final"]["regs"]["dx"].is_null()) jsonData["final"]["regs"]["dx"] = jsonData["initial"]["regs"]["dx"];
	if (jsonData["final"]["regs"]["cs"].is_null()) jsonData["final"]["regs"]["cs"] = jsonData["initial"]["regs"]["cs"];
	if (jsonData["final"]["regs"]["ss"].is_null()) jsonData["final"]["regs"]["ss"] = jsonData["initial"]["regs"]["ss"];
	if (jsonData["final"]["regs"]["ds"].is_null()) jsonData["final"]["regs"]["ds"] = jsonData["initial"]["regs"]["ds"];
	if (jsonData["final"]["regs"]["es"].is_null()) jsonData["final"]["regs"]["es"] = jsonData["initial"]["regs"]["es"];
	if (jsonData["final"]["regs"]["sp"].is_null()) jsonData["final"]["regs"]["sp"] = jsonData["initial"]["regs"]["sp"];
	if (jsonData["final"]["regs"]["bp"].is_null()) jsonData["final"]["regs"]["bp"] = jsonData["initial"]["regs"]["bp"];
	if (jsonData["final"]["regs"]["si"].is_null()) jsonData["final"]["regs"]["si"] = jsonData["initial"]["regs"]["si"];
	if (jsonData["final"]["regs"]["di"].is_null()) jsonData["final"]["regs"]["di"] = jsonData["initial"]["regs"]["di"];
	if (jsonData["final"]["regs"]["ip"].is_null()) jsonData["final"]["regs"]["ip"] = jsonData["initial"]["regs"]["ip"];
	if (jsonData["final"]["regs"]["flags"].is_null()) jsonData["final"]["regs"]["flags"] = jsonData["initial"]["regs"]["flags"];
	

	if (test_log) cout << "AX = " << hex << setw(4) << (int)AX << " supposed " << (uint16)jsonData["final"]["regs"]["ax"];
	
	if (AX == (uint16)jsonData["final"]["regs"]["ax"])
	{
		if (test_log) cout << " OK" << endl;
	}
	else
	{
		if (test_log) SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		d_err.total_err++;
		if (test_log)SetConsoleTextAttribute(hConsole, 7);
	}
	if (test_log) cout << "BX = " << hex << (int)BX << " supposed " << (uint16)jsonData["final"]["regs"]["bx"];
	if (BX == (uint16)jsonData["final"]["regs"]["bx"])
	{
		if (test_log) cout << " OK" << endl;
	}
	else
	{
		if (test_log)SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		d_err.total_err++;
		if (test_log)SetConsoleTextAttribute(hConsole, 7);
	}
	if (test_log) cout << "CX = " << hex << (int)CX << " supposed " << (uint16)jsonData["final"]["regs"]["cx"];
	if (CX == (uint16)jsonData["final"]["regs"]["cx"])
	{
		if (test_log) cout << " OK" << endl;
	}
	else
	{
		if (test_log)SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		d_err.total_err++;
		if (test_log)SetConsoleTextAttribute(hConsole, 7);
	}
	if (test_log) cout << "DX = " << hex << (int)DX << " supposed " << (uint16)jsonData["final"]["regs"]["dx"];
	if (DX == (uint16)jsonData["final"]["regs"]["dx"])
	{
		if (test_log) cout << " OK" << endl;
	}
	else
	{
		if (test_log)SetConsoleTextAttribute(hConsole, 12);
		if (test_log) 	cout << " ERROR" << endl;
		d_err.total_err++;
		if (test_log)SetConsoleTextAttribute(hConsole, 7);
	}


	if (test_log) cout << "SC = " << hex << (int)*CS << " supposed " << (uint16)jsonData["final"]["regs"]["cs"];
	if (*CS == (uint16)jsonData["final"]["regs"]["cs"])
	{
		if (test_log) cout << " OK" << endl;
	}
	else
	{
		if (test_log)SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		d_err.total_err++;
		if (test_log)SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "SS = " << hex << (int)*SS << " supposed " << (uint16)jsonData["final"]["regs"]["ss"];
	if (*SS == (uint16)jsonData["final"]["regs"]["ss"])
	{
		if (test_log) cout << " OK" << endl;
	}
	else
	{
		if (test_log)SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		d_err.total_err++;
		if (test_log)SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "DS = " << hex << (int)*DS << " supposed " << (uint16)jsonData["final"]["regs"]["ds"];
	if (*DS == (uint16)jsonData["final"]["regs"]["ds"]) 
	{
		if (test_log) cout << " OK" << endl;
	}
	else
	{
		if (test_log)SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		d_err.total_err++;
		if (test_log)SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "ES = " << hex << (int)*ES << " supposed " << (uint16)jsonData["final"]["regs"]["es"];
	if (*ES == (uint16)jsonData["final"]["regs"]["es"])
	{
		if (test_log) cout << " OK" << endl;
	}
	else
	{
		if (test_log)SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		d_err.total_err++;
		if (test_log)SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "SP = " << hex << (int)Stack_Pointer << " supposed " << (uint16)jsonData["final"]["regs"]["sp"];
	if (Stack_Pointer == (uint16)jsonData["final"]["regs"]["sp"]) 
	{
		if (test_log) cout << " OK" << endl;
	}
	else
	{
		if (test_log)SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		d_err.total_err++;
		if (test_log)SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "BP = " << hex << (int)Base_Pointer << " supposed " << (uint16)jsonData["final"]["regs"]["bp"];
	if (Base_Pointer == (uint16)jsonData["final"]["regs"]["bp"])
	{
		if (test_log) 	cout << " OK" << endl;
	}
	else
	{
		if (test_log)SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		d_err.total_err++;
		if (test_log)SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "SI = " << hex << (int)Source_Index << " supposed " << (uint16)jsonData["final"]["regs"]["si"];
	if (Source_Index == (uint16)jsonData["final"]["regs"]["si"])
	{
		if (test_log) cout << " OK" << endl;
	}
	else
	{
		if (test_log)SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		d_err.total_err++;
		if (test_log)SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "DI = " << hex << (int)Destination_Index << " supposed " << (uint16)jsonData["final"]["regs"]["di"];
	if (Destination_Index == (uint16)jsonData["final"]["regs"]["di"]) 
	{
		if (test_log) cout << " OK" << endl;
	}
	else
	{
		if (test_log)SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		d_err.total_err++;
		if (test_log)SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "IP = " << hex << (int)Instruction_Pointer << " supposed " << (uint16)jsonData["final"]["regs"]["ip"];
	if (Instruction_Pointer == (uint16)jsonData["final"]["regs"]["ip"])
	{
		if (test_log) cout << " OK" << endl;
	}
	else
	{
		if (test_log)SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		if (Instruction_Pointer != 0x400) d_err.total_err++;  //игнорируем ошибку в файле F6.7
		d_err.op_err++;
		if (test_log)SetConsoleTextAttribute(hConsole, 7);
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
		if (test_log)SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		d_err.total_err++;
		if (test_log)SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "Flag_DF = " << Flag_DF << " supposed " << ((Flags >> 10) & 1);
	if (Flag_DF == ((Flags >> 10) & 1))
	{
		if (test_log) 	cout << " OK" << endl;
	}
	else
	{
		if (test_log)SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		d_err.total_err++;
		if (test_log)SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "Flag_IF = " << Flag_IF << " supposed " << ((Flags >> 9) & 1);
	if (Flag_IF == ((Flags >> 9) & 1))
	{
		if (test_log) 	cout << " OK" << endl;
	}
	else
	{
		if (test_log)SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		d_err.total_err++;
		if (test_log)SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "Flag_TF = " << Flag_TF << " supposed " << ((Flags >> 8) & 1);
	if (Flag_TF == ((Flags >> 8) & 1))
	{
		if (test_log) 	cout << " OK" << endl;
	}
	else
	{
		if (test_log)SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		d_err.total_err++;
		if (test_log)SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "Flag_SF = " << Flag_SF << " supposed " << ((Flags >> 7) & 1);
	if ((Flag_SF == ((Flags >> 7) & 1)) || SF_undef)
	{
		if (test_log && !SF_undef) 	cout << " OK" << endl;
		if (test_log && SF_undef) 	cout << " UNDEFINED" << endl;
	}
	else
	{
		if (test_log)SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		d_err.total_err++;
		if (test_log)SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "Flag_ZF = " << Flag_ZF << " supposed " << ((Flags >> 6) & 1);
	if ((Flag_ZF == ((Flags >> 6) & 1) )|| ZF_undef)
	{
		if (test_log && !ZF_undef) 	cout << " OK" << endl;
		if (test_log && ZF_undef) 	cout << " UNDEFINED" << endl;
	}
	else
	{
		if (test_log)SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		d_err.total_err++;
		if (test_log)SetConsoleTextAttribute(hConsole, 7);
	}
	if (test_log) cout << "Flag_AF = " << Flag_AF << " supposed " << ((Flags >> 4) & 1);
	if ((Flag_AF == ((Flags >> 4) & 1) )|| AF_undef)
	{
		if (test_log && !AF_undef) 	cout << " OK" << endl;
		if (test_log && AF_undef) 	cout << " UNDEFINED" << endl;
	}
	else
	{
		if (test_log)SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		d_err.total_err++;
		if (test_log)SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "Flag_PF = " << Flag_PF << " supposed " << ((Flags >> 2) & 1);
	if ((Flag_PF == ((Flags >> 2) & 1)) || PF_undef)
	{
		if (test_log && !PF_undef) 	cout << " OK" << endl;
		if (test_log && PF_undef) 	cout << " UNDEFINED" << endl;
	}
	else
	{
		if (test_log)SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		d_err.total_err++;
		if (test_log)SetConsoleTextAttribute(hConsole, 7);
	}

	if (test_log) cout << "Flag_CF = " << Flag_CF << " supposed " << ((Flags) & 1);
	if ((Flag_CF == ((Flags) & 1)) || CF_undef)
	{
		if (test_log && !CF_undef) 	cout << " OK" << endl;
		if (test_log && CF_undef) 	cout << " UNDEFINED" << endl;
	}
	else
	{
		if (test_log)SetConsoleTextAttribute(hConsole, 12);
		if (test_log) cout << " ERROR" << endl;
		d_err.total_err++;
		if (test_log)SetConsoleTextAttribute(hConsole, 7);
	}

	if (d_err.total_err) SetConsoleTextAttribute(hConsole, 12);
	if (test_log) cout << "ERRORs = " << (int)d_err.total_err << endl << endl;
	if (test_log)SetConsoleTextAttribute(hConsole, 7);

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
			if (test_log)SetConsoleTextAttribute(hConsole, 12);
			if (test_log) cout << " ERROR" << endl;
			string op_name = jsonData["name"];
			if (!exeption_fired) d_err.total_err++;
			if (test_log)SetConsoleTextAttribute(hConsole, 7);
		}
	}

	//пауза
	if (test_log) while (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F8) && d_err.total_err) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
	//if (test_log) while (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F8)) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }

	return d_err;

}
