#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <conio.h>
#include <bitset>
#include <string>
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <memory>
#include "video.h"
#include "audio.h"
#include "keyboard.h"
#include "fdd.h"
#include "hdd.h"
#include "custom_classes.h"

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

using namespace std;

//текстура шрифта
extern sf::Texture font_texture_40;
extern sf::Sprite font_sprite_40;
extern sf::Texture font_texture_80;
extern sf::Sprite font_sprite_80;
extern sf::Texture font_texture_80_MDA;
extern sf::Sprite font_sprite_80_MDA;

//текстуры графической палитры
extern sf::Texture CGA_320_texture;
extern sf::Sprite CGA_320_palette_sprite;

//счетчики
extern uint32 op_counter;
extern uint8 service_counter;

//регистры процессора
extern uint16 AX; // AX
extern uint16 BX; // BX
extern uint16 CX; // CX
extern uint16 DX; // DX

//указатели на регистры
extern uint16* ptr_AX;
extern uint16* ptr_BX;
extern uint16* ptr_CX;
extern uint16* ptr_DX;
extern uint16* ptr_SP;
extern uint16* ptr_BP;
extern uint16* ptr_SI;
extern uint16* ptr_DI;

//половинки регистров
extern uint8* ptr_AL;
extern uint8* ptr_AH;
extern uint8* ptr_BL;
extern uint8* ptr_BH;
extern uint8* ptr_CL;
extern uint8* ptr_CH;
extern uint8* ptr_DL;
extern uint8* ptr_DH;

extern uint16 Status_Flags;	//Flags register
extern bool Flag_OF;		//Overflow flag
extern bool Flag_DF;		//Direction flag
extern bool Flag_IF;		//Int Enable flag
extern bool Flag_TF;		//Trap SS flag
extern bool Flag_SF;		//Signflag
extern bool Flag_ZF;		//Zeroflag
extern bool Flag_AF;		//Aux Carry flag
extern bool Flag_PF;		//Parity flag
extern bool Flag_CF;		//Carry flag

extern uint16 Stack_Pointer;			//указатель стека
extern uint16 Instruction_Pointer;		//указатель текущей команды
extern uint16 Base_Pointer;
extern uint16 Source_Index;
extern uint16 Destination_Index;

//указатели сегментов
extern uint16* CS;			//Code Segment
extern uint16* DS;			//Data Segment
extern uint16* SS;			//Stack Segment
extern uint16* ES;			//Extra Segment

//указатели сегментов
extern uint16 CS_data;			//Code Segment
extern uint16 DS_data;			//Data Segment
extern uint16 SS_data;			//Stack Segment
extern uint16 ES_data;			//Extra Segment

// флаги для изменения работы эмулятора
extern bool step_mode;		//ждать ли нажатия пробела для выполнения команд
extern bool log_to_console; //логирование команд на консоль
extern bool log_to_console_EGA;

extern string deferred_msg; //отладочные сообщения

//устройства
extern SoundMaker speaker;
extern IO_Ctrl IO_device;
extern IC8259 int_ctrl;
extern Mem_Ctrl memory;
extern Mem_Ctrl memory; //контроллер памяти
extern Monitor monitor;
extern KBD keyboard;
extern IC8237 dma_ctrl;
extern FDD_Ctrl FDD_A;
extern HDD_Ctrl HDD;
extern IC8253 pc_timer;

extern bool halt_cpu;		   //флаг остановки до получения прерывания

//флаги дополнительных окон
extern bool show_debug_window; //основное отладочное окно
extern bool show_fdd_window;   //отладочное окно FDD
extern bool show_hdd_window;   //отладочное окно HDD
extern bool show_audio_window; //отладочное окно звука
extern bool show_memory_window;//отладочное окно памяти

//timer
extern std::chrono::high_resolution_clock Hi_Res_Clk;

//таймер ПК
extern IC8253 pc_timer;

//==================== CGA videocard =============

