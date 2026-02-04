//Emulator of IBM PC/XT with i8088 processor.

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <chrono>
#include <thread>
#include <vector>
#include <conio.h>
#include <bitset>
#include <cmath>
#include <random>
#include <malloc.h>
#include "opcode_functions.h"
#include "custom_classes.h"
#include "joystick.h"
#include "video.h"
#include "audio.h"
#include "keyboard.h"
#include "fdd.h"
#include "hdd.h"
#include "loader.h"

using namespace std;
using namespace std::chrono;

template< typename T >
std::string int_to_hex(T i, int w)
{
	std::stringstream stream;
	stream << ""
		<< std::setfill('0') << std::setw(w)
		<< std::hex << (int)i;
	return stream.str();
}

template< typename T >
std::string int_to_bin(T i)
{
	std::stringstream stream;
	stream << ""
		<< (std::bitset<8>)i;
	return stream.str();
}

void print_mem();
extern void tester();
extern void tester2();
extern void tester3();

string deferred_msg = ""; //отложенные сообщения эмулятора

HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
uint8 global_error = 0; //глобальный код ошибки
bool cont_exec = true; //переменная для продолжения основного цикла. снимается HALT

string path = ""; //текущий каталог

//текстура шрифта
sf::Texture font_texture_40;
sf::Sprite font_sprite_40(font_texture_40);
sf::Texture font_texture_80;
sf::Sprite font_sprite_80(font_texture_80);

//текстуры графической палитры
sf::Texture CGA_320_texture;
sf::Sprite CGA_320_palette_sprite(CGA_320_texture);

//таймеры
//sf::Clock myclock;  //для тестов
//sf::Clock video_clock;
//sf::Clock cpu_clock; //таймер для выравнивания скорости процессора
//sf::Clock kbd_clock;
//sf::Clock speaker_clock;

//счетчики
uint32 op_counter = 0;
uint8 service_counter = 0;

//флаг исключений
uint8 exeption = 0;

Mem_Ctrl memory; //создаем контроллер памяти

uint8 memory_2[1024 * 1024 + 1024 * 1024]; //память 2.0



struct comment
{
	int address;
	string text;
};

//создаем монитор
Video_device monitor;

//второй экран для дебага
#ifdef DEBUG
Dev_mon_device debug_monitor(1000, 1240, "Debug Window", 3840 - 1640 - 1000, 0);
#endif
//окно для сообщений FDD
FDD_mon_device FDD_monitor(1000, 1600, "FDD Window", 0, 0);

//окно для сообщений HDD
FDD_mon_device HDD_monitor(1000, 1600, "HDD Window", 0, 0);

//отладочное окно для звука
#ifdef DEBUG
Audio_mon_device Audio_monitor(2000, 200, "Audio Window", 0, 1650);
#endif
//внутренние счетчики таймера
struct t_counter
{
	uint16 count = 0;
	uint16 initial_count = 0;
	bool latch_on = false;
	uint16 latch_value = 0;
	uint8 mode = 0; // 0 - int on TC, 1 - one-shot, 2 - Rate Gen, 3 - Square wave, 4 - Soft Trigg, 5 - HW trigg
	bool BCD_mode = false;
	uint8 RL_mode = 0;
	bool second_byte = false; //нужно считать/записать второй байт
	bool enabled = false;     // ON/OFF
	bool wait_for_data = false; //ожидание загрузки данных
	bool signal_high = false;  //сигнал на выходе
	bool one_shot_fired = false; //триггер для режима 0
};

//таймер
class IC8253
{
private:
	t_counter counters[4];
	std::chrono::steady_clock::time_point timer_start; //для отслеживания времени выполнения
	std::chrono::steady_clock::time_point timer_end;
	uint32 duration = 0;
public:
	
	void write_port(uint16 port, uint8 data);
	uint8 read_port(uint16 port);
	void sync();
	void enable_timer(uint8 n);
	void disable_timer(uint8 n);
	bool is_count_enabled(uint8 n);
	uint16 get_time(uint8 number);
	string get_ch_data(int channel);
};

//контроллер PPI
class IC8255
{
private:
	bool switches_hign = false;
	uint8 port_B_out = 0;
	bool port_B_6 = false; //уровень вывода 6 порта B
	bool port_B_7 = false; //уровень вывода 7 порта B

public:
	void write_port(uint16 port, uint8 data);
	uint8 read_port(uint16 port);
};

// создаем клавиатуру
KBD keyboard;

// создаем динамик
SoundMaker speaker;

// создаем общий контроллер ввода/вывода (HUB)
IO_Ctrl IO_device;
// создаем таймер
IC8253 pc_timer;
// создаем контроллер портов
IC8255 ppi_ctrl;
// создаем контроллер DMA
IC8237 dma_ctrl;
// создаем контроллер прерываний
IC8259 int_ctrl;
// создаем FDD
FDD_Ctrl FDD_A;
//создаем контроллер виртуального HDD
HDD_Ctrl HDD;

//джойстик
game_controller joystick;

//vector<string> filename_FDD = { "MS-DOS\\tst.img" ,"Demos\\8088mph.ima" ,"MS-DOS\\DOS33_test_2.img","MS-DOS\\DOS33_test_3.img" }; //массив с именами образов дискет

vector<comment> comments; //комментарии к программе
//файлы для загрузки по-умолчанию
//string filename_ROM = "IBM5160 BIOSes\\050986_XT_BIOS.bin";		//bios IBM pc/XT
//string filename_ROM = "IBM5160 BIOSes\\ruuds_test_rom.bin";	//test ROM
//string filename_ROM = "GLABIOS_0.8b0_8XY.ROM";				//GLaBIOS
//string filename_ROM = "IBM5160 BIOSes\\supersoft_test_rom.bin"; //Supersoft test
//string filename_HDD = "MS-DOS\\HDD.img";						// HDD
//string filename_HDD_ROM = "HDD ROMs\\IBM_62x0822.bin";			//HDD board ROM
//string filename_v_rom = "Ega-ibm.bin";							// video ROM EGA
//filename_FDD.push_back("MS-DOS\\tst.img");						// DOS 3.3
//filename_FDD[1] = "Demos\\8088mph.ima";					//нужен второй загрузочный диск
//filename_FDD[2] = "MS-DOS\\DOS33_test_2.img";
//filename_FDD[3] = "MS-DOS\\DOS33_test_3.img";
//string filename_FDD = "MS-DOS\\Disk01.img";   // DOS 3.3
//string filename_FDD = "MS-DOS\\DOS33_test.img";   // игра + демка+ CGA проверка
//string filename_FDD = "MS-DOS\\DOS33_test_5.img";   // демка 8088mph  не работает
//string filename_FDD = "MS-DOS\\Games\\Pacman.img";   // 
//string filename_FDD = "MS-DOS\\Games\\Bdash12.img";   // 
//string filename_FDD = "MS-DOS\\Games\\Montezum.img";   // 
//string filename_FDD = "MS-DOS\\Games\\F15cga.img";   // 
//string filename_FDD = "MS-DOS\\Games\\Galaxian.img";   // 
//string filename_FDD = "MS-DOS\\EXPLORING-THE-IBM-PC-100-CGA.img";


//переключатели на плате
//uint8 MB_switches = 0b01101101; //CGA + 2FDD
uint8 MB_switches = 0b00101101; //CGA
//uint8 MB_switches = 0b00111101; //MDA
//uint8 MB_switches = 0b00001101; //EGA

#ifdef DEBUG
vector<int> breakpoints;                    // точки останова
string tmp_s;
#endif

//регистры процессора
uint16 AX = 0; // AX
uint16 BX = 0; // BX
uint16 CX = 0; // CX
uint16 DX = 0; // DX

//указатели на регистры
uint16* ptr_AX = &AX;
uint16* ptr_BX = &BX;
uint16* ptr_CX = &CX;
uint16* ptr_DX = &DX;

//половинки регистров
uint8* ptr_AL = (uint8*)ptr_AX;
uint8* ptr_AH = ptr_AL + 1;
uint8* ptr_BL = (uint8*)ptr_BX;
uint8* ptr_BH = ptr_BL + 1;
uint8* ptr_CL = (uint8*)ptr_CX;
uint8* ptr_CH = ptr_CL + 1;
uint8* ptr_DL = (uint8*)ptr_DX;
uint8* ptr_DH = ptr_DL + 1;

//таблицы указателей
uint16* ptr_r16[8] = {0};
uint8* ptr_r8[8] = {0};

uint16 Status_Flags = 0;//Flags register
bool Flag_OF = false;	//Overflow flag
bool Flag_DF = false;	//Direction flag
bool Flag_IF = false;	//Int Enable flag
bool Flag_TF = false;	//Trap SS flag
bool Flag_SF = false;	//Signflag
bool Flag_ZF = false;	//Zeroflag
bool Flag_AF = false;	//Aux Carry flag
bool Flag_PF = false;	//Parity flag
bool Flag_CF = false;	//Carry flag

uint16 Stack_Pointer = 0x9000;			 //указатель стека
uint16 Instruction_Pointer = 0xfff0;  // адрес первой команды BIOS

//uint16 Instruction_Pointer = 0xFE6F2;  // адрес первой команды (без учета сегмента!)
uint16 Base_Pointer = 0;
uint16 Source_Index = 0;
uint16 Destination_Index = 0;

//указатели на регистры
uint16* ptr_SP = &Stack_Pointer;
uint16* ptr_BP = &Base_Pointer;
uint16* ptr_SI = &Source_Index;
uint16* ptr_DI = &Destination_Index;

//указатели сегментов
uint16 CS_data = 0;			//Code Segment
uint16 DS_data = 0;			//Data Segment
uint16 SS_data = 0;			//Stack Segment
uint16 ES_data = 0;			//Extra Segment

//указатели на указатели сегментов :)
uint16* CS = &CS_data;		//Code Segment
uint16* DS = &DS_data;		//Data Segment
uint16* SS = &SS_data;		//Stack Segment
uint16* ES = &ES_data;		//Extra Segment

bool Interrupts_enabled = true;//разрешение прерываний

//префикс замены сегмента
uint8 Flag_segment_override = 0;
bool keep_segment_override = false; //сохранить флаг между командами

//аппаратные флаги
bool Flag_hardware_INT = false;  //аппаратное прерывание
bool halt_cpu = 0;				 //флаг остановки до получения прерывания
bool negate_IDIV = false;	     //флаг инверсии частного IDIV после REP(N)
uint8 bus_lock = 0;				 //флаг блокировки шины

//временные регистры
uint16 temp_ACC_16 = 0;
uint8 temp_ACC_8 = 0;
uint16 temp_Addr = 0;

// флаги для изменения работы эмулятора
bool step_mode = false;		//ждать ли нажатия пробела для выполнения команд
bool go_forward;			//переменная для выхода из цикла обработки нажатий
bool log_to_console = false; //логирование команд на консоль
bool log_to_console_FDD = 0; //логирование команд FDD на консоль
bool log_to_console_HDD = 0; //логирование команд HDD на консоль
bool log_to_console_DMA = 0; //логирование команд DMA на консоль
bool log_to_console_INT = 0; //логирование команд INT на консоль
bool log_to_console_DOS = 0; //логирование команд DOS на консоль
bool log_to_console_8087 = 1; //логирование команд 8087 на консоль
bool run_until_CX0 = false; //останов при окончании цикла

bool test_mode = 0; //влияет на память

//string get_sym(int code);

//таблица указателей на функции для выполнения кодов операций
void (*op_code_table[256])() = { 0 };
void (*backup_table[256])() = { 0 };
void (*op_code_table_8087[64])() = { 0 };
//счетчик команд для статистики
int command_counter[256] = { 0 };
bool command_counter_ON = false;

//функция проверки четности
bool parity_check[256] = { 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1 };

bool debug_key_1 = false;
vector <uint16> command_history = { 0 };
vector <uint16> command_history_1 = { 0 };
vector <uint16> command_history_2 = { 0 };

//кол-во циклов для замедления
int empty_cycles = 0;

