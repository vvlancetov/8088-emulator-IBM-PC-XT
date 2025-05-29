#pragma once
#include <vector>

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

//���������� FDD
enum class FDD_states { idle, reset, enter_param, command_exec, read_result, seek, recalibrate };

class FDD_Ctrl
{
private:
	
	std::vector<uint8> out_buffer; //����� ������

	//������ �����������
	uint8 Digital_output_register; //DOR
	uint8 Tape_drive_register; //TSR
	uint8 Main_status_register;
	bool RQM = 1; //��� ���������� �������
	bool DIO = 0; //��� ������� �� ������ � �����(1)/������(0)
	bool CMD_BUSY = 0;//��� ��������� ������������ � ����� command-result
	uint8 IC = 0; //interrupt code for ST0
	bool SE = 0;  //������� Seek End
	uint8 data_rate_sel = 0; //����� ��������
	uint8 reset_N = 0;

	uint8 Data_rate_select_register = 2; //��������� ����� ���������
	std::vector<uint8> Data_fifo; //�����
	uint8 Digital_input_register;
	uint8 Configuration_control_register;
	FDD_states drv_state = FDD_states::idle; //������� ��������� ��������
	uint8 selected_drive = 0; //��������� ��������
	bool DMA_enabled = false;
	bool DMA_ON = false;  //������� ����� DMA
	uint8 motors_pin_enabled = 0; // ����� ����� <0000> 1 - ������ ��������� �������
	uint8 FDD_busy_bits = 0; // 0000 - ���� ��������� �������� �� ����� ������
	uint8 command = 0;
	uint8 params_left = 0; //������� ��� ���������� ������
	uint8 results_left = 0; //������� ��� ����������� �������

	//���������� ��������
	uint8 C_cylinder = 0; //������� �������
	bool DIR_control = 0; //����������� ������ 1 - � ������
	bool H_head = 0;	  //������� ������� �������
	bool MFM_mode = 0;    //0 - ��������� ���������, 1 - �������
	uint8 R_sector = 0;   //������� ������
	bool MT = 0;		  //multi-track
	uint16 Sector_size;	  //������ ������� � ������
	uint8 SectorSizeCode = 0;
	std::vector<uint8> sense_int_buffer;

	//��������������� ������
	int start_sec = 0;
	int end_sec = 0;

public:

	//������ ��������
	uint8* sector_data;    // [737280] = { 0 }; // 80 x 90 x 2 x 512 ����

	//������ �������
	uint8 diskette_heads = 1;  // 1 or 2
	uint8 diskette_sectors = 8; //8, 9 or 15

	FDD_Ctrl()
	{
		sector_data = (uint8*)calloc(1200 * 1024, 1);
	}

	uint8 sleep_counter = 0;//������� ��� �������� ����������
	void write_port(uint16 port, uint8 data);
	uint8 read_port(uint16 port);
	void sync();
	std::string get_state(uint8* ptr); //������ ��� �����������
	std::string get_command();
	void load_diskette(uint8 track, uint8 sector, bool head, uint16 byte_N, uint8 data);
	uint8 read_sector(uint8 track, uint8 sector, bool head);
	uint8 get_DMA_data();
	void put_DMA_data(uint8 data);
};

