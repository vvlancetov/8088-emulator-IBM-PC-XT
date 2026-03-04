#include <SFML/Window.hpp>
#include <conio.h>
#include <chrono>
#include <ctime>
#include <iostream>
#include "mouse.h"
#include "video.h"
#include "serial_port.h"

using namespace std;

extern Monitor monitor;
extern SerialPort COM1;

extern bool step_mode;
extern bool log_to_console;

Mouse::Mouse()
{
	//конструктор
}

void Mouse::reseive_data(uint8 data)
{
	//получение пакетов от ПК
	//if (data == 0xB && COM1.get_RTS()) out_buffer.push_back(77); //отвечаем на сигнал
	//cout << "Mouse received " << (int)data << endl;
    //if (COM1.get_RTS()) out_buffer.push_back(data); //отвечаем на сигнал
	//COM1.receive_data(data); //loop
}

void Mouse::poll_mouse()
{
	//не обновляемся в пошаговом режиме
	if (step_mode) return;
	//опрос курсора
	position_new = monitor.get_mouse_pos();
	
	//cout << "new_x = " << (int)position_new.mouse_x << " new_y = " << (int)position_new.mouse_y << endl;

	//опрос клавишь
	buttons_new = 0;
	if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))  buttons_new = buttons_new | 0b00100000;
	if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) buttons_new = buttons_new | 0b00010000;

	//рассчитываем изменения
	char Dx = (position_new.mouse_x - position_old.mouse_x) / position_new.screen_scale * 2;
	if (position_new.mouse_x < 0) Dx = - 50 / position_new.screen_scale;
	if (position_new.mouse_x > position_new.screen_size_x) Dx = 50 / position_new.screen_scale;
	
	char Dy = (position_new.mouse_y - position_old.mouse_y) / position_new.screen_scale * 2;
	if (position_new.mouse_y < 0) Dy = -50 / position_new.screen_scale;
	if (position_new.mouse_y > position_new.screen_size_y) Dy = 50 / position_new.screen_scale;

	if ((Dx || Dy || (buttons_old != buttons_new) )&& COM1.get_RTS())
	{
		out_buffer.push_back(64 + buttons_new + ((Dy & 0b11000000) >> 4) + ((Dx & 0b11000000) >> 6));
		out_buffer.push_back(Dx & 0b00111111);
		out_buffer.push_back(Dy & 0b00111111);
	}

	position_old = position_new;
	buttons_old = buttons_new;
}

void Mouse::sync()
{
	static bool reset = 0; //флаг перезагрузки
	
	if (out_buffer.size() && COM1.get_RTS() && COM1.get_DTR() && !reset)
	{
		
		//пора передать данные хосту
		if (log_to_console) cout << "Mouse sent data " << (int)out_buffer.at(0) << endl;
		COM1.receive_data(out_buffer.at(0));
		out_buffer.erase(out_buffer.begin());
	}
	
	if (reset && COM1.get_RTS() && COM1.get_DTR()) reset = 0;

	if (!COM1.get_RTS() && COM1.get_DTR() && !reset)
	{
		//перезагрузка мыши и отправка байта
		out_buffer.clear();
		out_buffer.push_back(77);
		if (log_to_console) cout << "Mouse RESET" << endl;
		reset = 1;
		//step_mode = 1;
	}
}

uint8 Mouse::get_modem_state(bool loopback)
{
	static uint8 old_state = 0;
	uint8 new_state = 0;
	//конструируем новое состояние
	if (loopback) new_state = (COM1.get_DTR() << 7) | (1 << 6)| (COM1.get_DTR() << 5) | (COM1.get_RTS() << 4);
	else new_state = (DCD << 7) | (IR << 6) | (DSR << 5) | (CTS << 4);

	new_state = new_state | ((old_state ^ new_state) >> 4);
	old_state = new_state & 0xF0;

	if (log_to_console) cout << "COM read reg_modem_state <- " << (int)new_state << endl;
	return new_state;
}


std::string Mouse::get_debug_data(uint8 id)
{
	if (id == 1) return "Mouse buffer size " + to_string(out_buffer.size());
	return "no data";
}