int main(int argc, char* argv[]) {

	//изначальные значения сегментов
	*CS = 0xf000; //обычный биос
	*DS = 0x40;
	*SS = 0x30;
	*ES = 0x40;

	//точки останова
#ifdef DEBUG
	
	//breakpoints.push_back(0xc81fa); //jmp to 7c00
	//breakpoints.push_back(0xc8003); //IBM HDD CTRL
	//breakpoints.push_back(0xc8127); //IBM HDD CTRL
	//breakpoints.push_back(0xc8251); //HDD INT13

	//breakpoints.push_back(0x7c5f);
	//breakpoints.push_back(0x7ca5);
	//breakpoints.push_back(0xfe3d4);    //KB TEST  POST st 736
	// err 101 st 924
	//breakpoints.push_back(0xe1f5);   //FDD
	//breakpoints.push_back(0xe55b);  //disk test
	//breakpoints.push_back(0xE437);  //disk test
	//DRIVE DET
	
	//breakpoints.push_back(0xFE6F2);
	//breakpoints.push_back(0xfecfe);

	//breakpoints.push_back(0xb88);
	//breakpoints.push_back(0xb8b);
	//breakpoints.push_back(0xb92);
	//breakpoints.push_back(0xb9e);
	//breakpoints.push_back(0xff0ac); // проверка FDD В тесте
	//breakpoints.push_back(0xfee95); // проверка FPU
	//breakpoints.push_back(0xfed38);  //MDA test
	//breakpoints.push_back(0xfec89);  //INT test

	//breakpoints.push_back(0xff505);
	//breakpoints.push_back(0xfea2f);
	//breakpoints.push_back(0xfee78);
	//breakpoints.push_back(0xff729);
	//breakpoints.push_back(0xff744);

	//breakpoints.push_back(0xfa5d);   //basic
	//breakpoints.push_back(0xb544);  //proverka PUSH
	//breakpoints.push_back(0xe609);  //KB ввод символа
	//breakpoints.push_back(0xe2);    //FFD statr
	//breakpoints.push_back(0x0b1b);  //2315 diskette - wait
	//breakpoints.push_back(0xe437);  //POST st 787
	//breakpoints.push_back(0xFFE261);  //CTR Line TEST POST st 576
	//breakpoints.push_back(0xff23);  //int 8
	//breakpoints.push_back(0xe3a3);  //check timer
	
	//breakpoints.push_back(0x0700); //начало IO.SYS
	//breakpoints.push_back(0xFE6F2); --
	//breakpoints.push_back(0x07DC8);  //после начала чтения MSDOS.SYS (2 сектора в 8000)

	//breakpoints.push_back(0x7c00);  // читаем дискету
	//breakpoints.push_back(0xF0718);  // читаем дискету
										 
	//заполняем таблицу комментариев
	//normal BIOS
	
	comments.push_back({ 0xFE0AC,"POST[198]: CPU test complete" });
	comments.push_back({ 0xFE0D7,"POST[224]: ROS CHECKSUM TEST I complete" });
	comments.push_back({ 0xFE166,"POST[343]: DMA INITIALIZATION complete" });
	comments.push_back({ 0xFE1CE,"POST[395]: MEMTEST complete" });
	comments.push_back({ 0xFE1DA,"POST[405]: Interrupt CTRL test START" });
	comments.push_back({ 0xFE20b,"POST[441]: DETERMINE CONFIGURATION" });
	comments.push_back({ 0xFE261,"POST[492]: INITIALIZE AND START CRT CONTROLLER" });
	comments.push_back({ 0xFE2ee,"POST[560]: SETUP VIDEO DATA ON SCREEN" });
	comments.push_back({ 0xfe33d,"POST[615]: ADVANCED VIDEO CARD " });
	comments.push_back({ 0xfe35c,"POST[640]: INTERRUPT CONTROLLER TEST" });
	comments.push_back({ 0xfe38f,"POST[684]: TEST TIMER" });
	comments.push_back({ 0xfe3d4,"POST[729]: KEYBOARD TEST" });
	comments.push_back({ 0xfe40f,"POST[736]: HARDWARE INT.VECTOR TABLE" });
	comments.push_back({ 0xfe437,"POST[787]: HARDWARE INT COPYED" });
	comments.push_back({ 0xfe499,"POST[863]: ADDITIONAL MEMTEST " });
	
	//	comments.push_back({ 0xff859,"POST: INT15" });
	//	comments.push_back({ 0xf1c78,"POST: RET from INT15" });

	//	comments.push_back({ 0xfeca0,"POST[1452]: WAITF" });
	//	comments.push_back({ 0xfecbc,"POST[1475]: RET from WAITF" });
	
	//	comments.push_back({ 0xf0b14,"Disk[2308]: WAIT_INT" });
	//	comments.push_back({ 0xf0b3b,"Disk[]: RET from WAIT_INT" });

	comments.push_back({ 0xf00ad,"DISK[352]: RET from INT13" });
	
	comments.push_back({ 0xf00e2,"Disk[389]: INT13 - Disk_reset" });
	comments.push_back({ 0xf0131,"DISK[432]: RET from Disk_reset" });

	comments.push_back({ 0xf0133,"Disk[437]: Error - @NEC_STATUS(0x442) BAD" });
	
	comments.push_back({ 0xf013a,"Disk[449]: Disk_Status" });

	comments.push_back({ 0xf0146,"Disk[469]: Disk_read" });
	comments.push_back({ 0xf0151,"Disk[472]: RET from Disk_read" });

	comments.push_back({ 0xf040B,"Disk[966]: DR_TYPE_CHECK" });
	comments.push_back({ 0xf0420,"Disk[977]: DSK TYPE NOT VALID" });
	comments.push_back({ 0xf0424,"Disk[980]: TYPE VALID (see in BX)" });
	comments.push_back({ 0xf042A,"Disk[984]: RET from DR_TYPE_CHECK" });
	comments.push_back({ 0xfe5a5,"POST[1027]: DISK ERROR" });

	comments.push_back({ 0xf050D,"Disk[1157]: RD_WR_VF" });
	comments.push_back({ 0xf05BA,"Disk[1263]: RET from RD_WR_VF" });
	comments.push_back({ 0xf0929,"Disk[1350]: MED_CHANGE" });
	comments.push_back({ 0xf0954,"Disk[1387]: RET from MED_CHANGE" });

	comments.push_back({ 0xf06c2,"Disk[1445]: DMA SETUP" });
	comments.push_back({ 0xf0724,"Disk[1504]: RET from DMA SETUP" });

	comments.push_back({ 0xf0725,"Disk[1527]: NEC (FDD) INIT" });
	comments.push_back({ 0xf074a,"Disk[1541]: RET from NEC (FDD) INIT" });

	comments.push_back({ 0xf074b,"Disk[1552]: RWV_COM (read/write common)" });
	comments.push_back({ 0xf0780,"Disk[1574]: RET from RWV_COM (read/write common)" });

	comments.push_back({ 0xf088c,"DISK[1767]: SETUP_END" });
	comments.push_back({ 0xf089f,"DISK[1775]: Error - @DSKETTE_STATUS(0x441) BAD " });
	comments.push_back({ 0xf08a5,"DISK[1780]: RET from SETUP_END" });
	
	comments.push_back({ 0xf08A6,"DISK[1790]: SETUP_DBL" });
	comments.push_back({ 0xf08FE,"DISK[1854]: RET from SETUP_DBL" });
	
	comments.push_back({ 0xf0914,"DISK[1865]: READ ID" });
	comments.push_back({ 0xf0928,"DISK[1875]: RET from READ ID" });

	comments.push_back({ 0xf0958,"DISK[1925]: GET_PARAM" });
	comments.push_back({ 0xf096c,"DISK[1939]: RET from GET_PARAM" });
	comments.push_back({ 0xf096d,"DISK[1961]: MOTOR ON" });
	comments.push_back({ 0xf09B7,"DISK[2008]: RET from MOTOR ON" });
	comments.push_back({ 0xf09b8,"DISK[2021]: TURN ON" });
	comments.push_back({ 0xf0A0D,"DISK[2064]: RET from TURN ON" });
	comments.push_back({ 0xf0A10,"DISK[2069]: RET from TURN ON" });

	comments.push_back({ 0xf0a11,"DISK[2080]: HD_WAIT" });
	comments.push_back({ 0xf0a4a,"Disk[2135]: NEC OUTPUT" });
	comments.push_back({ 0xf0a6d,"Disk[2166]: RET from NEC OUTPUT" });

	comments.push_back({ 0xf0a6e,"DISK[2182]: SEEK" });
	comments.push_back({ 0xf0AD5,"DISK[2242]: RET from SEEK" });

	comments.push_back({ 0xf0AD6,"DISK[2253]: RECALL" });
	comments.push_back({ 0xf0aec,"DISK[2265]: RET from RECALL" });

	comments.push_back({ 0xf0aed,"DISK[2277]: CHK_STAT_2" });
	comments.push_back({ 0xf0B11,"DISK[2297]: CHK_STAT_2 SET @DSKETTE_STATUS(BAD SEEK) in 0x441" });
	comments.push_back({ 0xf0B0B,"DISK[2293]: RET from CHK_STAT_2" });
	
	comments.push_back({ 0xf0b33,"Disk[2325]: Error - no INT" });
	comments.push_back({ 0xf0b3c,"Disk[2341]: RESULTS PROC" });
	comments.push_back({ 0xf0b7a,"Disk[2389]: RET from RESULTS PROC" });
		
	//comments.push_back({ 0xf0b4e,"Disk[2353]: POINT 1 (1100 0000)" });
	//comments.push_back({ 0xf0b5c,"Disk[2362]: POINT 3 (set carry)" });
	//comments.push_back({ 0xf0b6d,"Disk[2377]: POINT 2 (0001 0000)" });
	comments.push_back({ 0xf0b85,"DISK[2415]: DETERMINE DRIVE" });
	
	comments.push_back({ 0xf0be5,"DISK[2486]: DISK SETUP" });

	comments.push_back({ 0xf0be5,"DISK[2487]: DSKETTE_SETUP" });
	comments.push_back({ 0xF0c3b,"DISK[2518]: NEC_STATUS->SI (DSKETTE_SETUP)" });
	comments.push_back({ 0xF0c54,"DISK[2528]: ERR CHESK (DSKETTE_SETUP)" });

	comments.push_back({ 0xf0c60,"DISK[2539]: RET from DSKETTE_SETUP" });
		
	//comments.push_back({ 0xf0118,"Disk[]: POINT 4 CMP" });
	//comments.push_back({ 0xf011a,"Disk[]: POINT 5 CMP" });

	comments.push_back({ 0xFE6F2,"POST[1178]: BOOT STRAP LOADER " });
	comments.push_back({ 0xFE71F,"POST[1210]: BOOT UNABLE. GOTO RESIDENT BASIC (INT18)" });
	comments.push_back({ 0xFE707,"POST[1193]: INT13(LOAD) - RESET DISKETTE" });
	comments.push_back({ 0xFE718,"POST[1201]: INT13(LOAD) - READ SECTOR 1" });
	comments.push_back({ 0xFE71d,"POST[1205]: TRY READ BOOT SECTOR" });
	comments.push_back({ 0xFE71D,"POST[1205]: BOOT ERROR - RETRY" });
	comments.push_back({ 0xFE721,"POST[1215]: BOOT OK - JUMP TO BOOTLOADER" });
	comments.push_back({ 0xFE5BE,"POST[1047]: DISK ERROR" });
	//comments.push_back({ 0xFE5C1,"POST[1052]: SETUP PRINTER AND RS232" });
	comments.push_back({ 0xFE5F6,"POST[1075]: ERROR - 2 BEEPS" });
	//comments.push_back({ 0xFE604,"POST[1081]: WAIT FOR KEY" });
	comments.push_back({ 0xFE614,"POST[1088]: POST OK - 1 BEEP" });
	comments.push_back({ 0xFE621,"POST[1093]: LOOP -> BEGIN" });
	//comments.push_back({ 0xFE66B,"POST[1137]: SET UP EQUIP FLAG TO INDICATE NUMBER OF PRINTERS AND RS232 CARDS" });
	//comments.push_back({ 0xFE686,"POST[1154]: ENABLE NMI INTERRUPTS" });
	comments.push_back({ 0xFE694,"POST[1162]: THE BOOT LOADER (INT19)" });
	
	//comments.push_back({ 0xF0D82,"KBD[281]: KBD Interrupt (IRQ1)" });
	//comments.push_back({ 0xF0c86,"KBD[104]: GET KEY Pressed" });
	//comments.push_back({ 0xF0c61,"KBD[81]: INT16" });
	//comments.push_back({ 0xF0c92,"KBD[112]: K1" });
	//comments.push_back({ 0xF0cff,"KBD[180]: K1S" });
	//comments.push_back({ 0xF0d0c,"KBD[186]: INT15(dummy)" });
	//comments.push_back({ 0xF0d1b,"KBD[195]: key detected from INT" });
	//comments.push_back({ 0xF0d82,"KBD[280]: KB_INT_1" });
	//comments.push_back({ 0xF0d75,"KBD[268]: K4 inc KB buffer" });
	//comments.push_back({ 0xF0cdd,"KBD[158]: K500 (write to buffer) " });
	//comments.push_back({ 0xF1181,"KBD[896]: K61 (put char to buffer) " });
	//comments.push_back({ 0xFa19,"POST[2210]: TIMER_INT_1" });
	//comments.push_back({ 0xFa67,"POST[2252]: TIMER_INT_1 EXIT" });
	//comments.push_back({ 0xFa5d,"POST[]: IN1C user interrupt " });
	//comments.push_back({ 0xFf49,"POST[]: dummy return " });
	//comments.push_back({ 0xd3d,"KBD[]: K10_S_XLAT " });
	//comments.push_back({ 0xd32,"KBD[]: K10_E_XLAT " });
	
#endif

	setlocale(LC_ALL, "Russian");

	//заполняем таблицу функций
	opcode_table_init();
	opcode_8087_table_init(); //8087

	//загружаем конфигурацию и разные файлы
	loader(argc, argv);
	
	//запускаем таймеры
	auto timer_start = steady_clock::now();
	auto timer_end = steady_clock::now();
	uint32 timer_video = 0;
	uint32 timer_kb = 0;
	uint32 timer_speaker = 0;

	//настройка генератора RND для отладки
	std::random_device rd; // Источник энтропии
	std::mt19937 gen(rd()); // Mersenne Twister, seed из random_device
	std::uniform_int_distribution<> distrib(1, 2000);
	
	//предотвращение дребезга клавиш управления
	bool keys_up = true;

	//прогоняем процессорные тесты
	//test_mode = 1;
	//tester3(); cout << "Done " << endl;	while (1);

	//*CS = 0x100;
	//Instruction_Pointer = 0;
	//step_mode = 1;
	//log_to_console = 1;

	cout << "Running..." << hex << endl;
	//основной цикл программы

	while (cont_exec)
	{
		
		
		op_counter++;   //счетчик операций
		service_counter++;  //счетчик для вызова служебных процедур

		//переход в пошаговый режим при попадании в точку останова
#ifdef DEBUG

		for (int b = 0; b < breakpoints.size(); b++)
		{
			if ((Instruction_Pointer + *CS * 16) == breakpoints.at(b))   //breakpoints.at(b)
			{
				step_mode = true;
				cout << "Breakpoint at " << hex << (int)(Instruction_Pointer + *CS * 16) << endl;
				log_to_console = true;
				service_counter = 1;
				run_until_CX0 = false;
			}
		}

#endif

		//переход в пошаговый режим при CX=0
#ifdef DEBUG

		if ((CX == 0) && run_until_CX0)
		{
			//cout << "loop run OFF" << endl;
			run_until_CX0 = false;
			step_mode = true;
			log_to_console = true;
		}

#endif

		//служебные подпрограммы

		if (!service_counter || step_mode || log_to_console)
		{
			timer_end = steady_clock::now(); //останавливаем таймер
			uint32 duration = duration_cast<microseconds>(timer_end - timer_start).count();
			timer_video += duration;	//миллисекунды
			timer_kb += duration;		//миллисекунды
			timer_speaker += duration;

			//корректировка скорости в реалтайме
			if (!step_mode && !log_to_console)
			{
				//if (duration < 250 && empty_cycles < 8000) empty_cycles += 100;
				//if (duration > 260 && empty_cycles > 1000) empty_cycles -= 100;
			}


			//отрисовка экрана монитора
			if (timer_video > 16700) //16700
			{
				monitor.sync(timer_video); //синхроимпульс для монитора
#ifdef DEBUG
				debug_monitor.sync(timer_video); //синхроимпульс для монитора отладки
				FDD_monitor.sync();				//синхроимпульс для монитора FDD
				HDD_monitor.sync();				//синхроимпульс для монитора HDD
				Audio_monitor.sync();			//мониторинг звука
#endif
				timer_video = 0;
				op_counter = 0;
			}

			//опрос клавиатуры
			if (timer_kb > 8350)
			{
				keyboard.poll_keys(timer_kb);    //синхронизация клавиатуры (проверка нажатий)
				HDD.sync_data(timer_kb);		//синхронизация буфера HDD
				joystick.sync(timer_kb);
				timer_kb = 0;
			}

			//обновление спикера (генерация тона)
			if (timer_speaker > 50000)
			{
				//speaker.sync();		//DEL
				timer_speaker = 0;
			}

			//service_counter = 0;
			go_forward = false;

			//борьба с залипанием
			if (!(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F5) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) &&
				!(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F8) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) &&
				!(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F9) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) &&
				!(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F10) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) &&
				!(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F6) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) &&
				!(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F7) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl))) keys_up = true;

			//мониторинг нажатия клавиш в обычном режиме
