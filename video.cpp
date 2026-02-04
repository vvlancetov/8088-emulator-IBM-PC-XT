#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <conio.h>
#include <bitset>
#include <string>
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include "video.h"

//#define DEBUG

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;

using namespace std;

template< typename T >
extern string int_to_hex(T i, int w);

//текстура шрифта
extern sf::Texture font_texture_40;
extern sf::Sprite font_sprite_40;
extern sf::Texture font_texture_80;
extern sf::Sprite font_sprite_80;

//текстуры графической палитры
extern sf::Texture CGA_320_texture;
extern sf::Sprite CGA_320_palette_sprite;

//регистры процессора
extern uint16 AX; // AX
extern uint16 BX; // BX
extern uint16 CX; // CX
extern uint16 DX; // DX

// флаги для изменения работы эмулятора
extern bool step_mode;		//ждать ли нажатия пробела для выполнения команд
extern bool log_to_console; //логирование команд на консоль
//extern bool log_to_console_FDD; //логирование команд FDD на консоль

extern string deferred_msg; //отладочные сообщения

//видеокарта
void Video_device::sync(int elapsed_ms)
{
	main_window.clear();// очистка экрана

	//проверка бита включения экрана
	if ((CGA_Mode_Select_Register & 8) == 0) return; //карта отключена

	//цикл отрисовки экрана
	if (cursor_clock.getElapsedTime().asMicroseconds() > 300000) //мигание курсора
	{
		cursor_clock.restart();
		cursor_flipflop = !cursor_flipflop;
	}

	sf::Text text(font);		//обычный шрифт
	text.setCharacterSize(40);
	text.setFillColor(sf::Color::White);

	uint16 font_t_x, font_t_y;
	uint32 addr;
	bool attr_under = false;       //атрибут подчеркивания
	bool attr_blink = false;       //атрибут мигания
	bool attr_highlight = false;   //атрибут подсветки

	sf::Color border = CGA_colors[CGA_Color_Select_Register & 15];

	//начальный адрес экрана в памяти
	uint16 start_address = (registers[0xC] & 0b00111111) * 256 + registers[0xD];

	//адрес курсора
	uint16 cursor_pos = registers[0xE] * 256 + registers[0xF];

	//делим режимы на текст и графику
	if ((CGA_Mode_Select_Register & 2) == 0)
	{
		//текстовые режимы

		//заливаем цветом рамку
		sf::RectangleShape rectangle;
		rectangle.setScale(sf::Vector2f(1, 1));
		rectangle.setSize(sf::Vector2f(320 * 5 + 40, 200 * 1.2 * 5 + 40));
		rectangle.setPosition(sf::Vector2f(0, 0));
		rectangle.setFillColor(border);
		//rectangle.setFillColor(sf::Color(0,0,60));

		main_window.draw(rectangle);
		rectangle.setSize(sf::Vector2f(320 * 5, 200 * 1.2 * 5));
		rectangle.setPosition(sf::Vector2f(20, 20));
		rectangle.setFillColor(sf::Color(0, 0, 0));
		main_window.draw(rectangle);

		uint8 width;
		if (CGA_Mode_Select_Register & 1)
		{
			width = 80;
			//if (start_address > (0x4000 - (25 * 80 * 2))) start_address = 0x4000 - (25 * 80 * 2);
		}
		else
		{
			width = 40;
			//if (start_address > (0x4000 - (25 * 40 * 2))) start_address = 0x4000 - (25 * 40 * 2);
		}

		//настройка масштаба символов для разных режимов
		if (width == 40) rectangle.setSize(sf::Vector2f(5 * 8, 1.2 * 5 * 8));
		else rectangle.setSize(sf::Vector2f(5 * 0.5 * 8, 1.2 * 5 * 8));

		uint8 attrib = 0; //атрибуты символов

		sf::Color fg_color;
		sf::Color bg_color;

		//управление цветом
		bool color_enable = 0;  //наличие цвета
		if ((CGA_Mode_Select_Register & 4) == 0) color_enable = 1;

		//заполняем экран
		for (int y = 0; y < 25; y++)  //25 строк
		{
			for (int x = 0; x < width; x++)  //40 или 80 символов в строке
			{
				addr = 0xb8000 + start_address + (y * width * 2) + x * 2;
				
					font_t_y = memory_2[addr] >> 5;
					font_t_x = memory_2[addr] - (font_t_y << 5);
					attrib = memory_2[addr + 1];
					if (color_enable)
					{
						fg_color = CGA_colors[attrib & 15];
						bg_color = CGA_colors[(attrib >> 4) & 15];
					}
					else
					{
						fg_color = CGA_BW_colors[attrib & 15];
						bg_color = CGA_BW_colors[(attrib >> 4) & 15];
					}
					bool blink = ((attrib >> 7) & 1) & ((CGA_Mode_Select_Register >> 5) & 1); 

					//рисуем фон символа
					rectangle.setFillColor(bg_color);
					if (width == 40) rectangle.setPosition(sf::Vector2f(x * 8 * 5 + 20, y * 8 * 5 * 1.2 + 20));
					else rectangle.setPosition(sf::Vector2f(x * 8 * 5 * 0.5 + 20, y * 8 * 5 * 1.2 + 20));
					main_window.draw(rectangle);

					//рисуем сам символ
					if (!blink || cursor_flipflop)
					//if (!blink)
					{
						if (width == 40)
						{
							font_sprite_40.setTextureRect(sf::IntRect(sf::Vector2i(font_t_x * 8 * 5, font_t_y * 8 * 5), sf::Vector2i(8 * 5, 8 * 5)));
							font_sprite_40.setPosition(sf::Vector2f(x * 8 * 5 + 20, y * 8 * 5 * 1.2 + 20));
							font_sprite_40.setColor(fg_color);
							main_window.draw(font_sprite_40);
						}	
						else
						{
							font_sprite_80.setTextureRect(sf::IntRect(sf::Vector2i(font_t_x * 8 * 5 * 0.5, font_t_y * 8 * 5), sf::Vector2i(8 * 5 * 0.5, 8 * 5)));
							font_sprite_80.setPosition(sf::Vector2f(x * 8 * 5 * 0.5 + 20, y * 8 * 5 * 1.2 + 20));
							font_sprite_80.setColor(fg_color);
							main_window.draw(font_sprite_80);
						}
					}

					bool draw_cursor = false;
					if ((width == 40) && !(y * 40 + x - registers[0xe] * 256 - registers[0xf])) draw_cursor = true;
					if ((width == 80) && !(y * 80 + x - registers[0xe] * 256 - registers[0xf])) draw_cursor = true;

					//рисуем курсор
					if (draw_cursor && cursor_flipflop && ((registers[0xb] & 31)>= (registers[0xa] & 31)))
					{
						if (width == 40)
						{
							sf::RectangleShape cursor_rectangle; //прямоугольник курсора
							cursor_rectangle.setScale(sf::Vector2f(1, 1));
							cursor_rectangle.setSize(sf::Vector2f(40, (registers[0xb] - registers[0xa] + 1) * 6));
							cursor_rectangle.setPosition(sf::Vector2f(x * 8 * 5 + 20, (y * 8 + registers[0xa]) * 5 * 1.2 + 20));
							cursor_rectangle.setFillColor(fg_color);
							//rectangle.setFillColor(sf::Color(0,0,60));
							//font_sprite_40.setTextureRect(sf::IntRect(sf::Vector2i(31 * 8 * 5, 2 * 8 * 5), sf::Vector2i(8 * 5, 8 * 5)));
							//font_sprite_40.setPosition(sf::Vector2f(x * 8 * 5 + 20, y * 8 * 5 * 1.2 + 20));
							//font_sprite_40.setColor(sf::Color::White);
							main_window.draw(cursor_rectangle);
						}
						else 
						{
							//font_sprite_80.setTextureRect(sf::IntRect(sf::Vector2i(31 * 8 * 5 * 0.5, 2 * 8 * 5), sf::Vector2i(8 * 5 * 0.5, 8 * 5)));
							//font_sprite_80.setPosition(sf::Vector2f(x * 8 * 5 * 0.5 + 20, y * 8 * 5 * 1.2 + 20));
							//font_sprite_80.setColor(sf::Color::White);
							//main_window.draw(font_sprite_80);
							sf::RectangleShape cursor_rectangle; //прямоугольник курсора
							cursor_rectangle.setScale(sf::Vector2f(1, 1));
							cursor_rectangle.setSize(sf::Vector2f(20, (registers[0xb] - registers[0xa] + 1) * 6));
							cursor_rectangle.setPosition(sf::Vector2f(x * 8 * 5 * 0.5 + 20, (y * 8 + registers[0xa]) * 5 * 1.2 + 20));
							cursor_rectangle.setFillColor(fg_color);
							main_window.draw(cursor_rectangle);
						}
					}
			}
		}
	}
	else
	{
		//графические режимы
		if ((CGA_Mode_Select_Register & 16) == 0)
		{
			//режим 320х200

			//закрашиваем фон
			sf::RectangleShape rectangle;
			rectangle.setScale(sf::Vector2f(1, 1));
			rectangle.setSize(sf::Vector2f(320 * 5, 200 * 1.2 * 5));
			rectangle.setPosition(sf::Vector2f(20, 20));
			rectangle.setFillColor(CGA_colors[CGA_Color_Select_Register & 15]); //заливаем фон цветом "рамки" (бит 4 - интенсивность, учтена в таблице)
			main_window.draw(rectangle);

			//управление цветом
			bool color_enable = 0;  //наличие цвета
			if ((CGA_Mode_Select_Register & 4) == 0) color_enable = 1;
			
			uint8 palette_shift = 0;
			if (color_enable)
			{
				palette_shift = 20 + ((CGA_Color_Select_Register >> 5) & 1) * 20 + ((CGA_Color_Select_Register >> 4) & 1) * 40; //выбираем палитру
			}
			else
			{
				//режим без цвета
				palette_shift = 0; //первый ряд палитры
			}
			//start_address = 0;
			CGA_320_palette_sprite.setScale({ 1,1 });
			//cout << "colorEN " << (int)color_enable << " intens " << (int)intensity << " palette " << (int)((CGA_Mode_Select_Register >> 5) & 1) << " p_shift " << (int)palette_shift <<  endl;
			//четные строки
			for (int y = 0; y < 100; ++y)
			{
				for (int x = 0; x < 80; ++x)
				{
					uint32 dot_addr = start_address * 2 + 80 * y + x;
					dot_addr = dot_addr % 0x2000 + 0xB8000;
					CGA_320_palette_sprite.setTextureRect(sf::IntRect(sf::Vector2i(palette_shift, memory_2[dot_addr] * 6), sf::Vector2i(20, 6)));
					CGA_320_palette_sprite.setPosition(sf::Vector2f(x * 4 * 5 + 20, y * 2 * 6 + 20));
					main_window.draw(CGA_320_palette_sprite);
				}
			}

			//нечетные строки
			for (int y = 0; y < 100; ++y)
			{
				for (int x = 0; x < 80; ++x)
				{
					uint32 dot_addr = start_address * 2 + 80 * y + x;
					dot_addr = dot_addr % 0x2000 + 0xBA000;
					CGA_320_palette_sprite.setTextureRect(sf::IntRect(sf::Vector2i(palette_shift, memory_2[dot_addr] * 6), sf::Vector2i(20, 6)));
					CGA_320_palette_sprite.setPosition(sf::Vector2f(x * 4 * 5 + 20, (y * 2 + 1) * 6 + 20));
					main_window.draw(CGA_320_palette_sprite);
				}
			}
		}
		else
		{
			//режим 640х200 BW

			sf::RectangleShape dot;
			dot.setSize(sf::Vector2f(1, 1));
			dot.setOutlineThickness(0);

			int width = 640;
			int height = 200;

			//масштаб точек
			float display_x_scale = 2.5; // (float)(GAME_WINDOW_X_RES - 40) / width;  //1600    5 или 2,5
			float display_y_scale = 6; // (float)(GAME_WINDOW_Y_RES - 40) / height; //1200    6
			dot.setScale(sf::Vector2f(display_x_scale, display_y_scale));

			//start_address  - начало буфера
			//добавить переход в начало памяти
			
			for (int y = 0; y < 100; y++)
			{
				for (int x = 0; x < width; ++x)
				{
					//четные строки
					uint8 dot_color = ((memory_2[0xB8000 + start_address + 80 * y + (x >> 3)] >> (7 - (x % 8))) & 1);

					//рисуем пиксел
					dot.setPosition(sf::Vector2f(x * display_x_scale + 20, y * 2 * display_y_scale + 20)); //20 - бордюр
					if (dot_color)
					{
						dot.setFillColor(CGA_colors[CGA_Color_Select_Register & 15]);
						main_window.draw(dot);
					}

					//нечетные строки
					dot_color = (memory_2[0xBA000 + start_address + 80 * y + (x >> 3)] >> (7 - (x % 8))) & 1;
					dot.setPosition(sf::Vector2f(x * display_x_scale + 20, (y * 2 + 1) * display_y_scale + 20)); //20 - бордюр
					if (dot_color)
					{
						dot.setFillColor(CGA_colors[CGA_Color_Select_Register & 15]);
						main_window.draw(dot);
					}
				}
			}
		}
	}

	// вывод технической информации
	attr_blink = false;
	attr_highlight = false;
	attr_under = false;

	
	//тестовая информация джойстика
	joy_sence_show_timer -= elapsed_ms;
	if (joy_sence_show_timer < 0) joy_sence_show_timer = 0;
	if (joy_sence_show_timer)
	{
		
		//отладочная инфа джойстика
		//фон
		text.setString("Joystick, center point: ####################");
		text.setPosition(sf::Vector2f(100, 1000));
		text.setFillColor(sf::Color(0, 0, 0));
		text.setOutlineThickness(12.1);
		if (joy_sence_show_timer) main_window.draw(text);


		string t = "Joystick, center point: " + to_string(joy_sence_value) + " ";
		for (int r = 0; r < (joy_sence_value >> 3); r++)
		{
			t = t + "#";
		}
		text.setString(t);
		text.setPosition(sf::Vector2f(100, 1000));
		text.setFillColor(sf::Color(255, 0, 0));
		text.setOutlineThickness(12.1);
		if (joy_sence_show_timer) main_window.draw(text);
	}
	/*
	uint16 offset_x = ((registers[12] * 256 + registers[13]) & 0x3FFF) % 40;
	uint16 offset_y = floor((registers[12] * 256 + registers[13]) / 40);
	text.setString("offset_x " + to_string(offset_x) + " offset_y " + to_string(offset_y));
	text.setPosition(sf::Vector2f(300, 520));
	text.setOutlineThickness(6.1);
	
	main_window.draw(text);
	*/
	


	main_window.display();
	int_request = true;//устанавливаем флаг в конце кадра

	while (main_window.pollEvent()) {};
}
Video_device::Video_device()   // конструктор класса
{
	/*
		Режимы работы

		Текстовые
		0 - 40 x 25 символов (8х8), 2 байта на символ, 320х200 пикселей, подавление цвета (BW)
		1 - 40 x 25 символов (8х8), 2 байта на символ, 320х200 пикселей, цвет есть
		2 - 80 х 25 символов (8х8), 2 байта на символ, 640х200 пикселей, подавление цвета (BW)
		3 - 80 х 25 символов (8х8), 2 байта на символ, 640х200 пикселей, цвет есть
		Формат байта символа: 7 - мигание, 6-4 - цвет фона, 3 - интенсивность, 2-0 - цвет символа


		Графические:
		4 - (320х200)							2 бита на пиксель
		5 - (320х200) + подавление цвета (BW)	2 бита на пиксель
		6 - (640х200)							1 бит на пиксель

		Цвета CGA	00 - черный
					01 - синий/зеленый
					10 - малиновый/красный
					11 - белый/коричневый
		Выбор палитры - функция 0Bh прерывания INT 10h
		При вывода строк чередуются два банка памяти.

		порты:
		3D4h - регистр
		3D5h - данные
	*/

	display_mode = 3;

	//инициализируем графические константы
	GAME_WINDOW_X_RES = 320 * 5 + 40;	// масштаб 1:5 + 500 px для отладки
	GAME_WINDOW_Y_RES = 200 * 1.2 * 5 + 40;  // масштаб 1:5

	//получаем данные о текущем дисплее
	my_display_H = sf::VideoMode::getDesktopMode().size.y;
	my_display_W = sf::VideoMode::getDesktopMode().size.x;

	cout << "Video Init " << my_display_H << " x " << my_display_W << " display" << endl;

	//создаем главное окно

	main_window.create(sf::VideoMode(sf::Vector2u(GAME_WINDOW_X_RES, GAME_WINDOW_Y_RES)), "IBM PC/XT emulator", sf::Style::Titlebar, sf::State::Windowed);
	main_window.setPosition({ my_display_W - GAME_WINDOW_X_RES - 10, 0 });
	main_window.setFramerateLimit(120);
	main_window.setMouseCursorVisible(1);
	main_window.setKeyRepeatEnabled(0);
	main_window.setVerticalSyncEnabled(0);
	main_window.setActive(true);


	cursor_clock.restart(); //запускаем таймер мигания

	//создаем массив цветов CGA
	CGA_colors[0] = sf::Color(0, 0, 0);
	CGA_colors[1] = sf::Color(0, 0, 0xAA);
	CGA_colors[2] = sf::Color(0, 0xAA, 0);
	CGA_colors[3] = sf::Color(0, 0xAA, 0xAA);
	CGA_colors[4] = sf::Color(0xAA, 0, 0);
	CGA_colors[5] = sf::Color(0xAA, 0, 0xAA);
	CGA_colors[6] = sf::Color(0xAA, 0x55, 0);
	CGA_colors[7] = sf::Color(0xAA, 0xAA, 0xAA);
	CGA_colors[8] = sf::Color(0x55, 0x55, 0x55);
	CGA_colors[9] = sf::Color(0x55, 0x55, 0xFF);
	CGA_colors[10] = sf::Color(0x55, 0xFF, 0x55);
	CGA_colors[11] = sf::Color(0x55, 0xFF, 0xFF);
	CGA_colors[12] = sf::Color(0xFF, 0x55, 0x55);
	CGA_colors[13] = sf::Color(0xFF, 0x55, 0xFF);
	CGA_colors[14] = sf::Color(0xFF, 0xFF, 0x55);
	CGA_colors[15] = sf::Color(0xFF, 0xFF, 0xFF);

	//таблица черно-белых цветов
	CGA_BW_colors[1] = sf::Color(0, 0, 0);
	CGA_BW_colors[1] = sf::Color(0x55, 0x55, 0x55);
	CGA_BW_colors[2] = sf::Color(0x55, 0x55, 0x55);
	CGA_BW_colors[3] = sf::Color(0x55, 0x55, 0x55);
	CGA_BW_colors[4] = sf::Color(0x55, 0x55, 0x55);
	CGA_BW_colors[5] = sf::Color(0x55, 0x55, 0x55);
	CGA_BW_colors[6] = sf::Color(0x55, 0x55, 0x55);
	CGA_BW_colors[7] = sf::Color(0xAA, 0xAA, 0xAA);
	CGA_BW_colors[8] = sf::Color(0x77, 0x55, 0x55);
	CGA_BW_colors[9] = sf::Color(0xAA, 0xAA, 0xAA);
	CGA_BW_colors[10] = sf::Color(0xAA, 0xAA, 0xAA);
	CGA_BW_colors[11] = sf::Color(0xAA, 0xAA, 0xAA);
	CGA_BW_colors[12] = sf::Color(0xAA, 0xAA, 0xAA);
	CGA_BW_colors[13] = sf::Color(0xAA, 0xAA, 0xAA);
	CGA_BW_colors[14] = sf::Color(0xAA, 0xAA, 0xAA);
	CGA_BW_colors[15] = sf::Color(0xFF, 0xFF, 0xFF);
}
void Video_device::write_vram(uint16 address, uint8 data) //запись в видеопамять
{
	memory_2[(address + 0xB8000) & 0xFFFFF] = data;
}
void Video_device::write_port(uint16 port, uint8 data)	//запись в порт адаптера
{
	if (port == 0x3D4)
	{
		//выбор регистра для записи
		if (data < 18) sel_reg = data;
		//deferred_msg += "CGA select REG = " + to_string(data);
		if (sel_reg == 1 && log_to_console) cout << "CGA select REG -> CGA columns ";
		if (sel_reg == 6 && log_to_console) cout << "CGA select REG -> CGA lines ";
		if (sel_reg == 0x0C && log_to_console) cout << "CGA select REG -> CGA mem: high byte ";
		if (sel_reg == 0x0D && log_to_console) cout << "CGA select REG -> CGA mem: low byte ";
	}
	if (port == 0x3D5)
	{
		//запись в выбранный регистр
		registers[sel_reg] = data;
		if (sel_reg == 1 && log_to_console) cout << "CGA columns = " << int(data) << " ";
		if (sel_reg == 6 && log_to_console) cout << "CGA lines = " << int(data) << " ";
		if (sel_reg == 0x0C && log_to_console) cout << "CGA mem: high byte = " << int(data) << " ";
		if (sel_reg == 0x0D && log_to_console) cout << "CGA mem: low byte = " << int(data) << " ";
	}
	if (port == 0x3D8)
	{
		//установка режимов работы
		if (log_to_console) cout << "CGA set MODE = " << (bitset<5>)(data & 31) << endl;
		CGA_Mode_Select_Register = data;
	}
	if (port == 0x3D9)
	{
		//выбор режимов цвета
		if (log_to_console) cout << "CGA set COLOUR = " << (bitset<5>)(data) << " ";
		CGA_Color_Select_Register = data;
	}
}
uint8 Video_device::read_port(uint16 port)				//чтение из порта адаптера
{
	if (port == 0x3D5)
	{ 
		if (log_to_console) cout << "CGA: read PORT 0x3D5 (read registers) ";
		//считывание регистра
		if (sel_reg == 0x0C) return registers[sel_reg]; //старший байт начального адреса (Start Address Register - SAR, high byte)
		if (sel_reg == 0x0D) return registers[sel_reg]; //младший байт начального адреса (Start address Register - SAR, low byte)
		if (sel_reg == 0x0E) return registers[sel_reg]; //старший байт позиции курсора (Cursor Location Register - CLR, high byte)
		if (sel_reg == 0x0F) return registers[sel_reg]; //младший байт позиции курсора (Cursor Location Register - CLR, low byte)

	}
	if (port == 0x3DA)
	{
		if (log_to_console) cout << "CGA: read PORT 0x3DA (CRT ray) -> ";
		//считывание регистра состояния
		//меняем для симуляции обратного хода луча
		CRT_flip_flop = !CRT_flip_flop;
		if (CRT_flip_flop && log_to_console) cout << "0000 ";
		if (!CRT_flip_flop && log_to_console) cout << "1001 ";
		if (CRT_flip_flop) return 0b00000000; // бит 3 - обратный ход луча, бит 0 - разрешение записи в видеопамять
		else return 0b00001001;
	}
	return 0;
}
void Video_device::set_CGA_mode(uint8 mode)
{
	//преобразование режима в биты регистра
	if (mode == 0) CGA_Mode_Select_Register = 0b00001100;
	if (mode == 1) CGA_Mode_Select_Register = 0b00001000;
	if (mode == 2) CGA_Mode_Select_Register = 0b00001101;
	if (mode == 3) CGA_Mode_Select_Register = 0b00001001;
	if (mode == 4) CGA_Mode_Select_Register = 0b00001010;
	if (mode == 5) CGA_Mode_Select_Register = 0b00001110;
	if (mode == 6) CGA_Mode_Select_Register = 0b00011100;
	if (mode == 7) CGA_Mode_Select_Register = 0b00001000; // = mode 1
}
void Video_device::set_cursor_type(uint16 type)			//установка типа курсора
{
	registers[0xA] = (type >> 8) & 15;	// начальная линия курсора
	registers[0xB] = type & 15;			// конечная линия курсора
}
void Video_device::set_cursor_position(uint8 X, uint8 Y, uint8 Page)
{
	//установка позиции курсора
	uint8 width; //ширина экрана
	if ((CGA_Mode_Select_Register & 3) != 1) width = 40;
	else width = 80;
	uint16 position = width * Y + X + Page * (width * 25);
	registers[0xE] = (position >> 8) & 255;		//старший байт
	registers[0xF] = position & 255;			//младший байт
}
void Video_device::read_cursor_position()
{
	//чтение позиции курсора

	uint16 position = registers[0xF] + registers[0xE] * 256; //адрес курсора
	uint8 width; //ширина экрана
	if ((CGA_Mode_Select_Register & 3) != 1) width = 40;
	else width = 80;

	uint16 page_size = width * 25; //объем страницы

	uint8 page_N = floor(position / page_size);
	uint8 Y = floor((position - page_N * page_size) / width);
	uint8 X = position - page_N * page_size - Y * width;
	if ((CGA_Mode_Select_Register & 0x12) != 0) page_N = 0; //обнуляем страницу для графических режимов
	BX = (page_N << 8) | (BX & 255); //номер страницы
	DX = (Y << 8) | (X);			 //координаты
	CX = (registers[0xA] << 8) | (registers[0xB]);   //тип курсора
}
void Video_device::teletype(uint8 symbol)
{
	//выводим символ из AL на экран
	//адрес курсора
	uint32 cursor_pos = registers[0xF] + registers[0xE] * 256; //адрес курсора
	uint8 width; //ширина экрана	
	if ((CGA_Mode_Select_Register & 3) != 1) width = 40;
	else width = 80;
	uint8 page = floor(cursor_pos / (width * 25 * 2)); //рассчитываем номер страницы
	//координаты курсора
	uint8 y = floor((cursor_pos - page * (width * 25 * 2)) / (width * 2));
	uint8 x = (cursor_pos % width) >> 1;

	//cout << endl << "p=" << (int)page << " w=" << (int)width << " x=" << (int)x << " y=" << (int)y << " ";

	switch (symbol)
	{
	case 0x8: //backspace
		write_vram(cursor_pos, 0);
		write_vram(cursor_pos + 1, 0);
		if (y > 0 || x > 0)
		{
			cursor_pos -= 2; //двигаем курсор если он не в углу
			registers[0xE] = cursor_pos >> 8;
			registers[0xF] = cursor_pos & 255;
		}
		break;

	case 0xA: //перевод строки

		if (y == 24)
		{
			//сдвигаем строки вверх в пределах экрана
			for (int i = 0; i < 24 * width * 2; i++)
			{
				write_vram(page * width * 25 * 2 + i, memory_2[(0xB8000 + page * width * 25 * 2 + i + width * 2) & 0xFFFFF]);
			}
			//очищаем последнюю строку
			for (int i = 0; i < width * 2; i++)
			{
				write_vram(page * width * 25 * 2 + width * 24 * 2 + i, 0);
			}

		}
		else
		{
			cursor_pos += width * 2;
			//обновляем регистры монитора
			registers[0xE] = (cursor_pos >> 8) & 255;	//старший байт
			registers[0xF] = cursor_pos & 255;			//младший байт
		}

		break;

	case 0xD: //возврат каретки
		cursor_pos = (page * width * 25 + y * width) * 2;
		//новая позиция курсора
		registers[0xE] = cursor_pos >> 8;
		registers[0xF] = cursor_pos & 255;
		break;

	default:  //все остальные символы
		write_vram(cursor_pos, symbol);
		write_vram(cursor_pos + 1, 0x0F); //установка фона
		cursor_pos += 2;
		//новая позиция курсора
		registers[0xE] = cursor_pos >> 8;
		registers[0xF] = cursor_pos & 255;
		//cout << "TT->" << (int)symbol;
	}
	//sync(10); //обновляем картинку
}
string Video_device::get_mode_name()
{
	string mode_name;
	if ((CGA_Mode_Select_Register & 0x2) == 0)
	{
		mode_name = "text ";
		if ((CGA_Mode_Select_Register & 1) == 1) mode_name += "80x25 ";
		else  mode_name += "40x25 ";
		if ((CGA_Mode_Select_Register & 4) == 4) mode_name += "BW ";
		else  mode_name += "COL ";
	}
	else
	{
		mode_name = "graf ";
		if ((CGA_Mode_Select_Register & 16) == 0) mode_name += "320x200 ";
		else  mode_name += "640x200 ";
		if ((CGA_Mode_Select_Register & 4) == 4) mode_name += "BW ";
		else  mode_name += "COL ";
	}
	return mode_name;
}
void Video_device::show_joy_sence(uint8 central_point)
{
	joy_sence_show_timer = 1000000; //включаем таймер
	joy_sence_value = central_point;
}