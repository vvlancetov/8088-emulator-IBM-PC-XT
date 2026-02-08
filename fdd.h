#pragma once
#include "custom_classes.h"
#include <vector>
#include <string>

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

//контроллер FDD
enum class FDD_states { idle, reset, enter_param, command_exec, read_result, seek, recalibrate};

class FDD_Ctrl
{
private:
	
	std::vector<uint8> out_buffer; //буфер вывода
	std::vector<uint8> in_buffer; //буфер ввода

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

	uint8 active_drives = 1; //кол-во приводов от 1 до 4

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
	uint8 C_cylinder[4] = { 0 }; //текущий цилиндр
	bool DIR_control[4] = { 0 }; //направление поиска 1 - к центру
	bool H_head[4] = { 0 };	  //текущая сторона дискеты
	bool MFM_mode[4] = { 0 };    //0 - одинарная плотность, 1 - двойная
	uint8 R_sector[4] = { 0 };   //текущий сектор
	bool MT[4] = { 0 };		  //multi-track
	uint16 Sector_size[4] = { 0 };	  //размер сектора в байтах
	uint8 SectorSizeCode[4] = { 0 };
	std::vector<uint8> sense_int_buffer;

	//вспомогательные данные
	uint32 start_sec = 0;
	uint32 end_sec = 0;
	uint32 read_start = 0;
	uint32 read_end = 0;
	uint32 write_ptr = 0; //указатели для записи
	uint32 write_ptr_end = 0;
	uint8 EOT = 0; //номер последнего сектора на дорожке
	uint8 GPL = 0; //гэп 3 между секторами
	uint8 DTL = 0; //специальный размер сектора если Sector_size = 128 (факт < 128), иначе игнор
	//расчет адреса на диске
	uint32 real_address(uint8 selected_drive, bool head, uint8 cylinder, uint8 sector, uint16 sector_size);
	bool write_to_disk(uint32 address, uint8 data);
	void flush_buffer();

public:

	FDD_Ctrl();  //конструктор
	~FDD_Ctrl(); //деструктор

	//сигнал EOP от DMA
	bool EOP = 0;
	
	//данные секторов
	uint8* sector_data;//
	uint16 test_loaded = 0; //тест багов
	//файлы с образами дискет
	std::vector<std::string> filename_FDD;

	//данные дискеты
	uint8 diskette_heads[4] = { 1 };  // 1 or 2
	uint8 diskette_sectors[4] = { 8 }; //8, 9, 15, 18
	uint8 diskette_cylinders[4] = { 40 }; //40, 80
	uint32 file_size[4] = { 0 }; //размер файла дискеты
	bool write_protect[4] = { 0 }; //признак защиты от записи

	uint8 sleep_counter = 0;//счетчик для задержки выполнения
	void write_port(uint16 port, uint8 data);
	uint8 read_port(uint16 port);
	void sync();
	std::string get_state(uint8* ptr); //данные для мониторинга
	//std::string get_command();
	void load_diskette(uint32 address, uint8 data);
	bool diskette_in[4] = { 0 }; //флаг наличия дискеты в устройстве
	//uint8 read_sector(uint8 track, uint8 sector, bool head);
	uint8 read_DMA_data();
	void write_DMA_data(uint8 data);
	void set_active_drives(uint8 d);
	uint8 get_active_drives();
	void set_MFM(uint8 drive, bool MFM);
};