#ifdef DEBUG
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F9) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) && keys_up) { step_mode = !step_mode; keys_up = false; }

			//включаем пропуск цикла
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F5) && keys_up && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl))
			{
				//cout << "LOOP ON 1" << endl;
				run_until_CX0 = true;
				step_mode = false;
				log_to_console = false;
				keys_up = false;
			}

			//проверяем нажатие F10
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F10) && keys_up && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) { log_to_console = !log_to_console; keys_up = false; }
#endif
#ifdef DEBUG

			//задержка вывода по нажатию кнопки в пошаговом режиме
			while (!go_forward && step_mode)
			{
				if (!(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F5) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) &&
					!(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F8) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) &&
					!(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F9) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) &&
					!(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F10) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) &&
					!(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F6) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) &&
					!(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F7) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl))) keys_up = true;


				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F6) && keys_up && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl))
				{
					for (int a = 0; a < 0x10; a++)
					{
						cout << "sector " << (int)(a + 3);
						uint16 err = 0;
						for (int b = 0; b < 0x200; b++)
						{
							if (memory_2[0x600 + a * 512 + b] != FDD_A.sector_data[0x1600 + a * 512 + b]) err++;
						}
						cout << " errors " << (int)err << endl;
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(500));
				}



				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F9) && keys_up && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) { step_mode = !step_mode; keys_up = false; }
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F5) && keys_up && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl))
				{
					//cout << "LOOP ON 2" << endl;
					run_until_CX0 = true;
					step_mode = false;
					log_to_console = false;
					keys_up = false;
				}
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F8) && keys_up && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) { go_forward = true; }
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F10) && keys_up && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) { log_to_console = !log_to_console; keys_up = false; }

				if (!step_mode) {
					go_forward = true;
					break;
				}
				else
				{
					monitor.sync(0);		//синхроимпульс для монитора
					debug_monitor.sync(0);	//окно отладки
					FDD_monitor.sync();
					HDD_monitor.sync();
				}
			};
#endif			
			timer_start = steady_clock::now();//перезапускаем таймер
		}
		else
		{
			for (int r = 0; r < empty_cycles; r++); //замедление
		}


		pc_timer.sync();	//синхронизация таймера
		//dma_ctrl.sync();
		pc_timer.sync();
		//pc_timer.sync();
		//pc_timer.sync();
		//pc_timer.sync();

		//if (!(service_counter & 3)) FDD_A.sync();
		FDD_A.sync();
		HDD.sync();
		dma_ctrl.sync();	//синхронизация DMA
		keyboard.sync(); 	//синхронизация клавиатуры


		//замедление работы в пошаговом режиме
		if (step_mode) std::this_thread::sleep_for(std::chrono::milliseconds(50));

		//основной цикл 

		//проверка аппаратных прерываний
		if (Flag_IF)
		{
			uint8 hardware_int = int_ctrl.next_int();
			if (hardware_int < 8)
			{
				//cout << "HW INT " << (int)hardware_int << endl;
				//if (halt_cpu) cout << "HW irq " << (int)hardware_int << endl;
				//выполняем аппаратное прерывание, меняя IP
				//step_mode = true; log_to_console = true;
				Stack_Pointer--;
				memory.write_2(Stack_Pointer + SS_data * 16, 0xF0 | (Flag_OF * 8) | (Flag_DF * 4) | (Flag_IF * 2) | Flag_TF);
				Stack_Pointer--;
				memory.write_2(Stack_Pointer + SS_data * 16, 0x2 | (Flag_SF * 128) | (Flag_ZF * 64) | (Flag_AF * 16) | (Flag_PF * 4) | (Flag_CF));

				//помещаем в стек сегмент
				Stack_Pointer--;
				memory.write_2(Stack_Pointer + SS_data * 16, *CS >> 8);
				Stack_Pointer--;
				memory.write_2(Stack_Pointer + SS_data * 16, *CS & 255);

				//помещаем в стек IP
				Stack_Pointer--;
				memory.write_2(Stack_Pointer + SS_data * 16, (Instruction_Pointer) >> 8);
				Stack_Pointer--;
				memory.write_2(Stack_Pointer + SS_data * 16, (Instruction_Pointer) & 255);
				
				//определяем новый IP и CS
				Instruction_Pointer = memory.read_2((hardware_int + 8) * 4) + memory.read_2((hardware_int + 8) * 4 + 1) * 256;
				*CS = memory.read_2((hardware_int + 8) * 4 + 2) + memory.read_2((hardware_int + 8) * 4 + 3) * 256;

				Flag_IF = false;//запрет внешних прерываний
				Flag_TF = false;
				halt_cpu = false; //выход из останова
#ifdef DEBUG
				if (log_to_console) SetConsoleTextAttribute(hConsole, 10);
				//cout << endl << "HARD IRQ " << (int)hardware_int << "(INT #" << (int)(hardware_int + 8) << ") AX(" << (int)AX << ") -> " << (int)*CS << ":" << (int)Instruction_Pointer << endl;
				if (log_to_console) cout << endl << "HARD IRQ " << (int)hardware_int << "(INT #" << (int)(hardware_int + 8) << ") AX(" << (int)AX << ") -> " << (int)*CS << ":" << (int)Instruction_Pointer << endl;
				if (log_to_console) SetConsoleTextAttribute(hConsole, 7);
#endif
			}
		}

#ifdef DEBUG
		
		
		//комментарии к точкам БИОС
		/*
		for (int j = 0; j < comments.size(); j++)
		{
			if (comments.at(j).address == Instruction_Pointer + (*CS) * 16)
			{
				SetConsoleTextAttribute(hConsole, 14);
				string sss = comments.at(j).text; 
				cout << sss << endl;
				SetConsoleTextAttribute(hConsole, 7);
				break;
			}
		}
		*/
#endif
		
		if (halt_cpu) goto halt_jump; //перепрыгиваем инструкции в состоянии HALT

	cmd_rep:
#ifdef DEBUG
		if (log_to_console) 
		{
			cout << hex;
			//cout << int_to_hex(memory.read_2(0x441, 0), 2) << "  " << int_to_hex(memory.read_2(0x442, 0), 2) << " ";
			cout << std::setw(4) << *CS << ":" << std::setfill('0') << std::setw(4) << Instruction_Pointer << "  " <<
				std::setfill('0') << std::setw(2) << (int)memory_2[Instruction_Pointer + *CS * 16] << "  " <<
				std::setfill('0') << std::setw(2) << (int)memory_2[(Instruction_Pointer + 1) + *CS * 16] << "  " <<
				std::setfill('0') << std::setw(2) << (int)memory_2[(Instruction_Pointer + 2) + *CS * 16] << "  " <<
				std::setfill('0') << std::setw(2) << (int)memory_2[(Instruction_Pointer + 3) + *CS * 16] << "  " <<
				std::setfill('0') << std::setw(2) << (int)memory_2[(Instruction_Pointer + 4) + *CS * 16] << "  " <<
				std::setfill('0') << std::setw(2) << (int)memory_2[(Instruction_Pointer + 5) + *CS * 16] << "\t";
		}
#endif
		//uint8 l_code_1 = memory_2[Instruction_Pointer + *CS * 16];
		//uint8 l_code_2 = memory_2[Instruction_Pointer + 1 + *CS * 16];
		//uint16 IP_backup = Instruction_Pointer;
		if (command_counter_ON) command_counter[memory_2[Instruction_Pointer + *CS * 16]]++;
		//исполнение команды
		op_code_table[memory_2[Instruction_Pointer + *CS * 16]]();

		if (keep_segment_override) { 
			keep_segment_override = false; 
			goto cmd_rep; //выполняем еще одну команду
		} //сбрасываем флаг сохранения
		else { Flag_segment_override = 0; } //сбрасываем флаг смены сегмента

		//если установлен флаг negate_IDIV - выполняем еще одну команду
		if (negate_IDIV)
		{
			if (log_to_console) cout << endl;
			goto cmd_rep;
		}



#ifdef DEBUG	
		if (log_to_console) cout << "\t ZF=" << Flag_ZF << " CF=" << Flag_CF << " AF=" << Flag_AF << " SF=" << Flag_SF << " PF=" << Flag_PF << " OF=" << Flag_OF << " IF=" << Flag_IF;
		if (log_to_console) cout << endl;

		//вывод отложенного сообщения
		if (deferred_msg != "")
		{
			if (log_to_console) cout << deferred_msg << endl;
			deferred_msg = "";
		}
#endif				
		if (bus_lock == 2) bus_lock = 1;
		else
		{
			if (bus_lock == 1) bus_lock = 0;
		}

		//обрабока исключений
		if (exeption)
		{
			exeption -= 0x10; //убираем префикс исключения
			//помещаем в стек IP и переходим по адресу прерывания
			//новые адреса
			uint16 new_IP = memory.read_2(exeption * 4) + memory.read_2(exeption * 4 + 1) * 256;
			uint16 new_CS = memory.read_2(exeption * 4 + 2) + memory.read_2(exeption * 4 + 3) * 256;

			if (log_to_console || 1) SetConsoleTextAttribute(hConsole, 10);
			if (log_to_console || 1) cout << "EXEPTION " << (int)exeption << " jump to " << int_to_hex(new_CS, 4) << ":" << int_to_hex(new_IP, 4) << " ret to " << int_to_hex(*CS, 4) << ":" << int_to_hex(Instruction_Pointer + 2, 4) << endl;
			if (log_to_console || 1) SetConsoleTextAttribute(hConsole, 7);

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
			Instruction_Pointer = new_IP;
			exeption = 0; //сброс флага
		}

		//обработка Trap Flag
		if (Flag_TF)
		{
			exeption = 0x11; //прерывание 1
			cout << "It is a Trap!" << endl;
			Flag_TF = 0;
		}

halt_jump:
		//уменьшаем таймер сна контроллера прерываний после выполнения команды
		if (int_ctrl.sleep_timer) int_ctrl.sleep_timer--;

		continue;
	}
	monitor.sync(1);
	cout << "Program ended. Press SPACE" << endl;
	while (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) std::this_thread::sleep_for(std::chrono::milliseconds(100));
	return 0;
}

//контроллер памяти
void Mem_Ctrl::write_2(uint32 address, uint8 data) //запись значений в ячейки
{
	//if ((address & 0xF0000) == 0xF0000) return; //проверка на попадание в диапазон ПЗУ
	memory_2[address & 0xFFFFF] = data;
	return;
}
uint8 Mem_Ctrl::read_2(uint32 address) //чтение данных из памяти
{
	return memory_2[address & 0xFFFFF];
}

//контроллер ввода-вывода
void IO_Ctrl::output_to_port_8(uint16 address, uint8 data)	//вывод в порт, w - ширина=16
{
	//timer
	if (address >= 0x40 && address <= 0x43)
	{
		//обращение к таймеру
		pc_timer.write_port(address, data);
	}
	//DMA controller
	if (address >= 0x0 && address <= 0x1F)
	{
		return dma_ctrl.write_port(address, data);
	}
	//DMA registers
	if (address >= 0x80 && address <= 0x9F)
	{
		return dma_ctrl.write_port(address, data);
	}

	//PPI controller
	if (address >= 0x60 && address <= 0x63)
	{
		//обращение PPI
		return ppi_ctrl.write_port(address, data);
	}

	//INT controller
	if (address >= 0x20 && address <= 0x3F)
	{
		//обращение к контроллеру
		int_ctrl.write_port(address, data);
	}

	//port to control NMI
	if (address == 0xA0)
	{
		if (data == 0)
		{
			int_ctrl.NMI_enabled = false;
			//deferred_msg = "NMI disabled";
		}
		if (data == 0x80)
		{
			int_ctrl.NMI_enabled = true;
			//deferred_msg = "NMI enabled";
		}
	}

	//MDA, Hercules VIDEO ADAPTER
	if (address == 0x3B4)
	{
		//deferred_msg = "MDA, Hercules index port (0x3B4) write -> 0x" + int_to_hex(data, 2);
	}
	if (address == 0x3B5)
	{
		//deferred_msg = "MDA, Hercules data port (0x3B5) write -> 0x" + int_to_hex(data, 2);
	}

	//Monochrome
	if (address == 0xb4 || address == 0xb5)
	{
		//deferred_msg = "Monochrome Video port (0xb4 / 0xb5) write -> 0x" + int_to_hex(data, 2);
	}

	//CGA VIDEO ADAPTER
	if (address >= 0x3D0 && address <= 0x3DF)
	{
		monitor.write_port(address, data);
		//deferred_msg = "CGA port ("+ int_to_hex(address, 4) + ") write -> 0x" + int_to_hex(data, 2);
	}

	//FDD_A
	if (address >= 0x3F0 && address <= 0x3F7)
	{
		FDD_A.write_port(address, data);
		//deferred_msg = "-> CGA port -> 0x" + int_to_hex(data, 2);
	}

	//Джойстик
	if (address == 0x201)
	{
		//deferred_msg = "-> JOYSTICK port -> 0x" + int_to_hex(data, 2);
		joystick.start_meazure();
	}

	//HDD
	if (address >= 0x320 && address <= 0x327)
	{
		HDD.write_port(address, data);
		//deferred_msg = "write HDD port 0x" + int_to_hex(address, 4); + "-> 0x" + int_to_hex(data, 2);
	}

}
void IO_Ctrl::output_to_port_16(uint16 address, uint16 data)
{



}
uint8 IO_Ctrl::input_from_port_8(uint16 address)				//ввод из порта, w - ширина
{
	if (test_mode) return 0xFF;
	
	//timer
	if (address >= 0x40 && address <= 0x43)
	{
		//обращение к таймеру
		return pc_timer.read_port(address & 255);
	}
	//dma controller
	if (address >= 0 && address <= 0x1F)
	{
		//обращение к DMA
		
		return dma_ctrl.read_port(address & 255);
	}
	//PPI controller
	
	if (address >= 0x60 && address <= 0x63)
	{
		//обращение PPI
		return ppi_ctrl.read_port(address & 255);
	}
	//INT controller
	if (address >= 0x20 && address <= 0x3F)
	{
		//обращение к контроллеру
		return int_ctrl.read_port(address & 255);
	}

	//CGA VIDEO ADAPTER
	if (address >= 0x3D0 && address <= 0x3DF)
	{
		return monitor.read_port(address);
	}

	//FDD
	if (address >= 0x3F0 && address <= 0x3F7)
	{
		return FDD_A.read_port(address);
	}

	//Джойстик
	if (address == 0x201)
	{
		return joystick.get_input();
	}
	
	//HDD
	if (address >= 0x320 && address <= 0x327)
	{
		uint8 d = HDD.read_port(address);
		deferred_msg = "READ HDD port 0x" + int_to_hex(address, 4) + " <- 0x" + int_to_hex(d, 2);
		//cout << deferred_msg << endl;
		return d;
	}

	return 0;
}
uint16 IO_Ctrl::input_from_port_16(uint16 address)
{
	if (test_mode) return 0xFFFF;
	//копия 8-битной функции
	
	//timer
	if (address >= 0x40 && address <= 0x43)
	{
		//обращение к таймеру
		return pc_timer.read_port(address & 255);
	}
	//dma controller
	if (address >= 0 && address <= 0x1F)
	{
		//обращение к DMA
		
		return dma_ctrl.read_port(address & 255);
	}
	//PPI controller

	if (address >= 0x60 && address <= 0x63)
	{
		//обращение PPI
		return ppi_ctrl.read_port(address & 255);
	}
	//INT controller
	if (address >= 0x20 && address <= 0x3F)
	{
		//обращение к контроллеру
		return int_ctrl.read_port(address & 255);
	}

	//CGA VIDEO ADAPTER
	if (address >= 0x3D0 && address <= 0x3DF)
	{
		return monitor.read_port(address);
	}

	//FDD
	if (address >= 0x3F0 && address <= 0x3F7)
	{
		return FDD_A.read_port(address);
	}
	

	return 0;
}

