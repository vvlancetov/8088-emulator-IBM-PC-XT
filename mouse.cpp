#include <SFML/Window.hpp>
#include <conio.h>
#include <chrono>
#include <ctime>
#include <iostream>
#include "mouse.h"
#include "video.h"

using namespace std;

extern Monitor monitor;
extern IC8259 int_ctrl;

Mouse::Mouse()
{
	//конструктор


}

uint8 Mouse::read_port(uint16 address)
{

	if (address == 0x3FD)
	{
		if (power)
		{
			if (out_buffer.size())
			{
				uint8 out = out_buffer.at(0);
				out_buffer.erase(out_buffer.begin());
				cout << "read port 3FD -> " << (int)out << endl;
				return out;

			}
		}
	}
	
	if (address == 0x3FC)
	{
		if (power)
		{
			if (send_answer)
			{
				send_answer = 0;
				cout << "read port 3FC -> 77" << endl;
				return 77;
			}
		}
	}

	std::cout << "Mouse read port " << (int)address << " - > 0(nodata)" << std::endl;
	return 0;
}

void Mouse::write_port(uint16 address, uint8 data)
{
	//запись в порты 
	std::cout << "Mouse write port " << (int)address << " - > " << (int)data << std::endl;
	if (address == 0x3FC)
	{
		power = 1;
		if (data == 0xB) send_answer = 1;
	}



}

void Mouse::sync()
{

	// get the local mouse position (relative to a window)
	position_new = monitor.get_mouse_pos();
	
	//опрос клавишь
	buttons_new = 0;
	if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))  buttons_new = buttons_new | 0b00100000;
	if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) buttons_new = buttons_new | 0b00010000;

	if (position_new != position_old || buttons_new != buttons_old)
	{
		//инициируем обмен данными
		char Dx = position_new.x - position_old.x;
		char Dy = position_new.y - position_old.y;
		out_buffer.push_back(64 + buttons_new + ((Dy & 0b11000000) >> 4) + ((Dx & 0b11000000) >> 6));
		out_buffer.push_back(Dx & 0b00111111);
		out_buffer.push_back(Dy & 0b00111111);
		int_ctrl.request_IRQ(4);
	}

	position_old = position_new;
	buttons_old = buttons_new;

	if (out_buffer.size()) int_ctrl.request_IRQ(4);

}