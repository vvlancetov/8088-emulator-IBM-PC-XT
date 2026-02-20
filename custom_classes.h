#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/System.hpp>
#include <vector>
#include <bitset>
#include <chrono>
#include <string>
#include <sstream>
#include "json.hpp"

#define DEBUG

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

template< typename T >
std::string int_to_hex(T i, int w)
{
	std::stringstream stream;
	stream << ""
		<< std::setfill('0') << std::setw(w)
		<< std::hex << (int)i;
	return stream.str();
}

template< typename T >
std::string int_to_bin(T i)
{
	std::stringstream stream;
	stream << ""
		<< (std::bitset<8>)i;
	return stream.str();
}


class Mem_Ctrl // контроллер памяти
{
private:

public:
	Mem_Ctrl() {};
	void write(uint32 address, uint8 data); //запись значений в ячейки
	uint8 read(uint32 address); //чтение данных из памяти
};

class IO_Ctrl  //контроллер портов
{
private:

public:
	void output_to_port_8(uint16 address, uint8 data);		//вывод в 8-бит порт
	void output_to_port_16(uint16 address, uint16 data);	//вывод в 16-бит порт
	
	uint8 input_from_port_8(uint16 address);				//ввод из 8-бит порта
	uint16 input_from_port_16(uint16 address);				//ввод из 16-бит порта
};

//контроллер прерываний
class IC8259
{
private:
	
	uint8 next_ICW = 1;  //ожидаемое слово инициализации
	bool wait_ICW4 = false;
	uint16 INT_vector_addr_86 = 0; //адрес таблицы векторов режим 8086/88
	uint16 INT_vector_addr_80 = 0; //адрес таблицы векторов режим 8080/85
	uint8 next_reg_to_read = 1; // 1 - IR, 2 - IS, 3 - IM
	uint8 last_INT = 255; //текущее прерывание в работе
	

public:
	bool cascade_mode = false;
	bool nested_mode = true;
	bool ADDR_INTERVAL_4 = true;
	bool AUTO_EOI = false;
	bool mode_8086 = true;
	bool enabled = false;
	uint8 sleep_timer = 0; //таймер задержки

	uint8 IR_REG = 0; //Interrupt Request Register
	uint8 IS_REG = 0; //In-Service Register
	uint8 IM_REG = 0; //Interrupt Mask Register

	void write_port(uint16 port, uint8 data);
	uint8 read_port(uint16 port);
	bool NMI_enabled = false;
	uint8 request_IRQ(uint8 irq); //запрос прерывания от устройства
	uint8 get_last_int(); //номер последнего прерывания
	uint16 get_last_int_addr(); //адрес вектора прерывания
	uint8 next_int();  //прерывание для исполнения
	void set_timeout(uint8); //установить таймер задержки
	std::string get_ch_data(int ch); // данные канала
};

//параметры канала DMA
struct dma_cha_param
{
	uint16 curr_address = 0;
	uint16 base_address = 0;
	uint16 word_count = 0;
	uint16 base_word_count = 0;
	uint8 page = 0;  //DMA Page
	bool flip_flop = false;
	//uint8 tmp_reg = 0;
	bool request_bit = false;
	bool masked = false;
	uint8 mode_select = 0;			// 0 - demand mode, 1 - single mode, 2 - block mode, 3 - cascade mode
	uint8 transfer_select = 0;      // 0 - verify, 1 - Write, 2 - read
	bool autoinitialization = false;
	bool adress_decrement = false;
	bool pending = false;
	bool TC_reached = false; //достижение лимита передачи (если нет автопродления)
};

//контроллер DMA
class IC8237
{
private:
	dma_cha_param cha_attribute[4];
	bool M_to_M_enable = false;
	bool Ch0_adr_hold = false;
	bool compressed_timings = false;
	bool rotating_priority = false;
	bool extended_write = false;
	bool DREQ_sense = false;
	bool DACK_sense = false;
	//uint8 mask_REG = 0;
	uint8 status_REG = 0;
	uint8 temp_REG = 0;


public:
	bool Ctrl_disabled = false;

	std::string get_ch_data(int ch_num);
	std::string get_ch_data_2(int ch_num);
	std::string get_ch_data_3(int ch_num);
	void write_port(uint16 port, uint8 data);
	uint8 read_port(uint16 port);
	void sync();		//синхронизация
	void ram_refresh(); //обновление памяти
	uint8 request_hw_dma(int channel); //запрос DMA от устройства
};


//внутренние счетчики таймера
struct t_counter
{
	uint16 count = 0;
	uint16 initial_count = 0;
	bool latch_on = false;
	uint16 latch_value = 0;
	uint8 mode = 0; // 0 - int on TC, 1 - one-shot, 2 - Rate Gen, 3 - Square wave, 4 - Soft Trigg, 5 - HW trigg
	bool BCD_mode = false;
	uint8 RL_mode = 0;
	bool second_byte = false; //нужно считать/записать второй байт
	bool enabled = false;     // ON/OFF
	bool wait_for_data = false; //ожидание загрузки данных
	bool signal_high = false;  //сигнал на выходе
	bool one_shot_fired = false; //триггер для режима 0
};

//таймер
class IC8253
{
private:
	t_counter counters[4];
	std::chrono::steady_clock::time_point timer_start; //для отслеживания времени выполнения
	std::chrono::steady_clock::time_point timer_end;
	uint32 duration = 0;
public:
	int cycle_duration = 0;
	void write_port(uint16 port, uint8 data);
	uint8 read_port(uint16 port);
	void sync();
	void enable_timer(uint8 n);
	void disable_timer(uint8 n);
	bool is_count_enabled(uint8 n);
	uint16 get_time(uint8 number);
	std::string get_ch_data(int channel);
};

//контроллер PPI
class IC8255
{
private:
	bool switches_hign = false;
	uint8 port_B_out = 0;
	bool port_B_6 = false; //уровень вывода 6 порта B
	uint8 scan_code_latch = 0;

public:
	void write_port(uint16 port, uint8 data);
	uint8 read_port(uint16 port);
	void set_scancode(uint8 code)
	{
		scan_code_latch = code;
	};
};