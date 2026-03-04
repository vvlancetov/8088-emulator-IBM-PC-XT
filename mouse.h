#pragma once
#include <chrono>
#include <vector>
#include "custom_classes.h"

using namespace std;

class Mouse
{
private:
	mouse_xy position_new; //позиции в окне
	mouse_xy position_old;
	uint8 buttons_new = 0; //состояния кнопок
	uint8 buttons_old = 0;
	bool send_answer = 0;  //требуется ответ на запрос

	vector<uint8> out_buffer;

	bool DCD = 0; //наличие связи с линией
	bool DSR = 0; //готовность к передаче
	bool IR = 0;  //индикатор входящего звонка
	bool CTS = 0; //готовность к приему

	//enum class KBD_key {in_idle,in_wait, in_

		
public:
	Mouse(); //конструктор
	void reseive_data(uint8 data);
	void poll_mouse();	//опрос мыши
	void sync();		//синхронизация, обмен данными
	bool get_DCD() { return DCD; }
	bool get_DSR() { return DSR; }
	bool get_IR() { return IR; }
	bool get_CTS() { return CTS; }
	std::string get_debug_data(uint8 id);
	uint8 get_modem_state(bool loopback); //получаем состояние входящих линий и изменение
};