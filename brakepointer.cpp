#include <Windows.h>
#include <conio.h>
#include <iostream>
#include <string>
#include "custom_classes.h"
#include "breakpointer.h"

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

using namespace std;

extern uint16 Instruction_Pointer;
extern uint16* CS;
extern HANDLE hConsole;

extern bool step_mode;		//ждать ли нажатия пробела для выполнения команд
extern bool log_to_console; //логирование команд на консоль
extern bool run_until_CX0; //останов при окончании цикла

//счетчики
//uint32 op_counter = 0;
extern uint8 service_counter;

void Breakpointer::load_breakpoints()
{

//breakpoints.push_back({0xc81fa,"jmp to 7c00"});
//breakpoints.push_back({0xc8003,"IBM HDD CTRL"}); 
//breakpoints.push_back({0xc8127,"IBM HDD CTRL"}); 
//breakpoints.push_back({0xc8251,"HDD INT13"}); 
//breakpoints.push_back({0xc81d5,"HDD boot loader"}); 
//breakpoints.push_back({0xc8203,"HDD boot loader"}); 
//breakpoints.push_back({0xc820e,"HDD boot loader"});

}

void Breakpointer::load_comments()
{
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


}

Breakpointer::Breakpointer()
{
	load_breakpoints();
	load_comments();
}

void Breakpointer::check_points()
{

	if (comments_ON)
	{
		//вывод комментариев к точкам кода
		for (int j = 0; j < comments.size(); j++)
		{
			if (comments.at(j).address == Instruction_Pointer + (*CS) * 16)
			{
				SetConsoleTextAttribute(hConsole, 14);
				std::string sss = comments.at(j).text;
				cout << sss << endl;
				SetConsoleTextAttribute(hConsole, 7);
				break;
			}
		}

	}

	if (breakpoinst_ON)
	{
		for (int b = 0; b < breakpoints.size(); b++)
		{
			if ((Instruction_Pointer + *CS * 16) == breakpoints.at(b).address)   //breakpoints.at(b)
			{
				step_mode = true;
				cout << "Breakpoint " << breakpoints.at(b).text << " at " << hex << int_to_hex(*CS,4) << ":" << int_to_hex(Instruction_Pointer, 4) << endl;
				log_to_console = true;
				service_counter = 1;
				run_until_CX0 = false;
			}
		}
	}
}
