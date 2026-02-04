#pragma once
#include "custom_classes.h"
#include <vector>
#include <string>

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

//контроллер HDD
enum class HDD_states { idle, receive_command, command_exec, read_result, seek, recalibrate, enter_drive_params, write_buffer, read_sectors, write_sectors};

class HDD_Ctrl
{
private:

	std::vector<uint8> out_buffer; //буфер вывода
	std::vector<uint8> in_buffer; //буфер ввода
	std::vector<uint8> status_buffer_0; //буфер статуса HDD0
	std::vector<uint8> status_buffer_1; //буфер статуса HDD1
	
	//данные контроллера
	//регистр статуса, меняется после выполнения команды
	//тут всего два бита 5 - номер диска, 1 - наличие ошибки
	//uint8 Main_status_register = 0; //bit 1 - if ERROR, bit 5 - drive select (0 or 1)
	
	//Биты статуса
	bool HW_Status_IRQ = 0;		//IR request,	1 - контроллер ждет прерывания
	bool HW_Status_DRQ = 0;		//DMA reqest,	1 - контроллер ждет операции DMA
	bool HW_Status_BSY = 0;		//BUSY,			1 - контроллер выполняет команду, прием новых команд невозможен
	bool HW_Status_CD = 0;		//Control/DATA, тип данных: 0 - команда/статус, 1 - данные
	bool HW_Status_IO = 0;		//Input/Output, направление передачи: 0 - от диска к хосту (Output), 1 - от хоста к диску (Input). Уточнить!
	bool HW_Status_REQ = 0;		//Request,		1 - диск готов к обмену данными

	bool IRQ_EN = 0; //разрещение на вызов прерывания
	bool DMA_EN = 0; //разрещение на работу с DMA

	//флаг конца сектора
	bool write_illegal_sector = 0;

	//регистр перемычек
	//по сути это буфер fifo, через который идет обмен данными
	uint8 Jumpers = 255; // перемычки 
	
	bool Ctrl_active = 0; //бит активности контроллера, означающий, что он выбран хостом
	uint8 selected_drive = 0; //выбранный диск, 0 or 1

	HDD_states drv_state = HDD_states::idle; //текущее состояние автомата
	
	uint8 params_left = 0; //сколько еще параметров ввести
	uint32 data_left = 0;  //сколько данных еще нужно обработать
	uint32 data_ptr_current = 0; //указатель на данные на диске
	uint32 data_ptr_MAX = 0; //указатель границы дорожки
	uint16 buffer_pointer = 0; //указатель позиции буфера
	uint8 command = 0;   //код команды

	uint16 MAX_Cylinders = 0; //цилиндры (base 1)
	uint16 MAX_Heads = 0; //головки  (base 1)
	uint16 MAX_Sectors = 0; //сектора  (base 1)
	uint16 RWC = 0; //Starting Reduced Write Current Cylinder
	uint16 WPC = 0; //Starting Write Precompensation Cylinder
	uint16 ECC_burst = 0; //Maximum ECC Data Burst Length

	uint8 sector_buffer[512] = { 0 }; //внутренний буфер

	//параметры доступа к данным
	uint16 cylinder = 0; //текущий цилиндр (макс. 1024) base 0
	uint8 head = 0;	  //текущая головка (макс. 16)  base 0
	uint8 sector = 0;   //текущий сектор (обычно 17) base 0

	bool sectors_changed = 0; //флаг внесения измений в сектора

public:

	HDD_Ctrl();
	~HDD_Ctrl();
	//сигнал EOP от DMA
	bool EOP = 0;
	
	//данные секторов
	uint8* sector_data; // данные секторов

	std::string filename;
	void write_port(uint16 port, uint8 data);
	uint8 read_port(uint16 port);
	void sync();
	void sync_data(int elapsed_ms);
	void load_disk_data(uint32 address, uint8 data);
	uint8 read_DMA_data();
	void write_DMA_data(uint8 data);
	uint32 real_address(uint8 head, uint16 cylinder, uint8 sector);
	void flush_buffer();
};