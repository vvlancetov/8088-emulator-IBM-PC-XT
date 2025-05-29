#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <conio.h>
#include <iostream>
#include <vector>
#include "custom_classes.h"
#include "keyboard.h"

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;

using namespace std;

extern IC8259 int_ctrl;

//клавиатура
void KBD::sync(uint32 elapsed_us)
{
	//синхронизируем нажатия клавишь
	if (!enabled) return; //если отключена - возврат

	bool special = false;
	uint8 code = 0;

	//записываем нажатия в буфер

	// ====================  F1-F12 ============
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F1))
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::F1)])
		{
			out_buffer.push_back(0x3b);
			pressed_keys[(uint8)(KBD_key::F1)] = 1;
		}
		else
		{
			pressed_time_us[(uint8)(KBD_key::F1)] += elapsed_us;
			if (pressed_time_us[(uint8)(KBD_key::F1)] > 500000)
			{
				out_buffer.push_back(0x3b + 0x80);
				out_buffer.push_back(0x3b);
			}
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F2))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F3))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F4))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F5))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F6))
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

	// ==================== num 1 - 0 ==============

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num1))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num2))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num3))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num4))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num5))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num6))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num7))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num8))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num9))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num0))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::T))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Y))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::U))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::I))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::O))
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


	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::P))
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


	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
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
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
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
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F))
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
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::G))
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
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::H))
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
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::J))
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
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::K))
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
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Z))
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


	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::X))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::V))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::B))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::N))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::M))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter))
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::ENTER)])
		{
			out_buffer.push_back(0x1C);
			pressed_keys[(uint8)(KBD_key::ENTER)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::ENTER)])
		{
			out_buffer.push_back(0x1C + 0x80);
			pressed_keys[(int)(KBD_key::ENTER)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space))
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::SPACE)])
		{
			out_buffer.push_back(0x39);
			pressed_keys[(uint8)(KBD_key::SPACE)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::SPACE)])
		{
			out_buffer.push_back(0x39 + 0x80);
			pressed_keys[(int)(KBD_key::SPACE)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Grave))
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
		if (pressed_keys[(int)(KBD_key::Grave)])
		{
			out_buffer.push_back(0x89);
			pressed_keys[(int)(KBD_key::Grave)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Hyphen))
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
		if (pressed_keys[(int)(KBD_key::Hyphen)])
		{
			out_buffer.push_back(0x8C);
			pressed_keys[(int)(KBD_key::Hyphen)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Equal))
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
			out_buffer.push_back(0x82);
			pressed_keys[(int)(KBD_key::Equal)] = 0;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Backspace))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Tab))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift))
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
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Backslash))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LBracket))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RBracket))
	{
		//убираем повторение
		if (!pressed_keys[(uint8)(KBD_key::LBracket)])
		{
			out_buffer.push_back(0x1B);
			pressed_keys[(uint8)(KBD_key::LBracket)] = 1;
		}
	}
	else
	{
		if (pressed_keys[(int)(KBD_key::LBracket)])
		{
			out_buffer.push_back(0x9B);
			pressed_keys[(int)(KBD_key::LBracket)] = 0;
		}
	}


	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Semicolon))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Apostrophe))
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


	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Comma))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Period))
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Slash))
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

	//=================== ARROWS =========================

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
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
			if (pressed_time_us[(uint8)(KBD_key::Up)] > 500000)
			{
				out_buffer.push_back(0xE0);
				out_buffer.push_back(0xC8);
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
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
			if (pressed_time_us[(uint8)(KBD_key::Down)] > 500000)
			{
				out_buffer.push_back(0xE0);
				out_buffer.push_back(0xD0);
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
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
			if (pressed_time_us[(uint8)(KBD_key::Left)] > 500000)
			{
				out_buffer.push_back(0xE0);
				out_buffer.push_back(0xCB);
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

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
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
			if (pressed_time_us[(uint8)(KBD_key::Right)] > 500000)
			{
				out_buffer.push_back(0xE0);
				out_buffer.push_back(0xCD);
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

	if (out_buffer.size() > 0 && CLK_high)
	{
		//cout << "KB: hardware buf = " << (int)out_buffer.size() << " send irq_1" << endl;
		int_ctrl.request_IRQ(1);
	}
}
uint8 KBD::read_scan_code()
{
	uint8 code_out = 0;
	if (out_buffer.size())
	{
		code_out = out_buffer.at(0);
		out_buffer.erase(out_buffer.begin());
	}
	return code_out;
}
void KBD::set_CLK_high()
{
	if (!CLK_high && enabled) out_buffer.push_back(0xAA);
	CLK_high = true;
}
void KBD::set_CLK_low()
{
	CLK_high = false;
}
