#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/System.hpp>
#include <string>
#include "custom_classes.h"

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

extern uint8 memory_2[1024 * 1024 + 1024 * 1024]; //память 2.0

//видеокарты
enum class videocard_type { MDA, CGA, EGA, VGA };

//CGA videocard
class CGA_videocard
{
private:
	sf::RenderWindow main_window;
	bool visible = 0;		//наличие окна
	int my_display_H;
	int my_display_W;
	__int16 GAME_WINDOW_X_RES;
	__int16 GAME_WINDOW_Y_RES;

	int counter = 0;  //?
	sf::Clock cursor_clock;				//таймер мигания курсора
	bool cursor_flipflop = false;		//переменная для мигания
	int speed_history[33] = { 100000 };
	unsigned __int8 command_reg = 0;	//регистр команд
	unsigned __int8 count_param = 0;	//количество параметров для обработки
	unsigned __int8 status = 0;         //регистр статуса
										// 0 - FO, FIFO Overrun - переполнение буфера FIFO
										// 1 - DU, DMA Underrun - потеря данных во время процесса ПДП
										// 2 - VE, Video Enable - видеооперации с экраном разрешены
										// 3 - IC, Improper Command - ошибочное количество параметров
										// 4 - LP - если на входе светового пера присутствует активный уровень и загружен регистр светового пера
										// 5 - IR, Interrupt Request - устанавливается в начале последней строки на экране если установлен флаг разрешения прерывания
										// 6 - IE, Interrupt Enable - устанавливается/сбрасывается после командами
										// 7 - всегда 0
	bool video_enable = true;			//разрешение работы (отключить по умолчанию)
	bool int_enable = false;			//разрешение прерываний
	bool int_request = false;			//устанавливается каждый раз после отображения экрана
	bool improper_command = false;		//ошибка в параметрах
	uint8 cursor_x = 0;		//позиция курсора
	uint8 cursor_y = 0;
	uint8 display_lines = 30;  //кол-во строк на экране
	uint8 display_columns = 78;//кол-во столбцов на экране
	uint8 under_line_pos = 10;	 //позиция линии подчеркивания (по высоте)
	uint8 cursor_format = 1;	 //формат курсора: 0 - мигающий блок, 1 - мигающий штрих, 2 - инверсный блок, 3 - немигающий штрих
	bool transp_attr = true;			 //невидимый атрибут поля (при установке специальных атрибутов) 0 - невидимый, 1 - обычный (с разрывами)
	// ============= новые команды

	uint8 sel_reg = 0; //выбранный регистр для записи
	uint8 registers[18] = { 0 }; // массив регистров
	uint8 display_mode = 0;      //режим работы дисплея
	uint8 CGA_Color_Select_Register = 0; //режим выбора цвета
	uint8 CGA_Mode_Select_Register = 9;  //регистр режимов CGA
	sf::Color CGA_colors[16]; //массив цветов CGA для текста
	sf::Color CGA_BW_colors[16]; //массив цветов CGA для текста в режиме BW
	int joy_sence_show_timer = 0; //таймер отображения настроек джойстика
	uint8 joy_sence_value = 0; //центральная точка джойстика

	std::string window_title;		//заголовок окна
	int window_pos_x;				//позиция окна
	int window_pos_y;				//позиция окна
	int window_size_x;				//размер окна
	int window_size_y;				//размер окна

public:
	sf::Font font;
	//void select_register(uint8 data);	//команда контроллеру
	//void set_reg_data(uint8 data);		//параметры команды
	//void write_vram(uint16 address, uint8 data); //запись в видеопамять
	//uint8 read_vram(uint16 address);	//чтение из видеопамяти
	//uint8 read_vrom(uint16 address);	//чтение из ПЗУ
	//void load_v_rom(uint16 address, uint8 data);	 //видео БИОС
	//void write_vram_MDA(uint16 address, uint8 data); //запись в видеопамять
	//uint8 read_vram_MDA(uint16 address);			 //чтение из видеопамяти

	uint8 line_height = 10;						//высота строки в пикселях
	CGA_videocard();								// конструктор класса
	void sync(int elapsed_ms);					//импульс синхронизации
	void write_port(uint16 port, uint8 data);	//запись в порт адаптера
	uint8 read_port(uint16 port);				//чтение из порта адаптера
	void set_CGA_mode(uint8 mode);				//установка конкретного видеорежима
	void set_cursor_type(uint16 type);			//установка типа курсора
	void set_cursor_position(uint8 X, uint8 Y, uint8 Page);		//установка позиции курсора
	void read_cursor_position();				//чтение позиции курсора
	void teletype(uint8 symbol);
	std::string get_mode_name();
	void show_joy_sence(uint8 central_point);
	bool has_focus();
	std::string debug_mess_1;
	void show();								//создать окно
	void hide();								//убрать окно
	bool is_visible();							//проверка наличия окна
};

//монитор отладки
class Dev_mon_device
{
protected:
	sf::RenderWindow main_window;
	//int display_mode; //режим работы адаптера
	int my_display_H;
	int my_display_W;
	uint16 GAME_WINDOW_X_RES;
	uint16 GAME_WINDOW_Y_RES;
	bool visible = 0; //признак видимости
	std::string window_title;
	int window_pos_x;
	int window_pos_y;
	int window_size_x;
	int window_size_y;

public:
	Dev_mon_device(uint16 w, uint16 h, std::string title, uint16 x_pos, uint16 y_pos); //конструктор
	sf::Font font;
	void sync(int elapsed_ms);
	void show();
	void hide();
};

