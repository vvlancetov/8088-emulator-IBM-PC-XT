#pragma once
#include <vector>

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

//контроллер FDD
enum class FDD_states { idle, reset, enter_param, command_exec, read_result, seek, recalibrate };

class FDD_Ctrl
{
private:
	
	std::vector<uint8> out_buffer; //буфер вывода

	//данные контроллера
	uint8 Digital_output_register; //DOR
	uint8 Tape_drive_register; //TSR
	uint8 Main_status_register;
	bool RQM = 1; //бит разрешения доступа
	bool DIO = 0; //бит доступа на чтение с диска(1)/запись(0)
	bool CMD_BUSY = 0;//бит занятости констроллера в фазах command-result
	uint8 IC = 0; //interrupt code for ST0
	bool SE = 0;  //признак Seek End
	uint8 data_rate_sel = 0; //выбор скорости
	uint8 reset_N = 0;

	uint8 Data_rate_select_register = 2; //состояние после включения
	std::vector<uint8> Data_fifo; //буфер
	uint8 Digital_input_register;
	uint8 Configuration_control_register;
	FDD_states drv_state = FDD_states::idle; //текущее состояние автомата
	uint8 selected_drive = 0; //выбранный дисковод
	bool DMA_enabled = false;
	bool DMA_ON = false;  //текущий режим DMA
	uint8 motors_pin_enabled = 0; // набор битов <0000> 1 - значит двигатель включен
	uint8 FDD_busy_bits = 0; // 0000 - биты занятости приводов во время работы
	uint8 command = 0;
	uint8 params_left = 0; //сколько еще параметров ввести
	uint8 results_left = 0; //сколько еще результатов вывести

	//управление головкой
	uint8 C_cylinder = 0; //текущий цилиндр
	bool DIR_control = 0; //направление поиска 1 - к центру
	bool H_head = 0;	  //текущая сторона дискеты
	bool MFM_mode = 0;    //0 - одинарная плотность, 1 - двойная
	uint8 R_sector = 0;   //текущий сектор
	bool MT = 0;		  //multi-track
	uint16 Sector_size;	  //размер сектора в байтах
	uint8 SectorSizeCode = 0;
	std::vector<uint8> sense_int_buffer;

	//вспомогательные данные
	int start_sec = 0;
	int end_sec = 0;

public:

	//данные секторов
	uint8* sector_data;    // [737280] = { 0 }; // 80 x 90 x 2 x 512 байт

	//данные дискеты
	uint8 diskette_heads = 1;  // 1 or 2
	uint8 diskette_sectors = 8; //8, 9 or 15

	FDD_Ctrl()
	{
		sector_data = (uint8*)calloc(1200 * 1024, 1);
	}

	uint8 sleep_counter = 0;//счетчик для задержки выполнения
	void write_port(uint16 port, uint8 data);
	uint8 read_port(uint16 port);
	void sync();
	std::string get_state(uint8* ptr); //данные для мониторинга
	std::string get_command();
	void load_diskette(uint8 track, uint8 sector, bool head, uint16 byte_N, uint8 data);
	uint8 read_sector(uint8 track, uint8 sector, bool head);
	uint8 get_DMA_data();
	void put_DMA_data(uint8 data);
};

