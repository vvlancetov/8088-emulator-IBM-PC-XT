#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Window.hpp>
#include <conio.h>
#include <iostream>
#include <vector>
#include "custom_classes.h"
#include "keyboard.h"

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;

using namespace std;

extern IC8259 int_ctrl;
extern bool log_to_console;
extern bool step_mode;

extern IC8255 ppi_ctrl;

//клавиатура
void KBD::poll_keys(uint32 elapsed_us, bool has_focus)
{
	//has_focus - флаг наличия фокуса у окна монитора
	if (step_mode) has_focus = 0; //в пошаговом режиме отключаем нажатия
	//синхронизируем нажатия клавишь
	if (!enabled) has_focus = 0;  //отключаем новые нажатия
	if (!CLK_high) has_focus = 0; //

	bool special = false;
	uint8 code = 0;

	//записываем нажатия в буфер

	// ====================  F1-F12 ============
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F1) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::F1)])
		{
			out_buffer.push_back(0x3b);
			pressed_keys[(uint8)(KBD_key::F1)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::F1)])
		{
			out_buffer.push_back(0x3b + 0x80);
			pressed_keys[(int)(KBD_key::F1)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F2) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::F2)])
		{
			out_buffer.push_back(0x3c);
			pressed_keys[(uint8)(KBD_key::F2)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::F2)])
		{
			out_buffer.push_back(0x3c + 0x80);
			pressed_keys[(int)(KBD_key::F2)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F3) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::F3)])
		{
			out_buffer.push_back(0x3d);
			pressed_keys[(uint8)(KBD_key::F3)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::F3)])
		{
			out_buffer.push_back(0x3d + 0x80);
			pressed_keys[(int)(KBD_key::F3)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F4) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::F4)])
		{
			out_buffer.push_back(0x3e);
			pressed_keys[(uint8)(KBD_key::F4)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::F4)])
		{
			out_buffer.push_back(0x3e + 0x80);
			pressed_keys[(int)(KBD_key::F4)] = 0;
		}
	}
	
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F5) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::F5)])
		{
			out_buffer.push_back(0x3f);
			pressed_keys[(uint8)(KBD_key::F5)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::F5)])
		{
			out_buffer.push_back(0x3f + 0x80);
			pressed_keys[(int)(KBD_key::F5)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F6) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::F6)])
		{
			out_buffer.push_back(0x40);
			pressed_keys[(uint8)(KBD_key::F6)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::F6)])
		{
			out_buffer.push_back(0x40 + 0x80);
			pressed_keys[(int)(KBD_key::F6)] = 0;
		}
	}
	
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F7) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::F7)])
		{
			out_buffer.push_back(0x41);
			pressed_keys[(uint8)(KBD_key::F7)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::F7)])
		{
			out_buffer.push_back(0x41 + 0x80);
			pressed_keys[(int)(KBD_key::F7)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F8) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::F8)])
		{
			out_buffer.push_back(0x42);
			pressed_keys[(uint8)(KBD_key::F8)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::F8)])
		{
			out_buffer.push_back(0x42 + 0x80);
			pressed_keys[(int)(KBD_key::F8)] = 0;
		}
	}
	
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F9) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::F9)])
		{
			out_buffer.push_back(0x43);
			pressed_keys[(uint8)(KBD_key::F9)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::F9)])
		{
			out_buffer.push_back(0x43 + 0x80);
			pressed_keys[(int)(KBD_key::F9)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F10) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::F10)])
		{
			out_buffer.push_back(0x44);
			pressed_keys[(uint8)(KBD_key::F10)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::F10)])
		{
			out_buffer.push_back(0x44 + 0x80);
			pressed_keys[(int)(KBD_key::F10)] = 0;
		}
	}


	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F11) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::F11)])
		{
			out_buffer.push_back(0x57);
			pressed_keys[(uint8)(KBD_key::F11)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::F11)])
		{
			out_buffer.push_back(0x57 + 0x80);
			pressed_keys[(int)(KBD_key::F11)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F12) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::F12)])
		{
			out_buffer.push_back(0x58);
			pressed_keys[(uint8)(KBD_key::F12)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::F12)])
		{
			out_buffer.push_back(0x58 + 0x80);
			pressed_keys[(int)(KBD_key::F12)] = 0;
		}
	}


	// ==================== num 1 - 0 ==============

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num1) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Num1)])
		{
			out_buffer.push_back(0x02);
			pressed_keys[(uint8)(KBD_key::Num1)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Num1)])
		{
			out_buffer.push_back(0x02 + 0x80);
			pressed_keys[(int)(KBD_key::Num1)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num2) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Num2)])
		{
			out_buffer.push_back(0x03);
			pressed_keys[(uint8)(KBD_key::Num2)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Num2)])
		{
			out_buffer.push_back(0x03 + 0x80);
			pressed_keys[(int)(KBD_key::Num2)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num3) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Num3)])
		{
			out_buffer.push_back(0x04);
			pressed_keys[(uint8)(KBD_key::Num3)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Num3)])
		{
			out_buffer.push_back(0x04 + 0x80);
			pressed_keys[(int)(KBD_key::Num3)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num4) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Num4)])
		{
			out_buffer.push_back(0x05);
			pressed_keys[(uint8)(KBD_key::Num4)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Num4)])
		{
			out_buffer.push_back(0x05 + 0x80);
			pressed_keys[(int)(KBD_key::Num4)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num5) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Num5)])
		{
			out_buffer.push_back(0x06);
			pressed_keys[(uint8)(KBD_key::Num5)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Num5)])
		{
			out_buffer.push_back(0x06 + 0x80);
			pressed_keys[(int)(KBD_key::Num5)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num6) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Num6)])
		{
			out_buffer.push_back(0x07);
			pressed_keys[(uint8)(KBD_key::Num6)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Num6)])
		{
			out_buffer.push_back(0x07 + 0x80);
			pressed_keys[(int)(KBD_key::Num6)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num7) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Num7)])
		{
			out_buffer.push_back(0x08);
			pressed_keys[(uint8)(KBD_key::Num7)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Num7)])
		{
			out_buffer.push_back(0x08 + 0x80);
			pressed_keys[(int)(KBD_key::Num7)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num8) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Num8)])
		{
			out_buffer.push_back(0x09);
			pressed_keys[(uint8)(KBD_key::Num8)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Num8)])
		{
			out_buffer.push_back(0x09 + 0x80);
			pressed_keys[(int)(KBD_key::Num8)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num9) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Num9)])
		{
			out_buffer.push_back(0x0A);
			pressed_keys[(uint8)(KBD_key::Num9)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Num9)])
		{
			out_buffer.push_back(0x0A + 0x80);
			pressed_keys[(int)(KBD_key::Num9)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num0) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Num0)])
		{
			out_buffer.push_back(0x0B);
			pressed_keys[(uint8)(KBD_key::Num0)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Num0)])
		{
			out_buffer.push_back(0x0B + 0x80);
			pressed_keys[(int)(KBD_key::Num0)] = 0;
		}
	}


	// ======================= A - Z ===============

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Q)])
		{
			out_buffer.push_back(0x10);
			pressed_keys[(uint8)(KBD_key::Q)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Q)])
		{
			out_buffer.push_back(0x10 + 0x80);
			pressed_keys[(int)(KBD_key::Q)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::W)])
		{
			out_buffer.push_back(0x11);
			pressed_keys[(uint8)(KBD_key::W)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::W)])
		{
			out_buffer.push_back(0x11 + 0x80);
			pressed_keys[(int)(KBD_key::W)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::E)])
		{
			out_buffer.push_back(0x12);
			pressed_keys[(uint8)(KBD_key::E)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::E)])
		{
			out_buffer.push_back(0x12 + 0x80);
			pressed_keys[(int)(KBD_key::E)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::R)])
		{
			out_buffer.push_back(0x13);
			pressed_keys[(uint8)(KBD_key::R)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::R)])
		{
			out_buffer.push_back(0x93);
			pressed_keys[(int)(KBD_key::R)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::T) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::T)])
		{
			out_buffer.push_back(0x14);
			pressed_keys[(uint8)(KBD_key::T)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::T)])
		{
			out_buffer.push_back(0x94);
			pressed_keys[(int)(KBD_key::T)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Y) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Y)])
		{
			out_buffer.push_back(0x15);
			pressed_keys[(uint8)(KBD_key::Y)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Y)])
		{
			out_buffer.push_back(0x95);
			pressed_keys[(int)(KBD_key::Y)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::U) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::U)])
		{
			out_buffer.push_back(0x16);
			pressed_keys[(uint8)(KBD_key::U)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::U)])
		{
			out_buffer.push_back(0x96);
			pressed_keys[(int)(KBD_key::U)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::I) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::I)])
		{
			out_buffer.push_back(0x17);
			pressed_keys[(uint8)(KBD_key::I)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::I)])
		{
			out_buffer.push_back(0x97);
			pressed_keys[(int)(KBD_key::I)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::O) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::O)])
		{
			out_buffer.push_back(0x18);
			pressed_keys[(uint8)(KBD_key::O)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::O)])
		{
			out_buffer.push_back(0x98);
			pressed_keys[(int)(KBD_key::O)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::P) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::P)])
		{
			out_buffer.push_back(0x19);
			pressed_keys[(uint8)(KBD_key::P)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::P)])
		{
			out_buffer.push_back(0x99);
			pressed_keys[(int)(KBD_key::P)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::A)])
		{
			out_buffer.push_back(0x1E);
			pressed_keys[(uint8)(KBD_key::A)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::A)])
		{
			out_buffer.push_back(0x9E);
			pressed_keys[(int)(KBD_key::A)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::S)])
		{
			out_buffer.push_back(0x1F);
			pressed_keys[(uint8)(KBD_key::S)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::S)])
		{
			out_buffer.push_back(0x9F);
			pressed_keys[(int)(KBD_key::S)] = 0;
		}
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::D)])
		{
			out_buffer.push_back(0x20);
			pressed_keys[(uint8)(KBD_key::D)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::D)])
		{
			out_buffer.push_back(0xA0);
			pressed_keys[(int)(KBD_key::D)] = 0;
		}
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::F)])
		{
			out_buffer.push_back(0x21);
			pressed_keys[(uint8)(KBD_key::F)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::F)])
		{
			out_buffer.push_back(0xA1);
			pressed_keys[(int)(KBD_key::F)] = 0;
		}
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::G) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::G)])
		{
			out_buffer.push_back(0x22);
			pressed_keys[(uint8)(KBD_key::G)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::G)])
		{
			out_buffer.push_back(0xA2);
			pressed_keys[(int)(KBD_key::G)] = 0;
		}
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::H) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::H)])
		{
			out_buffer.push_back(0x23);
			pressed_keys[(uint8)(KBD_key::H)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::H)])
		{
			out_buffer.push_back(0xA3);
			pressed_keys[(int)(KBD_key::H)] = 0;
		}
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::J) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::J)])
		{
			out_buffer.push_back(0x24);
			pressed_keys[(uint8)(KBD_key::J)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::J)])
		{
			out_buffer.push_back(0xA4);
			pressed_keys[(int)(KBD_key::J)] = 0;
		}
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::K) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::K)])
		{
			out_buffer.push_back(0x25);
			pressed_keys[(uint8)(KBD_key::K)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::K)])
		{
			out_buffer.push_back(0xA5);
			pressed_keys[(int)(KBD_key::K)] = 0;
		}
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::L)])
		{
			out_buffer.push_back(0x26);
			pressed_keys[(uint8)(KBD_key::L)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::L)])
		{
			out_buffer.push_back(0xA6);
			pressed_keys[(int)(KBD_key::L)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Z) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Z)])
		{
			out_buffer.push_back(0x2C);
			pressed_keys[(uint8)(KBD_key::Z)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Z)])
		{
			out_buffer.push_back(0xAC);
			pressed_keys[(int)(KBD_key::Z)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::X) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::X)])
		{
			out_buffer.push_back(0x2D);
			pressed_keys[(uint8)(KBD_key::X)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::X)])
		{
			out_buffer.push_back(0xAD);
			pressed_keys[(int)(KBD_key::X)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::C)])
		{
			out_buffer.push_back(0x2E);
			pressed_keys[(uint8)(KBD_key::C)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::C)])
		{
			out_buffer.push_back(0xAE);
			pressed_keys[(int)(KBD_key::C)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::V) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::V)])
		{
			out_buffer.push_back(0x2F);
			pressed_keys[(uint8)(KBD_key::V)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::V)])
		{
			out_buffer.push_back(0xAF);
			pressed_keys[(int)(KBD_key::V)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::B) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::B)])
		{
			out_buffer.push_back(0x30);
			pressed_keys[(uint8)(KBD_key::B)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::B)])
		{
			out_buffer.push_back(0xB0);
			pressed_keys[(int)(KBD_key::B)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::N) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::N)])
		{
			out_buffer.push_back(0x31);
			pressed_keys[(uint8)(KBD_key::N)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::N)])
		{
			out_buffer.push_back(0xB1);
			pressed_keys[(int)(KBD_key::N)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::M) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::M)])
		{
			out_buffer.push_back(0x32);
			pressed_keys[(uint8)(KBD_key::M)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::M)])
		{
			out_buffer.push_back(0xB2);
			pressed_keys[(int)(KBD_key::M)] = 0;
		}
	}


	// ======================= Others ==============

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Enter) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Enter)])
		{
			out_buffer.push_back(0x1C);
			pressed_keys[(uint8)(KBD_key::Enter)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Enter)])
		{
			out_buffer.push_back(0x9C);
			pressed_keys[(int)(KBD_key::Enter)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Space)])
		{
			out_buffer.push_back(0x39);
			pressed_keys[(uint8)(KBD_key::Space)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Space)])
		{
			out_buffer.push_back(0x39 + 0x80);
			pressed_keys[(int)(KBD_key::Space)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Grave) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Grave)])
		{
			out_buffer.push_back(0x29);
			pressed_keys[(uint8)(KBD_key::Grave)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(uint8)(KBD_key::Grave)])
		{
			out_buffer.push_back(0xA9);
			pressed_keys[(uint8)(KBD_key::Grave)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Hyphen) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Hyphen)])
		{
			out_buffer.push_back(0x0C);
			pressed_keys[(uint8)(KBD_key::Hyphen)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(uint8)(KBD_key::Hyphen)])
		{
			out_buffer.push_back(0x8C);
			pressed_keys[(uint8)(KBD_key::Hyphen)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Equal) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Equal)])
		{
			out_buffer.push_back(0x0D);
			pressed_keys[(uint8)(KBD_key::Equal)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Equal)])
		{
			out_buffer.push_back(0x8D);
			pressed_keys[(uint8)(KBD_key::Equal)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Backspace) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Backspace)])
		{
			out_buffer.push_back(0x0E);
			pressed_keys[(uint8)(KBD_key::Backspace)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Backspace)])
		{
			out_buffer.push_back(0x8E);
			pressed_keys[(int)(KBD_key::Backspace)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Escape)])
		{
			out_buffer.push_back(0x01);
			pressed_keys[(uint8)(KBD_key::Escape)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Escape)])
		{
			out_buffer.push_back(0x81);
			pressed_keys[(int)(KBD_key::Escape)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Tab) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Tab)])
		{
			out_buffer.push_back(0x0F);
			pressed_keys[(uint8)(KBD_key::Tab)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Tab)])
		{
			out_buffer.push_back(0x8F);
			pressed_keys[(int)(KBD_key::Tab)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::LShift)])
		{
			out_buffer.push_back(0x2A);
			pressed_keys[(uint8)(KBD_key::LShift)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::LShift)])
		{
			out_buffer.push_back(0xAA);
			pressed_keys[(int)(KBD_key::LShift)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::RShift)])
		{
			out_buffer.push_back(0x36);
			pressed_keys[(uint8)(KBD_key::RShift)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::RShift)])
		{
			out_buffer.push_back(0xB6);
			pressed_keys[(int)(KBD_key::RShift)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::LControl)])
		{
			out_buffer.push_back(0x1D);
			pressed_keys[(uint8)(KBD_key::LControl)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::LControl)])
		{
			out_buffer.push_back(0x9D);
			pressed_keys[(int)(KBD_key::LControl)] = 0;
		}
	}
	
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::RControl)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0x1D);
			pressed_keys[(uint8)(KBD_key::RControl)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::RControl)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0x9D);
			pressed_keys[(int)(KBD_key::RControl)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::LAlt)])
		{
			out_buffer.push_back(0x38);
			pressed_keys[(uint8)(KBD_key::LAlt)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::LAlt)])
		{
			out_buffer.push_back(0xB8);
			pressed_keys[(int)(KBD_key::LAlt)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RAlt) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::RAlt)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0x38);
			pressed_keys[(uint8)(KBD_key::RAlt)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::RAlt)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0xB8);
			pressed_keys[(int)(KBD_key::RAlt)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Backslash) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Backslash)])
		{
			out_buffer.push_back(0x2B);
			pressed_keys[(uint8)(KBD_key::Backslash)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Backslash)])
		{
			out_buffer.push_back(0xAB);
			pressed_keys[(int)(KBD_key::Backslash)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LBracket) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::LBracket)])
		{
			out_buffer.push_back(0x1A);
			pressed_keys[(uint8)(KBD_key::LBracket)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::LBracket)])
		{
			out_buffer.push_back(0x9A);
			pressed_keys[(int)(KBD_key::LBracket)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RBracket) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::RBracket)])
		{
			out_buffer.push_back(0x1B);
			pressed_keys[(uint8)(KBD_key::RBracket)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(uint8)(KBD_key::RBracket)])
		{
			out_buffer.push_back(0x9B);
			pressed_keys[(uint8)(KBD_key::RBracket)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Semicolon) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Semicolon)])
		{
			out_buffer.push_back(0x27);
			pressed_keys[(uint8)(KBD_key::Semicolon)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Semicolon)])
		{
			out_buffer.push_back(0xA7);
			pressed_keys[(int)(KBD_key::Semicolon)] = 0;  //
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Apostrophe) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Apostrophe)])
		{
			out_buffer.push_back(0x28);
			pressed_keys[(uint8)(KBD_key::Apostrophe)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Apostrophe)])
		{
			out_buffer.push_back(0xA8);
			pressed_keys[(int)(KBD_key::Apostrophe)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Comma) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Comma)])
		{
			out_buffer.push_back(0x33);
			pressed_keys[(uint8)(KBD_key::Comma)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Comma)])
		{
			out_buffer.push_back(0xB3);
			pressed_keys[(int)(KBD_key::Comma)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Period) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Period)])
		{
			out_buffer.push_back(0x34);
			pressed_keys[(uint8)(KBD_key::Period)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Period)])
		{
			out_buffer.push_back(0xB4);
			pressed_keys[(int)(KBD_key::Period)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Slash) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Slash)])
		{
			out_buffer.push_back(0x35);
			pressed_keys[(uint8)(KBD_key::Slash)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Slash)])
		{
			out_buffer.push_back(0xB5);
			pressed_keys[(int)(KBD_key::Slash)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Delete) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Delete)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0x53);
			pressed_keys[(uint8)(KBD_key::Delete)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Delete)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0xD3);
			pressed_keys[(int)(KBD_key::Delete)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Insert) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Insert)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0x52);
			pressed_keys[(uint8)(KBD_key::Insert)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Insert)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0xD2);
			pressed_keys[(int)(KBD_key::Insert)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Home) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Home)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0x47);
			pressed_keys[(uint8)(KBD_key::Home)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Home)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0xC7);
			pressed_keys[(int)(KBD_key::Home)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::End) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::End)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0x4F);
			pressed_keys[(uint8)(KBD_key::End)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::End)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0xCF);
			pressed_keys[(int)(KBD_key::End)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::PageUp) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::PageUp)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0x49);
			pressed_keys[(uint8)(KBD_key::PageUp)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::PageUp)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0xC9);
			pressed_keys[(int)(KBD_key::PageUp)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::PageDown) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::PageDown)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0x51);
			pressed_keys[(uint8)(KBD_key::PageDown)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::PageDown)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0xD1);
			pressed_keys[(int)(KBD_key::PageDown)] = 0;
		}
	}
	
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::PrintScreen) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::PrintScreen)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0x2A);
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0x37);
			pressed_keys[(uint8)(KBD_key::PrintScreen)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::PrintScreen)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0xB7);
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0xAA);
			pressed_keys[(int)(KBD_key::PrintScreen)] = 0;
		}
	}
	
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::CapsLock) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::CapsLock)])
		{
			out_buffer.push_back(0x3A);
			pressed_keys[(uint8)(KBD_key::CapsLock)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(uint8)(KBD_key::CapsLock)])
		{
			out_buffer.push_back(0xBA);
			pressed_keys[(uint8)(KBD_key::CapsLock)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::NumLock) && has_focus)
	{
		cout << "Numlock!" << endl;
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::NumLock)])
		{
			out_buffer.push_back(0x45);
			pressed_keys[(uint8)(KBD_key::NumLock)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(uint8)(KBD_key::NumLock)])
		{
			out_buffer.push_back(0xC5);
			pressed_keys[(uint8)(KBD_key::NumLock)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::ScrollLock) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::ScrLock)])
		{
			out_buffer.push_back(0x46);
			pressed_keys[(uint8)(KBD_key::ScrLock)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(uint8)(KBD_key::ScrLock)])
		{
			out_buffer.push_back(0xC6);
			pressed_keys[(uint8)(KBD_key::ScrLock)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Pause) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Pause)])
		{
			out_buffer.push_back(0xE1);
			out_buffer.push_back(0x1D);
			out_buffer.push_back(0x45);
			out_buffer.push_back(0xE1);
			out_buffer.push_back(0x9D);
			out_buffer.push_back(0xC5);
			pressed_keys[(uint8)(KBD_key::Pause)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(uint8)(KBD_key::Pause)])
		{
			pressed_keys[(uint8)(KBD_key::Pause)] = 0;
		}
	}

	//=================== NumPad =========================

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::NumpadDivide) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::NumpadDivide)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0x35);
			pressed_keys[(uint8)(KBD_key::NumpadDivide)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(uint8)(KBD_key::NumpadDivide)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0xB5);
			pressed_keys[(uint8)(KBD_key::NumpadDivide)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::NumpadMultiply) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::NumpadMultiply)])
		{
			out_buffer.push_back(0x37);
			pressed_keys[(uint8)(KBD_key::NumpadMultiply)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(uint8)(KBD_key::NumpadMultiply)])
		{
			out_buffer.push_back(0xB7);
			pressed_keys[(uint8)(KBD_key::NumpadMultiply)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::NumpadMinus) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::NumpadMinus)])
		{
			out_buffer.push_back(0x4A);
			pressed_keys[(uint8)(KBD_key::NumpadMinus)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(uint8)(KBD_key::NumpadMinus)])
		{
			out_buffer.push_back(0xCA);
			pressed_keys[(uint8)(KBD_key::NumpadMinus)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::NumpadPlus) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::NumpadPlus)])
		{
			out_buffer.push_back(0x4E);
			pressed_keys[(uint8)(KBD_key::NumpadPlus)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(uint8)(KBD_key::NumpadPlus)])
		{
			out_buffer.push_back(0xCE);
			pressed_keys[(uint8)(KBD_key::NumpadPlus)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::NumpadEnter) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::NumpadEnter)])
		{
			//out_buffer.push_back(0xE0);
			//out_buffer.push_back(0x1C);
			pressed_keys[(uint8)(KBD_key::NumpadEnter)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(uint8)(KBD_key::NumpadEnter)])
		{
			//out_buffer.push_back(0xE0);
			//out_buffer.push_back(0x9C);
			pressed_keys[(uint8)(KBD_key::NumpadEnter)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Numpad0) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Numpad0)])
		{
			out_buffer.push_back(0x52);
			pressed_keys[(uint8)(KBD_key::Numpad0)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(uint8)(KBD_key::Numpad0)])
		{
			out_buffer.push_back(0xD2);
			pressed_keys[(uint8)(KBD_key::Numpad0)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::NumpadDecimal) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::NumpadDecimal)])
		{
			out_buffer.push_back(0x53);
			pressed_keys[(uint8)(KBD_key::NumpadDecimal)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(uint8)(KBD_key::NumpadDecimal)])
		{
			out_buffer.push_back(0xD3);
			pressed_keys[(uint8)(KBD_key::NumpadDecimal)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Numpad1) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Numpad1)])
		{
			out_buffer.push_back(0x4F);
			pressed_keys[(uint8)(KBD_key::Numpad1)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(uint8)(KBD_key::Numpad1)])
		{
			out_buffer.push_back(0xCF);
			pressed_keys[(uint8)(KBD_key::Numpad1)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Numpad2) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Numpad2)])
		{
			out_buffer.push_back(0x50);
			pressed_keys[(uint8)(KBD_key::Numpad2)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(uint8)(KBD_key::Numpad2)])
		{
			out_buffer.push_back(0xD0);
			pressed_keys[(uint8)(KBD_key::Numpad2)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Numpad3) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Numpad3)])
		{
			out_buffer.push_back(0x51);
			pressed_keys[(uint8)(KBD_key::Numpad3)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(uint8)(KBD_key::Numpad3)])
		{
			out_buffer.push_back(0xD1);
			pressed_keys[(uint8)(KBD_key::Numpad3)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Numpad4) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Numpad4)])
		{
			out_buffer.push_back(0x4B);
			pressed_keys[(uint8)(KBD_key::Numpad4)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(uint8)(KBD_key::Numpad4)])
		{
			out_buffer.push_back(0xCB);
			pressed_keys[(uint8)(KBD_key::Numpad4)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Numpad5) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Numpad5)])
		{
			out_buffer.push_back(0x4C);
			pressed_keys[(uint8)(KBD_key::Numpad5)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(uint8)(KBD_key::Numpad5)])
		{
			out_buffer.push_back(0xCC);
			pressed_keys[(uint8)(KBD_key::Numpad5)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Numpad6) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Numpad6)])
		{
			out_buffer.push_back(0x4D);
			pressed_keys[(uint8)(KBD_key::Numpad6)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(uint8)(KBD_key::Numpad6)])
		{
			out_buffer.push_back(0xCD);
			pressed_keys[(uint8)(KBD_key::Numpad6)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Numpad7) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Numpad7)])
		{
			out_buffer.push_back(0x47);
			pressed_keys[(uint8)(KBD_key::Numpad7)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(uint8)(KBD_key::Numpad7)])
		{
			out_buffer.push_back(0xC7);
			pressed_keys[(uint8)(KBD_key::Numpad7)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Numpad8) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Numpad8)])
		{
			out_buffer.push_back(0x48);
			pressed_keys[(uint8)(KBD_key::Numpad8)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(uint8)(KBD_key::Numpad8)])
		{
			out_buffer.push_back(0xC8);
			pressed_keys[(uint8)(KBD_key::Numpad8)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Numpad9) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Numpad9)])
		{
			out_buffer.push_back(0x49);
			pressed_keys[(uint8)(KBD_key::Numpad9)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(uint8)(KBD_key::Numpad9)])
		{
			out_buffer.push_back(0xC9);
			pressed_keys[(uint8)(KBD_key::Numpad9)] = 0;
		}
	}

	//=================== ARROWS =========================

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Up)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0x48);
			pressed_keys[(uint8)(KBD_key::Up)] = 1;
		}
		else
		{
			pressed_time_us[(uint8)(KBD_key::Up)] += elapsed_us;
			if (pressed_time_us[(uint8)(KBD_key::Up)] > 200000)
			{
				//out_buffer.push_back(0xE0);
				//out_buffer.push_back(0xC8);
				out_buffer.push_back(0xE0);
				out_buffer.push_back(0x48);
				pressed_time_us[(uint8)(KBD_key::Up)] = 0;
			}
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Up)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0xC8);
			pressed_keys[(int)(KBD_key::Up)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Down)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0x50);
			pressed_keys[(uint8)(KBD_key::Down)] = 1;
		}
		else
		{
			pressed_time_us[(uint8)(KBD_key::Down)] += elapsed_us;
			if (pressed_time_us[(uint8)(KBD_key::Down)] > 200000)
			{
				//out_buffer.push_back(0xE0);
				//out_buffer.push_back(0xD0);
				out_buffer.push_back(0xE0);
				out_buffer.push_back(0x50);
				pressed_time_us[(uint8)(KBD_key::Down)] = 0;
			}
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Down)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0xD0);
			pressed_keys[(int)(KBD_key::Down)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Left)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0x4B);
			pressed_keys[(uint8)(KBD_key::Left)] = 1;
		}
		else
		{
			pressed_time_us[(uint8)(KBD_key::Left)] += elapsed_us;
			if (pressed_time_us[(uint8)(KBD_key::Left)] > 200000)
			{
				//out_buffer.push_back(0xE0);
				//out_buffer.push_back(0xCB);
				out_buffer.push_back(0xE0);
				out_buffer.push_back(0x4B);
				pressed_time_us[(uint8)(KBD_key::Left)] = 0;
			}
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Left)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0xCB);
			pressed_keys[(int)(KBD_key::Left)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) && has_focus)
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::Right)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0x4D);
			pressed_keys[(uint8)(KBD_key::Right)] = 1;
		}
		else
		{
			pressed_time_us[(uint8)(KBD_key::Right)] += elapsed_us;
			if (pressed_time_us[(uint8)(KBD_key::Right)] > 200000)
			{
				//out_buffer.push_back(0xE0);
				//out_buffer.push_back(0xCD);
				out_buffer.push_back(0xE0);
				out_buffer.push_back(0x4D);
				pressed_time_us[(uint8)(KBD_key::Right)] = 0;
			}
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::Right)])
		{
			out_buffer.push_back(0xE0);
			out_buffer.push_back(0xCD);
			pressed_keys[(int)(KBD_key::Right)] = 0;
		}
	}
	// ======================= PROCESS ==============

	if (code) out_buffer.push_back(code); //отправляем в буффер

}
uint8 KBD::get_buf_size()
{
	return out_buffer.size();
}
uint8 KBD::read_scan_code()
{
	//FF - символ переполнения буфера
	static uint8 code_out = 0xAA;
	if (out_buffer.size())
	{
		code_out = out_buffer.at(0);
		//out_buffer.erase(out_buffer.begin());
	}
	//cout << "read scancode = 0x" << (int)code_out << " buf" << out_buffer.size() << endl;
	return code_out;
}
void KBD::set_CLK_high()
{
	if (!CLK_high)
	{
		//soft reset
		//cout << "KB_SOFT_RES" << endl;
		out_buffer.clear();
		out_buffer.push_back(0xAA);
		sleep_timer = 5; //небольшая задержка устройства
		fetch_new = 1;
		do_int = 0;
	}
	CLK_high = true;
}
void KBD::set_CLK_low()
{
	CLK_high = false;
}
void KBD::sync()  //синхронизация клавиатуры
{
	//проверка таймера задержки
	if (sleep_timer)  //уменьшаем таймер
	{
		sleep_timer--;
		return;
	}

	if (do_poll) return; //если происходит опрос кнопок, то ждем

	if (fetch_new)
	{
		if (get_buf_size() && enabled && CLK_high)
		{
			ppi_ctrl.set_scancode(out_buffer.at(0)); //записываем код в буфер PPI
			out_buffer.erase(out_buffer.begin());	//удаляем код из буфера клавиатуры
			fetch_new = 0;
			do_int = 1;
		}
	}

	//проверка буфера клавиатуры
	if (do_int && enabled)
	{
		if (!(int_ctrl.IS_REG & 0b00000010)) {
			//out_buffer.erase(out_buffer.begin());	//удаляем из буфера
			int_ctrl.request_IRQ(1);
			//enabled = 0; //отключаемся
			if (log_to_console) cout << "[KBD] INT 1 " << endl;
			sleep_timer = 5; //добавим небольшую задержку
		}
	}
}
void KBD::next()  //синхронизация клавиатуры
{
	fetch_new = 1;
	do_int = 0;
	ppi_ctrl.set_scancode(0); //записываем нули в буфер PPI
	//cout << "KB -> fetch new key" << endl;
	return;
	
	//if (!CLK_high) return;
	//if (!data_line_enabled) return; //если отключена - возврат
	//if (out_buffer.size()) out_buffer.erase(out_buffer.begin());
	if (log_to_console || 1)
	{
		//if (out_buffer.size()) cout << "\n[KB_next] 0x" << (int)out_buffer.at(0) << " ";
		//else cout << "\n[KB_next] 0x" << (int)0 << "(empty) ";
	}
}
void KBD::update(uint32 new_elapsed_us, bool has_focus)
{
	elapsed_us = new_elapsed_us;
	window_has_focus = has_focus;
	do_poll = 1;
}
void KBD::main_loop()
{
	//цикл в отдельном потоке
	while (do_main_loop)
	{
		if (do_poll)
		{
			//опрашиваем клавиатуру
			poll_keys(elapsed_us, window_has_focus);
			do_poll = 0;
		}
		else
		{
			//отдыхаем
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
}
KBD::KBD()
{
	out_buffer.clear();
	out_buffer.push_back(0xAA);
	do_main_loop = 1;

	//создаем поток
	std::thread new_t(&KBD::main_loop, this);
	t = std::move(new_t);
}
KBD::~KBD()
{
	do_main_loop = 0;	//сигнал потоку завершиться
	t.join();			//завершаем поток
}