//монитор FDD
class FDD_mon_device : public Dev_mon_device
{
private:
	std::vector<std::string> log_strings;
	std::string last_str = "";

public:
	using Dev_mon_device::Dev_mon_device;
	void sync();
	void log(std::string log_string);
};

//монитор HDD
class HDD_mon_device : public Dev_mon_device
{
private:
	std::vector<std::string> log_strings;
	std::string last_str = "";

public:
	using Dev_mon_device::Dev_mon_device;
	void sync();
	void log(std::string log_string);
};

//монитор памяти
class Mem_mon_device : public Dev_mon_device
{
private:
	uint16 segment = 0;
	uint16 offset = 0x400;
	uint8 cursor = 0;

public:
	using Dev_mon_device::Dev_mon_device;
	void sync();
};

//MDA videocard
class MDA_videocard
{
private:
	sf::RenderWindow main_window;
	bool visible = 0;		//наличие окна
	int my_display_H;
	int my_display_W;
	__int16 GAME_WINDOW_X_RES;
	__int16 GAME_WINDOW_Y_RES;
	int joy_sence_show_timer = 0; //таймер отображения настроек джойстика
	uint8 joy_sence_value = 0; //центральная точка джойстика
	sf::Clock cursor_clock;				//таймер мигания курсора
	bool cursor_flipflop = false;		//переменная для мигания
	uint8 sel_reg = 0; //выбранный регистр для записи
	uint8 registers[18] = { 0 }; // массив регистров
	uint8 MDA_Mode_Select_Register = 0;
	std::string window_title;		//заголовок окна
	int window_pos_x;				//позиция окна
	int window_pos_y;				//позиция окна
	int window_size_x;				//размер окна
	int window_size_y;				//размер окна

public:
	
	MDA_videocard();
	sf::Font font;
	std::string debug_mess_1;
	void write_port(uint16 port, uint8 data);	//запись в порт адаптера
	uint8 read_port(uint16 port);				//чтение из порта адаптера
	std::string get_mode_name();				//получить название режима для отладки
	void show_joy_sence(uint8 central_point);	//отобразить информацию джойстика
	bool has_focus();							//проверить наличие фокуса
	void sync(int elapsed_ms);					//синхронизация
	void show();								//создать окно
	void hide();								//убрать окно
	bool is_visible();							//проверка наличия окна
};

//EGA videocard
class EGA_videocard
{
private:
	sf::RenderWindow main_window;
	bool visible = 0;		//наличие окна
	int my_display_H;
	int my_display_W;
	__int16 GAME_WINDOW_X_RES;
	__int16 GAME_WINDOW_Y_RES;
	int joy_sence_show_timer = 0; //таймер отображения настроек джойстика
	uint8 joy_sence_value = 0; //центральная точка джойстика
	sf::Clock cursor_clock;				//таймер мигания курсора
	bool cursor_flipflop = false;		//переменная для мигания
	uint8 sel_reg = 0; //выбранный регистр для записи
	uint8 registers[18] = { 0 }; // массив регистров
	uint8 EGA_Mode_Select_Register = 0;
	std::string window_title;		//заголовок окна
	int window_pos_x;				//позиция окна
	int window_pos_y;				//позиция окна
	int window_size_x;				//размер окна
	int window_size_y;				//размер окна

	//отладка
	uint8 CGA_Mode_Select_Register = 0;
	uint8 CGA_Color_Select_Register = 0;

public:

	EGA_videocard();
	sf::Font font;
	std::string debug_mess_1;
	void write_port(uint16 port, uint8 data);	//запись в порт адаптера
	uint8 read_port(uint16 port);				//чтение из порта адаптера
	std::string get_mode_name();				//получить название режима для отладки
	void show_joy_sence(uint8 central_point);	//отобразить информацию джойстика
	bool has_focus();							//проверить наличие фокуса
	void sync(int elapsed_ms);					//синхронизация
	void show();								//создать окно
	void hide();								//убрать окно
	bool is_visible();							//проверка наличия окна
};


//=================== СТАВИТЬ после определения видеокарт

//монитор, включающий в себя видеокарты
class Monitor
{
private:
	videocard_type card_type = videocard_type::CGA;  //тип эмулируемой карты
	CGA_videocard CGA_card;	   //CGA карта
	MDA_videocard MDA_card;    //MDA карта
	EGA_videocard EGA_card;    //EGA карта

public:
	//конструктор
	Monitor();
	
	//Шрифт для отладочных сообщений
	sf::Font font;
	
	//установка режима видео
	void set_card_type(videocard_type new_mode);

	//функции для передачи данных далее
	void write_port(uint16 port, uint8 data);	//запись в порт адаптера
	uint8 read_port(uint16 port);				//чтение из порта адаптера
	std::string get_mode_name();				//получить название режима для отладки
	void show_joy_sence(uint8 central_point);	//отобразить информацию джойстика
	bool has_focus();							//проверить наличие фокуса
	void sync(int elapsed_ms);					//синхронизация
};