//Timer
uint8 IC8253::read_port(uint16 port)
{
	if (port < 0x40 || port > 0x42) return 0;
	uint8 n = port - 0x40; //номер счетчика
	
		if (counters[n].latch_on)   //запомненное значение
		{ 
			counters[n].latch_on = false;
			//cout << "read timer " << (int)n << " = " << counters[0].latch_value << endl;

			switch (counters[n].RL_mode)
			{
			case 1:   // 1 - Only MSB
				return counters[n].latch_value >> 8;
				break;
			case 2:   // 2 - Only LSB
				return (counters[n].latch_value & 255) | (int)floor(rand() / RAND_MAX + 0.5);
				break;
			case 3:   // 3 - LSB then MSB
				if (!counters[n].second_byte)
				{
					counters[n].second_byte = true;
					return (counters[n].latch_value & 255) | (int)floor(rand() / RAND_MAX + 0.5); //LSB
				}
				else
				{
					counters[n].second_byte = false;
					return counters[n].latch_value >> 8; //MSB
				}
			}
			
		}
		else  //текущее значение
		{
			//cout << "read timer 0 " << counters[0].count << endl;
			switch (counters[n].RL_mode)
			{
			case 1:   // 1 - Only MSB
				return counters[n].count >> 8;
				break;
			case 2:   // 2 - Only LSB
				return (counters[n].count & 255) | (int)floor(rand() / RAND_MAX + 0.5);
				break;
			case 3:   // 3 - LSB then MSB
				if (!counters[n].second_byte)
				{
					counters[n].count = true;
					return (counters[n].count & 255) | (int)floor(rand()/RAND_MAX + 0.5); //LSB
				}
				else
				{
					counters[n].second_byte = false;
					return counters[n].count >> 8; //MSB
				}
			}
			
		}
		return 0; //заглушка
}
void IC8253::sync()
{
	// режимы работы
	// 0 - int on TC, 1 - one - shot, 2 - Rate Gen, 3 - Square wave, 4 - Soft Trigg, 5 - HW trigg
	//синхронизация таймеров, счет
	for (int n = 0; n < 3; n++)
	{
		if (counters[n].enabled)
		{
			if (counters[n].mode == 0)
			{
				//единичный отсчет
				counters[n].count--;
				if ((counters[n].count) == 0) counters[n].signal_high = true;
			}
			if (counters[n].mode == 1)
			{
				//синхроимпульсы c инверсией
				counters[n].count--;
				if ((counters[n].count) == 0) counters[n].signal_high = false;
				else counters[n].signal_high = false;
			}

			if (counters[n].mode == 2)
			{
				//синхроимпульсы
				counters[n].count--;
				if ((counters[n].count) == 0) 
				{
					counters[n].signal_high = false;
					//перезагрузка счетчика
					counters[n].count = counters[n].initial_count;

				}
				else counters[n].signal_high = true;
				
				if (n == 2) //передача уровня сигнала в спикер
				{
					if (counters[n].signal_high) speaker.put_sample(1);
					else speaker.put_sample(-1);
				}

				if (n == 0) // прерывание таймера
				{
					if (!counters[n].signal_high) int_ctrl.request_IRQ(0);
				}

			}
			if (counters[n].mode == 3)
			{
				//квадратный сигнал
				if (counters[n].count == 0) counters[n].count = counters[n].initial_count;
				counters[n].count--;
				if (counters[n].count > (counters[n].initial_count >> 1)) counters[n].signal_high = true;
				else counters[n].signal_high = false;
				/*
				if (counters[n].signal_high == true)
				{
					if (counters[n].count < counters[n].initial_count/2) counters[n].signal_high = false;
				}
				*/
				if (n == 2) //передача уровня сигнала в спикер
				{
					if (counters[n].signal_high) speaker.put_sample(1);
					else speaker.put_sample(-1);
				}
				
			}
		}
	}
		
	//прерывания таймера
	if (counters[0].enabled && !counters[0].wait_for_data)  //Timer 0
	{
		if (counters[0].mode == 0 && counters[0].signal_high &&  !counters[0].one_shot_fired)
		{
			//при высоком сигнале вызываем прерывание 0
			//cout << "IRQ  t1" << endl;
			int_ctrl.request_IRQ(0);
			int_ctrl.set_timeout(5);
			counters[0].one_shot_fired = true; //срабатываение 1 раз
		}

		if (counters[0].mode == 3 && counters[0].count == 0)
		{
			//при нуле вызываем IRQ
			//if (halt_cpu) cout << " timer IRO 0 ";
			//cout << "IRQ  t2" << endl;
			int_ctrl.request_IRQ(0);
			int_ctrl.set_timeout(5);
		}
		
		if (counters[0].mode == 2 && counters[0].count == 0)
		{
			//при нуле вызываем IRQ
			//if (halt_cpu) cout << " timer IRO 0 ";
			//cout << "IRQ  t2" << endl;
			int_ctrl.request_IRQ(0);
			int_ctrl.set_timeout(5);
		}

	}

	//refresh DRAM
	if (counters[1].enabled) //Timer 1
	{
		if (counters[1].signal_high == 0)
		{
			//при нуле вызываем DMA 0
			dma_ctrl.request_hw_dma(0);
		}
	}
	
	if (counters[2].enabled)
	{
		//место для счетчика 2

	}
	
	//отдельный таймер для синхронизации реального времени
	counters[3].count--;
	if (counters[3].count == 0)
	{
		//синхронизируем скорость
		timer_end = chrono::steady_clock::now(); //считываем время
		duration = chrono::duration_cast<chrono::microseconds>(timer_end - timer_start).count();
		timer_start = chrono::steady_clock::now(); //засекаем заново
		if (duration > 65000) empty_cycles -= 50;
		if (duration > 55000) empty_cycles -= 5;
		if (duration < 44000) empty_cycles += 50;
		if (duration < 54000) empty_cycles += 5;
		//cout << (int)empty_cycles << "  ";
	}
}
void IC8253::write_port(uint16 port, uint8 data)
{
	//cout << "port = " << (int)port << " data = " << (int)data << endl;
	if (port >= 0x40 && port <= 0x42)
	{
		uint8 n = port - 0x40;
		//определяем порядок загрузки байтов
		//	1 - Only LSB
		//  2 - Only MSB
		//  3 - LSB then MSB
		switch (counters[n].RL_mode)
		{
		case 1:
			counters[n].initial_count = counters[n].count = data;
			counters[n].enabled = true;
			if (counters[n].mode == 0) counters[n].signal_high = false; //сброс выхода при перезагрузке
			counters[n].wait_for_data = false; //отмена ожидания загрузки
			if (n == 2) speaker.timer_freq = 1193000 / counters[n].initial_count; //если это порт №2 считаем частоту звука
			break;
		case 2:
			counters[n].initial_count = counters[n].count = data * 256;
			counters[n].enabled = true;
			if (counters[n].mode == 0) counters[n].signal_high = false; //сброс выхода при перезагрузке
			counters[n].wait_for_data = false; //отмена ожидания загрузки
			if (n == 2) speaker.timer_freq = 1193000 / counters[n].initial_count; //если это порт №2 считаем частоту звука
			break;
		case 3:
			if (!counters[n].second_byte)
			{
				counters[n].count = (counters[n].count & 0xFF00) | data;
				counters[n].second_byte = true;
				counters[n].enabled = false;  //stop
				//cout << "LB = " << hex << (int)data << endl;
			}
			else
			{
				//старший байт
				counters[n].count = (counters[n].count & 0xFF) | (data * 256);
				counters[n].initial_count = counters[n].count;
				counters[n].second_byte = false;
				counters[n].enabled = true;  //start
				if (counters[n].mode == 0) counters[n].signal_high = false; // //сброс выхода при перезагрузке
				counters[n].wait_for_data = false; //отмена ожидания загрузки
				//cout << "HB = " << hex << (int)data << endl;
				if (n == 2 && counters[n].initial_count) speaker.timer_freq = 1193000 / counters[n].initial_count; //если это порт №2 рассчитываем частоту звука
			}
		}
		//if (counters[n].mode == 0) counters[n].enabled;
		//deferred_msg = "counter " + to_string(n) + " set: count = " + to_string(counters[n].count);
		//cout << "counter " << to_string(n) << " set: count = " << to_string(counters[n].count) << endl;
	}
	
	//запись контрольного слова
	if (port == 0x43)
	{
		//вводим управляющий байт
		int timer_N = data >> 6; //номер таймера
		int RL = (data >> 4) & 3;  //00010000
		int mode = (data >> 1) & 7;
		int BCD_mode = data & 1;
		//deferred_msg = "set timer " + to_string(timer_N);
		//cout << "set timer " << to_string(timer_N) << endl;

		if (RL == 0) 
		{
			//фиксируем текущее значение счетчика
			counters[timer_N].latch_on = 1;
			counters[timer_N].latch_value = counters[timer_N].count;
			//deferred_msg = "counter " + to_string(timer_N) + " latch rq (" + to_string(counters[timer_N].latch_value) + ")";
			//cout << "counter " << to_string(timer_N) << " latch rq (" << to_string(counters[timer_N].latch_value) << ")" << endl;
		}
		else
		{
			//меняем режим чтения/записи значений
			counters[timer_N].RL_mode = RL;
			//	1 - Only LSB
			//  2 - Only MSB
			//  3 - LSB then MSB

			//режимы от 0 до 5
			if (mode == 0)
			{
				counters[timer_N].one_shot_fired = false; //переустанавливаем триггер для режима 0
				counters[timer_N].wait_for_data = true;   //ждем загрузки данных
			}
			
			counters[timer_N].mode = mode;
			if (mode == 6) counters[timer_N].mode = 2;
			if (mode == 7) counters[timer_N].mode = 3;

			counters[timer_N].BCD_mode = BCD_mode; // 1 - считаем в BCD
			
			//deferred_msg = "counter " + to_string(timer_N) + " set: RL = " + to_string(RL) + " mode = " + to_string(counters[timer_N].mode);
			//cout << "counter " << to_string(timer_N) << " set: RL = " << to_string(RL) << " mode = " << to_string(counters[timer_N].mode) << endl;
		}
	}
}
void IC8253::enable_timer(uint8 n)
{
	//if (n < 3) counters[n].enabled = 1;
}
void IC8253::disable_timer(uint8 n)
{
	//if (n < 3) counters[n].enabled = 0;
}
bool IC8253::is_count_enabled(uint8 n)
{
	if (n < 3)
	{
		if (counters[n].enabled) return true;
	}
	return false;
}
uint16 IC8253::get_time(uint8 number)
{
	if (number < 3)	return counters[number].count;
	else return 0;
}
string IC8253::get_ch_data(int channel)
{
	if (channel > 2) return ""; //ошибка
	string out;
	if (counters[channel].enabled) out += "ON   ";
	else out += "OFF  ";
	out += int_to_hex(counters[channel].count, 4) + " ";
	out += int_to_hex(counters[channel].initial_count, 4) + "   ";
	out += to_string(counters[channel].mode) + "  ";
	if (counters[channel].BCD_mode) out += "ON   ";
	else out += "OFF  ";
	out += to_string(counters[channel].RL_mode) + "  ";
	if (counters[channel].signal_high) out += "HIGH ";
	else out += "LOW  ";

	return out;
}

