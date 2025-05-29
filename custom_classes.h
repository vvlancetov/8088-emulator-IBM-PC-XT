#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/System.hpp>
#include <vector>
#include "json.hpp"

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

class Mem_Ctrl // контроллер памяти
{
private:
	//uint8 ram_array[640 * 1024] = { 0 };  //RAM
	//uint8 rom_array[64 * 1024] = { 0 };   //ROM
	//uint8 test_ram[1024 * 1024] = { 0 };  //тестовая память

public:
	Mem_Ctrl() {};
	//void flash_rom(uint16 address, uint8 data); //запись в ПЗУ
	//void load_ram(uint32 address, uint8 data); //запись прямо в ОЗУ
	//uint8 get_ram(uint32 address); //прямое считывание

	//апдейт

	void write_2(uint32 address, uint8 data); //запись значений в ячейки
	uint8 read_2(uint32 address); //чтение данных из памяти
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

class MyAudioStream : public sf::SoundStream
{
	bool onGetData(Chunk& data) override;
	void onSeek(sf::Time timeOffset) override;

public:
	MyAudioStream() 
	{
		initialize(2, 8000, { sf::SoundChannel::FrontLeft, sf::SoundChannel::FrontRight });
		setLooping(0);
	}

	std::int16_t* s_buffer;			//ссылка буфер для звука
	bool buffer_ready;				//
	int sample_size;				//
};

class SoundMaker //класс звуковой карты
{
private:

	int sample_size = 120;				//длина звукового сэмпла
	int16_t sound_sample[120];			//массив для сэмпла
	MyAudioStream audio_stream;


public:

	SoundMaker()  // конструктор класса
	{
		audio_stream.buffer_ready = false;
		audio_stream.sample_size = sample_size;
		audio_stream.s_buffer = sound_sample;
		//audio_stream.setLooping(true);
	};

	bool beeping = false;		//издает ли звук пищалка
	bool freq_changed = true;  // частота таймера изменилась
	uint16 timer_freq = 0;		//частота звука, запрограммированная на таймере
	void sync();				//синхронизация
	void beep_on();     //сигнал ВКЛ
	void beep_off();    //сигнал ВЫКЛ
	void change_tone(); //меняем тон в процессе игры
};

//контроллер прерываний
class IC8259
{
private:
//	bool enabled = false;
	uint8 next_ICW = 1;  //ожидаемое слово инициализации
	bool wait_ICW4 = false;
	uint16 INT_vector_addr = 0;
	bool cascade = false;

	uint8 next_reg_to_read = 1; // 1 - IR, 2 - IS, 3 - IM
	uint8 last_INT; //текущее прерывание в работе
	uint8 sleep_timer = 0; //таймер задержки

public:
	bool enabled = false;
	uint8 IR_REG = 0; //Interrupt Request Register
	uint8 IS_REG = 0; //In-Service Register
	uint8 IM_REG = 0; //Interrupt Mask Register



	void write_port(uint16 port, uint8 data);
	uint8 read_port(uint16 port);
	bool NMI_enabled = false;
	void request_IRQ(uint8 irq); //запрос прерывания от устройства
	uint8 get_last_int(); //номер последнего прерывания
	uint8 next_int();  //прерывание для исполнения
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
	bool request_hw_dma(int channel); //запрос DMA от устройства
};

