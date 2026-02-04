#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <bitset>
#include <conio.h>
#include "joystick.h"
#include "video.h"


typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;

extern Video_device monitor;

using namespace std;

game_controller::game_controller()
{
	//конструктор
	for (int i = 0; i < 8; i++)
	{
		if (sf::Joystick::isConnected(i))
		{
			//обнаружен джойстик
			joystick_id = i;
			joystick_active = 1;
			cout << "joystick active = " << (int)joystick_id << endl;
			break;
		}
	}
	if (!joystick_active) cout << "joystick not found" << endl;
}

void game_controller::start_meazure()
{
	
	countdown = 1;
	counter = 128;

	counter_X = central_point;
	counter_Y = central_point;

	//sf::Joystick::update(); //обновляем состояние

	if (joystick_active)
	{
		//крестовина
		if (sf::Joystick::getAxisPosition(joystick_id, sf::Joystick::Axis::PovY) == 100) counter_Y = 0;		//UP
		if (sf::Joystick::getAxisPosition(joystick_id, sf::Joystick::Axis::PovY) == -100) counter_Y = central_point * 2;	//DOWN
		if (sf::Joystick::getAxisPosition(joystick_id, sf::Joystick::Axis::PovX) == -100) counter_X = 0;	//LEFT
		if (sf::Joystick::getAxisPosition(joystick_id, sf::Joystick::Axis::PovX) == 100) counter_X = central_point * 2;	//RIGHT
	}
}

uint8 game_controller::get_input()
{
	if (!joystick_active) return 255;

	uint8 out = 0b11110000;
	if (joystick_active)
	{
		if (sf::Joystick::isButtonPressed(joystick_id, 0)) out = out & 0b11101111;
		if (sf::Joystick::isButtonPressed(joystick_id, 1)) out = out & 0b11011111;
		if (sf::Joystick::isButtonPressed(joystick_id, 2)) out = out & 0b11101111;
		if (sf::Joystick::isButtonPressed(joystick_id, 3)) out = out & 0b11011111;
	}

	if (countdown)
	{
		counter_X--; // счетчики осей
		counter_Y--;
		if (counter_X > 0) out = out | 1; //устанавливаем биты осей
		if (counter_Y > 0) out = out | 2;

		counter--;
		if (!counter) countdown = false; //отключение счетчика
	}

	//std::cout << "JOY ret " << (std::bitset<8>)out;
	
	return out;
	//return 0;
}

void game_controller::sync(uint32 elapsed_us)
{
	calibr_timer += elapsed_us;


	if (sf::Joystick::isButtonPressed(joystick_id, 4) && calibr_timer > 40000)
	{
		central_point--;
		if (central_point < 10) central_point = 10; //минимум 10
		//cout << " CP - (" << (int)central_point << ")" << endl;
		monitor.show_joy_sence(central_point);
		calibr_timer = 0;
	}
	if (sf::Joystick::isButtonPressed(joystick_id, 5) && calibr_timer > 40000)
	{
		central_point++;
		if (central_point > 128) central_point = 127; //максимум 128
		//cout << " CP + (" << (int)central_point << ")" << endl;
		monitor.show_joy_sence(central_point);
		calibr_timer = 0;
	}
	
	check_state_timer += elapsed_us;
	if (elapsed_us > 5000000)
	{
		//прошло 5 секунд
		sf::Joystick::update(); //обновляем состояние
		if (joystick_active)
		{
			//если джойстик отвалился
			if (!sf::Joystick::isConnected(joystick_id))
			{
				//отвал
				cout << "joystick lost id = " << (int)joystick_id << endl;
				joystick_active = 0;
			}
		}
		else
		{
			//вдруг появился новый джой в системе
			for (int i = 0; i < 8; i++)
			{
				if (sf::Joystick::isConnected(i))
				{
					//обнаружен новый джойстик
					joystick_id = i;
					joystick_active = 1;
					cout << "joystick found id = " << (int)joystick_id << endl;
					break;
				}
			}
		}
	}
}