//DMA controller
void IC8237::write_port(uint16 port, uint8 data)
{
	
	switch(port)
	{
	//запись параметров по отдельным каналам
	/*
		0 - текущий адрес канала 0
		1 - текущий счетчик канала 0
		2 - текущий адрес канала 1
		3 - текущий счетчик канала 1
		4 - текущий адрес канала 2
		5 - текущий счетчик канала 2
		6 - текущий адрес канала 3
		7 - текущий счетчик канала 3
		8 - регистр команд
		9 - запрос на DMA
		10 - установить бит маскирования
		11 - установить режим записи
		12 - очистка битов выбора разряда (flip/flops)
		13 - общая очистка регистров
		15 - очистка регистров маскирования
	*/
	case 0:
		//current address
		if (cha_attribute[0].flip_flop)
		{
			cha_attribute[0].curr_address = cha_attribute[0].curr_address & (0x00FF) | (data << 8);
			cha_attribute[0].base_address = cha_attribute[0].curr_address & (0x00FF) | (data << 8);
		}
		else 
		{ 
			cha_attribute[0].curr_address = cha_attribute[0].curr_address & (0xFF00) | (data); 
			cha_attribute[0].base_address = cha_attribute[0].curr_address & (0xFF00) | (data);
		}
		cha_attribute[0].flip_flop = !cha_attribute[0].flip_flop;
		if (log_to_console_DMA) cout << "write DMA 0 curr addr = " << to_string(cha_attribute[0].curr_address) << endl;
		break;
	case 1:
		//current world count
		if (cha_attribute[0].flip_flop)
		{
			cha_attribute[0].word_count = cha_attribute[0].word_count & (0x00FF) | (data << 8);
			cha_attribute[0].base_word_count = cha_attribute[0].word_count & (0x00FF) | (data << 8);
		}
		else
		{
			cha_attribute[0].word_count = cha_attribute[0].word_count & (0xFF00) | (data);
			cha_attribute[0].base_word_count = cha_attribute[0].word_count & (0xFF00) | (data);
		}
		cha_attribute[0].flip_flop = !cha_attribute[0].flip_flop;
		if (log_to_console_DMA) cout << "write DMA 0 world count = " << to_string(cha_attribute[0].word_count) << endl;
		break;
	case 2:
		//current address
		if (cha_attribute[1].flip_flop) 
		{ 
			cha_attribute[1].curr_address = cha_attribute[1].curr_address & (0x00FF) | (data << 8);
			cha_attribute[1].base_address = cha_attribute[1].curr_address & (0x00FF) | (data << 8);
		}
		else
		{
			cha_attribute[1].curr_address = cha_attribute[1].curr_address & (0xFF00) | (data);
			cha_attribute[1].base_address = cha_attribute[1].curr_address & (0xFF00) | (data);
		}
		cha_attribute[1].flip_flop = !cha_attribute[1].flip_flop;
		if (log_to_console_DMA) cout << "write DMA 1 curr addr = " << to_string(cha_attribute[1].curr_address) << endl;
		break;
	case 3:
		//current world count
		if (cha_attribute[1].flip_flop)
		{
			cha_attribute[1].word_count = cha_attribute[1].word_count & (0x00FF) | (data << 8);
			cha_attribute[1].base_word_count = cha_attribute[1].word_count & (0x00FF) | (data << 8);
		}
		else
		{
			cha_attribute[1].word_count = cha_attribute[1].word_count & (0xFF00) | (data);
			cha_attribute[1].base_word_count = cha_attribute[1].word_count & (0xFF00) | (data);
		}
		cha_attribute[1].flip_flop = !cha_attribute[1].flip_flop;
		if (log_to_console_DMA) cout << "write DMA 1 world count = " << to_string(cha_attribute[1].word_count) << endl;
		break;
	case 4:
		//current address
		if (cha_attribute[2].flip_flop)
		{
			cha_attribute[2].curr_address = cha_attribute[2].curr_address & (0x00FF) | (data << 8);
			cha_attribute[2].base_address = cha_attribute[2].curr_address & (0x00FF) | (data << 8);
		}
		else
		{
			cha_attribute[2].curr_address = cha_attribute[2].curr_address & (0xFF00) | (data);
			cha_attribute[2].base_address = cha_attribute[2].curr_address & (0xFF00) | (data);
		}
		cha_attribute[2].flip_flop = !cha_attribute[2].flip_flop;
		if (log_to_console_DMA) cout << "write DMA 2 curr addr = " << to_string(cha_attribute[2].curr_address) << endl;
		break;
	case 5:
		//current world count
		if (cha_attribute[2].flip_flop)
		{
			cha_attribute[2].word_count = cha_attribute[2].word_count & (0x00FF) | (data << 8);
			cha_attribute[2].base_word_count = cha_attribute[2].word_count & (0x00FF) | (data << 8);
		}
		else
		{
			cha_attribute[2].word_count = cha_attribute[2].word_count & (0xFF00) | (data);
			cha_attribute[2].base_word_count = cha_attribute[2].word_count & (0xFF00) | (data);
		}
		cha_attribute[2].flip_flop = !cha_attribute[2].flip_flop;
		if (log_to_console_DMA) cout << "write DMA 2 world count = " << to_string(cha_attribute[2].word_count) << endl;
		break;
	case 6:
		//current address
		if (cha_attribute[3].flip_flop)
		{
			cha_attribute[3].curr_address = cha_attribute[3].curr_address & (0x00FF) | (data << 8);
			cha_attribute[3].base_address = cha_attribute[3].curr_address & (0x00FF) | (data << 8);
		}
		else
		{
			cha_attribute[3].curr_address = cha_attribute[3].curr_address & (0xFF00) | (data);
			cha_attribute[3].base_address = cha_attribute[3].curr_address & (0xFF00) | (data);
		}
		cha_attribute[3].flip_flop = !cha_attribute[3].flip_flop;
		if (log_to_console_DMA) cout << "write DMA 3 curr addr = " << to_string(cha_attribute[3].curr_address) << endl;
		break;
	case 7:
		//current world count
		if (cha_attribute[3].flip_flop)
		{
			cha_attribute[3].word_count = cha_attribute[3].word_count & (0x00FF) | (data << 8);
			cha_attribute[3].base_word_count = cha_attribute[3].word_count & (0x00FF) | (data << 8);
		}
		else 
		{ 
			cha_attribute[3].word_count = cha_attribute[3].word_count & (0xFF00) | (data);
			cha_attribute[3].base_word_count = cha_attribute[3].word_count & (0xFF00) | (data);
		}
		cha_attribute[3].flip_flop = !cha_attribute[3].flip_flop;
		if (log_to_console_DMA) cout << "write DMA 3 world count = " << to_string(cha_attribute[3].word_count) << endl;
		break;
	case 8:
		if (log_to_console_DMA) cout << "DMA: Write command reg (" + int_to_bin(data) + ")";
		M_to_M_enable = data & 1;
		Ch0_adr_hold = (data >> 1) & 1;
		Ctrl_disabled = (data >> 2) & 1;
		if (Ctrl_disabled && log_to_console_DMA) cout << " DMA DISABLE";
		else { if (log_to_console_DMA) cout << " DMA ENABLE"; }
		compressed_timings = (data >> 3) & 1;
		rotating_priority = (data >> 4) & 1;
		extended_write = (data >> 5) & 1;
		DREQ_sense = (data >> 6) & 1;
		DACK_sense = (data >> 7) & 1;
		break;
	case 9:  //запрос на DMA
		cha_attribute[data & 3].request_bit = (data >> 2) & 1;
		if (log_to_console_DMA) cout << "DMA: Write request reg CHA " + to_string(data & 3) + " = " + to_string((data >> 2) & 1) << endl;
		break;
	case 10:
		cha_attribute[data & 3].masked = (data >> 2) & 1;
		if (log_to_console_DMA) cout << "DMA: channel " + to_string(data & 3) + " mack register bit = " + to_string(cha_attribute[data & 3].masked) << endl;
		break;
	case 11:
		cha_attribute[data & 3].mode_select = (data >> 6) & 3;
		cha_attribute[data & 3].adress_decrement = (data >> 5) & 1;
		cha_attribute[data & 3].autoinitialization = (data >> 4) & 1;
		if (cha_attribute[data & 3].mode_select != 3)
			{
			cha_attribute[data & 3].transfer_select = (data >> 2) & 3;
			}
		if (log_to_console_DMA) cout << "DMA: cha " + to_string(data & 3) + " set mode = " + to_string((data >> 6) & 3) + " adr_dec = " + to_string((data >> 5) & 1) + " autoinit = " + to_string((data >> 4) & 1) + " transfer(V-W-R) = " + to_string((data >> 2) & 3) << endl;
		//cout << "DMA set mode " << to_string((data >> 6) & 3) << endl;
		break;
	case 12:
		if (log_to_console_DMA) cout << "DMA: Clear byte pointer flip/flop (" + int_to_bin(data) + ")" << endl;
		cha_attribute[0].flip_flop = 0;
		cha_attribute[1].flip_flop = 0;
		cha_attribute[2].flip_flop = 0;
		cha_attribute[3].flip_flop = 0;
		break;
	case 13:
		if (log_to_console_DMA) cout << "DMA: Master clear (" + int_to_bin(data) + ")" << endl;
		M_to_M_enable = false;
		Ch0_adr_hold = false;
		//Ctrl_disabled = false;
		compressed_timings = false;
		rotating_priority = false;
		extended_write = false;
		DREQ_sense = false;
		DACK_sense = false;
		for (int i = 0; i < 4; i++)
		{
			cha_attribute[i].flip_flop = 0;
			cha_attribute[i].curr_address = 0;
			cha_attribute[i].word_count = 0;
			//cha_attribute[i].tmp_reg = 0;
			cha_attribute[i].request_bit = false;
			cha_attribute[i].masked = true;
			cha_attribute[i].mode_select = 0;
			cha_attribute[i].transfer_select = 0;
			cha_attribute[i].autoinitialization = false;
			cha_attribute[i].adress_decrement = false;
		}
		break;
	case 14:
		if (log_to_console_DMA) cout << "DMA: reset mask REG????" << endl;
		break;
	case 15:
		if (log_to_console_DMA) cout << "DMA: Write all mask register bits (" + int_to_bin(data) + ")" << endl;
		if (data & 1) cha_attribute[0].masked = true;
		else cha_attribute[0].masked = false;
		if ((data >> 1) & 1) cha_attribute[1].masked = true;
		else cha_attribute[1].masked = false;
		if ((data >> 2) & 1) cha_attribute[2].masked = true;
		else cha_attribute[2].masked = false;
		if ((data >> 3) & 1) cha_attribute[3].masked = true;
		else cha_attribute[3].masked = false;
		break;
	case 0x83:
		if (log_to_console_DMA) cout << "write DMA page REG = " + int_to_hex(data, 2) << endl;
		cha_attribute[1].page = data;
		break;
	case 0x81:
		if (log_to_console_DMA) cout << "write DMA page REG = " + int_to_hex(data, 2) << endl;
		cha_attribute[2].page = data;
		break;
	case 0x82:
		if (log_to_console_DMA) cout << "write DMA page REG = " + int_to_hex(data, 2) << endl;
		cha_attribute[3].page = data;
		break;
	}
}
uint8 IC8237::read_port(uint16 port)
{
	uint8 out = 0;  //data for return
	switch (port)
	{
	case 0:
		//current address
		if (cha_attribute[0].flip_flop) out = (cha_attribute[0].curr_address >> 8) & 255;
		else out = cha_attribute[0].curr_address & 255;
		cha_attribute[0].flip_flop = !cha_attribute[0].flip_flop;
		//deferred_msg = "read DMA 0 curr addr = " + to_string(out);
		return out;
	case 1:
		//current world count
		if (cha_attribute[0].word_count) out = (cha_attribute[0].word_count >> 8) & 255;
		else out = cha_attribute[0].word_count & 255;
		cha_attribute[0].flip_flop = !cha_attribute[0].flip_flop;
		//deferred_msg = "read DMA 0 world count = " + to_string(out);
		return out;
	case 2:
		//current address
		if (cha_attribute[1].flip_flop) out = (cha_attribute[1].curr_address >> 8) & 255;
		else out = cha_attribute[1].curr_address & 255;
		cha_attribute[1].flip_flop = !cha_attribute[1].flip_flop;
		//deferred_msg = "read DMA 1 curr addr = " + to_string(out);
		return out;
	case 3:
		//current world count
		if (cha_attribute[1].word_count) out = (cha_attribute[1].word_count >> 8) & 255;
		else out = cha_attribute[1].word_count & 255;
		cha_attribute[1].flip_flop = !cha_attribute[1].flip_flop;
		//deferred_msg = "read DMA 1 world count = " + to_string(out);
		return out;
	case 4:
		//current address
		if (cha_attribute[2].flip_flop) out = (cha_attribute[2].curr_address >> 8) & 255;
		else out = cha_attribute[2].curr_address & 255;
		cha_attribute[2].flip_flop = !cha_attribute[2].flip_flop;
		//deferred_msg = "read DMA 2 curr addr = " + to_string(out);
		return out;
	case 5:
		//current world count
		if (cha_attribute[2].word_count) out = (cha_attribute[2].word_count >> 8) & 255;
		else out = cha_attribute[2].word_count & 255;
		cha_attribute[2].flip_flop = !cha_attribute[2].flip_flop;
		//deferred_msg = "read DMA 2 world count = " + to_string(out);
		return out;
	case 6:
		//current address
		if (cha_attribute[3].flip_flop) out = (cha_attribute[3].curr_address >> 8) & 255;
		else out = cha_attribute[3].curr_address & 255;
		cha_attribute[3].flip_flop = !cha_attribute[3].flip_flop;
		//deferred_msg = "read DMA 3 curr addr = " + to_string(out);
		return out;
	case 7:
		//current world count
		if (cha_attribute[3].word_count) out = (cha_attribute[3].word_count >> 8) & 255;
		else out = cha_attribute[3].word_count & 255;
		cha_attribute[3].flip_flop = !cha_attribute[3].flip_flop;
		//deferred_msg = "read DMA 3 world count = " + to_string(out);
		return out;
	case 8:
		//deferred_msg = "DMA: Read status reg " + to_string(status_REG);
		status_REG =	(cha_attribute[3].request_bit * 128) | (cha_attribute[2].request_bit * 64) | (cha_attribute[1].request_bit * 32) | (cha_attribute[0].request_bit * 16) |
						(cha_attribute[3].TC_reached * 8) | (cha_attribute[2].TC_reached * 4) | (cha_attribute[1].TC_reached * 2) | cha_attribute[0].TC_reached;

		cha_attribute[3].TC_reached = cha_attribute[2].TC_reached = cha_attribute[1].TC_reached = cha_attribute[0].TC_reached = 0;  //сбрасываем биты TC
		
		return status_REG;
	case 13:
		//deferred_msg = "DMA: Read temp reg" + to_string(temp_REG);
		return temp_REG;
	case 0x81:
		//deferred_msg = "DMA read page REG_1 = " + int_to_hex(cha_attribute[1].page, 2);
		return cha_attribute[1].page;
	case 0x82:
		//deferred_msg = "DMA read page REG_2 = " + int_to_hex(cha_attribute[2].page, 2);
		return cha_attribute[2].page;
	case 0x83:
		//deferred_msg = "DMA read page REG_3 = " + int_to_hex(cha_attribute[3].page, 2);
		return cha_attribute[3].page;
	}

	if(test_mode) return out; //в тестовом режиме выходим сразу
	cout << "DMA: unknown request on port " << (int)port << " IP = " << (int)Instruction_Pointer;
	step_mode = 1; 
	log_to_console = 1; 
	return 255;
}
void IC8237::sync()
{
	uint8 mem_buffer = 0; //буфер для передачи M-to-M
	uint8* mem_ptr = &mem_buffer; //указатель на буфер

	if (Ctrl_disabled) return; //выход если отключен
	
	//проверяем наличие битов запроса - включаем канал
	for (int i = 0; i < 4; ++i)
	{
		if (cha_attribute[i].request_bit && !cha_attribute[i].masked && !cha_attribute[i].pending)
		{
			//снимаем бит запроса
			cha_attribute[i].request_bit = 0;
			//устанавливаем бит выполнения
			cha_attribute[i].pending = 1;
		}
	}

	//выполняем передачу данных
	//выполнение транзации по одному каналу - выход из обработки (параллельно пока не работаем)
	for (int i = 0; i < 4; ++i)
	{
		if (cha_attribute[i].pending)
		{
			//перебираем разные режимы

			if (cha_attribute[i].mode_select == 0)
			{
				//demand mode - пока нет
				continue;
			}

			if (cha_attribute[i].mode_select == 1)
			{
				//single mode, делаем одну транзакцию
				
				//выбираем режим
				switch (cha_attribute[i].transfer_select)    // 0 - verify, 1 - Write, 2 - read
				{
				case 0:   // verify - просто крутим счетчики

					if (cha_attribute[i].adress_decrement) --cha_attribute[i].curr_address;
					else ++cha_attribute[i].curr_address;
					--cha_attribute[i].word_count;
					break;

				case 1:   //Write - запись в память

					//берем данные из буфера IO
					//канал 0 только для чтения
					if (i == 1) memory.write_2(cha_attribute[i].curr_address + cha_attribute[i].page * 256 * 256, mem_buffer); //буфер для M-to-M
					if (i == 2) memory.write_2(cha_attribute[i].curr_address + cha_attribute[i].page * 256 * 256, FDD_A.get_DMA_data()); //буфер FDD
					if (i == 3) memory.write_2(cha_attribute[i].curr_address + cha_attribute[i].page * 256 * 256, HDD.read_DMA_data()); //буфер HDD

					if (cha_attribute[i].adress_decrement)	--cha_attribute[i].curr_address;
					else ++cha_attribute[i].curr_address;

					--cha_attribute[i].word_count;
					if (i == 2 && cha_attribute[i].word_count == 0xFFFF) FDD_A.EOP = 1; //сигнал конца передачи
					if (i == 3 && cha_attribute[i].word_count == 0xFFFF)
					{
						HDD.EOP = 1; //сигнал конца передачи
						//cout << "DMA HDD WRITE EOP" << endl;
						//step_mode = 1;
					}

					break;

				case 2:   //read - чтение из памяти - запись в IO

					//берем данные из оперативной памяти
					//cout << " - here - i = " << (int)i << endl;
					if (i == 0) mem_buffer = memory.read_2(cha_attribute[i].curr_address + cha_attribute[i].page * 256 * 256); //буфер для M-to-M
					//канал 1 только для записи
					if (i == 2) FDD_A.put_DMA_data(memory.read_2(cha_attribute[i].curr_address + cha_attribute[i].page * 256 * 256)); //буфер FDD
					if (i == 3) HDD.write_DMA_data(memory.read_2(cha_attribute[i].curr_address + cha_attribute[i].page * 256 * 256)); //буфер HDD

					if (cha_attribute[i].adress_decrement) --cha_attribute[i].curr_address;
					else ++cha_attribute[i].curr_address;

					--cha_attribute[i].word_count;
					if (i == 2 && cha_attribute[i].word_count == 0xFFFF) FDD_A.EOP = 1; //сигнал конца передачи
					if (i == 3 && cha_attribute[i].word_count == 0xFFFF)
					{
						HDD.EOP = 1; //сигнал конца передачи
						//cout << "DMA HDD READ EOP" << endl;
						//step_mode = 1;
					}

					break;
				}

				//засыпаем до следующего вызова
				cha_attribute[i].pending = 0;

				//завершение передачи если WC = 0
				if (cha_attribute[i].word_count == 0xFFFF)
				{
					//if (i == 2 && cha_attribute[i].request_bit) FDD_A.EOP = 1; //сигнал конца передачи
					//if (i == 3 && cha_attribute[i].request_bit) HDD.EOP = 1; //сигнал конца передачи

					cha_attribute[i].TC_reached = 1; //достижение предела передачи
					cha_attribute[i].request_bit = 0; //сброс бита запроса

					if (cha_attribute[i].autoinitialization)
					{
						cha_attribute[i].curr_address = cha_attribute[i].base_address;
						cha_attribute[i].word_count = cha_attribute[i].base_word_count;
					}
					else
					{
						//если нет авто, ставим маску
						cha_attribute[i].masked = 1;
					}
				}
				continue;
			}

			if (cha_attribute[i].mode_select == 2)
			{
				//block mode, передаем блок данных
				
				//выбираем режим
				switch (cha_attribute[i].transfer_select)    // 0 - verify, 1 - Write, 2 - read
				{
				case 0:   // verify - просто крутим счетчики
					
					//теоретически, этот цикл не нужен, но пока пусть будет
					while (cha_attribute[i].word_count)
					{
						if (cha_attribute[i].adress_decrement) --cha_attribute[i].curr_address;
						else ++cha_attribute[i].curr_address;
						--cha_attribute[i].word_count;
					}
					break;

				case 1:   //Write - запись в память
					
					while (cha_attribute[i].word_count)
					{

						//берем данные из буфера IO
						//канал 0 только для чтения
						if (i == 1) memory.write_2(cha_attribute[i].curr_address + cha_attribute[i].page * 256 * 256, mem_buffer); //буфер для M-to-M
						if (i == 2) memory.write_2(cha_attribute[i].curr_address + cha_attribute[i].page * 256 * 256, FDD_A.get_DMA_data()); //буфер FDD
						if (i == 3) memory.write_2(cha_attribute[i].curr_address + cha_attribute[i].page * 256 * 256, HDD.read_DMA_data()); //буфер HDD

						if (cha_attribute[i].adress_decrement) --cha_attribute[i].curr_address;
						else ++cha_attribute[i].curr_address;
						--cha_attribute[i].word_count;
					}
					
					break;

				case 2:   //read - чтение из памяти - запись в IO
					
					while (cha_attribute[i].word_count)
					{
						//берем данные из буфера IO
						
						if (i == 0) mem_buffer = memory.read_2(cha_attribute[i].curr_address + cha_attribute[i].page * 256 * 256); //буфер для M-to-M
						//канал 1 только для записи
						if (i == 2) FDD_A.put_DMA_data(memory.read_2(cha_attribute[i].curr_address + cha_attribute[i].page * 256 * 256)); //буфер FDD
						if (i == 3) HDD.write_DMA_data(memory.read_2(cha_attribute[i].curr_address + cha_attribute[i].page * 256 * 256)); //буфер HDD

						if (cha_attribute[i].adress_decrement) --cha_attribute[i].curr_address;
						else ++cha_attribute[i].curr_address;
						--cha_attribute[i].word_count;
					}
					break;
				}

				//завершение передачи
				cha_attribute[i].TC_reached = 1; //достижение предела передачи
				cha_attribute[i].pending = 0;
				if (cha_attribute[i].autoinitialization)
				{
					cha_attribute[i].curr_address = cha_attribute[i].base_address;
					cha_attribute[i].word_count = cha_attribute[i].base_word_count;
				}
				else
				{
					//если нет авто, ставим маску
					cha_attribute[i].masked = 1;
				}
				continue;
			}
			
			if (cha_attribute[i].mode_select == 3)
			{
				//cascade mode, каскадный режим не нужен
				continue;
			}
			//break; //выходим из цикла если отработал один канал
		}
	}

		   
	//работа канала 2 - FDD
	//работа канала 3 - HDD

}
uint8 IC8237::request_hw_dma(int channel)
{
	//устройство запрашивает передачу по каналу
	//возврат 1 - отказ

	//deferred_msg = "request DMA = " + int_to_hex(channel, 2);

	if (channel == 0) //refresh
	{
		if (cha_attribute[channel].masked) return 1; //возврат, если стоит маска
		if (cha_attribute[channel].request_bit) return 1; //возврат, бит уже установлен
		//if (cha_attribute[channel].pending) return; //возврат, если уже в работе - проверить, не нужно ли начать заново
		//if (cha_attribute[channel].TC_reached) return; //возврат, если передача уже закончена и нет автопродления
		cha_attribute[channel].request_bit = 1; //активируем запрос
		return 0;
	}
	
	if (channel == 2) //FDD
	{
		if (cha_attribute[2].masked || cha_attribute[2].request_bit) 
		{
			uint8 out = cha_attribute[2].masked + 2 * cha_attribute[2].request_bit; //возврат флагов
			//if (cha_attribute[2].TC_reached) cha_attribute[2].TC_reached = 0; //сброс флага ТС
			return out; //возврат, если стоит маска/запрос/конец передачи данных
		};
		
		cha_attribute[2].request_bit = 1; //активируем запрос
		//продумать способ активации работы DMA по запросу, чтобы не делать синхронизацию на каждом такте
		//cout << "DMA FDD 0 = OK" << endl;
		return 0;
	}

	if (channel == 3) //HDD
	{
		if (cha_attribute[3].masked || cha_attribute[3].request_bit)
		{
			uint8 out = cha_attribute[3].masked + 2 * cha_attribute[3].request_bit; //возврат флагов
			//if (cha_attribute[2].TC_reached) cha_attribute[2].TC_reached = 0; //сброс флага ТС
			return out; //возврат, если стоит маска/запрос/конец передачи данных
		};

		cha_attribute[3].request_bit = 1; //активируем запрос
		//продумать способ активации работы DMA по запросу, чтобы не делать синхронизацию на каждом такте
		//cout << "DMA FDD 0 = OK" << endl;
		return 0;
	}

	return 0;
}
string IC8237::get_ch_data(int ch_num)
{
	string out;
	if (cha_attribute[ch_num].request_bit) out += " +  ";
	else out += "    ";
	if (cha_attribute[ch_num].pending) out += "+  ";
	else out += "   ";
	switch (cha_attribute[ch_num].mode_select)
	{
	case 0:
		out += "dem  ";
		break;
	case 1:
		out += "sing ";
		break;
	case 2:
		out += "blok ";
		break;	
	case 3:
		out += "casc ";
		break;
	}
	
	if (cha_attribute[ch_num].masked) out += " # ";
	else out += "   ";
	
	if (cha_attribute[ch_num].autoinitialization) out += "   +  ";
	else out += "      ";

	switch (cha_attribute[ch_num].transfer_select)
	{
	case 0:
		out += " ver ";
		break;
	case 1:
		out += " writ ";
		break;
	case 2:
		out += " read ";
		break;
	case 3:
		out += " err ";
		break;
	}

	return out;
}
string IC8237::get_ch_data_2(int ch_num)
{
	string out;
	out += int_to_hex(cha_attribute[ch_num].page & 3, 2) + ":";
	out += int_to_hex(cha_attribute[ch_num].curr_address, 4) + "  ";
	out += int_to_hex(cha_attribute[ch_num].word_count, 4) + "   ";
	out += int_to_hex(cha_attribute[ch_num].base_address, 4) + "  ";
	out += int_to_hex(cha_attribute[ch_num].base_word_count, 4);
	return out;
}
string IC8237::get_ch_data_3(int ch_num)
{
	string out = "CTRL[";
	if (Ctrl_disabled) out += "OFF] ";
	else out += "ON]  ";
	if (M_to_M_enable) out += "M2M[ON]  ";
	else out += "M2M[OFF] ";
	if (rotating_priority) out += "PRIOR: ROT";
	else out += "PRIOR: FIX";

	return out;
}

