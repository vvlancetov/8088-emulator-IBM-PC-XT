#include <conio.h>
#include <chrono>
#include <ctime>
#include <iostream>
#include "rtc.h"

uint8 to_BCD(uint8 data)
{
	uint8 convert = ((data / 10) << 4) | (data % 10);
	//std::cout << "BCD convert " << (int)data << " - > " << int(convert >> 4) << "+" << (int)(convert & 15) << std::endl;
	return convert;
}

void Rtc::flash_rom(uint32 address, uint8 data)
{
	rom_memory[address] = data;
}
uint8 Rtc::read_rom(uint32 address)
{
	return rom_memory[address];
}

uint8  Rtc::read_port(uint16 address)
{
	//считываем время
	std::time_t time_now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	tm* ltm = new tm;
	localtime_s(ltm, &time_now);
	
	if (address == 0x71)
	{
		if (sel_reg < 51)
		{
			//std::cout << "RTC read reg " << (int)sel_reg << " = ";
			
			if (sel_reg == 0x0) return to_BCD(ltm->tm_sec);
			if (sel_reg == 0x2) return to_BCD(ltm->tm_min);
			if (sel_reg == 0x4) return to_BCD(ltm->tm_hour);
			if (sel_reg == 0x6) return to_BCD(ltm->tm_wday + 1);			//день недели
			if (sel_reg == 0x7) return to_BCD(ltm->tm_mday);				//день
			if (sel_reg == 0x8) return to_BCD(ltm->tm_mon + 1);				//месяц
			if (sel_reg == 0x9) return to_BCD(ltm->tm_year % 100);  //год
			if (sel_reg == 0x32) return to_BCD((ltm->tm_year+1900) / 100);  //год

			if (sel_reg == 0xA)
			{
				//std::cout << ((int)registers[0xA] & 0x7F) << std::endl;
				return registers[0xA] & 0x7F; //старший бит = 0 (разрешение на чтение данных)
			}

			if (sel_reg == 0xB)
			{
				//std::cout << (int)registers[0xB] << std::endl;
				return registers[0xB]; //B
			}

			if (sel_reg == 0xC)
			{
				//std::cout << (int)registers[0xC] << std::endl;
				return registers[0xC]; //C
			}

			if (sel_reg == 0xD)
			{
				//std::cout << (int)0x80 << std::endl;
				return 0x80; //D
			}
			
			//std::cout << (int)registers[sel_reg] << std::endl;
			return registers[sel_reg];
		}
	}
	
	return 0;
}

void  Rtc::write_port(uint16 address, uint8 data)
{
	if (address == 0x70) sel_reg = data;
	if (address == 0x71)
	{
		//std::cout << "RTC write reg " << (int)sel_reg << " = " << (int)data << std::endl;
		
		//регистры даты и времени
		if (sel_reg <= 0x9) registers[sel_reg] = data;
		
		//управляющие регистры
		if (sel_reg == 0xA) registers[sel_reg] = data;
		
		if (sel_reg == 0xB) registers[sel_reg] = data;

		//дополнительные регистры
		if (sel_reg > 0xD) registers[sel_reg] = data;
	}
}
Rtc::Rtc()
{
	//начальные значения

	registers[0xB] = 0b00000010; //24 формат, без прерываний, BCD-кодировка
	registers[0xA] = 0b00100110; //по-умолчанию для платы
	registers[0xC] = 0; //по-умолчанию для платы
	registers[0xD] = 0x80; //по-умолчанию для платы

}