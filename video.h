#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/System.hpp>
//#include <vector>

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

extern uint8 memory_2[1024 * 1024]; //������ 2.0

//�������
class Video_device
{
private:
	sf::RenderWindow main_window;
	//int display_mode; //����� ������ ��������
	int my_display_H;
	int my_display_W;
	__int16 GAME_WINDOW_X_RES;
	__int16 GAME_WINDOW_Y_RES;
	//uint8 memory_2[16 * 1024] = { 0 };//�����������
	//uint8 video_rom_array[32 * 1024] = { 0 }; //Video ROM
	//uint8 memory_2_MDA[4 * 1024] = { 0 };//����������� MDA

	int counter = 0;  //?
	sf::Clock cursor_clock;				//������ ������� �������
	bool cursor_flipflop = false;		//���������� ��� �������
	int speed_history[33] = { 100000 };
	unsigned __int8 command_reg = 0;	//������� ������
	unsigned __int8 count_param = 0;	//���������� ���������� ��� ���������
	unsigned __int8 status = 0;         //������� �������
										// 0 - FO, FIFO Overrun - ������������ ������ FIFO
										// 1 - DU, DMA Underrun - ������ ������ �� ����� �������� ���
										// 2 - VE, Video Enable - ������������� � ������� ���������
										// 3 - IC, Improper Command - ��������� ���������� ����������
										// 4 - LP - ���� �� ����� ��������� ���� ������������ �������� ������� � �������� ������� ��������� ����
										// 5 - IR, Interrupt Request - ��������������� � ������ ��������� ������ �� ������ ���� ���������� ���� ���������� ����������
										// 6 - IE, Interrupt Enable - ���������������/������������ ����� ���������
										// 7 - ������ 0
	bool video_enable = true;			//���������� ������ (��������� �� ���������)
	bool int_enable = false;			//���������� ����������
	bool int_request = false;			//��������������� ������ ��� ����� ����������� ������
	bool improper_command = false;		//������ � ����������
	uint8 cursor_x = 0;		//������� �������
	uint8 cursor_y = 0;
	uint8 display_lines = 30;  //���-�� ����� �� ������
	uint8 display_columns = 78;//���-�� �������� �� ������
	uint8 under_line_pos = 10;	 //������� ����� ������������� (�� ������)
	uint8 cursor_format = 1;	 //������ �������: 0 - �������� ����, 1 - �������� �����, 2 - ��������� ����, 3 - ���������� �����
	bool transp_attr = true;			 //��������� ������� ���� (��� ��������� ����������� ���������) 0 - ���������, 1 - ������� (� ���������)
	// ============= ����� �������

	uint8 sel_reg = 0; //��������� ������� ��� ������
	uint8 registers[18] = { 0 }; // ������ ���������
	uint8 display_mode = 0;      //����� ������ �������
	uint8 CGA_Color_Select_Register = 0; //����� ������ �����
	uint8 CGA_Mode_Select_Register = 9;  //������� ������� CGA
	sf::Color CGA_colors[16]; //������ ������ CGA ��� ������
	bool CRT_flip_flop = false;

public:
	sf::Font font;

	void select_register(uint8 data);	//������� �����������
	void set_reg_data(uint8 data);		//��������� �������
	void write_vram(uint16 address, uint8 data); //������ � �����������
	uint8 read_vram(uint16 address);	//������ �� �����������
	uint8 read_vrom(uint16 address);	//������ �� ���
	void load_v_rom(uint16 address, uint8 data);	 //����� ����
	void write_vram_MDA(uint16 address, uint8 data); //������ � �����������
	uint8 read_vram_MDA(uint16 address);			 //������ �� �����������



	uint8 line_height = 10;						//������ ������ � ��������
	Video_device();								// ����������� ������
	void sync(int elapsed_ms);					//������� �������������
	void write_port(uint16 port, uint8 data);	//������ � ���� ��������
	uint8 read_port(uint16 port);				//������ �� ����� ��������
	void set_CGA_mode(uint8 mode);				//��������� ����������� �����������
	void set_cursor_type(uint16 type);			//��������� ���� �������
	void set_cursor_position(uint8 X, uint8 Y, uint8 Page);		//��������� ������� �������
	void read_cursor_position();				//������ ������� �������
	void teletype(uint8 symbol);
	std::string get_mode_name();
};

//������� �������
class Dev_mon_device
{
protected:
	sf::RenderWindow main_window;
	//int display_mode; //����� ������ ��������
	int my_display_H;
	int my_display_W;
	__int16 GAME_WINDOW_X_RES;
	__int16 GAME_WINDOW_Y_RES;

public:
	Dev_mon_device(uint16 w, uint16 h, std::string title, uint16 x_pos, uint16 y_pos); //�����������
	sf::Font font;
	void sync(int elapsed_ms);
};

//������� FDD
class FDD_mon_device : public Dev_mon_device
{
private:
	std::vector<std::string> log_strings;

public:
	using Dev_mon_device::Dev_mon_device;
	void sync();
	void log(std::string log_string);
};