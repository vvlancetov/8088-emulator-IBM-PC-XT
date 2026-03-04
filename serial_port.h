#pragma once
#include <chrono>
#include <vector>
#include "custom_classes.h"

using namespace std;

enum class com_port_states { IN_IDLE, OUT_IDLE, WAIT_RECEIVER, IN_WAIT_HOST, IN_data_POPED };

class SerialPort
{
private:
	uint8 data_reg = 0;
	//uint8 reg_in_3F8 = 0;        //регистр приемника  
	//uint8 reg_out_3F8 = 0;      //регистр передатчика 
	uint16 speed_divider = 0;   //делитель скорости обмена данными
	uint8 reg_int_ctrl = 0;     //регистр управления прерываниями
	uint8 reg_int_ID = 1;       //Регистр идентификации прерывания
	uint8 reg_line_ctrl = 0;    //Регистр управления линией
	uint8 reg_modem_ctrl = 0;   //Регистр управления модемом
	uint8 reg_line_state = 0;   //Регистр состояния линии
	uint8 reg_modem_state = 0;  //Регистр состояния модема
	uint8 reg_modem_state_last = 0; //Регистр состояния модема (прошлое состояние)

	//состояние устройства
	com_port_states out_ctrl_state = com_port_states::OUT_IDLE;//состояние контроллера
	com_port_states in_ctrl_state = com_port_states::IN_IDLE;//состояние контроллера

public:
	SerialPort(); //конструктор
	uint8 read_port(uint16 address);
	void write_port(uint16 address, uint8 data);
	void sync();
	bool get_RTS() { return ((reg_modem_ctrl >> 1) & 1); }
	bool get_DTR() { return ((reg_modem_ctrl >> 0) & 1); }
	bool get_OUT1() { return ((reg_modem_ctrl >> 2) & 1); }
	bool get_OUT2() { return ((reg_modem_ctrl >> 3) & 1); }
	void receive_data(uint8 byte);
	std::string get_debug_data(uint8 id);
};