//PPI controller
void IC8255::write_port(uint16 port, uint8 data)
{
	switch (port)
	{
	case 0x60: //port A
		//diagnostic output 
		//deferred_msg = "BOOT CHECKPOINT " + to_string(data);
		break;
	case 0x61: //port B
		
		port_B_out = data; //запоминаем значение
		//IC parametrs
		//deferred_msg = "Port 61 out: " + int_to_bin(data) + " ";
		if ((data & 1) == 1)
		{
			//deferred_msg = deferred_msg + "T_Gate 2 ON  ";
			//pc_timer.enable_timer(2);
			//speaker.beep_on();
		}
		else 
		{ 
			//deferred_msg = deferred_msg + "T_Gate 2 OFF "; 
			//pc_timer.disable_timer(2);
			//speaker.beep_off();
		}
		
		if ((data & 2) == 2) // пины 0 и 1
		{
			//deferred_msg = deferred_msg + "SPK_ON  ";
			speaker.beep_on();
			//cout << "BEEP ON" << endl;
		}
		else
		{
			//deferred_msg = deferred_msg + "SPK_OFF ";
			speaker.beep_off();
			//cout << "BEEP OFF" << endl;
		}
		
		if (((data >> 3) & 1) == 1) 
		{
			//deferred_msg = deferred_msg + "SWITCHES_HIGH ";
			switches_hign = true;
		}
		else
		{
			//deferred_msg = deferred_msg + "SWITCHES_LOW ";
			switches_hign = false;
		}
		//if ((data & 16) == 0) deferred_msg = deferred_msg + "+RAM_parity_CHK ";
		//if ((data & 32) == 0) deferred_msg = deferred_msg + "+IO_CHECK ";
		
		if ((data & 64) == 0)  //pin6
		{
			//deferred_msg = deferred_msg + "+KB_clock_LOW ";
			keyboard.set_CLK_low();
		}
		else
		{
			//deferred_msg = deferred_msg + "+KB_clock_HIGH ";
			keyboard.set_CLK_high();
		}
		
		if ((data & 128) == 0) //pin7
		{
			//deferred_msg = deferred_msg + "enable_KB ";
			//if (port_B_7) keyboard.data_line_enabled = true; //включаем передачу данных
			keyboard.data_line_enabled = true;
			//cout << "KB_EN" << endl;
			port_B_7 = false;
		}
		else
		{
			//deferred_msg = deferred_msg + "clear_KB ";
			port_B_7 = true; //переключаем состояние
		}
		
		Audio_monitor.set_pinout(data & 0b11);
		break;
	case 0x63:
		//control register
		/*
		deferred_msg = "PPI control: ";
		if ((data & 16) == 16) deferred_msg += "port A(IN), ";
		else deferred_msg += "port A(OUT), ";
		if ((data & 8) == 8) deferred_msg += "port UPPER_C (IN), ";
		else deferred_msg += "port UPPER_C (OUT), ";
		//if ((data & 4) == 4) deferred_msg += "grB mode 1, ";
		//else deferred_msg += "grB mode 0, ";
		if ((data & 2) == 2) deferred_msg += "port B(IN), ";
		else deferred_msg += "port B(OUT), ";
		if ((data & 1) == 1) deferred_msg += "port LOWER_C(IN)";
		else deferred_msg += "port LOWER_C(OUT)";
		*/
		break;
	}
}
uint8 IC8255::read_port(uint16 port)
{
	uint8 out = 0;  //data for return
	switch (port)
	{
	case 0x60:
		//read scan code from keyboard
		out = keyboard.read_scan_code();
		//deferred_msg = "KB read 0x60 key_code = " + to_string(out);
		break;

	case 0x61:
		out = port_B_out;  //отправляем запомненное значение
		break;

	case 0x62:
		//read some data from MB
		if (switches_hign)
		{
			//deferred_msg = "read from port 62 - 0000 sw_high 0010";
			out = MB_switches >> 4;
		}
		else
		{
			//deferred_msg = "read from port 62 - 0000 sw_low 1101";
			out = MB_switches & 15;
		}
		break;
	}
	return out;
}

