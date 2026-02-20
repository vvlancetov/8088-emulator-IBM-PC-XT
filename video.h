#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/System.hpp>
#include <string>
#include <thread>
#include "custom_classes.h"

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

//видеокарты
enum class videocard_type { MDA, CGA, EGA, VGA };

//CGA videocard
class CGA_videocard
{
private:
	sf::RenderWindow main_window;
	bool visible = 0;		//наличие окна
	uint16 my_display_H;
	uint16 my_display_W;
	uint16 GAME_WINDOW_X_RES;
	uint16 GAME_WINDOW_Y_RES;

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
	bool render_complete = true; //флаг окончания рендеринга
	int elapsed_ms = 0;				//период времени
	std::thread t;					//указатель на поток
	bool do_render = true;			//запрос отрисовки

	//масштаб
	float display_scale = 5;
	bool do_resize = 0;

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
	CGA_videocard();							//конструктор класса
	void update(int new_elapsed_ms);				//синхронизация
	void main_loop();
	void render();								//рисуем содержимое
	void write_port(uint16 port, uint8 data);	//запись в порт адаптера
	uint8 read_port(uint16 port);				//чтение из порта адаптера
	void set_CGA_mode(uint8 mode);				//установка конкретного видеорежима
	void set_cursor_type(uint16 type);			//установка типа курсора
	void set_cursor_position(uint8 X, uint8 Y, uint8 Page);		//установка позиции курсора
	void read_cursor_position();				//чтение позиции курсора
	std::string get_mode_name();
	void show_joy_sence(uint8 central_point);
	bool has_focus();
	std::string debug_mess_1;
	void show();								//создать окно
	void hide();								//убрать окно
	bool is_visible();							//проверка наличия окна
	void scale_up();
	void scale_down();
};

//монитор отладки
class Dev_mon_device
{
protected:
	sf::RenderWindow main_window;
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
	bool do_render = true;			//запрос отрисовки
	uint32 cpu_speed[64] = { 0 };	//массив для усреднения скорости
	uint8 cpu_speed_ptr = 0;		//указатель массива
	std::thread t;					//указатель на поток
	int elapsed_ms = 0;				//период времени
	int ops = 0;					//количество команд для расчета скорости

public:
	Dev_mon_device(uint16 w, uint16 h, std::string title, uint16 x_pos, uint16 y_pos); //конструктор
	sf::Font font;
	void main_loop();
	void show();
	void hide();
	void render();
	void update(int new_elapsed_ms, int op_counter);
};

//монитор FDD
class FDD_mon_device : public Dev_mon_device
{
	using Dev_mon_device::Dev_mon_device;

private:
	std::vector<std::string> log_strings;
	std::string last_str = "";

public:
	
	void log(std::string log_string);
	void main_loop();
	void show();
	void hide();
	void render();
	void update(int new_elapsed_ms);
};

//монитор HDD
class HDD_mon_device : public Dev_mon_device
{
private:
	std::vector<std::string> log_strings;
	std::string last_str = "";

public:
	using Dev_mon_device::Dev_mon_device;
	void log(std::string log_string);
	void main_loop();
	void show();
	void hide();
	void render();
	void update(int new_elapsed_ms);
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
	void main_loop();
	void show();
	void hide();
	void render();
	void update(int new_elapsed_ms);
};

//MDA videocard
class MDA_videocard
{
private:
	sf::RenderWindow main_window;
	bool visible = 0;		//наличие окна
	uint16 my_display_H;
	uint16 my_display_W;
	uint16 GAME_WINDOW_X_RES;
	uint16 GAME_WINDOW_Y_RES;
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
	std::thread t;					//указатель на поток

	//константы
	sf::Color fg_color = sf::Color::Green;
	sf::Color fg_color_bright = sf::Color::Yellow;
	sf::Color fg_color_inverse = sf::Color::Black;
	sf::Color bg_color = sf::Color::Black;
	sf::Color bg_color_inverse = sf::Color::Green;
	
	bool do_render = 0;
	int elapsed_ms = 0;				//период времени
	//масштаб
	float display_scale = 5;
	bool do_resize = 0;

public:
	
