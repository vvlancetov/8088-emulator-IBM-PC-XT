#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <bitset>
#include <conio.h>
#include "joystick.h"

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;

void game_controller::start_meazure()
{
	countdown = 1;
	counter = 120;

	counter_X = 60;
	counter_Y = 60;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Numpad8)) counter_Y = 60; //UP
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Numpad2)) counter_Y = 0; //DOWN
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Numpad4)) counter_X = 0; //LEFT
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Numpad6)) counter_X = 60; //RIGHT
}

uint8 game_controller::get_input()
{
	uint8 out = 0b11110000;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Numpad7)) out = out & 0b11101111;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Numpad9)) out = out & 0b11011111;

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