#pragma once
#include <chrono>
#include "custom_classes.h"

class Rtc
{
private:

	uint8 rom_memory[2048] = { 0 };
	uint8 sel_reg = 0;		//выбранные регистр
	
	//регистры
	uint8 registers[51] = { 0 };

public:
	Rtc(); //конструктор
	void flash_rom(uint32 address, uint8 data);
	uint8 read_rom(uint32 address);
	uint8 read_port(uint16 address);
	void write_port(uint16 address, uint8 data);
};