	MDA_videocard();
	sf::Font font;
	std::string debug_mess_1;
	void write_port(uint16 port, uint8 data);	//запись в порт адаптера
	uint8 read_port(uint16 port);				//чтение из порта адаптера
	std::string get_mode_name();				//получить название режима для отладки
	void show_joy_sence(uint8 central_point);	//отобразить информацию джойстика
	bool has_focus();							//проверить наличие фокуса
	void main_loop();							//синхронизация
	void render();
	void show();								//создать окно
	void hide();								//убрать окно
	void update(int new_elapsed_ms);				//синхронизация
	bool is_visible();							//проверка наличия окна
	void scale_up();
	void scale_down();
};

//EGA videocard
class EGA_videocard
{
private:
	sf::RenderWindow main_window;
	bool visible = 0;		//наличие окна
	int my_display_H;
	int my_display_W;
	uint16 GAME_WINDOW_X_RES;
	uint16 GAME_WINDOW_Y_RES;
	int joy_sence_show_timer = 0; //таймер отображения настроек джойстика
	uint8 joy_sence_value = 0; //центральная точка джойстика
	sf::Clock cursor_clock;				//таймер мигания курсора
	bool cursor_flipflop = false;		//переменная для мигания

	std::string window_title;		//заголовок окна
	int window_pos_x;				//позиция окна
	int window_pos_y;				//позиция окна
	int window_size_x;				//размер окна
	int window_size_y;				//размер окна

	//регистры
	bool IOAddrSel = 0;
	bool EnRAM = 0;
	bool PageBit = 0;
	bool DisplayEN = 0;
	bool Lines_350 = 0;				//кол-во горизонтальных линий
	bool ac_flipflop = 0;			//Attribute	Controller flip_flop 0 - address, 1 - value
	uint8 On_board_switch_sel = 0;	//выбор считываемого переключателя на плате
	bool use_2_char_gen = 0;		//использовать две таблицы знакогенератора

	uint8 CRT_sel_reg = 0;			 //селектор для CRT Controller Registers
	uint8 SEQ_sel_reg = 0;			 //выбранный регистр для записи
	uint8 GC_sel_reg = 0;			 //селектор для Graphics Controller Registers
	uint8 GP1_reg = 0;				 //Graphics 1 Position Register
	uint8 GP2_reg = 0;				 //Graphics 2 Position Register
	uint8 AC_sel_reg = 0;			 //селектор для Attribute Controller Registers

	uint8 seq_registers[32] = { 0 }; // массив регистров
	uint8 crt_registers[32] = { 0 }; // CRT Controller Registers
	uint8 gc_registers[32] = { 0 };  // Graphics Controller Registers
	uint8 ac_registers[32] = { 0 };  // Attribute Controller Registers

	uint32 video_mem_base = 0;
		
	//палитра
	sf::Color EGA_colors[16]; //массив цветов EGA для текста
	int elapsed_ms = 0;				//период времени
	std::thread t;					//указатель на поток
	bool do_render = 0;

	//отладка DELETE
	uint8 CGA_Mode_Select_Register = 0;
	uint8 CGA_Color_Select_Register = 0;
	uint8 EGA_Mode_Select_Register = 0;

	//масштаб
	float display_scale = 5;
	bool do_resize = 0;

public:

	EGA_videocard();
	sf::Font font;
	std::string debug_mess_1;
	void write_port(uint16 port, uint8 data);	//запись в порт адаптера
	uint8 read_port(uint16 port);				//чтение из порта адаптера
	std::string get_mode_name();				//получить название режима для отладки
	void show_joy_sence(uint8 central_point);	//отобразить информацию джойстика
	void main_loop();
	bool has_focus();							//проверить наличие фокуса
	void update(int new_elapsed_ms);				//синхронизация
	void render();								//рисуем содержимое
	void show();								//создать окно
	void hide();								//убрать окно
	bool is_visible();							//проверка наличия окна
	void scale_up();
	void scale_down();
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
	void update(int new_elapsed_ms);				//синхронизация
	void scale_up();
	void scale_down();
};