//INT controller
void IC8259::write_port(uint16 port, uint8 data)
{
	if(log_to_console_INT) cout << endl << "write port " << port << " " << (bitset<8>)data <<  " next_ICW=" << (int)next_ICW << endl;
	
	//Initialization Commands

	if ((port == 0x20) && ((data >> 4) & 1) && (next_ICW == 1))
	{
		//Initialization Command Word 1
		if (log_to_console_INT) deferred_msg = "INT_Ctrl Init_1: ";
		
		if (data & 1) 
		{ 
			if (log_to_console_INT) deferred_msg += "ICW4_ON ";
			wait_ICW4 = 1;
		}
		else 
		{ 
			if (log_to_console_INT) deferred_msg += "ICW4_OFF ";
			wait_ICW4 = 0; 
		}
		
		if (data & 2) {
			if (log_to_console_INT) deferred_msg += "SINGLE_MODE ";
			cascade_mode = false;
		}
		else 
		{ 
			if (log_to_console_INT) deferred_msg += "CASCADE_MODE ";
			cascade_mode = true;
		}

		if (data & 4) {
			if (log_to_console_INT) deferred_msg += "ADDR_INTERVAL_4 ";
			ADDR_INTERVAL_4 = true;
		}
		else 
		{ 
			if (log_to_console_INT) deferred_msg += "ADDR_INTERVAL_8 ";
			ADDR_INTERVAL_4 = false;
		}
		
		if (data & 8)
		{
			if (log_to_console_INT) deferred_msg += "LEVEL_MODE ";
		}
		else
		{
			if (log_to_console_INT) deferred_msg += "EDGE_MODE ";
		}
		INT_vector_addr_80 = data & 0xE0;//выделяем биты А7-А5
		next_ICW = 2;
		return;
	}
	
	if ((port == 0x21) && next_ICW == 2)
	{
		//Initialization Command Word 2
		if (log_to_console_INT) deferred_msg = "INT_Ctrl Init_2: ";
		INT_vector_addr_86 = data & 0xF8;// биты А7 - А3
		INT_vector_addr_80 += data * 256;// старшие биты адреса таблицы
		if (log_to_console_INT) deferred_msg += "INT_vector_addr " + int_to_bin(data);
		if (cascade_mode) next_ICW = 3;
		else
		{
			if (wait_ICW4) next_ICW = 4;
			else {enabled = true; next_ICW = 0;}
		}
		return;
	}
	
	if ((port == 0x21) && next_ICW == 3)
	{
		if (log_to_console_INT) deferred_msg = "INT_Ctrl Init_3: it should not be here. ERROR.";
		if (wait_ICW4) next_ICW = 4;
		else { enabled = true; next_ICW = 0; }
		return;
	}

	if ((port == 0x21) && !((data >> 5) & 7) && next_ICW == 4)
	{
		//Initialization Command Word 4
		if (log_to_console_INT) deferred_msg = "INT_Ctrl Init_4: ";

		if (data & 16) {
			if (log_to_console_INT) deferred_msg += "NESTED_ON "; nested_mode = true;
		}
		else {
			if (log_to_console_INT) deferred_msg += "NESTED_OFF "; nested_mode = false;
		}

		if ((data & 8) && log_to_console_INT) deferred_msg += "BUFFERED ";
		else { if (log_to_console_INT) deferred_msg += "NOT_BUFFERED "; }
		if (data & 2) {
			if (log_to_console_INT) deferred_msg += "AUTO_EOI "; AUTO_EOI = true;
		}
		else {
			if (log_to_console_INT) deferred_msg += "NORMAL_EOI "; AUTO_EOI = false;
		}
		if (data & 2) 
		{ 
			if (log_to_console_INT) deferred_msg += "8086_mode "; mode_8086 = true;
		}
		else 
		{
			if (log_to_console_INT) deferred_msg += "8085_mode "; mode_8086 = false;
		}
		
		//включение
		enabled = true;
		next_ICW = 0;
		return;
	}

	//Operation Command Words

	if ((port == 0x21) && (next_ICW == 0) && enabled)  // next_ICW == 0 && ENABLED - инициализация завершена
	{
		//Operation Command Word 1
		if (log_to_console_INT) deferred_msg = "INT_Ctrl Command_1: masked bits " + int_to_bin(data);
		IM_REG = data;
		sleep_timer = 2;
		return;
	}
	
	if ((port == 0x20) && (next_ICW == 0) && !(((data >> 3) & 3) == 1) && enabled) // OCW2
	{
		//Operation Command Word 2
		if (log_to_console_INT) deferred_msg = "INT_Ctrl Command_2: IR_LEVEL=" + to_string((int)(data & 7)) + " cmd_code=" + to_string((data >> 5) & 7);

		if (((data >> 5) & 7) == 1)
		{
			//сбрасываем бит самого высокого уровня в IS_REG
			if (log_to_console_INT) cout << "EIO ISR " << int_to_bin(IS_REG) << " -> ";
			for (uint8 v = 0; v < 8; v++)
			{
				if ((IS_REG >> v) & 1)
				{
					IS_REG = IS_REG & (~(1 << v));
					break;
				}
			}
			if (log_to_console_INT) cout << int_to_bin(IS_REG);
		}

		if (((data >> 5) & 7) == 3)
		{
			//сбрасываем указанный бит в IS_REG
			uint8 b = data & 7; //номер прерывания для сброса
			if (log_to_console_INT) cout << "EIO ISR " << int_to_bin(IS_REG) << " -> ";
			IS_REG = IS_REG & (~(1 << b));
			if (log_to_console_INT) cout << int_to_bin(IS_REG);
		}
		
		sleep_timer = 2;
		return;
	}

	if ((port == 0x20) && (next_ICW == 0) && (((data >> 3) & 3) == 1) && enabled) // OCW3
	{
		//Operation Command Word 3
		if (log_to_console_INT) deferred_msg = "INT_Ctrl Command_3: " + int_to_bin(data) + " ";
		switch (data & 3)
		{
		case 2:
			//read IR REG
			next_reg_to_read = 1;
			if (log_to_console_INT) deferred_msg += "SET next_reg_to_read = IRR ";
			break;
		case 3:
			//read IS REG
			next_reg_to_read = 2;
			if (log_to_console_INT) deferred_msg += "SET next_reg_to_read = ISR ";
			break;
		}
		
		if ((data >> 2) & 1)
		{
			// poll command
			if (log_to_console_INT) deferred_msg += "DO_POLL  ";
		}
		
		switch ((data >> 5) & 3)
		{
		case 2:
			//Reset special mask
			if (log_to_console_INT) deferred_msg += "RESET_SPEC_MASK ";
			break;
		case 3:
			//SET special mask
			if (log_to_console_INT) deferred_msg += "SET_SPEC_MASK ";
			break;
		}
		
		return;
	}
}
uint8 IC8259::read_port(uint16 port)
{
	if (log_to_console_INT)  cout << endl << "read port " << port << endl;
	if ((next_reg_to_read == 1) && ((port & 1) == 0))
	{
		if (log_to_console_INT) deferred_msg = "INT_Ctrl read IRR = " + int_to_bin(IR_REG);
		return IR_REG;
	}

	if ((next_reg_to_read == 2) && ((port & 1) == 0))
	{
		if (log_to_console_INT) deferred_msg = "INT_Ctrl read ISR = " + int_to_bin(IS_REG);
		return IS_REG;
	}

	if (port & 1)
	{
		if (log_to_console_INT) deferred_msg = "INT_Ctrl read IMR = " + int_to_bin(IM_REG);
		return IM_REG;
	}

	return 0;
}
uint8 IC8259::request_IRQ(uint8 irq)
{
	//обработка запросов и изменение IR_REG
	
	if (!enabled)
	{
		if (log_to_console_INT) deferred_msg = "INT_Ctr is OFF. IRQ_" + to_string(irq) + " request denied ";
		return 1; //выход если отключен сам контроллер
	}
						  
	//проверяем маски
	if ((IM_REG >> irq) & 1) 
	{
		if (log_to_console_INT) deferred_msg = "INT_Ctr: IRQ_" + to_string(irq) + " is masked and request denied ";
		return 2; //выход если маскировано данное INT
	}
	if ((IS_REG >> irq) & 1)
	{
		if (log_to_console_INT) deferred_msg = "INT_Ctr: IRQ_" + to_string(irq) + " in service. Request denied ";
		return 3; //выход если данное INT уже обслуживается
	}

	//дополняем регистр запросов
	IR_REG = IR_REG | (1 << irq);
	return 0;
}
uint8 IC8259::get_last_int() { return last_INT; }
uint16 IC8259::get_last_int_addr() //возврат адреса вектора
{
	if (mode_8086)
	{
		//mode 8086
		return INT_vector_addr_86 + last_INT; // T7-T3 + INT
	}
	else
	{
		// mode 8080/85
		return INT_vector_addr_80 + last_INT; 
	}
	
}

uint8 IC8259::next_int()
{
	if (sleep_timer) 
	{
		sleep_timer--;
		if (log_to_console_INT) deferred_msg = "sleep timer = " + to_string(sleep_timer) + " return 255";
		return 255;
	}
	
	if (!enabled) 
	{
		if (log_to_console_INT) deferred_msg = "INT Ctrl OFF return 255";
		return 255; //контроллер отключен
	}

	// просматриваем вектора прерываний
	
	for (uint8 v = 0; v < 8; v++)
	{
		if (((IR_REG >> v) & 1) && !((IS_REG >> v) & 1) && !((IM_REG >> v) & 1))
		{
			IR_REG = IR_REG & ~(1 << v); //отключаем бит в регистре запросов
			IS_REG = IS_REG | (1 << v); //устанавливаем бит в регистре обслуживания
			last_INT = v;//запоминаем текущее прерывание
			//SetConsoleTextAttribute(hConsole, 10);
			//cout << "INT CTRL EN = " << (int)enabled << " IR_REG = " << (bitset<8>)IR_REG << " IS_REG = " << (bitset<8>)IS_REG << " IM_REG = " << (bitset<8>)IM_REG << endl;
			//SetConsoleTextAttribute(hConsole, 7);
			return v; //возвращаем номер активного прерывания
		}
		if ((IS_REG >> v) & 1) return 255; // если уже обслуживается более высокий уровень - возврат
	}
	
	return 255;
}
string IC8259::get_ch_data(int ch)
{
	string out;
	if ((IM_REG >> ch) & 1) out += " #   ";
	else out += "     ";
	if ((IR_REG >> ch) & 1) out += " +  ";
	else out += "    ";
	if ((IS_REG >> ch) & 1) out += "  V";
	else out += "   ";

	return out;
}
void IC8259::set_timeout(uint8 delay)
{
	sleep_timer = delay;
}

void print_mem()
{
	for (int i = 0; i < 256; i++)
	{
		cout << hex << (int)i << "\t" << (int)command_counter[i] << endl;
	}
}

Dev_mon_device::Dev_mon_device(uint16 w, uint16 h, string title, uint16 x_pos, uint16 y_pos)   // конструктор класса
{
	//инициализируем графические константы
	GAME_WINDOW_X_RES = w;
	GAME_WINDOW_Y_RES = h;

	//получаем данные о текущем дисплее (пока не нужны)
	my_display_H = sf::VideoMode::getDesktopMode().size.y;
	my_display_W = sf::VideoMode::getDesktopMode().size.x;

	cout << "Debug window Init " << (int)h << " x " << (int)w << " display name " << title << endl;
#ifdef DEBUG
	//создаем главное окно
	main_window.create(sf::VideoMode(sf::Vector2u(GAME_WINDOW_X_RES, GAME_WINDOW_Y_RES)), title, sf::Style::Titlebar, sf::State::Windowed);
	main_window.setPosition({ x_pos, y_pos });
	main_window.setFramerateLimit(60);
	main_window.setMouseCursorVisible(1);
	main_window.setKeyRepeatEnabled(0);
	main_window.setVerticalSyncEnabled(1);
	main_window.setActive(true);
#endif
}

