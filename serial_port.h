#pragma once
#include <chrono>
#include <vector>
#include "custom_classes.h"

using namespace std;

class SerialPort
{
private:
	uint8 reg_in_3F8 = 0; //регистр приемника
	uint8 reg_out_3F8 = 0; //регистр передатчика
	uint16 speed = 0;      //скорость обмена данными
	uint8 reg_int_ctrl = 0; //регистр управления прерываниями



public:
	SerialPort(); //конструктор
	uint8 read_port(uint16 address);
	void write_port(uint16 address, uint8 data);
	//void sync();
};