void CGA_videocard::main_loop()   // синхронизация
{
	//создаем окно
	main_window.create(sf::VideoMode(sf::Vector2u(320 * 5 + 40, 240 * 5 + 40)), window_title, sf::Style::Titlebar, sf::State::Windowed);
	main_window.setPosition({ window_pos_x, window_pos_y });
	main_window.setSize(sf::Vector2u(window_size_x, window_size_y));
	main_window.setFramerateLimit(0);
	main_window.setMouseCursorVisible(1);
	main_window.setKeyRepeatEnabled(0);
	main_window.setVerticalSyncEnabled(1);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	main_window.setActive(1);

	while (visible)
	{
		if (do_render)
		{
			do_render = 0;
			if (do_resize)
			{
				main_window.setSize(sf::Vector2u(window_size_x, window_size_y));
				do_resize = 0;
			}
			//рисуем окно
			render();
			//события окна
			while (main_window.pollEvent()) {};
		}
		else
		{
			//отдыхаем
			//_mm_pause();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(5)); //задержка перед завершением
	main_window.close();
}
void CGA_videocard::update(int new_elapsed_ms)
{
	elapsed_ms = new_elapsed_ms;
	do_render = 1;					//обновить экран
}
void CGA_videocard::render()
{
	static auto Hi_Res_t_start = Hi_Res_Clk.now();
	static auto Hi_Res_t_end = Hi_Res_Clk.now();

	main_window.setActive(1);
	bool attr_under = false;       //атрибут подчеркивания
	bool attr_blink = false;       //атрибут мигания
	bool attr_highlight = false;   //атрибут подсветки
	sf::RectangleShape rectangle;
	uint8 width;
	uint8 attrib = 0; //атрибуты символов
	bool color_enable = 0;  //наличие цвета

	uint16 font_t_x, font_t_y;
	uint32 addr;

	sf::Color border = CGA_colors[CGA_Color_Select_Register & 15];

	//начальный адрес экрана в памяти
	uint16 start_address = (registers[0xC] & 0b00111111) * 256 + registers[0xD];

	//адрес курсора
	uint16 cursor_pos = registers[0xE] * 256 + registers[0xF];

	//шрифт для отладки
	sf::Text text(font);		//обычный шрифт

	main_window.setActive(1);
	main_window.clear();		// очистка экрана

	//проверка бита включения экрана
	if ((CGA_Mode_Select_Register & 8) == 0) goto exit; //карта отключена

	//цикл отрисовки экрана
	if (cursor_clock.getElapsedTime().asMicroseconds() > 300000) //мигание курсора
	{
		cursor_clock.restart();
		cursor_flipflop = !cursor_flipflop;
	}

	text.setCharacterSize(20);
	text.setFillColor(sf::Color::White);

	//делим режимы на текст и графику
	if ((CGA_Mode_Select_Register & 2) == 0)
	{
		//текстовые режимы

		//заливаем цветом рамку
		rectangle.setScale(sf::Vector2f(1, 1));
		rectangle.setSize(sf::Vector2f(320 * 5 + 40, 200 * 1.2 * 5 + 40));
		rectangle.setPosition(sf::Vector2f(0, 0));
		rectangle.setFillColor(border);
		main_window.draw(rectangle);

		rectangle.setSize(sf::Vector2f(320 * 5, 200 * 1.2 * 5));
		rectangle.setPosition(sf::Vector2f(20, 20));
		rectangle.setFillColor(sf::Color(0, 0, 0));
		main_window.draw(rectangle);

		if (CGA_Mode_Select_Register & 1) width = 80;
		else width = 40;

		//настройка масштаба символов для разных режимов
		if (width == 40) rectangle.setSize(sf::Vector2f(5 * 8, 1.2 * 5 * 8));
		else rectangle.setSize(sf::Vector2f(5 * 0.5 * 8, 1.2 * 5 * 8));

		sf::Color fg_color;
		sf::Color bg_color;

		//управление цветом

		if ((CGA_Mode_Select_Register & 4) == 0) color_enable = 1;

		//debug_mess_1 = to_string(start_address);

		//заполняем экран
		for (int y = 0; y < 25; y++)  //25 строк
		{
			for (int x = 0; x < width; x++)  //40 или 80 символов в строке
			{
				addr = 0xb8000 + start_address * 2 + (y * width * 2) + x * 2;

				font_t_y = memory.read(addr) >> 5;
				font_t_x = memory.read(addr) - (font_t_y << 5);
				attrib = memory.read(addr + 1);
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
				{
					if (width == 40)
					{
						font_sprite_40.setTextureRect(sf::IntRect(sf::Vector2i(font_t_x * 8 * 5, font_t_y * 8 * 5), sf::Vector2i(8 * 5, 8 * 5)));
						//font_sprite_40.setScale(sf::Vector2f(1, 1));
						font_sprite_40.setPosition(sf::Vector2f(x * 8 * 5 + 20, y * 8 * 5 * 1.2 + 20));
						font_sprite_40.setColor(fg_color);
						main_window.draw(font_sprite_40);
					}
					else
					{
						font_sprite_80.setTextureRect(sf::IntRect(sf::Vector2i(font_t_x * 8 * 5 * 0.5, font_t_y * 8 * 5), sf::Vector2i(8 * 5 * 0.5, 8 * 5)));
						//font_sprite_80.setScale(sf::Vector2f(1, 1));
						font_sprite_80.setPosition(sf::Vector2f(x * 8 * 5 * 0.5 + 20, y * 8 * 5 * 1.2 + 20));
						font_sprite_80.setColor(fg_color);
						main_window.draw(font_sprite_80);
					}
				}

				bool draw_cursor = false;
				if ((width == 40) && !(y * 40 + x - registers[0xe] * 256 - registers[0xf])) draw_cursor = true;
				if ((width == 80) && !(y * 80 + x - registers[0xe] * 256 - registers[0xf])) draw_cursor = true;

				//рисуем курсор
				if (draw_cursor && cursor_flipflop && ((registers[0xb] & 31) >= (registers[0xa] & 31)))
				{
					if (width == 40)
					{
						sf::RectangleShape cursor_rectangle; //прямоугольник курсора
						cursor_rectangle.setScale(sf::Vector2f(1, 1));
						cursor_rectangle.setSize(sf::Vector2f(8 * 5, (registers[0xb] - registers[0xa] + 1) * 1.2 * 5));
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
						cursor_rectangle.setSize(sf::Vector2f(8 * 5, (registers[0xb] - registers[0xa] + 1) * 1.2 * 5));
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

			//cout << "colorEN " << (int)color_enable << " intens " << (int)intensity << " palette " << (int)((CGA_Mode_Select_Register >> 5) & 1) << " p_shift " << (int)palette_shift <<  endl;
			//четные строки
			for (int y = 0; y < 100; ++y)
			{
				for (int x = 0; x < 80; ++x)
				{
					uint32 dot_addr = start_address * 2 + 80 * y + x;
					dot_addr = dot_addr % 0x2000 + 0xB8000;
					CGA_320_palette_sprite.setTextureRect(sf::IntRect(sf::Vector2i(palette_shift, memory.read(dot_addr) * 6), sf::Vector2i(20, 6)));
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
					CGA_320_palette_sprite.setTextureRect(sf::IntRect(sf::Vector2i(palette_shift, memory.read(dot_addr) * 6), sf::Vector2i(20, 6)));
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
			float display_x_scale = 5 / 2.0; // (float)(GAME_WINDOW_X_RES - 40) / width;  //1600    5 или 2,5
			float display_y_scale = 5 * 1.2; // (float)(GAME_WINDOW_Y_RES - 40) / height; //1200    6
			dot.setScale(sf::Vector2f(display_x_scale, display_y_scale));

			//start_address  - начало буфера
			//добавить переход в начало памяти

			for (int y = 0; y < 100; y++)
			{
				for (int x = 0; x < width; ++x)
				{
					//четные строки
					uint8 dot_color = ((memory.read(0xB8000 + start_address + 80 * y + (x >> 3)) >> (7 - (x % 8))) & 1);

					//рисуем пиксел
					dot.setPosition(sf::Vector2f(x * display_x_scale + 20, y * 2 * display_y_scale + 20)); //20 - бордюр
					if (dot_color)
					{
						dot.setFillColor(CGA_colors[CGA_Color_Select_Register & 15]);
						main_window.draw(dot);
					}

					//нечетные строки
					dot_color = (memory.read(0xBA000 + start_address + 80 * y + (x >> 3)) >> (7 - (x % 8))) & 1;
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


	Hi_Res_t_end = Hi_Res_Clk.now();
	//информация для отладки
	debug_mess_1 = to_string(1000000 / (duration_cast<microseconds>(Hi_Res_t_end - Hi_Res_t_start).count() + 1));

	text.setString(debug_mess_1);
	text.setPosition(sf::Vector2f(0, 0));
	text.setFillColor(sf::Color(255, 0, 0));
	text.setOutlineThickness(3.1);
	if (debug_mess_1 != "") main_window.draw(text);

exit:
	main_window.display();
	int_request = true;//устанавливаем флаг в конце кадра
	main_window.setActive(0);
	Hi_Res_t_start = Hi_Res_Clk.now();
}
CGA_videocard::CGA_videocard()   // конструктор класса
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

	//получаем данные о текущем дисплее
	my_display_H = sf::VideoMode::getDesktopMode().size.y;
	my_display_W = sf::VideoMode::getDesktopMode().size.x;

	//базовое разрешение
	GAME_WINDOW_X_RES = 320 * 5 + 40;		 // масштаб 1:5 + 500 px для отладки
	GAME_WINDOW_Y_RES = 200 * 1.2 * 5 + 40;  // масштаб 1:5

	display_scale = floor(5 * my_display_H / GAME_WINDOW_Y_RES);

	cout << "Video Init (CGA) " << my_display_H << " x " << my_display_W << " display" << " scale " << (int)display_scale << endl;

	//задаем переменные окна
	window_title = "IBM PC/XT emulator. CGA videocard.";		//заголовок окна
	window_pos_x = (my_display_W - GAME_WINDOW_X_RES * display_scale / 5) / 2;		//позиция окна
	window_pos_y = (my_display_H - GAME_WINDOW_Y_RES * display_scale / 5 - 30) / 2;		//позиция окна
	window_size_x = GAME_WINDOW_X_RES * display_scale / 5;			//размер окна
	window_size_y = GAME_WINDOW_Y_RES * display_scale / 5;			//размер окна
	cursor_clock.restart();										//запускаем таймер мигания

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
void CGA_videocard::write_port(uint16 port, uint8 data)	//запись в порт адаптера
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
uint8 CGA_videocard::read_port(uint16 port)				//чтение из порта адаптера
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
		
		uint8 out = 0;
		//рассчитываем позицию луча
		uint16 frame_pos = pc_timer.get_time(3) % (21845);
		if (frame_pos < 782) out = 9; //период гашения
		else
		{
			//экран
			uint16 line_pos = frame_pos % 100;
			if (line_pos < 19) out = 1; //гашение линии
			else out = 0;				//отображение пикселя
		}

		return out;
		
		/*
		static bool CRT_flip_flop = 0;
		CRT_flip_flop = !CRT_flip_flop;
		//if (CRT_flip_flop && log_to_console) cout << "0000 ";
		//if (!CRT_flip_flop && log_to_console) cout << "1001 ";
		if (!CRT_flip_flop) return 0b00000000; // бит 3 - обратный ход луча, бит 0 - разрешение записи в видеопамять
		else return 0b00001001;
		*/
	}
	return 0;
}
void CGA_videocard::set_CGA_mode(uint8 mode)
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
void CGA_videocard::set_cursor_type(uint16 type)			//установка типа курсора
{
	registers[0xA] = (type >> 8) & 15;	// начальная линия курсора
	registers[0xB] = type & 15;			// конечная линия курсора
}
void CGA_videocard::set_cursor_position(uint8 X, uint8 Y, uint8 Page)
{
	//установка позиции курсора
	uint8 width; //ширина экрана
	if ((CGA_Mode_Select_Register & 3) != 1) width = 40;
	else width = 80;
	uint16 position = width * Y + X + Page * (width * 25);
	registers[0xE] = (position >> 8) & 255;		//старший байт
	registers[0xF] = position & 255;			//младший байт
}
void CGA_videocard::read_cursor_position()
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
string CGA_videocard::get_mode_name()
{
	string mode_name;
	if ((CGA_Mode_Select_Register & 0x2) == 0)
	{
		mode_name = "[CGA] TXT ";
		if ((CGA_Mode_Select_Register & 1) == 1) mode_name += "80x25 ";
		else  mode_name += "40x25 ";
		if ((CGA_Mode_Select_Register & 4) == 4) mode_name += "BW ";
		else  mode_name += "COL ";
	}
	else
	{
		mode_name = "[CGA] GRF ";
		if ((CGA_Mode_Select_Register & 16) == 0) mode_name += "320x200 ";
		else  mode_name += "640x200 ";
		if ((CGA_Mode_Select_Register & 4) == 4) mode_name += "BW ";
		else  mode_name += "COL ";
	}
	return mode_name;
}
void CGA_videocard::show_joy_sence(uint8 central_point)
{
	joy_sence_show_timer = 1000000; //включаем таймер
	joy_sence_value = central_point;
}
bool CGA_videocard::has_focus()
{
	return main_window.hasFocus();
}
void CGA_videocard::show()
{
	//признак видимости
	visible = 1;

	//создаем поток
	std::thread new_t(&CGA_videocard::main_loop, this);
	t = std::move(new_t);
}
void CGA_videocard::hide()
{
	//запоминаем координаты
	sf::Vector2i p = main_window.getPosition();
	window_pos_x = p.x;
	window_pos_y = p.y;

	//скрываем окно
	visible = 0;
	if (t.joinable()) t.join();
}
bool CGA_videocard::is_visible()
{
	return visible;
}
void CGA_videocard::scale_up()
{
	if (display_scale < 8) display_scale++;
	window_size_x = 320 * display_scale + 40;	// масштаб 1:5 + 500 px для отладки
	window_size_y = 200 * 1.2 * display_scale + 40;  // масштаб 1:5
	do_resize = 1;
}
void CGA_videocard::scale_down()
{
	if (display_scale > 1) display_scale--;
	window_size_x = 320 * display_scale + 40;	// масштаб 1:5 + 500 px для отладки
	window_size_y = 200 * 1.2 * display_scale + 40;  // масштаб 1:5
	do_resize = 1;
}
void CGA_videocard::mem_write(uint32 address, uint8 data)
{
	videomemory[address] = data;
}
uint8 CGA_videocard::mem_read(uint32 address)
{
	return videomemory[address];
}
sf::Vector2i CGA_videocard::get_mouse_pos()
{
	sf::Vector2i localPosition = sf::Mouse::getPosition(main_window); // window is a sf::Window
	
	int pos_x = localPosition.x;
	int pos_y = localPosition.y;

	if (pos_x < 0) pos_x = 0;
	if (pos_x >= main_window.getSize().x) pos_x = main_window.getSize().x - 1;
	if (pos_y < 0) pos_y = 0;
	if (pos_y >= main_window.getSize().y) pos_y = main_window.getSize().y - 1;

	return sf::Vector2i({pos_x,pos_y});
}

//отладочные экраны

Dev_mon_device::Dev_mon_device(uint16 w, uint16 h, string title, uint16 x_pos, uint16 y_pos)   // конструктор класса
{
	//внутренние переменные
	window_title = title;
	window_pos_x = x_pos;
	window_pos_y = y_pos;
	window_size_x = w;
	window_size_y = h;

	//получаем данные о текущем дисплее (пока не нужны)
	my_display_H = sf::VideoMode::getDesktopMode().size.y;
	my_display_W = sf::VideoMode::getDesktopMode().size.x;

	//cout << "Debug window Init " << (int)h << " x " << (int)w << " display name " << title << endl;
}
void Dev_mon_device::main_loop()   // синхронизация
{
	//создаем окно
	main_window.create(sf::VideoMode(sf::Vector2u(window_size_x, window_size_y)), window_title, sf::Style::Titlebar, sf::State::Windowed);
	main_window.setPosition({ window_pos_x, window_pos_y });
	main_window.setFramerateLimit(0);
	main_window.setMouseCursorVisible(1);
	main_window.setKeyRepeatEnabled(0);
	main_window.setVerticalSyncEnabled(1);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	main_window.setActive(1);

	while (visible)
	{
		if (do_render)
		{
			do_render = 0;
			//рисуем окно
			render();
			//события окна
			while (main_window.pollEvent()) {};
		}
		else
		{
			//отдыхаем
			_mm_pause();
			//std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(5)); //задержка перед завершением
	main_window.close();
}
void Dev_mon_device::show()
{
	//признак видимости
	visible = 1;

	//создаем поток
	std::thread new_t(&Dev_mon_device::main_loop, this);
	t = std::move(new_t);
}
void Dev_mon_device::hide()
{
	//запоминаем координаты
	sf::Vector2i p = main_window.getPosition();
	window_pos_x = p.x;
	window_pos_y = p.y;

	//скрываем окно
	visible = 0;
	if (t.joinable()) t.join();

}
void Dev_mon_device::render()
{
	//выводим содержимое в отдельном потоке
	main_window.setActive(1);
	main_window.clear(sf::Color::Black);// очистка экрана

	sf::Text text(font);				//обычный шрифт
	text.setCharacterSize(30);
	text.setFillColor(sf::Color::White);

	//вывод регистров и стека

	text.setString("Registers");
	text.setPosition(sf::Vector2f(10, 10));
	main_window.draw(text);

	text.setString("----------------");
	text.setPosition(sf::Vector2f(10, 40));
	main_window.draw(text);

	text.setString("AX    " + int_to_hex(AX, 4));
	text.setPosition(sf::Vector2f(10, 70));
	main_window.draw(text);

	text.setString("BX    " + int_to_hex(BX, 4));
	text.setPosition(sf::Vector2f(10, 100));
	main_window.draw(text);

	text.setString("CX    " + int_to_hex(CX, 4));
	text.setPosition(sf::Vector2f(10, 130));
	main_window.draw(text);

	text.setString("DX    " + int_to_hex(DX, 4));
	text.setPosition(sf::Vector2f(10, 160));
	main_window.draw(text);

	text.setString("----------------");
	text.setPosition(sf::Vector2f(10, 190));
	main_window.draw(text);

	text.setString("Segments & ptrs");
	text.setPosition(sf::Vector2f(10, 220));
	main_window.draw(text);

	text.setString("----------------");
	text.setPosition(sf::Vector2f(10, 250));
	main_window.draw(text);

	text.setString("CS:IP " + int_to_hex(*CS, 4) + ":" + int_to_hex(Instruction_Pointer, 4));
	text.setPosition(sf::Vector2f(10, 280));
	main_window.draw(text);

	text.setString("SS:SP " + int_to_hex(*SS, 4) + ":" + int_to_hex(Stack_Pointer, 4));
	text.setPosition(sf::Vector2f(10, 310));
	main_window.draw(text);

	text.setString("SS:BP " + int_to_hex(*SS, 4) + ":" + int_to_hex(Base_Pointer, 4));
	text.setPosition(sf::Vector2f(10, 340));
	main_window.draw(text);

	text.setString("DS:SI " + int_to_hex(*DS, 4) + ":" + int_to_hex(Source_Index, 4));
	text.setPosition(sf::Vector2f(10, 370));
	main_window.draw(text);

	text.setString("DS:DI " + int_to_hex(*DS, 4) + ":" + int_to_hex(Destination_Index, 4));
	text.setPosition(sf::Vector2f(10, 400));
	main_window.draw(text);

	text.setString("ES:DI " + int_to_hex(*ES, 4) + ":" + int_to_hex(Destination_Index, 4));
	text.setPosition(sf::Vector2f(10, 430));
	main_window.draw(text);

	text.setString("----------------");
	text.setPosition(sf::Vector2f(10, 460));
	main_window.draw(text);

	text.setString("Flags");
	text.setPosition(sf::Vector2f(10, 490));
	main_window.draw(text);

	text.setString("----------------");
	text.setPosition(sf::Vector2f(10, 520));
	main_window.draw(text);

	text.setString("A=" + to_string(Flag_AF) + " C=" + to_string(Flag_CF) + " P=" + to_string(Flag_PF) + " S=" + to_string(Flag_SF));
	text.setPosition(sf::Vector2f(10, 550));
	main_window.draw(text);

	text.setString("Z=" + to_string(Flag_ZF) + " D=" + to_string(Flag_DF) + " I=" + to_string(Flag_IF) + " O=" + to_string(Flag_OF));
	text.setPosition(sf::Vector2f(10, 580));
	main_window.draw(text);

	text.setString("----------------");
	text.setPosition(sf::Vector2f(10, 610));
	main_window.draw(text);

	text.setString("Stack(16)");
	text.setPosition(sf::Vector2f(10, 640));
	main_window.draw(text);

	text.setString("----------------");
	text.setPosition(sf::Vector2f(10, 670));
	main_window.draw(text);

	for (int s = 0; s < 16; s++)
	{
		text.setString("[" + int_to_hex(*SS, 4) + ":" + int_to_hex(Stack_Pointer + s * 2, 4) + "] " + int_to_hex((int)(memory.read(Stack_Pointer + (s * 2) + SS_data * 16) + memory.read(Stack_Pointer + (s * 2) + 1 + SS_data * 16) * 256), 4));
		text.setPosition(sf::Vector2f(10, 700 + s * 30));
		main_window.draw(text);
	}

	//скорость работы
	if (!step_mode && elapsed_ms)
	{
		//обновляем массив времени кадра
		cpu_speed[cpu_speed_ptr] = round(ops * 1000000.0 / elapsed_ms);
		cpu_speed_ptr++;
		if (cpu_speed_ptr == 64) cpu_speed_ptr = 0;
		int avg = 0;
		for (int i = 0; i < 64; i++) avg += cpu_speed[i] / 64;
		text.setString(to_string((int)floor(avg / 1000)) + "K op/s");
		text.setFillColor(sf::Color::White);
		text.setPosition(sf::Vector2f(10, 1200));
		main_window.draw(text);
	}

	if (log_to_console) text.setString("LOG ON");
	else text.setString("LOG OFF");
	text.setPosition(sf::Vector2f(180, 1200));
	text.setFillColor(sf::Color::White);
	main_window.draw(text);

	if (step_mode) text.setString("STEP ON");
	else text.setString("STEP OFF");
	text.setPosition(sf::Vector2f(310, 1200));
	text.setFillColor(sf::Color::White);
	main_window.draw(text);

	//видеорежим
	text.setString(monitor.get_mode_name());
	text.setPosition(sf::Vector2f(460, 1200));
	text.setFillColor(sf::Color::White);
	text.setScale({ 0.7,1.0 });
	main_window.draw(text);
	text.setScale({ 1.0,1.0 });

	//данные пищалки
	if (speaker.beeping && !halt_cpu)
	{
		text.setString(to_string(speaker.timer_freq) + " Hz");
		text.setPosition(sf::Vector2f(730, 1200));
		text.setFillColor(sf::Color::Yellow);
		main_window.draw(text);
	}

	if (halt_cpu)
	{
		text.setString("HALT");
		text.setPosition(sf::Vector2f(730, 1200));
		text.setFillColor(sf::Color::Yellow);
		main_window.draw(text);
	}

	float t = round(memory.read(0x46c) / 1.82) / 10;
	if (t > 10) t -= 10;
	string sec = to_string(t);
	sec.resize(sec.find_first_of(",") + 2);
	text.setString(sec);
	text.setPosition(sf::Vector2f(820, 1200));
	text.setFillColor(sf::Color::Red);
	main_window.draw(text);
	text.setFillColor(sf::Color::White);

	//выводим регистры таймера

	text.setString("Timer Count Init Mode BCD RL Signal");
	text.setPosition(sf::Vector2f(300, 10));
	main_window.draw(text);

	text.setString("----------------------------------");
	text.setPosition(sf::Vector2f(300, 40));
	main_window.draw(text);

	for (int i = 0; i < 3; i++)
	{
		text.setString(to_string(i) + " " + pc_timer.get_ch_data(i));
		text.setPosition(sf::Vector2f(300, 70 + i * 30));
		main_window.draw(text);
	}

	text.setString("----------------------------------");
	text.setPosition(sf::Vector2f(300, 160));
	main_window.draw(text);

	//выводим регистры DMA

	text.setString(dma_ctrl.get_ch_data_3(0));
	text.setPosition(sf::Vector2f(300, 200));
	main_window.draw(text);

	text.setString("----------------------------------");
	text.setPosition(sf::Vector2f(300, 230));
	main_window.draw(text);

	text.setString("DMA   Req ON Mode Mask AUTO Trans");
	text.setPosition(sf::Vector2f(300, 260));
	main_window.draw(text);

	text.setString("----------------------------------");
	text.setPosition(sf::Vector2f(300, 290));
	main_window.draw(text);

	//первая таблица
	for (int i = 0; i < 4; i++)
	{
		string name;
		switch (i)
		{
		case 0:
			name = "ref";
			break;
		case 1:
			name = "mem";
			break;
		case 2:
			name = "FDD";
			break;
		case 3:
			name = "HDD";
			break;
		}

		text.setString(to_string(i) + " " + name + " " + dma_ctrl.get_ch_data(i));
		text.setPosition(sf::Vector2f(300, 320 + i * 30));
		main_window.draw(text);
	}

	text.setString("----------------------------------");
	text.setPosition(sf::Vector2f(300, 440));
	main_window.draw(text);

	text.setString("DMA   Page:ADDR Count  BaseA BaseC");
	text.setPosition(sf::Vector2f(300, 480));
	main_window.draw(text);

	text.setString("----------------------------------");
	text.setPosition(sf::Vector2f(300, 510));
	main_window.draw(text);

	for (int i = 0; i < 4; i++)
	{
		string name;
		switch (i)
		{
		case 0:
			name = "ref";
			break;
		case 1:
			name = "mem";
			break;
		case 2:
			name = "FDD";
			break;
		case 3:
			name = "HDD";
			break;
		}

		text.setString(to_string(i) + " " + name + " " + dma_ctrl.get_ch_data_2(i));
		text.setPosition(sf::Vector2f(300, 540 + i * 30));
		main_window.draw(text);
	}

	text.setString("----------------------------------");
	text.setPosition(sf::Vector2f(300, 660));
	main_window.draw(text);

	//INT controller
	text.setString("INT Mask Req Act");
	text.setPosition(sf::Vector2f(300, 690));
	main_window.draw(text);

	text.setString("----------------------------------");
	text.setPosition(sf::Vector2f(300, 720));
	main_window.draw(text);

	for (int i = 0; i < 8; i++)
	{
		text.setString(" " + to_string(i) + "   " + int_ctrl.get_ch_data(i));
		text.setPosition(sf::Vector2f(300, 750 + i * 30));
		main_window.draw(text);
	}

	//ON or OFF
	if (int_ctrl.enabled)
	{
		text.setString("En");
		text.setFillColor(sf::Color::Green);
	}
	else
	{
		text.setString("Dis");
		text.setFillColor(sf::Color::Red);
	}
	text.setPosition(sf::Vector2f(580, 690));
	main_window.draw(text);

	//INT sleep timer
	text.setString(to_string(int_ctrl.sleep_timer));
	text.setPosition(sf::Vector2f(580, 750));
	text.setFillColor(sf::Color::White);
	main_window.draw(text);


	//keyboard buffer data
	text.setString("| Keyboard");
	text.setPosition(sf::Vector2f(630, 690));
	text.setFillColor(sf::Color::White);
	main_window.draw(text);

	text.setString("| KB_buf=" + to_string(keyboard.get_buf_size()));
	text.setPosition(sf::Vector2f(630, 750));
	main_window.draw(text);

	//keyboard DOS buffer
	text.setString("| DOS_buf=" + to_string(memory.read(0x41c) + memory.read(0x41d) * 256 - memory.read(0x41a) - memory.read(0x41b) * 256));
	text.setPosition(sf::Vector2f(630, 780));
	main_window.draw(text);

	//KB_line status
	text.setString("| KB_line=" + to_string(keyboard.enabled));
	text.setPosition(sf::Vector2f(630, 810));
	main_window.draw(text);

	text.setString("|------------");
	text.setPosition(sf::Vector2f(630, 840));
	main_window.draw(text);

	text.setString("| HDD  " + to_string(HDD.get_drv()));
	text.setPosition(sf::Vector2f(630, 870));
	main_window.draw(text);

	text.setString("| ST:" + HDD.get_state());
	text.setPosition(sf::Vector2f(630, 900));
	main_window.draw(text);

	text.setString("| CSH  ");
	text.setPosition(sf::Vector2f(630, 930));
	main_window.draw(text);

	text.setString("| " + HDD.get_CSH());
	text.setPosition(sf::Vector2f(630, 960));
	main_window.draw(text);



	/*
	//cascade mode
	if (int_ctrl.cascade_mode) text.setString("Cascade ON");
	else text.setString("Cascade OFF");
	text.setPosition(sf::Vector2f(620, 810));
	main_window.draw(text);

	//nested mode
	if (int_ctrl.nested_mode) text.setString("Nested ON");
	else text.setString("Nested OFF");
	text.setPosition(sf::Vector2f(620, 840));
	main_window.draw(text);

	//ADDR_INTERVAL
	if (int_ctrl.ADDR_INTERVAL_4) text.setString("ADDR_INTERVAL = 4");
	else text.setString("ADDR_INTERVAL = 8");
	text.setPosition(sf::Vector2f(620, 870));
	main_window.draw(text);

	//AUTO_EOI
	if (int_ctrl.AUTO_EOI) text.setString("AUTO_EOI ON");
	else text.setString("AUTO_EOI OFF");
	text.setPosition(sf::Vector2f(620, 900));
	main_window.draw(text);

	//mode_8086
	if (int_ctrl.mode_8086) text.setString("Mode 86/88");
	else text.setString("Mode 80/85");
	text.setPosition(sf::Vector2f(620, 930));
	main_window.draw(text);

	//sleep_timer
	text.setString("sleep_timer = " + to_string(int_ctrl.sleep_timer));
	text.setPosition(sf::Vector2f(620, 960));
	main_window.draw(text);
	*/

	text.setString("----------------------------------");
	text.setPosition(sf::Vector2f(300, 990));
	main_window.draw(text);

	//заполняем массив по ссылке
	/*
	*ptr = CMD_BUSY;
	*(ptr + 1) = selected_drive;
	*(ptr + 2) = DMA_enabled;
	*(ptr + 3) = DMA_ON;
	*(ptr + 4) = motors_pin_enabled;
	*(ptr + 5) = FDD_busy_bits;
	*/

	//FDD controller
	uint8 raw_data[20];
	uint8* ptr = raw_data;
	text.setString("FDD state:" + FDD_A.get_state(ptr));
	text.setPosition(sf::Vector2f(300, 1020));
	main_window.draw(text);

	if (raw_data[0])
	{
		text.setFillColor(sf::Color::Red);
		text.setString("   BUSY");
	}
	else
	{
		text.setString("NO BUSY");
	}
	text.setPosition(sf::Vector2f(570, 1020));
	main_window.draw(text);

	text.setFillColor(sf::Color::White);
	text.setString("SEL DRV:" + to_string(raw_data[1]));
	text.setPosition(sf::Vector2f(695, 1020));
	main_window.draw(text);

	text.setString("----------------------------------");
	text.setPosition(sf::Vector2f(300, 1050));
	main_window.draw(text);

	if (raw_data[2])
	{
		text.setFillColor(sf::Color::Green);
		text.setString("DMA EN");
	}
	else
	{
		text.setString("DMA DIS");
	}

	text.setPosition(sf::Vector2f(300, 1080));
	main_window.draw(text);
	text.setFillColor(sf::Color::White);

	if (raw_data[3])
	{
		text.setFillColor(sf::Color::Green);
		text.setString("DMA ON");
	}
	else
	{
		text.setString("DMA OFF");
	}

	text.setPosition(sf::Vector2f(414, 1080));
	main_window.draw(text);

	text.setString("MOTORS[");
	text.setFillColor(sf::Color::White);
	text.setPosition(sf::Vector2f(530, 1080));
	main_window.draw(text);

	for (int i = 0; i < 4; ++i)
	{
		if ((raw_data[4] >> i) & 1)
		{
			text.setFillColor(sf::Color::Green);
			text.setString("0");
		}
		else
		{
			text.setFillColor(sf::Color::White);
			text.setString("-");
		}
		text.setPosition(sf::Vector2f(635 + i * 15, 1080));
		main_window.draw(text);
	}
	text.setString("]");
	text.setFillColor(sf::Color::White);
	text.setPosition(sf::Vector2f(695, 1080));
	main_window.draw(text);

	text.setString("BUSY[");
	text.setFillColor(sf::Color::White);
	text.setPosition(sf::Vector2f(710, 1080));
	main_window.draw(text);

	for (int i = 0; i < 4; ++i)
	{
		if ((raw_data[5] >> i) & 1)
		{
			text.setFillColor(sf::Color::Red);
			text.setString("0");
		}
		else
		{
			text.setFillColor(sf::Color::White);
			text.setString("-");
		}
		text.setPosition(sf::Vector2f(785 + i * 15, 1080));
		main_window.draw(text);
	}
	text.setString("]");
	text.setFillColor(sf::Color::White);
	text.setPosition(sf::Vector2f(840, 1080));
	main_window.draw(text);

	text.setString("----------------------------------");
	text.setPosition(sf::Vector2f(300, 1110));
	main_window.draw(text);

	text.setString("H[" + to_string(raw_data[8]) + "] Cyl[" + to_string(raw_data[6]) + "] Sec[" + to_string(raw_data[7]) + "] MT[" + to_string(raw_data[11]) + "][" + int_to_bin(memory.read(0x490)) + "]");
	text.setPosition(sf::Vector2f(300, 1140));
	main_window.draw(text);

	main_window.display();
	main_window.setActive(0);
}
void Dev_mon_device::update(int new_elapsed_ms, int op_counter)
{
	elapsed_ms = new_elapsed_ms;
	ops = op_counter;
	do_render = 1;				//обновить
}

void FDD_mon_device::main_loop()
{
	//создаем окно
	main_window.create(sf::VideoMode(sf::Vector2u(window_size_x, window_size_y)), window_title, sf::Style::Titlebar, sf::State::Windowed);
	main_window.setPosition({ window_pos_x, window_pos_y });
	main_window.setFramerateLimit(0);
	main_window.setMouseCursorVisible(1);
	main_window.setKeyRepeatEnabled(0);
	main_window.setVerticalSyncEnabled(1);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	while (visible)
	{
		if (do_render)
		{
			//рисуем окно
			render();
			//события окна
			while (main_window.pollEvent()) {};
			do_render = 0;
		}
		else
		{
			//отдыхаем
			_mm_pause();
			//std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(5)); //задержка перед завершением
	main_window.close();
}
void FDD_mon_device::show()
{
	//признак видимости
	visible = 1;

	//создаем поток
	std::thread new_t(&FDD_mon_device::main_loop, this);
	t = std::move(new_t);
}
void FDD_mon_device::hide()
{
	//запоминаем координаты
	sf::Vector2i p = main_window.getPosition();
	window_pos_x = p.x;
	window_pos_y = p.y;

	//скрываем окно
	visible = 0;
	if (t.joinable()) t.join();
}
void FDD_mon_device::log(string log_string)
{
	// пишем в массив строк
	if (last_str != log_string) log_strings.push_back(log_string);
	last_str = log_string;
}
void FDD_mon_device::render()
{
	main_window.setActive(1);
	main_window.clear(sf::Color::Black);// очистка экрана

	sf::Text text(font);				//обычный шрифт
	text.setCharacterSize(25);
	text.setFillColor(sf::Color::White);

	int max_s = 60; // 1600 / 25
	int begin = max(int(0), int(log_strings.size() - max_s));
	//cout << "for i=" << (int)begin << " to " << (int)log_strings.size() << endl;
	for (int i = begin; i < log_strings.size(); i++)
	{
		text.setFillColor(sf::Color::White);
		if (log_strings.at(i).find("INT13(READ)") != std::string::npos) text.setFillColor(sf::Color::Green);
		if (log_strings.at(i).find("INT13") != std::string::npos) text.setFillColor(sf::Color::Green);
		if (log_strings.at(i).find("INT13(WRITE)") != std::string::npos) text.setFillColor(sf::Color::Red);
		if (log_strings.at(i).find("CMD") != std::string::npos) text.setFillColor(sf::Color::Cyan);
		//if (log_strings.at(i).find("Result OK") != std::string::npos) text.setFillColor(sf::Color::Green);
		//if (log_strings.at(i).find("ERR") != std::string::npos) text.setFillColor(sf::Color::Red);

		text.setString(log_strings.at(i));
		text.setPosition(sf::Vector2f(0, 50 + (i - begin) * 25));
		main_window.draw(text);
	}
	main_window.display();
	main_window.setActive(0);
}
void FDD_mon_device::update(int new_elapsed_ms)
{
	do_render = 1;				//обновить
}

void HDD_mon_device::main_loop()
{
	//создаем окно
	main_window.create(sf::VideoMode(sf::Vector2u(window_size_x, window_size_y)), window_title, sf::Style::Titlebar, sf::State::Windowed);
	main_window.setPosition({ window_pos_x, window_pos_y });
	main_window.setFramerateLimit(0);
	main_window.setMouseCursorVisible(1);
	main_window.setKeyRepeatEnabled(0);
	main_window.setVerticalSyncEnabled(1);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	while (visible)
	{
		if (do_render)
		{
			//рисуем окно
			render();
			//события окна
			while (main_window.pollEvent()) {};
			do_render = 0;
		}
		else
		{
			//отдыхаем
			_mm_pause();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(5)); //задержка перед завершением
	main_window.close();
}
void HDD_mon_device::show()
{
	//признак видимости
	visible = 1;

	//создаем поток
	std::thread new_t(&HDD_mon_device::main_loop, this);
	t = std::move(new_t);
}
void HDD_mon_device::hide()
{
	//запоминаем координаты
	sf::Vector2i p = main_window.getPosition();
	window_pos_x = p.x;
	window_pos_y = p.y;

	//скрываем окно
	visible = 0;
	if (t.joinable()) t.join();
}
void HDD_mon_device::log(string log_string)
{
	// пишем в массив строк
	if (last_str != log_string) log_strings.push_back(log_string);
	last_str = log_string;
}
void HDD_mon_device::render()
{
	main_window.setActive(1);
	main_window.clear(sf::Color::Black);// очистка экрана

	sf::Text text(font);				//обычный шрифт
	text.setCharacterSize(25);
	text.setFillColor(sf::Color::White);

	//HDD controller

	int max_s = 60; // 1600 / 25
	int begin = max(int(0), int(log_strings.size() - max_s));
	//cout << "for i=" << (int)begin << " to " << (int)log_strings.size() << endl;
	for (int i = begin; i < log_strings.size(); i++)
	{
		text.setFillColor(sf::Color::White);
		if (log_strings.at(i).find("INT13(READ)") != std::string::npos) text.setFillColor(sf::Color::Green);
		if (log_strings.at(i).find("INT13") != std::string::npos) text.setFillColor(sf::Color::Green);
		if (log_strings.at(i).find("INT13(WRITE)") != std::string::npos) text.setFillColor(sf::Color::Red);
		if (log_strings.at(i).find("EXEcu") != std::string::npos) text.setFillColor(sf::Color::Cyan);
		if (log_strings.at(i).find("Result OK") != std::string::npos) text.setFillColor(sf::Color::Green);
		if (log_strings.at(i).find("ERR") != std::string::npos) text.setFillColor(sf::Color::Red);

		text.setString(log_strings.at(i));
		text.setPosition(sf::Vector2f(0, 50 + (i - begin) * 25));
		main_window.draw(text);
	}
	main_window.display();
	main_window.setActive(0);
}
void HDD_mon_device::update(int new_elapsed_ms)
{
	do_render = 1;				//обновить
}

void Mem_mon_device::main_loop()
{
	//создаем окно
	main_window.create(sf::VideoMode(sf::Vector2u(window_size_x, window_size_y)), window_title, sf::Style::Titlebar, sf::State::Windowed);
	main_window.setPosition({ window_pos_x, window_pos_y });
	main_window.setFramerateLimit(0);
	main_window.setMouseCursorVisible(1);
	main_window.setKeyRepeatEnabled(0);
	main_window.setVerticalSyncEnabled(1);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	while (visible)
	{
		if (do_render)
		{
			//рисуем окно
			render();
			//события окна
			while (main_window.pollEvent()) {};
			do_render = 0;
		}
		else
		{
			//отдыхаем
			_mm_pause();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(5)); //задержка перед завершением
	main_window.close();
}
void Mem_mon_device::show()
{
	//признак видимости
	visible = 1;

	//создаем поток
	std::thread new_t(&Mem_mon_device::main_loop, this);
	t = std::move(new_t);
}
void Mem_mon_device::hide()
{
	//запоминаем координаты
	sf::Vector2i p = main_window.getPosition();
	window_pos_x = p.x;
	window_pos_y = p.y;

	//скрываем окно
	visible = 0;
	if (t.joinable()) t.join();
}
void Mem_mon_device::render()
{
	main_window.setActive(1);
	static bool keys_up;

	if (!isKeyPressed(sf::Keyboard::Key::Left) && !isKeyPressed(sf::Keyboard::Key::Right) && !isKeyPressed(sf::Keyboard::Key::Up) && !isKeyPressed(sf::Keyboard::Key::Down)) keys_up = 1;

	if (main_window.hasFocus() && keys_up)
	{
		//проверяем клавиатуру
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
		{
			//движение курсора влево
			cursor -= 1;
			if (cursor == 4) cursor = 3;
			if (cursor == 255) cursor = 7;
			keys_up = 0; //предотвращение залипания
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
		{
			//движение курсора вправо
			cursor += 1;
			if (cursor == 9) cursor = 0;
			if (cursor == 4) cursor = 5;
			keys_up = 0; //предотвращение залипания
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) && cursor != 4)
		{
			//увеличение цифры под курсором
			if (cursor < 4)
			{
				uint8 d = ((segment >> ((3 - cursor) * 4)) & 0xF) + 1;//цифра под курсором
				if (d == 16) d = 0;
				//cout << "d= " << (int)d << endl;
				segment = segment & ~(0xF << ((3 - cursor) * 4));
				segment = segment | (d << ((3 - cursor) * 4));
			}
			if (cursor > 4)
			{
				uint8 d = ((offset >> ((8 - cursor) * 4)) & 0xF) + 1;//цифра под курсором
				if (d == 16) d = 0;
				//cout << "d= " << (int)d << endl;
				offset = offset & ~(0xF << ((8 - cursor) * 4));
				offset = offset | (d << ((8 - cursor) * 4));
			}
			keys_up = 0; //предотвращение залипания
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) && cursor != 4)
		{
			//увеличение цифры под курсором
			if (cursor < 4)
			{
				uint8 d = ((segment >> ((3 - cursor) * 4)) & 0xF) - 1;//цифра под курсором

				if (d > 15) d = 15;
				//cout << "d= " << (int)d << endl;
				segment = segment & ~(0xF << ((3 - cursor) * 4));
				segment = segment | (d << ((3 - cursor) * 4));
			}
			if (cursor > 4)
			{
				uint8 d = ((offset >> ((8 - cursor) * 4)) & 0xF) - 1;//цифра под курсором

				if (d > 15) d = 15;
				//cout << "d= " << (int)d << endl;
				offset = offset & ~(0xF << ((8 - cursor) * 4));
				offset = offset | (d << ((8 - cursor) * 4));
			}
			keys_up = 0; //предотвращение залипания
		}

	}

	main_window.clear(sf::Color::Black);// очистка экрана

	sf::Text text(font);				//обычный шрифт
	text.setCharacterSize(30);
	//text.setScale({ 1.5,1.0 });
	text.setFillColor(sf::Color::White);

	//заголовок
	text.setString("Mem segment: ");
	text.setPosition({ 3,2 });
	//text.setStyle(sf::Text::Bold);
	main_window.draw(text);

	//выводим адрес по символам
	string addr = int_to_hex(segment, 4) + ":" + int_to_hex(offset, 4);

	for (int pos = 0; pos < 9; pos++)
	{
		if (pos == 4) continue;
		text.setString(addr.substr(pos, 1));
		//text.setScale({ 1.5,1.0 });
		//text.setStyle(sf::Text::Bold);
		text.setPosition(sf::Vector2f(220 + pos * 23, 2));
		main_window.draw(text);
		sf::RectangleShape rectangle({ 19.f, 29.f });
		rectangle.setPosition(sf::Vector2f(pos * 23.0 + 220.0, 7.0));
		if (pos == cursor) rectangle.setOutlineColor(sf::Color::Red);
		else rectangle.setOutlineColor(sf::Color::White);
		rectangle.setOutlineThickness(2.0);
		rectangle.setFillColor(sf::Color::Transparent);
		main_window.draw(rectangle);
	}

	//выводим содержимое ячеек памяти
	text.setCharacterSize(22);
	for (int y = 0; y < 64; y++)  //строки
	{
		uint16 new_segment = segment + y;
		string ns = int_to_hex(new_segment, 4) + ":" + int_to_hex(offset, 4) + "|";
		transform(ns.begin(), ns.end(), ns.begin(), ::toupper);
		text.setString(ns);
		text.setPosition(sf::Vector2f(3, y * 25 + 25 + 20 + floor(y / 16) * 5));
		main_window.draw(text);

		for (int x = 0; x < 16; x++)  //столбцы
		{
			string s = int_to_hex(memory.read(segment * 16 + offset + y * 16 + x), 2);
			transform(s.begin(), s.end(), s.begin(), ::toupper);
			text.setString(s);
			//text.setScale({ 1.5,1.0 });
			//text.setStyle(sf::Text::Bold);
			text.setPosition(sf::Vector2f(133 + x * 32 + 5 + floor(x / 8) * 3, y * 25 + 25 + 20 + floor(y / 16) * 5));
			main_window.draw(text);
		}
		ns = "| ";
		for (int x = 0; x < 16; x++)  //столбцы
		{
			if ((memory.read(segment * 16 + offset + y * 16 + x)) < 32) ns += ".";
			else ns += (char)memory.read(segment * 16 + offset + y * 16 + x);
		}
		text.setString(ns);
		text.setPosition(sf::Vector2f(656, y * 25 + 25 + 20 + floor(y / 16) * 5));
		main_window.draw(text);
	}
	main_window.display();
	main_window.setActive(0);
}
void Mem_mon_device::update(int new_elapsed_ms)
{
	do_render = 1;				//обновить
}

void EGA_mon_device::main_loop()
{
	//создаем окно
	main_window.create(sf::VideoMode(sf::Vector2u(window_size_x, window_size_y)), window_title, sf::Style::Titlebar, sf::State::Windowed);
	main_window.setPosition({ window_pos_x, window_pos_y });
	main_window.setFramerateLimit(0);
	main_window.setMouseCursorVisible(1);
	main_window.setKeyRepeatEnabled(0);
	main_window.setVerticalSyncEnabled(1);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	while (visible)
	{
		if (do_render)
		{
			//рисуем окно
			render();
			//события окна
			while (main_window.pollEvent()) {};
			do_render = 0;
		}
		else
		{
			//отдыхаем
			_mm_pause();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(5)); //задержка перед завершением
	main_window.close();
}
void EGA_mon_device::show()
{
	//признак видимости
	visible = 1;

	//создаем поток
	std::thread new_t(&EGA_mon_device::main_loop, this);
	t = std::move(new_t);
}
void EGA_mon_device::hide()
{
	//запоминаем координаты
	sf::Vector2i p = main_window.getPosition();
	window_pos_x = p.x;
	window_pos_y = p.y;

	//скрываем окно
	visible = 0;
	if (t.joinable()) t.join();
}
void EGA_mon_device::render()
{
	main_window.setActive(1);
	main_window.clear(sf::Color::Black);// очистка экрана

	sf::Text text(font);				//обычный шрифт
	text.setCharacterSize(30);
	text.setFillColor(sf::Color::White);

	//заголовок
	text.setString("EGA registers (" + monitor.get_debug_data(6) + ")");
	text.setPosition({ 3, 3 });
	main_window.draw(text);

	//Miscelaneous Output Register
	text.setString(monitor.get_debug_data(0));
	text.setPosition({ 3, 40 });
	main_window.draw(text);

	//Sequencer registers
	text.setString("Sequencer registers");
	text.setPosition({ 3, 80 });
	text.setFillColor(sf::Color::Yellow);
	main_window.draw(text);
	text.setFillColor(sf::Color::White);

	for (int i = 0; i < 5; i++)
	{
		int p1 = monitor.get_debug_data(i + 1).find_first_of(":");
		text.setString(monitor.get_debug_data(i + 1).substr(0, p1 + 1));
		text.setPosition({ 3, 110 + (float)i * 30 });
		main_window.draw(text);
		text.setString(monitor.get_debug_data(i + 1).substr(p1 + 1, 20));
		text.setPosition({ 300, 110 + (float)i * 30 });
		main_window.draw(text);
	}
		
	//CRT controller
	text.setString("CRT controller Registers");
	text.setPosition({ 3, 270 });
	text.setFillColor(sf::Color::Yellow);
	main_window.draw(text);
	text.setFillColor(sf::Color::White);

	for (int i = 0; i < 20; i++)
	{
		int p1 = monitor.get_debug_data(i + 7).find_first_of(":");
		text.setString(monitor.get_debug_data(i + 7).substr(0, p1 + 1));
		text.setPosition({ 3, 300 + (float)i*30 });
		main_window.draw(text);
		text.setString(monitor.get_debug_data(i + 7).substr(p1 + 1, 20));
		text.setPosition({300, 300 + (float)i * 30 });
		main_window.draw(text);
	}

	//Grafics controller register
	text.setString("Grafics controller Registers");
	text.setPosition({ 3, 930 });
	text.setFillColor(sf::Color::Yellow);
	main_window.draw(text);
	text.setFillColor(sf::Color::White);

	for (int i = 0; i < 4; i++)
	{
		int p1 = monitor.get_debug_data(i + 27).find_first_of(":");
		text.setString(monitor.get_debug_data(i + 27).substr(0, p1 + 1));
		text.setPosition({ 3, 960 + (float)i * 30 });
		main_window.draw(text);
		text.setString(monitor.get_debug_data(i + 27).substr(p1 + 1, 20));
		text.setPosition({ 300, 960 + (float)i * 30 });
		main_window.draw(text);
	}
	
	//Grafics SI register
	text.setString("Grafics SI Registers");
	text.setPosition({600, 80 });
	text.setFillColor(sf::Color::Yellow);
	main_window.draw(text);
	text.setFillColor(sf::Color::White);

	for (int i = 0; i < 19; i++)
	{
		int p1 = monitor.get_debug_data(i + 31).find_first_of(":");
		text.setString(monitor.get_debug_data(i + 31).substr(0, p1 + 1));
		text.setPosition({ 600, 110 + (float)i * 30 });
		main_window.draw(text);
		text.setString(monitor.get_debug_data(i + 31).substr(p1 + 1, 20));
		text.setPosition({ 900, 110 + (float)i * 30 });
		main_window.draw(text);
	}
	
	//Grafics controller register (block 2)
	text.setString("Block 2");
	text.setPosition({ 600, 930 });
	text.setFillColor(sf::Color::Yellow);
	main_window.draw(text);
	text.setFillColor(sf::Color::White);

	for (int i = 0; i < 5; i++)
	{
		int p1 = monitor.get_debug_data(i + 50).find_first_of(":");
		text.setString(monitor.get_debug_data(i + 50).substr(0, p1 + 1));
		text.setPosition({ 600, 960 + (float)i * 30 });
		main_window.draw(text);
		text.setString(monitor.get_debug_data(i + 50).substr(p1 + 1, 20));
		text.setPosition({ 900, 960 + (float)i * 30 });
		main_window.draw(text);
	}


	
	main_window.display();
	main_window.setActive(0);
}
void EGA_mon_device::update(int new_elapsed_ms)
{
	do_render = 1;				//обновить
}

//==================== MDA videocard =============

void MDA_videocard::main_loop()
{
	//создаем окно
	main_window.create(sf::VideoMode(sf::Vector2u(320 * 5 + 40, 240 * 5 + 40)), window_title, sf::Style::Titlebar, sf::State::Windowed);
	main_window.setPosition({ window_pos_x, window_pos_y });
	main_window.setSize(sf::Vector2u(window_size_x, window_size_y));
	main_window.setFramerateLimit(0);
	main_window.setMouseCursorVisible(1);
	main_window.setKeyRepeatEnabled(0);
	main_window.setVerticalSyncEnabled(1);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	main_window.setActive(1);

	while (visible)
	{
		if (do_render)
		{
			do_render = 0;
			if (do_resize)
			{
				main_window.setSize(sf::Vector2u(window_size_x, window_size_y));
				do_resize = 0;
			}
			//рисуем окно
			render();
			//события окна
			while (main_window.pollEvent()) {};
		}
		else
		{
			//отдыхаем
			_mm_pause();
			//std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(5)); //задержка перед завершением
	main_window.close();
}
void MDA_videocard::update(int new_elapsed_ms)
{
	do_render = 1;				//обновить
	elapsed_ms = new_elapsed_ms;
}
void MDA_videocard::render()					//синхронизация
{
	static auto Hi_Res_t_start = Hi_Res_Clk.now();
	static auto Hi_Res_t_end = Hi_Res_Clk.now();

	main_window.setActive(1);
	main_window.clear();// очистка экрана

	uint16 font_t_x, font_t_y;
	uint32 addr;
	bool attr_under = false;       //атрибут подчеркивания
	bool attr_blink = false;       //атрибут мигания
	bool attr_highlight = false;   //атрибут подсветки

	//начальный адрес экрана в памяти
	uint16 start_address = (registers[0xC] & 0b00111111) * 256 + registers[0xD];

	//адрес курсора
	uint16 cursor_pos = registers[0xE] * 256 + registers[0xF];

	sf::RectangleShape rectangle;  //объект для рисования

	uint8 width = 80;
	uint8 attrib = 0;			//атрибуты символов

	sf::Text text(font);		//обычный шрифт

	//проверка бита включения экрана
	if ((MDA_Mode_Select_Register & 8) == 0) goto exit; //карта отключена

	//цикл отрисовки экрана
	if (cursor_clock.getElapsedTime().asMicroseconds() > 300000) //мигание курсора
	{
		cursor_clock.restart();
		cursor_flipflop = !cursor_flipflop;
	}

	text.setCharacterSize(40);
	text.setFillColor(sf::Color::White);

	rectangle.setScale(sf::Vector2f(0.4444, 0.6857));

	//настройка масштаба символов для разных режимов
	rectangle.setSize(sf::Vector2f(5 * 9, 5 * 14));

	//Каждый символ мог обладать следующими атрибутами : невидимый, подчёркнутый, обычный, яркий(жирный), инвертированный и мигающий.
	//Бит 5 MDA_Mode_Select_Register — 1 для включения мигания, 0 — для его отключения.
	//Если бит 5 установлен в 1, символы с установленным битом 7 атрибута будут мигать, если нет — иметь фон с высокой интенсивностью
	//бит 3 - интенсивность

	//заполняем экран
	for (int y = 0; y < 25; y++)  //25 строк
	{
		for (int x = 0; x < width; x++)  //80 символов в строке
		{
			//задаем цвета
			sf::Color fg_color = sf::Color::Green;
			sf::Color fg_color_bright = sf::Color::Yellow;
			sf::Color fg_color_inverse = sf::Color::Black;
			sf::Color bg_color = sf::Color::Black;
			sf::Color bg_color_inverse = sf::Color::Green;

			addr = 0xb0000 + (y * width * 2) + x * 2;

			font_t_y = memory.read(addr) >> 5;
			font_t_x = memory.read(addr) - (font_t_y << 5);
			attrib = memory.read(addr + 1);

			bool bright = (attrib >> 3) & 1;
			bool underline = ((attrib & 7) == 1);
			bool blink = ((attrib >> 7) & 1) & ((MDA_Mode_Select_Register >> 5) & 1);
			bool inversed = ((attrib >> 4) & 1);

			//исключения

			if (attrib == 0x0 || attrib == 0x08 || attrib == 0x80 || attrib == 0x88) fg_color = sf::Color::Black;
			if (attrib == 0x70 || attrib == 0xF0) { fg_color = sf::Color::Black; sf::Color bg_color = sf::Color(0, 192, 0); }
			if (attrib == 0x78 || attrib == 0xF8) { fg_color = sf::Color(0, 64, 0); sf::Color bg_color = sf::Color(0, 192, 0); }


			//рисуем фон символа
			if (!inversed) rectangle.setFillColor(bg_color);
			else rectangle.setFillColor(bg_color_inverse);
			rectangle.setPosition(sf::Vector2f(x * 8 * 5 * 0.5 + 20, y * 8 * 5 * 1.2 + 20));
			main_window.draw(rectangle);

			//рисуем сам символ
			if (!blink || cursor_flipflop)

			{
				//font_t_x = 1;
				//font_t_y = 0;
				font_sprite_80_MDA.setTextureRect(sf::IntRect(sf::Vector2i(font_t_x * 40, font_t_y * 96), sf::Vector2i(40, 96)));
				font_sprite_80_MDA.setPosition(sf::Vector2f(x * 8 * 5 * 0.5 + 20, y * 8 * 5 * 1.2 + 20));
				font_sprite_80_MDA.setScale(sf::Vector2f(0.5, 0.5));
				font_sprite_80_MDA.setColor(fg_color);
				if (inversed) font_sprite_80_MDA.setColor(fg_color_inverse);
				if (bright) font_sprite_80_MDA.setColor(fg_color_bright);
				main_window.draw(font_sprite_80_MDA);
			}
			if (underline) //подчеркивание
			{
				sf::RectangleShape ul; //прямоугольник курсора
				ul.setSize(sf::Vector2f(45 * 0.4444, 2));
				ul.setPosition(sf::Vector2f(x * 8 * 5 * 0.5 + 20, (y * 14 * 5 * 0.6857 + 20 + 45)));
				ul.setFillColor(fg_color);
				main_window.draw(ul);
			}

			bool draw_cursor = false;
			if ((width == 80) && !(y * 80 + x - registers[0xe] * 256 - registers[0xf])) draw_cursor = true;

			//рисуем курсор

			if (draw_cursor && cursor_flipflop && ((registers[0xb] & 31) >= (registers[0xa] & 31)))
			{

				//font_sprite_80.setTextureRect(sf::IntRect(sf::Vector2i(31 * 8 * 5 * 0.5, 2 * 8 * 5), sf::Vector2i(8 * 5 * 0.5, 8 * 5)));
				//font_sprite_80.setPosition(sf::Vector2f(x * 8 * 5 * 0.5 + 20, y * 8 * 5 * 1.2 + 20));
				//font_sprite_80.setColor(sf::Color::White);
				//main_window.draw(font_sprite_80);
				sf::RectangleShape cursor_rectangle; //прямоугольник курсора
				cursor_rectangle.setScale(sf::Vector2f(0.4444, 0.6857));
				cursor_rectangle.setSize(sf::Vector2f(45, 4));
				cursor_rectangle.setPosition(sf::Vector2f(x * 20 + 20, y * 48 + 43 + 20));
				cursor_rectangle.setFillColor(fg_color);
				main_window.draw(cursor_rectangle);
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

	Hi_Res_t_end = Hi_Res_Clk.now();
	//информация для отладки

	debug_mess_1 = to_string(1000000 / (duration_cast<microseconds>(Hi_Res_t_end - Hi_Res_t_start).count() + 1));
	text.setString(debug_mess_1);
	text.setCharacterSize(20);
	text.setPosition(sf::Vector2f(0, 0));
	text.setFillColor(sf::Color(255, 0, 0));
	text.setOutlineThickness(3.1);
	if (debug_mess_1 != "") main_window.draw(text);

exit:
	main_window.display();
	do_render = 0;
	main_window.setActive(0);
	Hi_Res_t_start = Hi_Res_Clk.now();
}
void MDA_videocard::write_port(uint16 port, uint8 data)	//запись в порт адаптера
{
	if (log_to_console) cout << "MDA write port " << (int)port << " data " << (bitset<8>)data << endl;
	if (port == 0x3b4)
	{
		//выбор регистра для записи
		if (data < 18) sel_reg = data;
		//deferred_msg += "CGA select REG = " + to_string(data);
		if (sel_reg == 1 && log_to_console) cout << "MDA select REG -> CGA columns ";
		if (sel_reg == 6 && log_to_console) cout << "MDA select REG -> CGA lines ";
		if (sel_reg == 0x0C && log_to_console) cout << "MDA select REG -> CGA mem: high byte ";
		if (sel_reg == 0x0D && log_to_console) cout << "MDA select REG -> CGA mem: low byte ";
	}
	if (port == 0x3b5)
	{
		//запись в выбранный регистр
		registers[sel_reg] = data;
		if (sel_reg == 1 && log_to_console) cout << "MDA columns = " << int(data) << " ";
		if (sel_reg == 6 && log_to_console) cout << "MDA lines = " << int(data) << " ";
		if (sel_reg == 0x0C && log_to_console) cout << "MDA mem: high byte = " << int(data) << " ";
		if (sel_reg == 0x0D && log_to_console) cout << "MDA mem: low byte = " << int(data) << " ";
	}
	if (port == 0x3b8)
	{
		//установка режимов работы
		if (log_to_console) cout << "MDA set MODE = " << (bitset<5>)(data & 31) << endl;
		MDA_Mode_Select_Register = data;
	}
	if (port == 0x3b9)
	{
		//выбор режимов цвета
		if (log_to_console) cout << "MDA set COLOUR = " << (bitset<5>)(data) << " ";
		//CGA_Color_Select_Register = data;
	}
}
uint8 MDA_videocard::read_port(uint16 port)				//чтение из порта адаптера
{
	if (port == 0x3B5)
	{
		//if (log_to_console) cout << "MDA: read PORT 0x3B5 (read registers) ";
		//считывание регистра
		if (sel_reg == 0x0C) return registers[sel_reg]; //старший байт начального адреса (Start Address Register - SAR, high byte)
		if (sel_reg == 0x0D) return registers[sel_reg]; //младший байт начального адреса (Start address Register - SAR, low byte)
		if (sel_reg == 0x0E) return registers[sel_reg]; //старший байт позиции курсора (Cursor Location Register - CLR, high byte)
		if (sel_reg == 0x0F) return registers[sel_reg]; //младший байт позиции курсора (Cursor Location Register - CLR, low byte)

	}
	if (port == 0x3BA)
	{
		//if (log_to_console) cout << "MDA: read PORT 0x3BA (CRT ray) -> ";
		//считывание регистра состояния
		//меняем для симуляции обратного хода луча
		static uint8 CRT_flip_flop = 0;
		CRT_flip_flop++;
		//if (CRT_flip_flop && log_to_console) cout << "0000 ";
		//if (!CRT_flip_flop && log_to_console) cout << "1001 ";
		if (!CRT_flip_flop) return 0b00000000; // бит 3 - обратный ход луча, бит 0 - разрешение записи в видеопамять
		else return 0b00001001;
	}
	return 0;
}
std::string MDA_videocard::get_mode_name()				//получить название режима для отладки
{
	return "[MDA] text 80x25 BW";
}
void MDA_videocard::show_joy_sence(uint8 central_point)	//отобразить информацию джойстика
{
	joy_sence_show_timer = 1000000; //включаем таймер
	joy_sence_value = central_point;
}
bool MDA_videocard::has_focus()							//проверить наличие фокуса
{
	return main_window.hasFocus();
}
MDA_videocard::MDA_videocard()							//конструктор
{
	//инициализируем графические константы
	GAME_WINDOW_X_RES = 320 * 5 + 40;	// масштаб 1:5 + 500 px для отладки
	GAME_WINDOW_Y_RES = 200 * 1.2 * 5 + 40;  // масштаб 1:5

	//получаем данные о текущем дисплее
	my_display_H = sf::VideoMode::getDesktopMode().size.y;
	my_display_W = sf::VideoMode::getDesktopMode().size.x;

	//базовое разрешение
	GAME_WINDOW_X_RES = 320 * 5 + 40;		 // масштаб 1:5 + 500 px для отладки
	GAME_WINDOW_Y_RES = 200 * 1.2 * 5 + 40;  // масштаб 1:5

	display_scale = floor(5 * my_display_H / GAME_WINDOW_Y_RES);

	cout << "Video Init (MDA) " << my_display_H << " x " << my_display_W << " display" << " scale " << (int)display_scale << endl;

	//задаем переменные окна
	window_title = "IBM PC/XT emulator. MDA videocard.";		//заголовок окна
	window_pos_x = (my_display_W - GAME_WINDOW_X_RES * display_scale / 5) / 2;		//позиция окна
	window_pos_y = (my_display_H - GAME_WINDOW_Y_RES * display_scale / 5 - 30) / 2;		//позиция окна
	window_size_x = GAME_WINDOW_X_RES * display_scale / 5;			//размер окна
	window_size_y = GAME_WINDOW_Y_RES * display_scale / 5;			//размер окна
	cursor_clock.restart();										//запускаем таймер мигания
}
void MDA_videocard::show()
{
	//признак видимости
	visible = 1;

	//создаем поток
	std::thread new_t(&MDA_videocard::main_loop, this);
	t = std::move(new_t);
}
void MDA_videocard::hide()
{
	//запоминаем координаты
	sf::Vector2i p = main_window.getPosition();
	window_pos_x = p.x;
	window_pos_y = p.y;

	//скрываем окно
	visible = 0;
	if (t.joinable()) t.join();
}
bool MDA_videocard::is_visible()
{
	return visible;
}
void MDA_videocard::scale_up()
{
	if (display_scale < 8) display_scale++;
	window_size_x = 320 * display_scale + 40;	// масштаб 1:5 + 500 px для отладки
	window_size_y = 200 * 1.2 * display_scale + 40;  // масштаб 1:5
	do_resize = 1;
}
void MDA_videocard::scale_down()
{
	if (display_scale > 1) display_scale--;
	window_size_x = 320 * display_scale + 40;	// масштаб 1:5 + 500 px для отладки
	window_size_y = 200 * 1.2 * display_scale + 40;  // масштаб 1:5
	do_resize = 1;
}
void MDA_videocard::mem_write(uint32 address, uint8 data)
{
	videomemory[address] = data;
}
uint8 MDA_videocard::mem_read(uint32 address)
{
	return videomemory[address];
}
sf::Vector2i MDA_videocard::get_mouse_pos()
{
	return sf::Vector2i({ 0,0 });
}

//==================== EGA videocard =============

EGA_videocard::EGA_videocard()							//конструктор
{

	//инициализируем графические константы
	GAME_WINDOW_X_RES = 320 * 5 + 40;	// масштаб 1:5 + 500 px для отладки
	GAME_WINDOW_Y_RES = 200 * 1.2 * 5 + 40;  // масштаб 1:5

	//получаем данные о текущем дисплее
	my_display_H = sf::VideoMode::getDesktopMode().size.y;
	my_display_W = sf::VideoMode::getDesktopMode().size.x;

	//базовое разрешение
	GAME_WINDOW_X_RES = 320 * 5 + 40;		 // масштаб 1:5 + 500 px для отладки
	GAME_WINDOW_Y_RES = 200 * 1.2 * 5 + 40;  // масштаб 1:5

	display_scale = floor(5 * my_display_H / GAME_WINDOW_Y_RES);

	cout << "Video Init (EGA) " << my_display_H << " x " << my_display_W << " display" << " scale " << (int)display_scale << endl;

	//задаем переменные окна
	window_title = "IBM PC/XT emulator. EGA videocard.";		//заголовок окна
	window_pos_x = (my_display_W - GAME_WINDOW_X_RES * display_scale / 5) / 2;		//позиция окна
	window_pos_y = (my_display_H - GAME_WINDOW_Y_RES * display_scale / 5 - 30) / 2;		//позиция окна
	window_size_x = GAME_WINDOW_X_RES * display_scale / 5;			//размер окна
	window_size_y = GAME_WINDOW_Y_RES * display_scale / 5;			//размер окна
	cursor_clock.restart();										//запускаем таймер мигания

	//массив цветов для текста
	TXT_colors[0] = sf::Color(0, 0, 0);
	TXT_colors[1] = sf::Color(0, 0, 0xAA);
	TXT_colors[2] = sf::Color(0, 0xAA, 0);
	TXT_colors[3] = sf::Color(0, 0xAA, 0xAA);
	TXT_colors[4] = sf::Color(0xAA, 0, 0);
	TXT_colors[5] = sf::Color(0xAA, 0, 0xAA);
	TXT_colors[6] = sf::Color(0xAA, 0x55, 0);
	TXT_colors[7] = sf::Color(0xAA, 0xAA, 0xAA);
	TXT_colors[8] = sf::Color(0x55, 0x55, 0x55);
	TXT_colors[9] = sf::Color(0x55, 0x55, 0xFF);
	TXT_colors[10] = sf::Color(0x55, 0xFF, 0x55);
	TXT_colors[11] = sf::Color(0x55, 0xFF, 0xFF);
	TXT_colors[12] = sf::Color(0xFF, 0x55, 0x55);
	TXT_colors[13] = sf::Color(0xFF, 0x55, 0xFF);
	TXT_colors[14] = sf::Color(0xFF, 0xFF, 0x55);
	TXT_colors[15] = sf::Color(0xFF, 0xFF, 0xFF);

	//таблица черно-белых цветов для текста
	TXT_BW_colors[1] = sf::Color(0, 0, 0);
	TXT_BW_colors[1] = sf::Color(0x55, 0x55, 0x55);
	TXT_BW_colors[2] = sf::Color(0x55, 0x55, 0x55);
	TXT_BW_colors[3] = sf::Color(0x55, 0x55, 0x55);
	TXT_BW_colors[4] = sf::Color(0x55, 0x55, 0x55);
	TXT_BW_colors[5] = sf::Color(0x55, 0x55, 0x55);
	TXT_BW_colors[6] = sf::Color(0x55, 0x55, 0x55);
	TXT_BW_colors[7] = sf::Color(0xAA, 0xAA, 0xAA);
	TXT_BW_colors[8] = sf::Color(0x77, 0x55, 0x55);
	TXT_BW_colors[9] = sf::Color(0xAA, 0xAA, 0xAA);
	TXT_BW_colors[10] = sf::Color(0xAA, 0xAA, 0xAA);
	TXT_BW_colors[11] = sf::Color(0xAA, 0xAA, 0xAA);
	TXT_BW_colors[12] = sf::Color(0xAA, 0xAA, 0xAA);
	TXT_BW_colors[13] = sf::Color(0xAA, 0xAA, 0xAA);
	TXT_BW_colors[14] = sf::Color(0xAA, 0xAA, 0xAA);
	TXT_BW_colors[15] = sf::Color(0xFF, 0xFF, 0xFF);
	
	//цвета для графического режима
	EGA_colors[0] = sf::Color(0, 0, 0);
	EGA_colors[1] = sf::Color(0, 0, 0xAA);
	EGA_colors[2] = sf::Color(0, 0xAA, 0);
	EGA_colors[3] = sf::Color(0, 0xAA, 0xAA);
	EGA_colors[4] = sf::Color(0xAA, 0, 0);
	EGA_colors[5] = sf::Color(0xAA, 0, 0xAA);
	EGA_colors[6] = sf::Color(0xAA, 0x55, 0);
	EGA_colors[7] = sf::Color(0xAA, 0xAA, 0xAA);
	EGA_colors[8] = sf::Color(0x55, 0x55, 0x55);
	EGA_colors[9] = sf::Color(0x55, 0x55, 0xFF);
	EGA_colors[10] = sf::Color(0x55, 0xFF, 0x55);
	EGA_colors[11] = sf::Color(0x55, 0xFF, 0xFF);
	EGA_colors[12] = sf::Color(0xFF, 0x55, 0x55);
	EGA_colors[13] = sf::Color(0xFF, 0x55, 0xFF);
	EGA_colors[14] = sf::Color(0xFF, 0xFF, 0x55);
	EGA_colors[15] = sf::Color(0xFF, 0xFF, 0xFF);
}
void EGA_videocard::show()
{
	//признак видимости
	visible = 1;

	//создаем поток
	std::thread new_t(&EGA_videocard::main_loop, this);
	t = std::move(new_t);
}
void EGA_videocard::hide()
{
	//запоминаем координаты
	sf::Vector2i p = main_window.getPosition();
	window_pos_x = p.x;
	window_pos_y = p.y;

	//скрываем окно
	visible = 0;
	if (t.joinable()) t.join();
}
void EGA_videocard::update(int new_elapsed_ms)
{
	do_render = 1;				//обновить
	elapsed_ms = new_elapsed_ms;
}
void EGA_videocard::main_loop()
{
	//создаем окно
	main_window.create(sf::VideoMode(sf::Vector2u(320 * 5 + 40, 240 * 5 + 40)), window_title, sf::Style::Titlebar, sf::State::Windowed);
	main_window.setPosition({ window_pos_x, window_pos_y });
	main_window.setSize(sf::Vector2u(window_size_x, window_size_y));
	main_window.setFramerateLimit(0);
	main_window.setMouseCursorVisible(1);
	main_window.setKeyRepeatEnabled(0);
	main_window.setVerticalSyncEnabled(1);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	main_window.setActive(1);

	while (visible)
	{
		if (do_render)
		{
			do_render = 0;
			if (do_resize)
			{
				main_window.setSize(sf::Vector2u(window_size_x, window_size_y));
				do_resize = 0;
			}
			//рисуем окно
			render();
			_mm_pause();
			//события окна
			while (main_window.pollEvent()) {};
		}
		else
		{
			//отдыхаем
			_mm_pause();
			_mm_pause();
			_mm_pause();
			_mm_pause();
			if (step_mode || log_to_console) std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(5)); //задержка перед завершением
	main_window.close();
}
void EGA_videocard::write_port(uint16 port, uint8 data)	//запись в порт адаптера
{
	//if (log_to_console_EGA) cout << "EGA write port " << (int)port << " data " << (bitset<8>)data << endl;

	if (port == 0x3c2)  //Miscelaneous Output Register
	{
		MOR = data;
		IOAddrSel = data & 1;
		EnRAM = (data >> 1) & 1;
		On_board_switch_sel = (data >> 2) & 3; //выбор номера переключателя
		High_pix_clock = (data >> 2) & 1; //повышенная частота (720 пикселей)
		PageBit = (data >> 5) & 1;
		Lines_350 = (data >> 7) & 1;
		if (log_to_console_EGA)
		{
			cout << "EGA: bit0(BW/COL)=" << (int)IOAddrSel << " En_RAM=" << (int)EnRAM << " PageBit=" << (int)PageBit;
			if (On_board_switch_sel == 0) cout << " clk=14MHz";
			if (On_board_switch_sel == 1) cout << " clk=16MHz";
			if (On_board_switch_sel == 2) cout << " clk=Ext";
			cout << " IntDRV=" << (int)((data >> 4) & 1) << " 64K_page=" << (int)PageBit << " bit_6_7=" << (int)((data >> 6) & 3) << endl;
		}
	}

	if ((port == 0x3ba && !IOAddrSel) || (port == 0x3da && IOAddrSel)) //Feature Control Register
	{
		//управление доп разъемом
		ac_flipflop = 0; //
	}

	if (port == 0x3C4) //Sequencer Address Register
	{
		SEQ_sel_reg = data & 31;
		if (log_to_console_EGA) cout << "EGA set Seq_addr_reg = " << (int)SEQ_sel_reg << endl;
	}
	if (port == 0x3c5)
	{
		//запись в выбранный регистр
		seq_registers[SEQ_sel_reg] = data;
		if (SEQ_sel_reg == 0)
		{
			if ((data & 3) != 3)
			{
				if (log_to_console_EGA) cout << "EGA Reset Sequencer" << endl;  //добавить сброс регистров
				seq_registers[1] = 0;
				seq_registers[2] = 0;
				seq_registers[3] = 0;
				seq_registers[4] = 0;
			}
		}
		if (SEQ_sel_reg == 1 && log_to_console_EGA) cout << "EGA Clocking Mode Register" << (bitset<8>)data << endl;
		if (SEQ_sel_reg == 2 && log_to_console_EGA) cout << "EGA Map Mask Register" << (bitset<8>)data << endl;
		if (SEQ_sel_reg == 3 && log_to_console_EGA) cout << "EGA Character Map Select Register" << (bitset<8>)data << endl;

		use_2_char_gen = ((seq_registers[3] & 0b00010011) != ((seq_registers[3] >> 1) & 0b00010011)); //если таблицы разные - используем две

		if (SEQ_sel_reg == 4 && log_to_console_EGA) cout << "EGA Memory Mode Register" << (bitset<8>)data << endl;
		return;
	}
	if (port == 0x3b4 || port == 0x3d4) //CRT Controller Address Register
	{
		CRT_sel_reg = data & 31;
		//if (log_to_console_EGA) cout << "EGA CRT sel reg -> " << (int)CRT_sel_reg << " ";
	}
	if (port == 0x3b5 || port == 0x3d5)
	{
		//запись в выбранный регистр
		crt_registers[CRT_sel_reg] = data;
		//if (log_to_console_EGA) cout << "EGA write CRT reg -> " << (int)(data) << " ";
	}
	if (port == 0x3cc)  //Graphics 1 Position Register
	{
		GP1_reg = data;
	}
	if (port == 0x3ca)  //Graphics 2 Position Register
	{
		GP2_reg = data;
	}
	if (port == 0x3ce)  //Graphics 1 and 2 Address Register
	{
		GC_sel_reg = data & 15;
		//if (log_to_console_EGA) cout << "EGA GC sel reg -> " << (int)GC_sel_reg << " ";
	}
	if (port == 0x3cf)  //Graphics controller registers
	{
		gc_registers[GC_sel_reg] = data;
		//if (log_to_console_EGA) cout << "EGA write GC reg -> " << (int)data << " ";
	}

	if (port == 0x3c0)  //Attribute controller register
	{
		if (ac_flipflop)
		{
			AC_sel_reg = data & 31; //выбор номера регистра
			//if (log_to_console_EGA) cout << "EGA AC sel reg -> " << hex << (int)AC_sel_reg << " ";
		}
		else
		{
			//запись в регистр
			ac_registers[AC_sel_reg] = data;
			//if (log_to_console_EGA) cout << "EGA write AC reg -> " << hex << (int)data << " ";
		}
		ac_flipflop = !ac_flipflop;  //переключаем тип ввода
	}

}
uint8 EGA_videocard::read_port(uint16 port)				//чтение из порта адаптера
{
	if (port == 0x3c2) //Input Status Register Zero
	{
		static bool clock = 0;
		clock = !clock;
		uint8 out = 0;
		switch (On_board_switch_sel)
		{
		case 0:
			out = 0b00010000; //переключатель 1 = ON
			break;
		case 1:
			out = 0b00010000; //переключатель 2 = ON
			break;
		case 2:
			out = 0b00010000; //переключатель 3 = ON
			break;
		case 3:
			out = 0b00000000; //переключатель 4 = OFF
			break;
		}

		if (clock) out = out | 0b10000000;		//идет показ изображения
		else out = out & 0b01111111;			//можно читать свитчи + экран погашен

		return out;
	}

	if ((port == 0x3ba && !IOAddrSel) || (port == 0x3da && IOAddrSel)) //Input Status Register One
	{
		uint8 out = 0;
		
		//рассчитываем позицию луча
		uint16 frame_pos = pc_timer.get_time(3) % (256*256);
		if (frame_pos < 712) out = 8; //период гашения
		else
		{
			//экран
			uint16 line_pos = frame_pos % 91;
			if (line_pos < 17) out = 0; //гашение линии
			else out = 1;				//отображение пикселя
		}

		return out;
	}

	if (port == 0x3b5 || port == 0x3d5)
	{
		//чтение из выбранного регистра
		return crt_registers[CRT_sel_reg];
	}

	return 0;
}
std::string EGA_videocard::get_mode_name()				//получить название режима для отладки
{
	string mode_name;
	bool color_enable = ~((ac_registers[16] >> 2) & 1);  //наличие цвета

	switch (current_mode)
	{
	case video_modes::EGA_01_200:
		return "[EGA01] TXT 40x25";
	case video_modes::EGA_01_350:
		return "[EGA01] TXT 40x25[H]";
	case video_modes::EGA_23_200:
		return "[EGA23] TXT 80x25";
	case video_modes::EGA_23_350:
		return "[EGA23] TXT 80x25[H]";
	case video_modes::EGA_7_720:
		return "[EGA7] TXT 80x25[720]";
	case video_modes::EGA_45_320:
		return "[EGA45] GFX 320x200";
	case video_modes::EGA_6_640:
		return "[EGA6] GFX 640x200";
	case video_modes::EGA_D_320:
		return "[EGAD] GFX 320x200";
	case video_modes::EGA_E_200:
		return "[EGAE] GFX 640x200";
	case video_modes::EGA_F_350:
		return "[EGAF] GFX 640x350";
	case video_modes::EGA_10_350:
		return "[EGA10] GFX 640x350";
	}
	return "Error";
}
void EGA_videocard::show_joy_sence(uint8 central_point)	//отобразить информацию джойстика
{
	joy_sence_show_timer = 1000000; //включаем таймер
	joy_sence_value = central_point;
}
bool EGA_videocard::has_focus()							//проверить наличие фокуса
{
	return main_window.hasFocus();
}
void EGA_videocard::render()					//синхронизация
{
	/*
		Режимы работы
		video_modes {EGA_01_200, EGA_01_350, EGA_23_200, EGA_23_350, EGA_7_720, EGA_45_320, EGA_6_640, EGA_D_320, EGA_E_200, EGA_F_350, EGA_10_350};
		
		Текстовые
		0   - 40 x 25 символов (8х8),  16 цветов,    320х200 пикселей, адрес B8000, 8 страниц
		0*  - 40 x 25 символов (8х14), 16/64 цветов, 320х350 пикселей, адрес B8000, 8 страниц
		1   - 40 x 25 символов (8х8),  16 цветов,    320х200 пикселей, адрес B8000, 8 страниц
		1*  - 40 x 25 символов (8х14), 16/64 цветов, 320х350 пикселей, адрес B8000, 8 страниц

		2   - 80 х 25 символов (8х8),  16 цветов,    640х200 пикселей, адрес B8000, 8 страниц
		2*  - 80 х 25 символов (8х14), 16/64 цветов, 640х350 пикселей, адрес B8000, 8 страниц
		3   - 80 х 25 символов (8х8),  16 цветов,    640х200 пикселей, адрес B8000, 8 страниц
		3*  - 80 х 25 символов (8х14), 16/64 цветов, 640х350 пикселей, адрес B8000, 8 страниц

		7   - 80 х 25 символов (9х14), 4 цвета,      720х350 пикселей, адрес B0000, 8 страниц (аналог MDA)

		Графические
		4   - 40 x 25 символов (8х8),  4 цвета,      320х200 пикселей, адрес B8000, 1 страница
		5   - 40 x 25 символов (8х8),  4 цвета,      320х200 пикселей, адрес B8000, 1 страница
		6   - 80 х 25 символов (8х8),  2 цвета,      640х200 пикселей, адрес B8000, 1 страница
		D   - 40 x 25 символов (8х8),  16 цветов,    320х200 пикселей, адрес A0000, 8 страниц
		E   - 80 х 25 символов (8х8),  16 цветов,    640х200 пикселей, адрес A0000, 8 страниц
		F   - 80 х 25 символов (8х14), 2 цвета,      640х350 пикселей, адрес A0000, 2 страницы (монохромный графический)
		10* - 80 х 25 символов (8х14), 16/64 цвета,  640х350 пикселей, адрес A0000, 2 страницы

		* - улучшеный дисплей

		gc_registers[4] - выбор страницы памяти
		gc_registers[5] - режим работы, использование регистров-защелок для копирования/вставки данных,  бит 4 - четный/нечетный режим (для текста), бит5 - для режимов 4 и 5 (граф)
		ac_registers [0 - 15] палитры
		ac_registers[16] - режимы
		ac_registers[17] - цвет рамки
		ac_registers[18] - отключение цветовых слоев
		ac_registers[19] - сдвиг влево попиксельно (0 - 7 пикселей)
	*/

	sf::Text text(monitor.font);		//обычный шрифт
	text.setCharacterSize(20);
	text.setFillColor(sf::Color::White);

	uint16 font_t_x, font_t_y;
	uint32 addr;
	bool attr_under = false;       //атрибут подчеркивания
	bool attr_blink = false;       //атрибут мигания
	bool attr_highlight = false;   //атрибут подсветки

	sf::Color border = sf::Color((((ac_registers[17] >> 2) & 1) + ((ac_registers[17] >> 5) & 1) * 2) * 85, (((ac_registers[17] >> 1) & 1) + ((ac_registers[17] >> 4) & 1) * 2) * 85, (((ac_registers[17] >> 0) & 1) + ((ac_registers[17] >> 3) & 1) * 2) * 85); // ac_registers[17]
	
	//начальный адрес экрана в памяти
	uint16 start_address = (crt_registers[0xC]) * 256 + crt_registers[0xD];

	//адрес курсора
	uint16 cursor_pos = crt_registers[0xE] * 256 + crt_registers[0xF];

	//используемые адреса видеопамяти
	switch ((gc_registers[6] >> 2) & 3)
	{
	case 0:
	case 1:
		video_mem_base = 0xA0000; //Режимы EGA
		break;
	case 2:
		video_mem_base = 0xB0000; //Монохромный режим
		break;
	case 3:
		video_mem_base = 0xB8000; //Совместимость с CGA
		break;
	}

	main_window.clear();// очистка экрана

	//проверка бита включения экрана
	//if ((CGA_Mode_Select_Register & 8) == 0) return; //карта отключена

	//мигание курсора
	if (cursor_clock.getElapsedTime().asMicroseconds() > 300000) //мигание курсора
	{
		cursor_clock.restart();
		cursor_flipflop = !cursor_flipflop;
	}

	//управление цветом
	bool color_enable = ~((ac_registers[16] >> 1) & 1);  //наличие цвета

	//рисование символов
	bool dot9_enable = ((ac_registers[16] >> 2) & 1);  //дорисовка 9 точки у псевдографики (коды 0xC0 - 0xDF)

	//выбор атрибута: интенсивность/мигание
	bool intensity_blink = ((ac_registers[16] >> 3) & 1);  //0 - повышенная яркость или 1 - мигание 

	//Определение номеров режимов
	if (gc_registers[6] & 1)
	{
		//debug_mess_1 = "graf branch";
		//графические режимы 4,5,6, D, E, F, 10
		if ((seq_registers[1] >> 3) & 1)  //пиксельная частота
		{
			//ширина 320 режимы 4, 5, D
			if (!gc_registers[5]) current_mode = video_modes::EGA_D_320;
			else current_mode = video_modes::EGA_45_320;
		}
		else
		{
			//ширина 640  режимы 6, E, F, 10
			if (Lines_350)
			{
				//режимы 350 линий - F и 10
				if (IOAddrSel) current_mode = video_modes::EGA_10_350;
				else current_mode = video_modes::EGA_F_350;
			}
			else
			{
				//режимы 200 линий - 6 и E
				if (((gc_registers[6] >> 2) & 3) == 3) current_mode = video_modes::EGA_6_640;
				else current_mode = video_modes::EGA_E_200;
			}
		}
	}
	else
	{
		//debug_mess_1 = "txt branch";
		//текстовые режимы 0, 1, 2, 3, 7
		//ширина
		if ((seq_registers[1] >> 3) & 1)  //пиксельная частота
		{
			//режимы шириной 40 - 0 и 1
			//линии 200/350 - переменная Lines_350
			if (Lines_350) current_mode = video_modes::EGA_01_350;
			else current_mode = video_modes::EGA_01_200;
		}
		else
		{
			//режимы шириной 80 - 2, 3, 7 (dot9)
			//линии 200/350 - переменная Lines_350
			if (High_pix_clock)
			{
				//режим 7 - 80 х 25, высота 14 пикселей (720х350)
				current_mode = video_modes::EGA_7_720;
			}
			else
			{
				//режимы 2 и 3 - 80 х 25, высота 8/14 пикселей (200/350 линий)
				if (Lines_350) current_mode = video_modes::EGA_23_350;
				else current_mode = video_modes::EGA_23_200;
			}
		}
	}

	//выводим режимы
	//текст 40х25
	if (current_mode == video_modes::EGA_01_200)
	{
		//текстовые режимы 0 и 1 - 40x25
		//8 страниц по 2КБ каждая

		//заливаем цветом рамку
		sf::RectangleShape rectangle;
		rectangle.setScale(sf::Vector2f(1, 1));
		rectangle.setSize(sf::Vector2f(320 * 5 + 40, 200 * 1.2 * 5 + 40));
		rectangle.setPosition(sf::Vector2f(0, 0));
		rectangle.setFillColor(border);

		main_window.draw(rectangle);
		rectangle.setSize(sf::Vector2f(320 * 5, 200 * 1.2 * 5));
		rectangle.setPosition(sf::Vector2f(20, 20));
		rectangle.setFillColor(sf::Color(0, 0, 0));
		main_window.draw(rectangle);

		//символов в строке
		uint8 width = 40;

		//настройка масштаба символов для разных режимов
		if (width == 40) rectangle.setSize(sf::Vector2f(5 * 8, 1.2 * 5 * 8));
		else rectangle.setSize(sf::Vector2f(5 * 0.5 * 8, 1.2 * 5 * 8));

		uint8 attrib = 0; //атрибуты символов

		sf::Color fg_color;
		sf::Color bg_color;

		//заполняем экран
		for (int y = 0; y < 25; y++)  //25 строк
		{
			for (int x = 0; x < width; x++)  //40 или 80 символов в строке
			{
				addr = start_address * 2 + (y * width * 2) + x * 2;

				font_t_y = videomemory[addr] >> 5;
				font_t_x = videomemory[addr] - (font_t_y << 5);
				attrib = videomemory[addr + 1];

				if (color_enable)
				{
					//цветной режим
					fg_color = EGA_colors[attrib & 15];
					if (!use_2_char_gen && ((attrib >> 3) & 1))  fg_color = EGA_colors[attrib & 31];  //повышенная интенсивность
					bg_color = EGA_colors[(attrib >> 4) & 15];
					if (((attrib >> 7) & 1) & !((ac_registers[16] >> 3) & 1)) bg_color = EGA_colors[(attrib >> 4) & 31];//повышенная интенсивность
				}
				else
				{
					//монохромный режим
					fg_color = EGA_colors[8];
					bg_color = EGA_colors[0];

					if (!use_2_char_gen && ((attrib >> 3) & 1))
					{
						//повышенная интенсивность
						fg_color = EGA_colors[15];
					}

					if (((attrib >> 7) & 1) & !((ac_registers[16] >> 3) & 1))
					{
						//повышенная интенсивность
						//fg_color = EGA_colors[15];
					}

					if (((attrib >> 4) & 7) == 7)
					{
						//инверсия
						sf::Color t = fg_color;
						fg_color = bg_color;
						bg_color = t;
					}
				}
				bool blink = ((attrib >> 7) & 1) & intensity_blink;

				//рисуем фон символа
				rectangle.setFillColor(bg_color);
				if (width == 40) rectangle.setPosition(sf::Vector2f(x * 8 * 5 + 20, y * 8 * 5 * 1.2 + 20));
				else rectangle.setPosition(sf::Vector2f(x * 8 * 5 * 0.5 + 20, y * 8 * 5 * 1.2 + 20));
				main_window.draw(rectangle);

				//рисуем сам символ
				if (!blink || cursor_flipflop)
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

				//подчеркивание в монохромном режиме
				/*
				if (!color_enable && ((attrib & 7) == 1))
				{
					sf::RectangleShape ul; //прямоугольник курсора
					if (width == 40)
					{
						ul.setSize(sf::Vector2f(40, 2));
						ul.setPosition(sf::Vector2f(font_t_x * 8 * 5, font_t_y * 8 * 5));
					}
					else
					{
						ul.setSize(sf::Vector2f(20, 2));
						ul.setPosition(sf::Vector2f(font_t_x * 8 * 5 * 0.5, font_t_y * 8 * 5));
					}
					ul.setFillColor(fg_color);
					main_window.draw(ul);
				}*/

				bool draw_cursor = false;
				if ((width == 40) && !(y * 40 + x - crt_registers[0xe] * 256 - crt_registers[0xf])) draw_cursor = true;
				if ((width == 80) && !(y * 80 + x - crt_registers[0xe] * 256 - crt_registers[0xf])) draw_cursor = true;

				//рисуем курсор
				if (draw_cursor && cursor_flipflop && ((crt_registers[0xb] & 31) >= (crt_registers[0xa] & 31)))
				{
					if (width == 40)
					{
						sf::RectangleShape cursor_rectangle; //прямоугольник курсора
						cursor_rectangle.setScale(sf::Vector2f(1, 1));
						cursor_rectangle.setSize(sf::Vector2f(40, (crt_registers[0xb] - crt_registers[0xa] + 1) * 6));
						cursor_rectangle.setPosition(sf::Vector2f(x * 8 * 5 + 20, (y * 8 + crt_registers[0xa]) * 5 * 1.2 + 20));
						cursor_rectangle.setFillColor(fg_color);
						main_window.draw(cursor_rectangle);
					}
					else
					{
						sf::RectangleShape cursor_rectangle; //прямоугольник курсора
						cursor_rectangle.setScale(sf::Vector2f(1, 1));
						cursor_rectangle.setSize(sf::Vector2f(20, (crt_registers[0xb] - crt_registers[0xa] + 1) * 6));
						cursor_rectangle.setPosition(sf::Vector2f(x * 8 * 5 * 0.5 + 20, (y * 8 + crt_registers[0xa]) * 5 * 1.2 + 20));
						cursor_rectangle.setFillColor(fg_color);
						main_window.draw(cursor_rectangle);
					}
				}
			}
		}
	}
	
	//текст 80х25
	if (current_mode == video_modes::EGA_23_200)
	{
		//текстовые режимы 2 и 3 - 80x25
		//4 страницы по 4КБ каждая 
		//если памяти > 64К, то 8 страниц

		//заливаем цветом рамку
		sf::RectangleShape rectangle;
		rectangle.setScale(sf::Vector2f(1, 1));
		rectangle.setSize(sf::Vector2f(320 * 5 + 40, 200 * 1.2 * 5 + 40));
		rectangle.setPosition(sf::Vector2f(0, 0));
		rectangle.setFillColor(border);

		main_window.draw(rectangle);
		rectangle.setSize(sf::Vector2f(320 * 5, 200 * 1.2 * 5));
		rectangle.setPosition(sf::Vector2f(20, 20));
		rectangle.setFillColor(sf::Color(0, 0, 0));
		main_window.draw(rectangle);

		//символов в строке
		uint8 width = 80;

		//настройка масштаба символов для разных режимов
		if (width == 40) rectangle.setSize(sf::Vector2f(5 * 8, 1.2 * 5 * 8));
		else rectangle.setSize(sf::Vector2f(5 * 0.5 * 8, 1.2 * 5 * 8));

		uint8 attrib = 0; //атрибуты символов

		sf::Color fg_color;
		sf::Color bg_color;

		//заполняем экран
		for (int y = 0; y < 25; y++)  //25 строк
		{
			for (int x = 0; x < width; x++)  //40 или 80 символов в строке
			{
				addr = start_address * 2 + (y * width * 2) + x * 2;

				font_t_y = videomemory[addr] >> 5;
				font_t_x = videomemory[addr] - (font_t_y << 5);
				attrib = videomemory[addr+1];

				if (color_enable)
				{
					//цветной режим
					fg_color = TXT_colors[attrib & 15];
					if (!use_2_char_gen && ((attrib >> 3) & 1))  fg_color = TXT_colors[attrib & 31];  //повышенная интенсивность
					bg_color = TXT_colors[(attrib >> 4) & 15];
					if (((attrib >> 7) & 1) & !((ac_registers[16] >> 3) & 1)) bg_color = TXT_colors[(attrib >> 4) & 31];//повышенная интенсивность
				}
				else
				{
					//монохромный режим
					fg_color = TXT_BW_colors[8];
					bg_color = TXT_BW_colors[0];

					if (!use_2_char_gen && ((attrib >> 3) & 1))
					{
						//повышенная интенсивность
						fg_color = TXT_BW_colors[15];
					}

					if (((attrib >> 7) & 1) & !((ac_registers[16] >> 3) & 1))
					{
						//повышенная интенсивность
						fg_color = TXT_BW_colors[15];
					}

					if (((attrib >> 4) & 7) == 7)
					{
						//инверсия
						sf::Color t = fg_color;
						fg_color = bg_color;
						bg_color = t;
					}
				}
				bool blink = ((attrib >> 7) & 1) & intensity_blink;

				//рисуем фон символа
				rectangle.setFillColor(bg_color);
				if (width == 40) rectangle.setPosition(sf::Vector2f(x * 8 * 5 + 20, y * 8 * 5 * 1.2 + 20));
				else rectangle.setPosition(sf::Vector2f(x * 8 * 5 * 0.5 + 20, y * 8 * 5 * 1.2 + 20));
				main_window.draw(rectangle);

				//рисуем сам символ
				if (!blink || cursor_flipflop)
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

				//подчеркивание в монохромном режиме
				/*
				if (!color_enable && ((attrib & 7) == 1))
				{
					sf::RectangleShape ul; //прямоугольник курсора
					if (width == 40)
					{
						ul.setSize(sf::Vector2f(40, 2));
						ul.setPosition(sf::Vector2f(font_t_x * 8 * 5, font_t_y * 8 * 5));
					}
					else
					{
						ul.setSize(sf::Vector2f(20, 2));
						ul.setPosition(sf::Vector2f(font_t_x * 8 * 5 * 0.5, font_t_y * 8 * 5));
					}
					ul.setFillColor(fg_color);
					main_window.draw(ul);
				}*/

				bool draw_cursor = false;
				if ((width == 40) && !(y * 40 + x - crt_registers[0xe] * 256 - crt_registers[0xf])) draw_cursor = true;
				if ((width == 80) && !(y * 80 + x - crt_registers[0xe] * 256 - crt_registers[0xf])) draw_cursor = true;

				//рисуем курсор
				if (draw_cursor && cursor_flipflop && ((crt_registers[0xb] & 31) >= (crt_registers[0xa] & 31)))
				{
					if (width == 40)
					{
						sf::RectangleShape cursor_rectangle; //прямоугольник курсора
						cursor_rectangle.setScale(sf::Vector2f(1, 1));
						cursor_rectangle.setSize(sf::Vector2f(40, (crt_registers[0xb] - crt_registers[0xa] + 1) * 6));
						cursor_rectangle.setPosition(sf::Vector2f(x * 8 * 5 + 20, (y * 8 + crt_registers[0xa]) * 5 * 1.2 + 20));
						cursor_rectangle.setFillColor(fg_color);
						main_window.draw(cursor_rectangle);
					}
					else
					{
						sf::RectangleShape cursor_rectangle; //прямоугольник курсора
						cursor_rectangle.setScale(sf::Vector2f(1, 1));
						cursor_rectangle.setSize(sf::Vector2f(20, (crt_registers[0xb] - crt_registers[0xa] + 1) * 6));
						cursor_rectangle.setPosition(sf::Vector2f(x * 8 * 5 * 0.5 + 20, (y * 8 + crt_registers[0xa]) * 5 * 1.2 + 20));
						cursor_rectangle.setFillColor(fg_color);
						main_window.draw(cursor_rectangle);
					}
				}
			}
		}
	}

	//графика 320х200
	if (current_mode == video_modes::EGA_45_320)
	{
		//графический режим 320х200
		//одна страница памяти, цвета CGA

		//закрашиваем фон
		sf::RectangleShape rectangle;
		rectangle.setScale(sf::Vector2f(1, 1));
		rectangle.setSize(sf::Vector2f(320 * 5, 200 * 1.2 * 5));
		rectangle.setPosition(sf::Vector2f(20, 20));
		rectangle.setFillColor(sf::Color::Black); //заливаем фон
		main_window.draw(rectangle);

		for (int y = 0; y < 100; ++y)
		{
			for (int x = 0; x < 80; ++x)
			{
				uint32 dot_addr = (start_address * 2 + 80 * y + x) % 0x2000;
				
				for (int sub_x = 0; sub_x < 4; sub_x++)
				{
					rectangle.setSize(sf::Vector2f(5, 6));
					rectangle.setPosition(sf::Vector2f(x * 4 * 5 + 20 + sub_x * 5, (y * 6) * 2 + 20));
					if ((videomemory[dot_addr] >> ((6 - sub_x * 2)) & 3) == 0) rectangle.setFillColor(sf::Color::Black);
					if ((videomemory[dot_addr] >> ((6 - sub_x * 2)) & 3) == 1) rectangle.setFillColor(sf::Color::Cyan);
					if ((videomemory[dot_addr] >> ((6- sub_x * 2)) & 3) == 2) rectangle.setFillColor(sf::Color::Magenta);
					if ((videomemory[dot_addr] >> ((6 - sub_x * 2)) & 3) == 3) rectangle.setFillColor(sf::Color::White);
					main_window.draw(rectangle);
				}
			}
		}

		for (int y = 0; y < 100; ++y)
		{
			for (int x = 0; x < 80; ++x)
			{
				uint32 dot_addr = (start_address * 2  + 80 * y + x) % 0x2000 + 0x2000;

				for (int sub_x = 0; sub_x < 4; sub_x++)
				{
					rectangle.setSize(sf::Vector2f(5, 6));
					rectangle.setPosition(sf::Vector2f(x * 4 * 5 + 20 + sub_x * 5, (y * 6) * 2 + 6 + 20));
					if ((videomemory[dot_addr] >> ((6 - sub_x * 2)) & 3) == 0) rectangle.setFillColor(sf::Color::Black);
					if ((videomemory[dot_addr] >> ((6 - sub_x * 2)) & 3) == 1) rectangle.setFillColor(sf::Color::Cyan);
					if ((videomemory[dot_addr] >> ((6 - sub_x * 2)) & 3) == 2) rectangle.setFillColor(sf::Color::Magenta);
					if ((videomemory[dot_addr] >> ((6 - sub_x * 2)) & 3) == 3) rectangle.setFillColor(sf::Color::White);
					main_window.draw(rectangle);
				}
			}
		}
	}

	//графика 640х200
	if (current_mode == video_modes::EGA_6_640)
	{
		//режим 640х200, 2 цвета
		//аналог режима высокого разрешения CGA

		sf::RectangleShape dot;
		dot.setOutlineThickness(0);

		//масштаб точек
		float display_x_scale = 2.5; // (float)(GAME_WINDOW_X_RES - 40) / width;  //1600    5 или 2,5
		float display_y_scale = 6; // (float)(GAME_WINDOW_Y_RES - 40) / height; //1200    6
		dot.setScale(sf::Vector2f(display_x_scale, display_y_scale));

		//start_address  - начало буфера
		//добавить переход в начало памяти
		
		dot.setSize(sf::Vector2f(2.5, 6));
		for (int y = 0; y < 100; y++)
		{
			for (int x = 0; x < 80; ++x)
			{
				for (int sub_x = 0; sub_x < 8; sub_x++)
				{
					//четные
					uint32 dot_addr = (80 * y + x);
					dot.setPosition(sf::Vector2f(x * 4 * 5 + 20 + sub_x * 2.5, (y * 6) * 2 + 20));
					if (((videomemory[dot_addr] >> (7 - sub_x)) & 1) == 0) dot.setFillColor(sf::Color::Black);
					else dot.setFillColor(sf::Color::White);
					main_window.draw(dot);
					//нечетные
					dot_addr = (0x2000 + 80 * y + x);
					dot.setPosition(sf::Vector2f(x * 4 * 5 + 20 + sub_x * 2.5, (y * 6) * 2 + 6 + 20));
					if (((videomemory[dot_addr] >> (7 - sub_x)) & 1) == 0) dot.setFillColor(sf::Color::Black);
					else dot.setFillColor(sf::Color::White);
					main_window.draw(dot);
				}
			}
		}
	}

	//текстовый монохромный режим 25х80 (MDA)
	if (current_mode == video_modes::EGA_7_720)
	{
		//4 страницы по 4кб (если > 64К, то 8 страниц)

		main_window.setActive(1);
		main_window.clear();// очистка экрана

		uint16 font_t_x, font_t_y;
		uint32 addr;
		bool attr_under = false;       //атрибут подчеркивания
		bool attr_blink = false;       //атрибут мигания
		bool attr_highlight = false;   //атрибут подсветки

		//начальный адрес экрана в памяти
		//uint16 start_address = (registers[0xC] & 0b00111111) * 256 + registers[0xD];

		//адрес курсора
		//uint16 cursor_pos = registers[0xE] * 256 + registers[0xF];

		sf::RectangleShape rectangle;  //объект для рисования

		uint8 attrib = 0;			//атрибуты символов

		sf::Text text(font);		//обычный шрифт

		//проверка бита включения экрана
		//if ((MDA_Mode_Select_Register & 8) == 0) goto exit; //карта отключена

		//цикл отрисовки экрана
		if (cursor_clock.getElapsedTime().asMicroseconds() > 300000) //мигание курсора
		{
			cursor_clock.restart();
			cursor_flipflop = !cursor_flipflop;
		}

		text.setCharacterSize(40);
		text.setFillColor(sf::Color::White);

		rectangle.setScale(sf::Vector2f(0.4444, 0.6857));

		//настройка масштаба символов для разных режимов
		rectangle.setSize(sf::Vector2f(5 * 9, 5 * 14));

		//Каждый символ мог обладать следующими атрибутами : невидимый, подчёркнутый, обычный, яркий(жирный), инвертированный и мигающий.
		//Бит 5 MDA_Mode_Select_Register — 1 для включения мигания, 0 — для его отключения.
		//Если бит 5 установлен в 1, символы с установленным битом 7 атрибута будут мигать, если нет — иметь фон с высокой интенсивностью
		//бит 3 - интенсивность

		//заполняем экран
		for (int y = 0; y < 25; y++)  //25 строк
		{
			for (int x = 0; x < 80; x++)  //80 символов в строке
			{
				//задаем цвета
				sf::Color fg_color = sf::Color::Green;
				sf::Color fg_color_bright = sf::Color::Yellow;
				sf::Color fg_color_inverse = sf::Color::Black;
				sf::Color bg_color = sf::Color::Black;
				sf::Color bg_color_inverse = sf::Color::Green;

				addr = (y * 80 * 2) + x * 2;

				font_t_y = videomemory[addr] >> 5;
				font_t_x = videomemory[addr] - (font_t_y << 5);
				attrib = videomemory[addr + 1];

				bool bright = (attrib >> 3) & 1;
				bool underline = ((attrib & 7) == 1);
				bool blink = ((attrib >> 7) & 1);
				bool inversed = ((attrib >> 4) & 1);

				//исключения

				if (attrib == 0x0 || attrib == 0x08 || attrib == 0x80 || attrib == 0x88) fg_color = sf::Color::Black;
				if (attrib == 0x70 || attrib == 0xF0) { fg_color = sf::Color::Black; sf::Color bg_color = sf::Color(0, 192, 0); }
				if (attrib == 0x78 || attrib == 0xF8) { fg_color = sf::Color(0, 64, 0); sf::Color bg_color = sf::Color(0, 192, 0); }


				//рисуем фон символа
				if (!inversed) rectangle.setFillColor(bg_color);
				else rectangle.setFillColor(bg_color_inverse);
				rectangle.setPosition(sf::Vector2f(x * 8 * 5 * 0.5 + 20, y * 8 * 5 * 1.2 + 20));
				main_window.draw(rectangle);

				//рисуем сам символ
				if (!blink || cursor_flipflop)

				{
					//font_t_x = 1;
					//font_t_y = 0;
					font_sprite_80_MDA.setTextureRect(sf::IntRect(sf::Vector2i(font_t_x * 40, font_t_y * 96), sf::Vector2i(40, 96)));
					font_sprite_80_MDA.setPosition(sf::Vector2f(x * 8 * 5 * 0.5 + 20, y * 8 * 5 * 1.2 + 20));
					font_sprite_80_MDA.setScale(sf::Vector2f(0.5, 0.5));
					font_sprite_80_MDA.setColor(fg_color);
					if (inversed) font_sprite_80_MDA.setColor(fg_color_inverse);
					if (bright) font_sprite_80_MDA.setColor(fg_color_bright);
					main_window.draw(font_sprite_80_MDA);
				}
				if (underline) //подчеркивание
				{
					sf::RectangleShape ul; //прямоугольник курсора
					ul.setSize(sf::Vector2f(45 * 0.4444, 2));
					ul.setPosition(sf::Vector2f(x * 8 * 5 * 0.5 + 20, (y * 14 * 5 * 0.6857 + 20 + 45)));
					ul.setFillColor(fg_color);
					main_window.draw(ul);
				}

				bool draw_cursor = false;
				//if ((width == 80) && !(y * 80 + x - registers[0xe] * 256 - registers[0xf])) draw_cursor = true;

				//рисуем курсор

				//if (draw_cursor && cursor_flipflop && ((registers[0xb] & 31) >= (registers[0xa] & 31)))
				{

					//font_sprite_80.setTextureRect(sf::IntRect(sf::Vector2i(31 * 8 * 5 * 0.5, 2 * 8 * 5), sf::Vector2i(8 * 5 * 0.5, 8 * 5)));
					//font_sprite_80.setPosition(sf::Vector2f(x * 8 * 5 * 0.5 + 20, y * 8 * 5 * 1.2 + 20));
					//font_sprite_80.setColor(sf::Color::White);
					//main_window.draw(font_sprite_80);
					sf::RectangleShape cursor_rectangle; //прямоугольник курсора
					cursor_rectangle.setScale(sf::Vector2f(0.4444, 0.6857));
					cursor_rectangle.setSize(sf::Vector2f(45, 4));
					cursor_rectangle.setPosition(sf::Vector2f(x * 20 + 20, y * 48 + 43 + 20));
					cursor_rectangle.setFillColor(fg_color);
					main_window.draw(cursor_rectangle);
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


		
		text.setString(debug_mess_1);
		text.setCharacterSize(20);
		text.setPosition(sf::Vector2f(0, 0));
		text.setFillColor(sf::Color(255, 0, 0));
		text.setOutlineThickness(3.1);
		if (debug_mess_1 != "") main_window.draw(text);

	exit:
		main_window.display();
		do_render = 0;
		main_window.setActive(0);
	}

	//графика 320х200 (только EGA)
	if (current_mode == video_modes::EGA_D_320)
	{
		//16 цветов вместо 4
		//2 страницы (до 8 при 256К)
		//адреса страниц отличаются на 4К (уточнить, должно быть 8К разница)

		sf::RectangleShape rectangle;
		rectangle.setScale(sf::Vector2f(1, 1));
		rectangle.setSize(sf::Vector2f(320 * 5, 200 * 1.2 * 5));
		rectangle.setPosition(sf::Vector2f(20, 20));
		rectangle.setFillColor(sf::Color::Black); //заливаем фон
		main_window.draw(rectangle);

		rectangle.setSize(sf::Vector2f(5, 6));
		for (int y = 0; y < 200; ++y)
		{
			for (int x = 0; x < 320; ++x)
			{
				uint32 dot_addr = (start_address * 2 + 40 * y + (x >> 3));
				uint8 col = ((videomemory[dot_addr]          >> (7 - (x % 8))) & 1) * 8 + 
					        ((videomemory[dot_addr + 0x4000] >> (7 - (x % 8))) & 1) * 4 + 
					        ((videomemory[dot_addr + 0x8000] >> (7 - (x % 8))) & 1) * 2 + 
					        ((videomemory[dot_addr + 0xC000] >> (7 - (x % 8))) & 1) * 1;
				
				rectangle.setPosition(sf::Vector2f(x * 5 + 20, (y * 6) + 20));
				rectangle.setFillColor(EGA_colors[col]);
				main_window.draw(rectangle);
				
			}
		}
	}

	//графика 640х200 (только EGA)
	if (current_mode == video_modes::EGA_E_200)
	{
		//16 цветов 
		//1 страница размером 64К (до 4 при 256К)
		//адреса страниц отличаются на 16К

		sf::RectangleShape rectangle;
		rectangle.setScale(sf::Vector2f(1, 1));
		rectangle.setSize(sf::Vector2f(320 * 5, 200 * 1.2 * 5));
		rectangle.setPosition(sf::Vector2f(20, 20));
		rectangle.setFillColor(sf::Color::Black); //заливаем фон
		main_window.draw(rectangle);

		rectangle.setSize(sf::Vector2f(5, 6));
		for (int y = 0; y < 200; ++y)
		{
			for (int x = 0; x < 640; ++x)
			{
				uint32 dot_addr = (start_address * 2 + 80 * y + (x >> 3));
				uint8 col = ((videomemory[dot_addr] >> (7 - (x % 8))) & 1) * 8 + ((videomemory[dot_addr + 0x4000] >> (7 - (x % 8))) & 1) * 4 + ((videomemory[dot_addr + 0x8000] >> (7 - (x % 8))) & 1) * 2 + ((videomemory[dot_addr + 0xC000] >> (7 - (x % 8))) & 1) * 1;

				rectangle.setPosition(sf::Vector2f(x * 2.5 + 20, (y * 6) + 20));
				rectangle.setFillColor(EGA_colors[col]);
				main_window.draw(rectangle);

			}
		}
	}
	

	//монохромная графика 640х350 (только EGA)
	if (current_mode == video_modes::EGA_F_350)
	{
		//2 цвета 
		//1 страница размером 64К (до 2 при большей памяти)
		//адреса страниц отличаются на 32К

		sf::RectangleShape dot;
		dot.setOutlineThickness(0);

		//масштаб точек
		float display_x_scale = 2.5; // (float)(GAME_WINDOW_X_RES - 40) / width;  //1600    5 или 2,5
		float display_y_scale = 6; // (float)(GAME_WINDOW_Y_RES - 40) / height; //1200    6
		dot.setScale(sf::Vector2f(display_x_scale, display_y_scale));

		//start_address  - начало буфера
		//добавить переход в начало памяти

		dot.setSize(sf::Vector2f(2.5, 6));
		for (int y = 0; y < 100; y++)
		{
			for (int x = 0; x < 80; ++x)
			{
				for (int sub_x = 0; sub_x < 8; sub_x++)
				{
					//четные
					uint32 dot_addr = (80 * y + x);
					dot.setPosition(sf::Vector2f(x * 4 * 5 + 20 + sub_x * 2.5, (y * 6) * 2 + 20));
					if (((videomemory[dot_addr] >> (7 - sub_x)) & 1) == 0) dot.setFillColor(sf::Color::Black);
					else dot.setFillColor(sf::Color::White);
					main_window.draw(dot);
					//нечетные
					dot_addr = (0x2000 + 80 * y + x);
					dot.setPosition(sf::Vector2f(x * 4 * 5 + 20 + sub_x * 2.5, (y * 6) * 2 + 6 + 20));
					if (((videomemory[dot_addr] >> (7 - sub_x)) & 1) == 0) dot.setFillColor(sf::Color::Black);
					else dot.setFillColor(sf::Color::White);
					main_window.draw(dot);
				}
			}
		}
	}

	//графика 640х350 (только EGA)
	if (current_mode == video_modes::EGA_10_350)
	{
		//4 цвета при 64К (до 16 при большей памяти)
		//1 страница
		
		sf::RectangleShape rectangle;
		rectangle.setScale(sf::Vector2f(1, 1));
		rectangle.setSize(sf::Vector2f(320 * 5, 200 * 1.2 * 5));
		rectangle.setPosition(sf::Vector2f(20, 20));
		rectangle.setFillColor(sf::Color::Black); //заливаем фон
		main_window.draw(rectangle);

		rectangle.setSize(sf::Vector2f(2.5, 6 / 350 * 200));
		for (int y = 0; y < 350; ++y)
		{
			for (int x = 0; x < 640; ++x)
			{
				uint32 dot_addr = (start_address * 2 + 80 * y + (x >> 3));
				uint8 col = ((videomemory[dot_addr] >> (7 - (x % 8))) & 1) * 8 + ((videomemory[dot_addr + 0x4000] >> (7 - (x % 8))) & 1) * 4 + ((videomemory[dot_addr + 0x8000] >> (7 - (x % 8))) & 1) * 2 + ((videomemory[dot_addr + 0xC000] >> (7 - (x % 8))) & 1) * 1;

				rectangle.setPosition(sf::Vector2f(x * 2.5 + 20, (y * 6) + 20));
				rectangle.setFillColor(EGA_colors[col]);
				main_window.draw(rectangle);

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

	text.setString(debug_mess_1);
	text.setPosition(sf::Vector2f(100, 1000));
	text.setFillColor(sf::Color(255, 0, 0));
	text.setOutlineThickness(3.1);
	if (debug_mess_1 != "") main_window.draw(text);

	main_window.display();
}
bool EGA_videocard::is_visible()
{
	return visible;
}
void EGA_videocard::scale_up()
{
	if (display_scale < 8) display_scale++;
	window_size_x = 320 * display_scale + 40;	// масштаб 1:5 + 500 px для отладки
	window_size_y = 200 * 1.2 * display_scale + 40;  // масштаб 1:5
	do_resize = 1;
}
void EGA_videocard::scale_down()
{
	if (display_scale > 1) display_scale--;
	window_size_x = 320 * display_scale + 40;	// масштаб 1:5 + 500 px для отладки
	window_size_y = 200 * 1.2 * display_scale + 40;  // масштаб 1:5
	do_resize = 1;
}
void EGA_videocard::mem_write(uint32 address, uint8 data)
{
	uint8 bitmask = gc_registers[8]; //маска пикселей
	uint8 mapmask = seq_registers[2] & 15; //маска слоев
	uint8 memory_mode = gc_registers[5] & 3; //режим записи
	uint8 rotate_cnt = gc_registers[3] & 7 ; //циклы ротации
	uint8 bit_function = (gc_registers[3] >> 3)  & 3; //побитовая функция

	//операции над плоскостями
	for (int plane = 0; plane < 4; plane++)
	{
		if (memory_mode == 0)
		{
			if ((mapmask >> plane) & 1)
			{
				//побитовые операции
				switch(bit_function)
				{
				case 0: //без изменений
					videomemory[address + plane * 0x4000] = data & bitmask;
					break;
				case 1: //AND latch
					videomemory[address + plane * 0x4000] = (data & bitmask) & latch_reg[plane];
					break;
				case 2: //OR latch
					videomemory[address + plane * 0x4000] = (data & bitmask) | latch_reg[plane];
					break;
				case 3: //XOR latch
					videomemory[address + plane * 0x4000] = (data & bitmask) ^ latch_reg[plane];
					break;
				}
			}
			else
			{
				videomemory[address + plane * 0x4000] = latch_reg[plane];
			}
		}

		if (memory_mode == 1)
		{
			if (mapmask & plane) videomemory[address + plane * 0x4000] = latch_reg[plane];
		}

		if (memory_mode == 2)
		{
			//пишем бит напрямую + маска
			videomemory[address + plane * 0x4000] = data & bitmask;
		}
	}

	//videomemory[address] = data;
}
uint8 EGA_videocard::mem_read(uint32 address)
{
	uint8 memory_mode = (gc_registers[5] >> 3)  & 1; //режим записи
	uint8 color_compare = gc_registers[2];		  	 //регистр сравнения

	uint8 data_out = 0;

	//запоминаем данные в регистрах
	latch_reg[0] = videomemory[address];
	latch_reg[1] = videomemory[address + 0x4000];
	latch_reg[2] = videomemory[address + 0x8000];
	latch_reg[3] = videomemory[address + 0xC000];
	
	uint8 map_select = gc_registers[4] & 3;

	//чтения
	if (memory_mode == 0)
	{
		//чтение через map_select
		address = address + map_select * 0x4000;
		return videomemory[address];
	}
	else
	{
		//сравнение цветов
		for (int bit = 0; bit < 7; bit++)
		{
			if ((((videomemory[address] >> bit) & 1) & ((color_compare >> 3) & 1)) & (((videomemory[address + 0x4000] >> bit) & 1) & ((color_compare >> 2) & 1)) & (((videomemory[address + 0x8000] >> bit) & 1) & ((color_compare >> 1) & 1)) & (((videomemory[address + 0xC000] >> bit) & 1) & ((color_compare >> 0) & 1)))
			{
				//цвет совпал
				data_out += pow(2, bit);
			}
		}
		return data_out;
	}
	
}
void EGA_videocard::flash_rom(uint32 address, uint8 data)
{
	rom[address] = data;
}
uint8 EGA_videocard::read_rom(uint32 address)
{
	return rom[address];
}
std::string EGA_videocard::get_debug_data(uint8 i)
{
	if (i == 0)
	{
		std::string out;
		out = "MOR: " + int_to_bin(MOR) + " BW/COL=" + to_string(IOAddrSel) + " En_RAM=" + to_string(EnRAM) + " PageBit=" + to_string(PageBit);
		if (On_board_switch_sel == 0) out += " clk=14MHz";
		if (On_board_switch_sel == 1) out += " clk=16MHz";
		if (On_board_switch_sel == 2) out += " clk=Ext";
		out += " IntDRV=" + to_string((MOR >> 4) & 1) + " 64K_page=" + to_string(PageBit) + " bit_6_7=" + to_string((MOR >> 6) & 3);
		return out;
	}

	if (i == 6)	return "video mem base: 0x" + int_to_hex(video_mem_base, 5);

	if (i == 1)	return "Reset: " + int_to_hex(seq_registers[0], 2) + "   " + int_to_bin(seq_registers[0]);
	if (i == 2)	return "Clk mode: " + int_to_hex(seq_registers[1], 2) + "   " + int_to_bin(seq_registers[1]);
	if (i == 3)	return "Map msk: " + int_to_hex(seq_registers[2], 2) + "   " + int_to_bin(seq_registers[2]);
	if (i == 4) return "Char gen sel: " + int_to_hex(seq_registers[3], 2) + "   " + int_to_bin(seq_registers[3]);
	if (i == 5)	return "Mem mode: " + int_to_hex(seq_registers[4], 2) + "   " + int_to_bin(seq_registers[4]);
	
	if (i == 7)	return "Horiz total: " + int_to_hex(crt_registers[0], 2) + "   " + int_to_bin(crt_registers[0]);
	if (i == 8)	return "Hrz Disp End: " + int_to_hex(crt_registers[1], 2) + "   " + int_to_bin(crt_registers[1]);
	if (i == 9)	return "Strt Hrz Blk: " + int_to_hex(crt_registers[2], 2) + "   " + int_to_bin(crt_registers[2]);
	if (i == 10)	return "End Hrz Blk: " + int_to_hex(crt_registers[3], 2) + "   " + int_to_bin(crt_registers[3]);
	if (i == 11)	return "Strt Hrz Retr: " + int_to_hex(crt_registers[4], 2) + "   " + int_to_bin(crt_registers[4]);
	if (i == 12)	return "End Hrz Retr: " + int_to_hex(crt_registers[5], 2) + "   " + int_to_bin(crt_registers[5]);
	if (i == 13)	return "Vert total: " + int_to_hex(crt_registers[6], 2) + "   " + int_to_bin(crt_registers[6]);
	if (i == 14)	return "Overflow: " + int_to_hex(crt_registers[7], 2) + "   " + int_to_bin(crt_registers[7]);
	if (i == 15)	return "Max Scan Line: " + int_to_hex(crt_registers[9], 2) + "   " + int_to_bin(crt_registers[9]);
	if (i == 16)	return "Cursor start: " + int_to_hex(crt_registers[10], 2) + "   " + int_to_bin(crt_registers[10]);
	if (i == 17)	return "Cursor End: " + int_to_hex(crt_registers[11], 2) + "   " + int_to_bin(crt_registers[11]);
	if (i == 18)	return "Vrt retr Strt: " + int_to_hex(crt_registers[16], 2) + "   " + int_to_bin(crt_registers[16]);
	if (i == 19)	return "Vert retr End: " + int_to_hex(crt_registers[17], 2) + "   " + int_to_bin(crt_registers[17]);
	if (i == 20)	return "Vrt Disp End: " + int_to_hex(crt_registers[18], 2) + "   " + int_to_bin(crt_registers[18]);
	if (i == 21)	return "Offset: " + int_to_hex(crt_registers[19], 2) + "   " + int_to_bin(crt_registers[19]);
	if (i == 22)	return "Underline loc: " + int_to_hex(crt_registers[20], 2) + "   " + int_to_bin(crt_registers[20]);
	if (i == 23)	return "Strt vert Blk: " + int_to_hex(crt_registers[21], 2) + "   " + int_to_bin(crt_registers[21]);
	if (i == 24)	return "End vert Blk: " + int_to_hex(crt_registers[22], 2) + "   " + int_to_bin(crt_registers[22]);
	if (i == 25)	return "Mode control: " + int_to_hex(crt_registers[23], 2) + "   " + int_to_bin(crt_registers[23]);
	if (i == 26)	return "Line compare: " + int_to_hex(crt_registers[24], 2) + "   " + int_to_bin(crt_registers[24]);

	if (i == 27)	return "Mode register: " + int_to_hex(gc_registers[5], 2) + "   " + int_to_bin(gc_registers[5]);
	if (i == 28)	return "Miscelaneous: " + int_to_hex(gc_registers[6], 2) + "   " + int_to_bin(gc_registers[6]);
	if (i == 29)	return "Color no care: " + int_to_hex(gc_registers[7], 2) + "   " + int_to_bin(gc_registers[7]);
	if (i == 30)	return "Bit mask: " + int_to_hex(gc_registers[8], 2) + "   " + int_to_bin(gc_registers[8]);
	
	if (i >= 31 && i < 47)	return "Palette " + int_to_hex(i-31,2) +  ": " + int_to_hex(ac_registers[i-31], 2) + "   " + int_to_bin(ac_registers[i-31]);

	if (i == 47)	return "Mode control: " + int_to_hex(ac_registers[16], 2) + "   " + int_to_bin(ac_registers[16]);
	if (i == 48)	return "Overscan: " + int_to_hex(ac_registers[17], 2) + "   " + int_to_bin(ac_registers[17]);
	if (i == 49)	return "Color plane: " + int_to_hex(ac_registers[18], 2) + "   " + int_to_bin(ac_registers[18]);

	if (i == 50)	return "Set Reset: " + int_to_hex(gc_registers[0], 2) + "   " + int_to_bin(gc_registers[0]);
	if (i == 51)	return "Enable S/R: " + int_to_hex(gc_registers[1], 2) + "   " + int_to_bin(gc_registers[1]);
	if (i == 52)	return "Color comp: " + int_to_hex(gc_registers[2], 2) + "   " + int_to_bin(gc_registers[2]);
	if (i == 53)	return "Data rotate: " + int_to_hex(gc_registers[3], 2) + "   " + int_to_bin(gc_registers[3]);
	if (i == 54)	return "Read map sel: " + int_to_hex(gc_registers[4], 2) + "   " + int_to_bin(gc_registers[4]);




	/*
	uint8 seq_registers[32] = { 0 }; // массив регистров
	uint8 crt_registers[32] = { 0 }; // CRT Controller Registers
	uint8 gc_registers[32] = { 0 };  // Graphics Controller Registers
	uint8 ac_registers[32] = { 0 };  // Attribute Controller Registers
	*/

	return " no data";
}
sf::Vector2i EGA_videocard::get_mouse_pos()
{
	return sf::Vector2i({ 0,0 });
}

//====================== Monitor =================

void Monitor::set_card_type(videocard_type new_mode)
{
	card_type = new_mode;

	//показываем или скрываем экраны
	switch (card_type)
	{
	case videocard_type::CGA:
		if (MDA_card.is_visible()) MDA_card.hide();
		if (EGA_card.is_visible()) EGA_card.hide();
		if (!CGA_card.is_visible()) CGA_card.show();
		CGA_card.font = font;
		break;
	case videocard_type::MDA:
		if (CGA_card.is_visible()) CGA_card.hide();
		if (EGA_card.is_visible()) EGA_card.hide();
		if (!MDA_card.is_visible()) MDA_card.show();
		MDA_card.font = font;
		break;
	case videocard_type::EGA:
		if (MDA_card.is_visible()) MDA_card.hide();
		if (CGA_card.is_visible()) CGA_card.hide();
		if (!EGA_card.is_visible()) EGA_card.show();
		EGA_card.font = font;
		break;
	}
}
videocard_type Monitor::get_card_type()
{
	return card_type;
}

void Monitor::update(int elapsed_ms)
{
	switch (card_type)
	{
	case videocard_type::CGA:
		CGA_card.update(elapsed_ms);
		break;
	case videocard_type::MDA:
		MDA_card.update(elapsed_ms);
		break;
	case videocard_type::EGA:
		EGA_card.update(elapsed_ms);
		break;
	}
}
void Monitor::write_port(uint16 port, uint8 data)
{
	switch (card_type)
	{
	case videocard_type::CGA:
		CGA_card.write_port(port, data);
		break;
	case videocard_type::MDA:
		MDA_card.write_port(port, data);
		break;
	case videocard_type::EGA:
		EGA_card.write_port(port, data);
		break;
	}
}
uint8 Monitor::read_port(uint16 port)
{
	switch (card_type)
	{
	case videocard_type::CGA:
		return CGA_card.read_port(port);
	case videocard_type::MDA:
		return MDA_card.read_port(port);
	case videocard_type::EGA:
		return EGA_card.read_port(port);
	}
	return 255;
}
std::string Monitor::get_mode_name()				//получить название режима для отладки
{
	switch (card_type)
	{
	case videocard_type::CGA:
		return CGA_card.get_mode_name();
	case videocard_type::MDA:
		return MDA_card.get_mode_name();
	case videocard_type::EGA:
		return EGA_card.get_mode_name();
	}
	return "no data";
}
void Monitor::show_joy_sence(uint8 central_point)	//отобразить информацию джойстика
{
	switch (card_type)
	{
	case videocard_type::CGA:
		CGA_card.show_joy_sence(central_point);
		return;
	case videocard_type::MDA:
		MDA_card.show_joy_sence(central_point);
		return;
	case videocard_type::EGA:
		EGA_card.show_joy_sence(central_point);
		return;
	}
}
bool Monitor::has_focus()							//проверить наличие фокуса
{
	switch (card_type)
	{
	case videocard_type::CGA:
		return CGA_card.has_focus();
	case videocard_type::MDA:
		return MDA_card.has_focus();
	case videocard_type::EGA:
		return EGA_card.has_focus();
	}
	return 0;
}
void Monitor::scale_up()
{
	switch (card_type)
	{
	case videocard_type::CGA:
		return CGA_card.scale_up();
	case videocard_type::MDA:
		return MDA_card.scale_up();
	case videocard_type::EGA:
		return EGA_card.scale_up();
	}
}
void Monitor::scale_down()
{
	switch (card_type)
	{
	case videocard_type::CGA:
		return CGA_card.scale_down();
	case videocard_type::MDA:
		return MDA_card.scale_down();
	case videocard_type::EGA:
		return EGA_card.scale_down();
	}
}
void Monitor::mem_write(uint32 address, uint8 data)
{
	//записываем в память данные в зависимости от адреса и текущей видеокарты
	switch (card_type)
	{
	case videocard_type::CGA:
		if (address >= 0xB8000 && address < 0xBC000)
		{
			CGA_card.mem_write(address - 0xB8000, data);
		}
		return;
	case videocard_type::MDA:
		if (address >= 0xB0000 && address < 0xB1000)
		{
			MDA_card.mem_write(address - 0xB0000, data);
		}
		return;
	case videocard_type::EGA:
		if (address >= 0xA0000 && address < 0xA4000)
		{
			EGA_card.mem_write(address - 0xA0000, data);
			return;
		}
		if (address >= 0xB0000 && address < 0xB4000)
		{
			EGA_card.mem_write(address - 0xB0000, data);
			return;
		}
		if (address >= 0xB8000 && address < 0xBC000)
		{
			EGA_card.mem_write(address - 0xB8000, data);
			return;
		}
	}
}
uint8 Monitor::mem_read(uint32 address)
{
	//считываем данные из видеопамяти в зависимости от адреса и текущей видеокарты
	switch (card_type)
	{
	case videocard_type::CGA:
		if (address >= 0xB8000 && address < 0xBC000) return CGA_card.mem_read(address - 0xB8000);
		break;
	case videocard_type::MDA:
		if (address >= 0xB0000 && address < 0xB1000) return	MDA_card.mem_read(address - 0xB0000);
		break;
	case videocard_type::EGA:
		if (address >= 0xA0000 && address < 0xA4000)
		{
			return EGA_card.mem_read(address - 0xA0000);
		}
		if (address >= 0xB0000 && address < 0xB4000)
		{
			return EGA_card.mem_read(address - 0xB0000);
		}
		if (address >= 0xB8000 && address < 0xBC000)
		{
			return EGA_card.mem_read(address - 0xB8000);
		}
		break;
	}
	return 0xFF;
}
void Monitor::flash_rom(uint32 address, uint8 data)
{
	//записываем данные флеш ПЗУ 
	switch (card_type)
	{
	case videocard_type::CGA:
		//тут нет ПЗУ
		return;
	case videocard_type::MDA:
		//тут тоже нет ПЗУ
		return;
	case videocard_type::EGA:
		if (address >= 0xC0000 && address < 0xC4000)
		{
			EGA_card.flash_rom(address - 0xC0000, data);
		}
		return;
	}
}
uint8 Monitor::read_rom(uint32 address)
{
	//читаем данные из флеш ПЗУ 
	switch (card_type)
	{
	case videocard_type::CGA:
		//тут нет ПЗУ
		break;
	case videocard_type::MDA:
		//тут тоже нет ПЗУ
		break;
	case videocard_type::EGA:
		if (address >= 0xC0000 && address < 0xC4000)
		{
			return EGA_card.read_rom(address - 0xC0000);
		}
		break;
	}
	return 0xFF;
}
std::string Monitor::get_debug_data(uint8 i)
{
	return EGA_card.get_debug_data(i);
}
sf::Vector2i Monitor::get_mouse_pos()
{
	//читаем данные из флеш ПЗУ 
	switch (card_type)
	{
	case videocard_type::CGA:
		return CGA_card.get_mouse_pos();
		break;
	case videocard_type::MDA:
		return MDA_card.get_mouse_pos();
		break;
	case videocard_type::EGA:
		return EGA_card.get_mouse_pos();
		break;
	}
	return sf::Vector2i({ 0,0 });

}
Monitor::Monitor()
{
	//конструктор
}