void Dev_mon_device::sync(int elapsed_ms)   // синхронизация
{
	main_window.clear(sf::Color::Black);// очистка экрана

	sf::Text text(font);				//обычный шрифт
	text.setCharacterSize(30);
	text.setFillColor(sf::Color::White);

	//вывод регистров и стека

	text.setString("Registers");
	text.setPosition(sf::Vector2f(10, 10));
	main_window.draw(text);

	text.setString("----------------");
	text.setPosition(sf::Vector2f(10, 40));
	main_window.draw(text);

	text.setString("AX    " + int_to_hex(AX, 4));
	text.setPosition(sf::Vector2f(10, 70));
	main_window.draw(text);

	text.setString("BX    " + int_to_hex(BX, 4));
	text.setPosition(sf::Vector2f(10, 100));
	main_window.draw(text);

	text.setString("CX    " + int_to_hex(CX, 4));
	text.setPosition(sf::Vector2f(10, 130));
	main_window.draw(text);

	text.setString("DX    " + int_to_hex(DX, 4));
	text.setPosition(sf::Vector2f(10, 160));
	main_window.draw(text);

	text.setString("----------------");
	text.setPosition(sf::Vector2f(10, 190));
	main_window.draw(text);

	text.setString("Segments & ptrs");
	text.setPosition(sf::Vector2f(10, 220));
	main_window.draw(text);

	text.setString("----------------");
	text.setPosition(sf::Vector2f(10, 250));
	main_window.draw(text);

	text.setString("CS:IP " + int_to_hex(*CS, 4) + ":" + int_to_hex(Instruction_Pointer, 4));
	text.setPosition(sf::Vector2f(10, 280));
	main_window.draw(text);

	text.setString("SS:SP " + int_to_hex(*SS, 4) + ":" + int_to_hex(Stack_Pointer, 4));
	text.setPosition(sf::Vector2f(10, 310));
	main_window.draw(text);

	text.setString("SS:BP " + int_to_hex(*SS, 4) + ":" + int_to_hex(Base_Pointer, 4));
	text.setPosition(sf::Vector2f(10, 340));
	main_window.draw(text);

	text.setString("DS:SI " + int_to_hex(*DS, 4) + ":" + int_to_hex(Source_Index, 4));
	text.setPosition(sf::Vector2f(10, 370));
	main_window.draw(text);

	text.setString("DS:DI " + int_to_hex(*DS, 4) + ":" + int_to_hex(Destination_Index, 4));
	text.setPosition(sf::Vector2f(10, 400));
	main_window.draw(text);

	text.setString("ES:DI " + int_to_hex(*ES, 4) + ":" + int_to_hex(Destination_Index, 4));
	text.setPosition(sf::Vector2f(10, 430));
	main_window.draw(text);

	text.setString("----------------");
	text.setPosition(sf::Vector2f(10, 460));
	main_window.draw(text);

	text.setString("Flags");
	text.setPosition(sf::Vector2f(10, 490));
	main_window.draw(text);

	text.setString("----------------");
	text.setPosition(sf::Vector2f(10, 520));
	main_window.draw(text);

	text.setString("A=" + to_string(Flag_AF) + " C=" + to_string(Flag_CF) + " P=" + to_string(Flag_PF) + " S=" + to_string(Flag_SF));
	text.setPosition(sf::Vector2f(10, 550));
	main_window.draw(text);

	text.setString("Z=" + to_string(Flag_ZF) + " D=" + to_string(Flag_DF) + " I=" + to_string(Flag_IF) + " O=" + to_string(Flag_OF));
	text.setPosition(sf::Vector2f(10, 580));
	main_window.draw(text);

	text.setString("----------------");
	text.setPosition(sf::Vector2f(10, 610));
	main_window.draw(text);

	text.setString("Stack(16)");
	text.setPosition(sf::Vector2f(10, 640));
	main_window.draw(text);

	text.setString("----------------");
	text.setPosition(sf::Vector2f(10, 670));
	main_window.draw(text);

	for (int s = 0; s < 16; s++)
	{
		text.setString("[" + int_to_hex(*SS, 4) + ":" + int_to_hex(Stack_Pointer + s * 2, 4) + "] " + int_to_hex((int)(memory.read_2(uint16(Stack_Pointer + (s * 2)) + SS_data * 16) + memory.read_2(uint16(Stack_Pointer + (s * 2) + 1) + SS_data * 16) * 256), 4));
		text.setPosition(sf::Vector2f(10, 700 + s * 30));
		main_window.draw(text);
	}

	//скорость работы
	if (!step_mode && elapsed_ms) {

		//обновляем массив времени кадра
		text.setString(to_string((int)round(op_counter * 1000.0 / (elapsed_ms + 1.0))) + "K op/s");
		text.setFillColor(sf::Color::White);
		text.setPosition(sf::Vector2f(10, 1200));
		main_window.draw(text);
	}

	
	if (log_to_console) text.setString("LOG ON");
	else text.setString("LOG OFF");
	text.setPosition(sf::Vector2f(180, 1200));
	text.setFillColor(sf::Color::White);
	main_window.draw(text);

	if (step_mode) text.setString("STEP ON");
	else text.setString("STEP OFF");
	text.setPosition(sf::Vector2f(310, 1200));
	text.setFillColor(sf::Color::White);
	main_window.draw(text);
	
	//видеорежим
	text.setString(monitor.get_mode_name());
	text.setPosition(sf::Vector2f(460, 1200));
	text.setFillColor(sf::Color::White);
	main_window.draw(text);

	//данные пищалки
	if (speaker.beeping && !halt_cpu)
	{
		text.setString(to_string(speaker.timer_freq) + " Hz");
		text.setPosition(sf::Vector2f(730, 1200));
		text.setFillColor(sf::Color::Yellow);
		main_window.draw(text);
	}

	if (halt_cpu)
	{
		text.setString("HALT");
		text.setPosition(sf::Vector2f(730, 1200));
		text.setFillColor(sf::Color::Yellow);
		main_window.draw(text);
	}


	string sec = to_string(round(memory.read_2(0x46c) / 1.82) / 10);
	sec.resize(sec.find_first_of(",") + 2);
	text.setString("SysT " + sec);
	text.setPosition(sf::Vector2f(860, 1200));
	text.setFillColor(sf::Color::Red);
	main_window.draw(text);
	text.setFillColor(sf::Color::White);

	//выводим регистры таймера

	text.setString("Timer Count Init Mode BCD RL Signal");
	text.setPosition(sf::Vector2f(300, 10));
	main_window.draw(text);

	text.setString("----------------------------------");
	text.setPosition(sf::Vector2f(300, 40));
	main_window.draw(text);

	for (int i = 0; i < 3; i++)
	{
		text.setString(to_string(i) + " " + pc_timer.get_ch_data(i));
		text.setPosition(sf::Vector2f(300, 70 + i*30));
		main_window.draw(text);
	}

	text.setString("----------------------------------");
	text.setPosition(sf::Vector2f(300, 160));
	main_window.draw(text);

	//выводим регистры DMA

	text.setString(dma_ctrl.get_ch_data_3(0));
	text.setPosition(sf::Vector2f(300, 200));
	main_window.draw(text);

	text.setString("----------------------------------");
	text.setPosition(sf::Vector2f(300, 230));
	main_window.draw(text);

	text.setString("DMA   Req ON Mode Mask AUTO Trans");
	text.setPosition(sf::Vector2f(300, 260));
	main_window.draw(text);

	text.setString("----------------------------------");
	text.setPosition(sf::Vector2f(300, 290));
	main_window.draw(text);

	//первая таблица
	for (int i = 0; i < 4; i++)
	{
		string name;
		switch (i)
		{
		case 0:
			name = "ref";
			break;
		case 1:
			name = "mem";
			break;
		case 2:
			name = "FDD";
			break;
		case 3:
			name = "HDD";
			break;
		}
		
		text.setString(to_string(i) + " " + name + " " + dma_ctrl.get_ch_data(i));
		text.setPosition(sf::Vector2f(300, 320 + i * 30));
		main_window.draw(text);
	}

	text.setString("----------------------------------");
	text.setPosition(sf::Vector2f(300, 440));
	main_window.draw(text);

	text.setString("DMA   Page:ADDR Count  BaseA BaseC");
	text.setPosition(sf::Vector2f(300, 480));
	main_window.draw(text);

	text.setString("----------------------------------");
	text.setPosition(sf::Vector2f(300, 510));
	main_window.draw(text);

	for (int i = 0; i < 4; i++)
	{
		string name;
		switch (i)
		{
		case 0:
			name = "ref";
			break;
		case 1:
			name = "mem";
			break;
		case 2:
			name = "FDD";
			break;
		case 3:
			name = "HDD";
			break;
		}

		text.setString(to_string(i) + " " + name + " " + dma_ctrl.get_ch_data_2(i));
		text.setPosition(sf::Vector2f(300, 540 + i * 30));
		main_window.draw(text);
	}

	text.setString("----------------------------------");
	text.setPosition(sf::Vector2f(300, 660));
	main_window.draw(text);

	//INT controller
	text.setString("INT Mask Req Active");
	text.setPosition(sf::Vector2f(300, 690));
	main_window.draw(text);

	text.setString("----------------------------------");
	text.setPosition(sf::Vector2f(300, 720));
	main_window.draw(text);

	for (int i = 0; i < 8; i++)
	{
		text.setString(" " + to_string(i) + "   " + int_ctrl.get_ch_data(i));
		text.setPosition(sf::Vector2f(300, 750 + i * 30));
		main_window.draw(text);
	}
	
	//ON or OFF
	if (int_ctrl.enabled) text.setString("Enabled");
	else text.setString("Disabled");
	text.setPosition(sf::Vector2f(620, 690));
	main_window.draw(text);

	//keyboard buffer size
	text.setString("KB_buffer = " + to_string(keyboard.get_buf_size()));
	text.setPosition(sf::Vector2f(620, 750));
	main_window.draw(text);

	//KB_line status
	text.setString("KB_line = " + to_string(keyboard.data_line_enabled));
	text.setPosition(sf::Vector2f(620, 780));
	main_window.draw(text);

	//cascade mode
	if (int_ctrl.cascade_mode) text.setString("Cascade ON");
	else text.setString("Cascade OFF");
	text.setPosition(sf::Vector2f(620, 810));
	main_window.draw(text);

	//nested mode
	if (int_ctrl.nested_mode) text.setString("Nested ON");
	else text.setString("Nested OFF");
	text.setPosition(sf::Vector2f(620, 840));
	main_window.draw(text);

	//ADDR_INTERVAL
	if (int_ctrl.ADDR_INTERVAL_4) text.setString("ADDR_INTERVAL = 4");
	else text.setString("ADDR_INTERVAL = 8");
	text.setPosition(sf::Vector2f(620, 870));
	main_window.draw(text);

	//AUTO_EOI
	if (int_ctrl.AUTO_EOI) text.setString("AUTO_EOI ON");
	else text.setString("AUTO_EOI OFF");
	text.setPosition(sf::Vector2f(620, 900));
	main_window.draw(text);

	//mode_8086
	if (int_ctrl.mode_8086) text.setString("Mode 86/88");
	else text.setString("Mode 80/85");
	text.setPosition(sf::Vector2f(620, 930));
	main_window.draw(text);

	//sleep_timer
	text.setString("sleep_timer = " + to_string(int_ctrl.sleep_timer));
	text.setPosition(sf::Vector2f(620, 960));
	main_window.draw(text);

	text.setString("----------------------------------");
	text.setPosition(sf::Vector2f(300, 990));
	main_window.draw(text);

	//заполняем массив по ссылке
	/*
	*ptr = CMD_BUSY;
	*(ptr + 1) = selected_drive;
	*(ptr + 2) = DMA_enabled;
	*(ptr + 3) = DMA_ON;
	*(ptr + 4) = motors_pin_enabled;
	*(ptr + 5) = FDD_busy_bits;
	*/

	//FDD controller
	uint8 raw_data[20];
	uint8* ptr = raw_data;
	text.setString("FDD state:" + FDD_A.get_state(ptr));
	text.setPosition(sf::Vector2f(300, 1020));
	main_window.draw(text);

	if (raw_data[0])
	{ 
		text.setFillColor(sf::Color::Red);
		text.setString("   BUSY");
	}
	else
	{
		text.setString("NO BUSY");
	}
	text.setPosition(sf::Vector2f(570, 1020));
	main_window.draw(text);

	text.setFillColor(sf::Color::White);
	text.setString("SEL DRV:" + to_string(raw_data[1]));
	text.setPosition(sf::Vector2f(695, 1020));
	main_window.draw(text);

	text.setString("----------------------------------");
	text.setPosition(sf::Vector2f(300, 1050));
	main_window.draw(text);

	if (raw_data[2])
	{
		text.setFillColor(sf::Color::Green);
		text.setString("DMA EN");
	}
	else
	{
		text.setString("DMA DIS");
	}

	text.setPosition(sf::Vector2f(300, 1080));
	main_window.draw(text);
	text.setFillColor(sf::Color::White);

	if (raw_data[3])
	{
		text.setFillColor(sf::Color::Green);
		text.setString("DMA ON");
	}
	else
	{
		text.setString("DMA OFF");
	}

	text.setPosition(sf::Vector2f(414, 1080));
	main_window.draw(text);
	
	text.setString("MOTORS[");
	text.setFillColor(sf::Color::White);
	text.setPosition(sf::Vector2f(530, 1080));
	main_window.draw(text);

	for (int i = 0; i < 4; ++i)
	{
		if ((raw_data[4] >> i) & 1)
		{
			text.setFillColor(sf::Color::Green);
			text.setString("0");
		}
		else
		{
			text.setFillColor(sf::Color::White);
			text.setString("-");
		}
		text.setPosition(sf::Vector2f(635 + i * 15, 1080));
		main_window.draw(text);
	}
	text.setString("]");
	text.setFillColor(sf::Color::White);
	text.setPosition(sf::Vector2f(695, 1080));
	main_window.draw(text);

	text.setString("BUSY[");
	text.setFillColor(sf::Color::White);
	text.setPosition(sf::Vector2f(710, 1080));
	main_window.draw(text);

	for (int i = 0; i < 4; ++i)
	{
		if ((raw_data[5] >> i) & 1)
		{
			text.setFillColor(sf::Color::Red);
			text.setString("0");
		}
		else
		{
			text.setFillColor(sf::Color::White);
			text.setString("-");
		}
		text.setPosition(sf::Vector2f(785 + i * 15, 1080));
		main_window.draw(text);
	}
	text.setString("]");
	text.setFillColor(sf::Color::White);
	text.setPosition(sf::Vector2f(840, 1080));
	main_window.draw(text);

	text.setString("----------------------------------");
	text.setPosition(sf::Vector2f(300, 1110));
	main_window.draw(text);

	text.setString("H[" + to_string(raw_data[8]) + "] Cyl[" + to_string(raw_data[6]) + "] Sec[" + to_string(raw_data[7]) + "] MT[" + to_string(raw_data[11]) + "][" + int_to_bin(memory.read_2(0x490)) + "]");
	text.setPosition(sf::Vector2f(300, 1140));
	main_window.draw(text);

	//управление головкой
	/*
	(ptr + 6) = C_cylinder;	//текущий цилиндр
	*(ptr + 7) = R_sector;		//текущий сектор
	*(ptr + 8) = H_head;		//текущая сторона дискеты
	*(ptr + 9) = DIR_control;	//направление поиска 1 - к центру
	*(ptr + 10) = MFM_mode;		//0 - одинарная плотность, 1 - двойная
	*(ptr + 11) = MT;			//multi-track
	*/
	main_window.display();
}

void FDD_mon_device::sync()
{
	main_window.clear(sf::Color::Black);// очистка экрана

	sf::Text text(font);				//обычный шрифт
	text.setCharacterSize(25);
	text.setFillColor(sf::Color::White);
	
	//FDD controller

	int max_s = 60; // 1600 / 25
	int begin = max(int(0), int(log_strings.size() - max_s));
	//cout << "for i=" << (int)begin << " to " << (int)log_strings.size() << endl;
	for (int i = begin; i < log_strings.size(); i++)
	{
		text.setFillColor(sf::Color::White);
		if (log_strings.at(i).find("INT13(READ)") != std::string::npos) text.setFillColor(sf::Color::Green);
		if (log_strings.at(i).find("INT13")!=std::string::npos) text.setFillColor(sf::Color::Green);
		if (log_strings.at(i).find("INT13(WRITE)") != std::string::npos) text.setFillColor(sf::Color::Red);
		if (log_strings.at(i).find("EXEcu") != std::string::npos) text.setFillColor(sf::Color::Cyan);
		if (log_strings.at(i).find("Result OK") != std::string::npos) text.setFillColor(sf::Color::Green);
		if (log_strings.at(i).find("ERR") != std::string::npos) text.setFillColor(sf::Color::Red);
		
		text.setString(log_strings.at(i));
		text.setPosition(sf::Vector2f(0, 50 + (i - begin) * 25));
		main_window.draw(text);
	}
#ifdef DEBUG
	main_window.display();
#endif


}

void FDD_mon_device::log(string log_string)
{
	//запоминаем последнюю строку
	//last_str = "";
	
	// пишем в массив строк
	if (last_str != log_string) log_strings.push_back(log_string);
	last_str = log_string;
}




