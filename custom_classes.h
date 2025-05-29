#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/System.hpp>
#include <vector>
#include "json.hpp"

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

class Mem_Ctrl // ���������� ������
{
private:
	//uint8 ram_array[640 * 1024] = { 0 };  //RAM
	//uint8 rom_array[64 * 1024] = { 0 };   //ROM
	//uint8 test_ram[1024 * 1024] = { 0 };  //�������� ������

public:
	Mem_Ctrl() {};
	//void flash_rom(uint16 address, uint8 data); //������ � ���
	//void load_ram(uint32 address, uint8 data); //������ ����� � ���
	//uint8 get_ram(uint32 address); //������ ����������

	//������

	void write_2(uint32 address, uint8 data); //������ �������� � ������
	uint8 read_2(uint32 address); //������ ������ �� ������
};

class IO_Ctrl  //���������� ������
{
private:

public:
	void output_to_port_8(uint16 address, uint8 data);		//����� � 8-��� ����
	void output_to_port_16(uint16 address, uint16 data);	//����� � 16-��� ����
	
	uint8 input_from_port_8(uint16 address);				//���� �� 8-��� �����
	uint16 input_from_port_16(uint16 address);				//���� �� 16-��� �����
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

	std::int16_t* s_buffer;			//������ ����� ��� �����
	bool buffer_ready;				//
	int sample_size;				//
};

class SoundMaker //����� �������� �����
{
private:

	int sample_size = 120;				//����� ��������� ������
	int16_t sound_sample[120];			//������ ��� ������
	MyAudioStream audio_stream;


public:

	SoundMaker()  // ����������� ������
	{
		audio_stream.buffer_ready = false;
		audio_stream.sample_size = sample_size;
		audio_stream.s_buffer = sound_sample;
		//audio_stream.setLooping(true);
	};

	bool beeping = false;		//������ �� ���� �������
	bool freq_changed = true;  // ������� ������� ����������
	uint16 timer_freq = 0;		//������� �����, ������������������� �� �������
	void sync();				//�������������
	void beep_on();     //������ ���
	void beep_off();    //������ ����
	void change_tone(); //������ ��� � �������� ����
};

//���������� ����������
class IC8259
{
private:
//	bool enabled = false;
	uint8 next_ICW = 1;  //��������� ����� �������������
	bool wait_ICW4 = false;
	uint16 INT_vector_addr = 0;
	bool cascade = false;

	uint8 next_reg_to_read = 1; // 1 - IR, 2 - IS, 3 - IM
	uint8 last_INT; //������� ���������� � ������
	uint8 sleep_timer = 0; //������ ��������

public:
	bool enabled = false;
	uint8 IR_REG = 0; //Interrupt Request Register
	uint8 IS_REG = 0; //In-Service Register
	uint8 IM_REG = 0; //Interrupt Mask Register



	void write_port(uint16 port, uint8 data);
	uint8 read_port(uint16 port);
	bool NMI_enabled = false;
	void request_IRQ(uint8 irq); //������ ���������� �� ����������
	uint8 get_last_int(); //����� ���������� ����������
	uint8 next_int();  //���������� ��� ����������
	std::string get_ch_data(int ch); // ������ ������
};

//��������� ������ DMA
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
	bool TC_reached = false; //���������� ������ �������� (���� ��� �������������)
};

//���������� DMA
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
	void sync();		//�������������
	void ram_refresh(); //���������� ������
	bool request_hw_dma(int channel); //������ DMA �� ����������
};

