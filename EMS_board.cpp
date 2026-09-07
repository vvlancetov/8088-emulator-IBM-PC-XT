#include "EMS_board.h"
#include <iostream>

using namespace std;

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

void EMS_board::page_0_select(uint8 page) 
{ 
	//cout << "EMS: bank 0 = " << (int)(page & 0x7F) <<  endl;
	page_selectors[0] = page & 0x7F; 
}
void EMS_board::page_1_select(uint8 page) 
{ 
	//cout << "EMS: bank 1 = " << (int)(page & 0x7F) << endl;
	page_selectors[1] = page & 0x7F; 
}
void EMS_board::page_2_select(uint8 page) 
{ 
	//cout << "EMS: bank 2 = " << (int)(page & 0x7F) << endl;
	page_selectors[2] = page & 0x7F; 
}
void EMS_board::page_3_select(uint8 page) 
{ 
	//cout << "EMS: bank 3 = " << (int)(page & 0x7F) << endl;
	page_selectors[3] = page & 0x7F; 
}
void EMS_board::write_ctrl_reg(uint8 data)
{
	//включение или выключение платы
	board_enable = data & 1;
	//cout << "board_EN = " << (int)board_enable << endl;
}
uint8 EMS_board::read_status() 
{ 
	//cout << "EMS: read status" << endl;
	return 0xA0; 
}
void EMS_board::write_config_1(uint8 data) { config_register_1 = data; }
void EMS_board::write_config_2(uint8 data) { config_register_2 = data; }
uint8 EMS_board::read_config_1() { return config_register_1; }
uint8 EMS_board::read_config_2() { return config_register_2; }
void EMS_board::write_mem(uint16 address, uint8 data)
{
	//if (!board_enable) return;
	if (address < 0x4000) { board_mem[address + page_selectors[0] * 16 * 1024] = data; return; }
	if (address < 0x8000) { board_mem[address + page_selectors[1] * 16 * 1024 - 0x4000] = data; return; }
	if (address < 0xC000) { board_mem[address + page_selectors[2] * 16 * 1024 - 0x8000] = data; return; }
	
	board_mem[address + page_selectors[3] * 16 * 1024 - 0xC000] = data;
	return;
}
uint8 EMS_board::read_mem(uint16 address)
{
	//if (!board_enable) return 255;
	if (address < 0x4000) return board_mem[address + page_selectors[0] * 16 * 1024];
	if (address < 0x8000) return board_mem[address + page_selectors[1] * 16 * 1024 - 0x4000];
	if (address < 0xC000) return board_mem[address + page_selectors[2] * 16 * 1024 - 0x8000];
	return board_mem[address + page_selectors[3] * 16 * 1024 - 0xC000];
}