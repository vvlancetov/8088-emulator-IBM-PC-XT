#include <iostream>
#include <bitset>
#include <Windows.h>
#include <fstream>
#include <chrono>
#include <conio.h>
#include <string>
#include "custom_classes.h"
#include "video.h"
#include "hdd.h"


//#define DEBUG

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

using namespace std;

extern HANDLE hConsole;

//extern bool log_to_console_FDD;
extern bool step_mode;
extern bool log_to_console;
extern bool log_to_console_HDD;

//консоль отладки
extern HDD_mon_device HDD_monitor;

//путь к файлу диска
extern string filename_HDD;
extern string path; //текущий каталог

#ifdef DEBUG 
//extern FDD_mon_device FDD_monitor;
#endif
extern IC8259 int_ctrl;
extern IC8237 dma_ctrl;

HDD_Ctrl::HDD_Ctrl()
{
	MAX_Cylinders = 306; //цилиндры
	MAX_Heads = 8; //головки
	MAX_Sectors = 17; //сектора
	sector_data = (uint8*)calloc(MAX_Heads * MAX_Cylinders * MAX_Sectors * 512, 1);
	status_buffer_0.resize(5);//создаем буфер статуса
	status_buffer_1.resize(5);//создаем буфер статуса
}

HDD_Ctrl::~HDD_Ctrl()
{
	while (flush_active); //ждем если активен сброс на диск
	flush_buffer();
	free(sector_data);
}

uint32 HDD_Ctrl::real_address(uint8 head, uint16 cylinder, uint8 sector)
{
	//рассчитываем адрес байта в массиве
	//добавить проверку корректности данных
	return (cylinder * MAX_Sectors * MAX_Heads + MAX_Sectors * head + sector) * 512;
}

void HDD_Ctrl::flush_buffer()
{

	//сбрасываем данные на диск
	fstream file_HDD(path + filename, ios::binary | ios::out);

	if (!file_HDD.is_open())
	{
		if (log_to_console_HDD) cout << "File " << filename << " not found!" << endl;
		return;
	}

	for (int i = 0; i < MAX_Cylinders * MAX_Heads * MAX_Sectors * 512; i++)
	{
		file_HDD.write((char*)&sector_data[i], 1);
	}
	//file_HDD.flush();
	file_HDD.close();
	flush_active = 0;//снимаем флаг
	//if (log_to_console_FDD) FDD_monitor.log("Buffer flushed to Disk#" + to_string(selected_drive + 1) + "(" + filename_FDD.at(selected_drive) + ")");
}

uint8 HDD_Ctrl::read_DMA_data()
{
	if (!Ctrl_active) return 0; //выход, если не было SELECT
	if (drv_state == HDD_states::read_sectors)
	{

		if (EOP)
		{
			//сигнал окончания от контроллера DMA
			//data_left = 0;
			EOP = 0;
			if (log_to_console_HDD) cout << "HDD READ - EOP!" << endl;
		}
		
		if (!data_left)
		{
			if (log_to_console_HDD) cout << "HDD READ - No data!" << endl;
			return 0; //если счетчик обнулен - выход
		}

		//считываем данные из основного массива
		uint8 data_out = sector_data[data_ptr_current];
		data_ptr_current++;
		data_left--;

		//пишем во внутренний буфер
		sector_buffer[buffer_pointer] = data_out;
		buffer_pointer++;
		if (buffer_pointer == 512) buffer_pointer = 0; //не даем выйти за пределы массива
		return data_out;
	}
	return 0;
}
void HDD_Ctrl::write_DMA_data(uint8 data)
{
	if (!Ctrl_active) return; //выход, если не было SELECT
	//получаем данные через DMA
	//cout << "write_DMA_data = " << (int)data << " data_left = " << (int)data_left << endl;
	if (drv_state == HDD_states::write_buffer)
	{
		if (EOP)
		{
			//сигнал окончания от контроллера DMA
			data_left = 0;
			EOP = 0;
		}

		if (!data_left) return; //если счетчик обнулен - выход
		data_left--;
		sector_buffer[buffer_pointer] = data;
		buffer_pointer++;
		if (buffer_pointer == 512) buffer_pointer = 0; //не даем выйти за пределы массива
		//cout << "Write buffer = " << (int)data << endl;
	}

	if (drv_state == HDD_states::write_sectors)
	{
		if (EOP)
		{
			//сигнал окончания от контроллера DMA
			//data_left = 0;
			if (log_to_console_HDD) cout << "HDD EOP!" << endl;
			EOP = 0;
		}

		if (data_left == 0) return; //закончились данные

		//заполняем внутренний буфер
		sector_buffer[buffer_pointer] = data;
		buffer_pointer++;
		if (buffer_pointer == 512) buffer_pointer = 0; //не даем выйти за пределы массива
		//if (buffer_pointer == 10) cout << (int)(data_ptr_current - 5) << "=[" << hex << (int)sector_buffer[0] << ", " << (int)sector_buffer[1] << ", " << (int)sector_buffer[2] << ", " << (int)sector_buffer[3] << ", " << (int)sector_buffer[4] << "]" << endl;

		//записываем данные в основной массив
		sector_data[data_ptr_current] = data;
		data_ptr_current++;
		if (data_ptr_current == data_ptr_MAX)
		{
			//if (head < MAX_Heads) cout << "Переход на новую головку  " << (int)sector_data[data_ptr_current - 1] << " >> " << (int)sector_data[data_ptr_current] << endl;
			//if (head == MAX_Heads) cout << "Переход на новую дорожку" << endl;
			
			//data_ptr_current -= MAX_Sectors * 512;//переходим в начало дорожки
			//cout << "Write sectors data_ptr_current correction from " << dec << (int)data_ptr_current;
			//data_ptr_current = data_ptr_current - 17 * 512;
			//cout << " to " << (int)data_ptr_current << endl;
			//остановка записи
			//write_illegal_sector = 1; //ставим флаг ошибки
			
			//data_left = 1;
		}

		data_left--;
	}
}

void HDD_Ctrl::write_port(uint16 port, uint8 data)
{
	
	if (port == 0x320)
	{
		if (!Ctrl_active) return;//выход, если не было SELECT
		//порт обмена данными
		//заполняем входной буфер
		if (drv_state == HDD_states::receive_command)
		{
			static string out_str = " params: "; //строка для составления сообщения
			in_buffer.push_back(data);
			params_left--;
			out_str += " " + to_string(data);

			if (!params_left)
			{
				//параметры введены, запускаем выполнения
				HW_Status_REQ = 0;
				drv_state = HDD_states::command_exec;
				command = in_buffer.at(0);
				//if (log_to_console_HDD) HDD_monitor.log("HDD Ctrl -> EXE CMD #" + to_string(command) + " DRV #" + to_string((in_buffer.at(1) >> 5) & 1) + out_str);
				out_str = " params: ";
				out_buffer.clear(); //очищаем выходной буфер
			}
			return;
		}

		if (drv_state == HDD_states::enter_drive_params)
		{
			in_buffer.push_back(data);
			params_left--;
			if (!params_left)
			{
				//параметры введены, присваиваем значения внутренним переменным
				MAX_Cylinders = in_buffer.at(0) * 256 + in_buffer.at(1); //цилиндры
				MAX_Heads = in_buffer.at(2); //головки
				RWC = in_buffer.at(3) * 256 + in_buffer.at(4); //Starting Reduced Write Current Cylinder
				WPC = in_buffer.at(5) * 256 + in_buffer.at(6); //Starting Write Precompensation Cylinder
				ECC_burst = in_buffer.at(7); //Maximum ECC Data Burst Length
				in_buffer.clear(); //очистка входного буфрера
				
				//выставляем биты статуса
				HW_Status_IRQ = 0;		//IR request,	1 - контроллер ждет прерывания
				HW_Status_DRQ = 0;		//DMA reqest,	1 - контроллер ждет операции DMA
				HW_Status_BSY = 1;		//BUSY,			1 - контроллер выполняет команду, прием новых команд невозможен
				HW_Status_CD = 1;		//Control/DATA, тип данных: 0 - команда/статус, 1 - данные
				HW_Status_IO = 1;		//Input/Output, направление передачи: 0 - от диска к хосту (Output), 1 - от хоста к диску (Input). Уточнить!
				HW_Status_REQ = 1;		//Request,		1 - диск готов к обмену данными
				

				out_buffer.push_back(0); //код ошибки
				drv_state = HDD_states::read_result; //ожидаем чтения статуса от хоста
				//if (log_to_console_HDD) HDD_monitor.log("HDD Ctrl -> Drive Params refreshed ");
			}
			return;

		}


		cout << "input without proper state = " << (int)data << endl;
		step_mode = 1;

	}

	if (port == 0x321)
	{
		//сброс контроллера
		//if (log_to_console_HDD) HDD_monitor.log("HDD Ctrl -> RESET");
		if (log_to_console_HDD) cout << "HDD RESET" << endl;
		//out_buffer.push_back(selected_drive << 5); //кладем на выход байт статуса завершения команды

		//выставляем биты статуса
		HW_Status_IRQ = 0;		//IR request,	1 - контроллер ждет прерывания
		HW_Status_DRQ = 0;		//DMA reqest,	1 - контроллер ждет операции DMA
		HW_Status_BSY = 0;		//BUSY,			1 - контроллер выполняет команду, прием новых команд невозможен
		HW_Status_CD = 0;		//Control/DATA, тип данных: 0 - команда/статус, 1 - данные
		HW_Status_IO = 0;		//Input/Output, направление передачи: 0 - от диска к хосту (Output), 1 - от хоста к диску (Input). Уточнить!
		HW_Status_REQ = 0;		//Request,		1 - диск готов к обмену данными

		drv_state = HDD_states::idle; //состояние ожидания
		Ctrl_active = 0; //Ждем команды Select
		in_buffer.clear();
		out_buffer.clear();
	}

	if (port == 0x322)
	{
		
		//Writing to port 322 selects the WD 1 002S - WX2,
		//	sets the Busy bit in the Status Registerand prepares
		//	it to receive a command.
		
		Ctrl_active = 1; //выбран контроллер HDD
		//if (log_to_console_HDD) HDD_monitor.log("HDD Ctrl -> SELECT -> " + to_string(data) + " -> OK");

		//переводим в статус ожидания команды хоста
		HW_Status_BSY = 1;	//ставим признак занят
		HW_Status_CD = 1;	//данные
		HW_Status_IO = 0;	//вывод
		HW_Status_REQ = 1;	//ожидание хоста

		drv_state = HDD_states::receive_command; //ждем ввод команды
		in_buffer.clear();
		params_left = 6; //сколько байт команды осталось ввести
	}

	if (port == 0x323)
	{
		if (!Ctrl_active) return; ////выход, если не было SELECT
		//Write pattern to DMA and interrupt mask register
		//if (log_to_console_HDD) HDD_monitor.log("HDD Ctrl -> DMA/INT pattern -> IRQ_EN = " + to_string((data >> 1) & 1) + " DMA_EN = " + to_string(data & 1));
		//cout << ("HDD Ctrl -> DMA/INT pattern -> IRQ_EN = " + to_string((data >> 1) & 1) + " DMA_EN = " + to_string(data & 1)) << endl;
		IRQ_EN = (data >> 1) & 1;
		DMA_EN = data & 1;
		//!!!!!!!!!!!!!!!
		//биты 2 и 3 позволяет выбрать конкретный диск, к которому относится команда, в прошивке 62х0822
	}
}
uint8 HDD_Ctrl::read_port(uint16 port)
{
	if (!Ctrl_active) return 0; //выход, если не было SELECT
	if (port == 0x320)
	{
		//порт обмена данными
		//тут можно считать код завершения последней операции
		//cout << "HDD Ctrl READ DATA = ";
		if (out_buffer.size())
		{
			//отдаем данные из буфера
			uint8 d = out_buffer.at(0);
			out_buffer.erase(out_buffer.begin()); //удаляем первый элемент
			//if (log_to_console_HDD) cout << (int)d << endl;

			if (out_buffer.size() == 0)
			{
				//если буфер пустой, обновляем данные
				HW_Status_BSY = 0;		//вывод закончен
				HW_Status_REQ = 0;		//диск в простое
			}

			if (d) 
			{
				//if (log_to_console_HDD) HDD_monitor.log("Result ERROR=" + to_string(d) + "(drv# " + to_string(d >> 5) + ")");//Ошибка
			}
			else
			{
				//if (log_to_console_HDD) HDD_monitor.log("Result OK");
			}

			return d;
		}
		else
		{
			//буфер пустой
			//if (log_to_console_HDD) HDD_monitor.log("Result OK");
			return 0;
		}
	}

	if (port == 0x321)
	{
		//Read controller hardware status
		uint8 status = (HW_Status_IRQ << 5) | (HW_Status_DRQ << 4) | (HW_Status_BSY << 3) | (HW_Status_CD << 2) | (HW_Status_IO << 1) | HW_Status_REQ;
		//if (log_to_console_HDD) HDD_monitor.log("HDD READ STATUS = IRQ=" + to_string(HW_Status_IRQ) + " DRQ=" + to_string(HW_Status_DRQ) + " BUSY=" + to_string(HW_Status_BSY) + " C/D=" + to_string(HW_Status_CD) + " I/O=" + to_string(HW_Status_IO) + " REQ=" + to_string(HW_Status_REQ));
		//drv_state = HDD_states::receive_command; //переключаемся в режим ожидания команды
		
		return status;
	}

	if (port == 0x322)
	{
		//Read option jumpers
		//cout << "HDD Ctrl READ JUMPERS = " << (int)Jumpers << endl;
		return Jumpers;
	}

	return 0xFF;
}

void HDD_Ctrl::sync_data(int elapsed_ms)
{
	static uint32 flush_timer = 0;   //таймер сброса на диск
	if (sectors_changed) flush_timer += elapsed_ms;

	if (flush_timer > 2000000 && !flush_active)  //засекаем 2 сек
	{
		//сбрасываем данные на диск
		flush_active = 1;
		cout << "flush sectors to HDD" << endl;
		std::thread t(&HDD_Ctrl::flush_buffer, this);
		t.detach();
		sectors_changed = 0; //сброс флага
		flush_timer = 0;     //сброс таймера
	}
}



void HDD_Ctrl::sync()
{

	if (!Ctrl_active) return; //выход, если не было SELECT
	//синхронизация HDD
	//задействует прерывание 5
	//машина состояний HDD_states { idle, reset, enter_param, command_exec, read_result, seek, recalibrate };

	if (drv_state == HDD_states::idle)
	{
		//простой
		return;
	}

	if (drv_state == HDD_states::receive_command)
	{
		//ждем заполнения входного буфера
		return;
	}

	if (drv_state == HDD_states::command_exec)
	{
		
		selected_drive = (in_buffer.at(1) >> 5) & 1;
		EOP = 0; //сбрасываем бит окончания DMA

		//выполняем команду
		string command_name; //наименование команды для комментария
		switch (command)
		{
		case 0:
			command_name = "TEST DRIVE READY drv# " + to_string(selected_drive);
			break;
		case 1:
			command_name = "RECALIBRATE drv# " + to_string(selected_drive);
			break;
		case 3:
			command_name = "READ STATUS OF LAST OPERATION drv# " + to_string(selected_drive);
			break;
		case 4:
			command_name = "FORMAT DRIVE drv# " + to_string(selected_drive);
			break;
		case 5:
			command_name = "VERIFY SECTORS drv# " + to_string(selected_drive);
			break;
		case 6:
			command_name = "FORMAT TRACK drv# " + to_string(selected_drive);
			break;
		case 7:
			command_name = "FORMAT BAD TRACK drv# " + to_string(selected_drive);
			break;
		case 8:
			command_name = "READ SECTORS drv# " + to_string(selected_drive);
			break;
		case 0xA:
			command_name = "WRITE SECTORS drv# " + to_string(selected_drive);
			break;
		case 0xB:
			command_name = "SEEK drv# " + to_string(selected_drive);
			break;
		case 0xC:
			command_name = "INITIALIZE DRIVE PARAMS drv# " + to_string(selected_drive);
			break;
		case 0xD:
			command_name = "READ ECC ERROR LENGHT drv# " + to_string(selected_drive);
			break;
		case 0xE:
			command_name = "READ SECTOR BUFFER drv# " + to_string(selected_drive);
			break;
		case 0xF:
			command_name = "WRITE SECTOR BUFFER drv# " + to_string(selected_drive);
			break;
		case 0xE0:
			command_name = "SECTOR BUFFER DIAGNOSTIC drv# " + to_string(selected_drive);
			break;
		case 0xE3:
			command_name = "DRIVE DIAGNOSTIC drv# " + to_string(selected_drive);
			break;
		case 0xE4:
			command_name = "CTRL DIAGNOSTIC drv# " + to_string(selected_drive);
			break;
		case 0xE5:
			command_name = "READ LONG drv# " + to_string(selected_drive);
			break;
		case 0xE6:
			command_name = "WRITE LONG drv# " + to_string(selected_drive);
			break;
		}
		//if (log_to_console_HDD) HDD_monitor.log("EXEcuting command " + to_string(command) + " " + command_name);

		//TEST DRIVE READY
		if (command == 0)
		{
			HW_Status_IRQ = 1;		//IR request,	1 - контроллер ждет прерывания
			HW_Status_DRQ = 0;		//DMA reqest,	1 - контроллер ждет операции DMA
			HW_Status_BSY = 1;		//BUSY,			1 - контроллер выполняет команду, прием новых команд невозможен
			HW_Status_CD = 0;		//Control/DATA, тип данных: 0 - команда/статус, 1 - данные
			HW_Status_IO = 0;		//Input/Output, направление передачи: 0 - от диска к хосту (Output), 1 - от хоста к диску (Input). Уточнить!
			HW_Status_REQ = 1;		//Request,		1 - диск готов к обмену данными
			drv_state = HDD_states::read_result; //ожидаем чтения статуса от хоста
			in_buffer.clear();		//очищаем входной буффер
			//если выбран диск #1 - выдаем ошибку
			if (selected_drive == 1)
			{
				out_buffer.push_back(0b00100010); //признак ошибки диска #1
				out_buffer.push_back(0b00000100); //код 04
				out_buffer.push_back(0b00100000); //диск #1
				out_buffer.push_back(0b00000000); //просто нули
				out_buffer.push_back(0b00000000); //просто нули
				//копируем в буфер статуса
				for (int i = 0; i < 5; i++) status_buffer_1.at(i) = out_buffer.at(i);
			}
			else status_buffer_0.at(0) = 0;
			return;
		}

		//RECALIBRATE 

		if (command == 1)
		{
			HW_Status_IRQ = 1;		//IR request,	1 - контроллер ждет прерывания
			HW_Status_DRQ = 0;		//DMA reqest,	1 - контроллер ждет операции DMA
			HW_Status_BSY = 1;		//BUSY,			1 - контроллер выполняет команду, прием новых команд невозможен
			HW_Status_CD = 0;		//Control/DATA, тип данных: 0 - команда/статус, 1 - данные
			HW_Status_IO = 0;		//Input/Output, направление передачи: 0 - от диска к хосту (Output), 1 - от хоста к диску (Input). Уточнить!
			HW_Status_REQ = 1;		//Request,		1 - диск готов к обмену данными
			in_buffer.clear();	//очищаем входной буффер
			//перемещает головки на дорожку 0
			drv_state = HDD_states::read_result; //ожидаем чтения статуса от хоста
			//если выбран диск #1 - выдаем ошибку
			if (selected_drive == 1)
			{
				out_buffer.push_back(0b00100010); //признак ошибки диска #1
				out_buffer.push_back(0b00000100); //код 04
				out_buffer.push_back(0b00100000); //диск #1
				out_buffer.push_back(0b00000000); //просто нули
				out_buffer.push_back(0b00000000); //просто нули
				//копируем в буфер статуса
				for (int i = 0; i < 5; i++) status_buffer_1.at(i) = out_buffer.at(i);
			}
			else status_buffer_0.at(0) = 0;
			return;
		}
		
		//READ STATUS OF LAST OPERATION
		if (command == 3)
		{
			HW_Status_IRQ = 1;		//IR request,	1 - контроллер ждет прерывания
			HW_Status_DRQ = 0;		//DMA reqest,	1 - контроллер ждет операции DMA
			HW_Status_BSY = 1;		//BUSY,			1 - контроллер выполняет команду, прием новых команд невозможен
			HW_Status_CD = 1;		//Control/DATA, тип данных: 0 - команда/статус, 1 - данные
			HW_Status_IO = 1;		//Input/Output, направление передачи: 0 - от диска к хосту (Output), 1 - от хоста к диску (Input). Уточнить!
			HW_Status_REQ = 1;		//Request,		1 - диск готов к обмену данными
			//если выбран диск #1 - выдаем ошибку
			//в противном случае по умолчанию = 0
			
			//копируем из буфера статуса
			if (selected_drive == 0)
			{
				if (status_buffer_0.at(0) == 0) out_buffer.push_back(status_buffer_0.at(0));
				else for (int i = 0; i < 5; i++) out_buffer.push_back(status_buffer_0.at(i));
			}
			else
			{
				if (status_buffer_1.at(0) == 0) out_buffer.push_back(status_buffer_1.at(0));
				else for (int i = 0; i < 5; i++) out_buffer.push_back(status_buffer_1.at(i));
			}
			drv_state = HDD_states::read_result; //ожидаем чтения статуса от хоста
			return;
		}

		//VERIFY SECTORS
		if (command == 5)
		{
			HW_Status_IRQ = 1;		//IR request,	1 - контроллер ждет прерывания
			HW_Status_DRQ = 0;		//DMA reqest,	1 - контроллер ждет операции DMA
			HW_Status_BSY = 1;		//BUSY,			1 - контроллер выполняет команду, прием новых команд невозможен
			HW_Status_CD = 1;		//Control/DATA, тип данных: 0 - команда/статус, 1 - данные
			HW_Status_IO = 1;		//Input/Output, направление передачи: 0 - от диска к хосту (Output), 1 - от хоста к диску (Input). Уточнить!
			HW_Status_REQ = 1;		//Request,		1 - диск готов к обмену данными

			drv_state = HDD_states::read_result; //ожидаем чтения статуса от хоста
			//если выбран диск #1 - выдаем ошибку
			//в противном случае по умолчанию = 0
			if (selected_drive == 1)
			{
				out_buffer.push_back(0b00100010); //признак ошибки диска #1
				out_buffer.push_back(0b00000100); //код 04
				out_buffer.push_back(0b00100000); //диск #1
				out_buffer.push_back(0b00000000); //просто нули
				out_buffer.push_back(0b00000000); //просто нули
				//копируем в буфер статуса
				for (int i = 0; i < 5; i++) status_buffer_1.at(i) = out_buffer.at(i);
			}
			else status_buffer_0.at(0) = 0;
			return;
		}

		//READ SECTORS
		if (command == 8)
		{
			//if (log_to_console_HDD) cout << "HDD - > READ SECTORS C/H/S=" << (int)((in_buffer.at(2) >> 6) * 256 + in_buffer.at(3)) << "/" << (int)(in_buffer.at(1) & 31) << "/" << (int)(in_buffer.at(2) & 63) << " (" << (int)in_buffer.at(4) << " sectors)" << endl;
			//step_mode = 1;
			//выставляем биты статуса
			HW_Status_IRQ = 0;		//IR request,	1 - контроллер ждет прерывания
			HW_Status_DRQ = 1;		//DMA reqest,	1 - контроллер ждет операции DMA
			HW_Status_BSY = 1;		//BUSY,			1 - контроллер выполняет команду, прием новых команд невозможен
			HW_Status_CD = 1;		//Control/DATA, тип данных: 0 - команда/статус, 1 - данные
			HW_Status_IO = 0;		//Input/Output, направление передачи: 0 - от диска к хосту (Output), 1 - от хоста к диску (Input). Уточнить!
			HW_Status_REQ = 1;		//Request,		1 - диск готов к обмену данными
			
			//устанавливам начальную позицию чтения
			head = in_buffer.at(1) & 31;	  //стартовая головка
			cylinder = (in_buffer.at(2) >> 6)  * 256 + in_buffer.at(3);		//стартовый цилиндр
			sector = in_buffer.at(2) & 63;   //стартовый сектор
			data_ptr_current = real_address(head, cylinder, sector);
			data_left = in_buffer.at(4) * 512; //кол-во байт для считывания
			buffer_pointer = 0; //внутренний буфер данных
			in_buffer.clear();	//очищаем входной буффер
			drv_state = HDD_states::read_sectors; //выполняем команду
			//if (log_to_console_HDD) HDD_monitor.log("READ DRV=" + to_string(selected_drive) + " head=" + to_string(head) + " cyl=" + to_string(cylinder) + " sector = " + to_string(sector) + " prt=" + to_string(data_ptr_current) + " buffer=" + to_string(data_left) + " bytes");
			return;
		}	
			
		//WRITE SECTORS
		if (command == 0xA)
		{
			//if (log_to_console_HDD) HDD_monitor.log("HDD - > WRITE SECTORS");
			//step_mode = 1;
			//выставляем биты статуса
			HW_Status_IRQ = 0;		//IR request,	1 - контроллер ждет прерывания
			HW_Status_DRQ = 1;		//DMA reqest,	1 - контроллер ждет операции DMA
			HW_Status_BSY = 1;		//BUSY,			1 - контроллер выполняет команду, прием новых команд невозможен
			HW_Status_CD = 1;		//Control/DATA, тип данных: 0 - команда/статус, 1 - данные
			HW_Status_IO = 1;		//Input/Output, направление передачи: 0 - от диска к хосту (Output), 1 - от хоста к диску (Input). Уточнить!
			HW_Status_REQ = 1;		//Request,		1 - диск готов к обмену данными

			//устанавливам начальную позицию чтения
			head = in_buffer.at(1) & 31;	  //стартовая головка
			cylinder = (in_buffer.at(2) >> 6) * 256 + in_buffer.at(3);		//стартовый цилиндр
			sector = in_buffer.at(2) & 63;   //стартовый сектор
			data_ptr_current = real_address(head, cylinder, sector);
			data_ptr_MAX = real_address(head, cylinder, MAX_Sectors); //адрес конца дорожки
			data_left = in_buffer.at(4) * 512; //кол-во байт для считывания
			buffer_pointer = 0; //внутренний буфер данных
			out_buffer.clear();	//очищаем входной буффер
			drv_state = HDD_states::write_sectors; //выполняем команду
			//cout << "cyl = " << dec << (int)cylinder << endl;
			//cout << "head = " << dec << (int)head << endl;
			//cout << "sector = " << dec << (int)sector << endl;
			//cout << "data_ptr_current = " << dec << (int)data_ptr_current << endl;
			//cout << "data_ptr_MAX = " << dec << (int)data_ptr_MAX << endl;
			write_illegal_sector = 0; //сброс флага
			//if (log_to_console_HDD) HDD_monitor.log("WRITE DRV=" + to_string(selected_drive) + " head=" + to_string(head) + " cyl=" + to_string(cylinder) + " sector = " + to_string(sector) + " prt=" + to_string(data_ptr_current) + " buffer=" + to_string(data_left) + " bytes");
			//cout << "WRITE HDD C/H/S = " << (int)cylinder << "/" << (int)head << "/" << (int)sector << " QTY = " << (int)(data_left / 512) << " ptr_begin = " << (int)data_ptr_current << endl;
			//step_mode = 1;
			return;
		}

		//SEEK
		if (command == 0xb)
		{
			//выставляем биты статуса
			HW_Status_IRQ = 1;		//IR request,	1 - контроллер ждет прерывания
			HW_Status_DRQ = 0;		//DMA reqest,	1 - контроллер ждет операции DMA
			HW_Status_BSY = 1;		//BUSY,			1 - контроллер выполняет команду, прием новых команд невозможен
			HW_Status_CD = 1;		//Control/DATA, тип данных: 0 - команда/статус, 1 - данные
			HW_Status_IO = 0;		//Input/Output, направление передачи: 0 - от диска к хосту (Output), 1 - от хоста к диску (Input). Уточнить!
			HW_Status_REQ = 1;		//Request,		1 - диск готов к обмену данными

			//устанавливам начальную позицию чтения
			head = in_buffer.at(1) & 31;	  //стартовая головка
			cylinder = (in_buffer.at(2) >> 6) * 256 + in_buffer.at(3);		//стартовый цилиндр
			sector = in_buffer.at(2) & 63;   //стартовый сектор
			out_buffer.clear();	//очищаем выходной буффер
			out_buffer.push_back(selected_drive << 5); //статус для выбранного диска
			drv_state = HDD_states::read_result; //чтение результатов
			//if (log_to_console_HDD) HDD_monitor.log("SEEK DRV=" + to_string(selected_drive) + " head=" + to_string(head) + " cyl=" + to_string(cylinder) + " sector = " + to_string(sector));
			return;
		}

		//INITIALIZE DRIVE PARAMETERS
		if (command == 0xC)
		{
			HW_Status_IRQ = 0;		//IR request,	1 - контроллер ждет прерывания
			HW_Status_DRQ = 0;		//DMA reqest,	1 - контроллер ждет операции DMA
			HW_Status_BSY = 1;		//BUSY,			1 - контроллер выполняет команду, прием новых команд невозможен
			HW_Status_CD = 0;		//Control/DATA, тип данных: 0 - команда/статус, 1 - данные
			HW_Status_IO = 0;		//Input/Output, направление передачи: 0 - от диска к хосту (Output), 1 - от хоста к диску (Input). Уточнить!
			HW_Status_REQ = 1;		//Request,		1 - диск готов к обмену данными
			in_buffer.clear();	//очищаем входной буффер
			//получаем параметры от хоста (8 байт)
			drv_state = HDD_states::enter_drive_params; //ожидаем запись параметров в порт
			params_left = 8;
			return;
		}
	
		//WRITE SECTOR BUFFER
		if (command == 0xF)
		{
			//if (log_to_console_HDD)  HDD_monitor.log("HDD - > Write buffer");
			//step_mode = 1;
			//выставляем биты статуса
			HW_Status_IRQ = 0;		//IR request,	1 - контроллер ждет прерывания
			HW_Status_DRQ = 1;		//DMA reqest,	1 - контроллер ждет операции DMA
			HW_Status_BSY = 1;		//BUSY,			1 - контроллер выполняет команду, прием новых команд невозможен
			HW_Status_CD = 1;		//Control/DATA, тип данных: 0 - команда/статус, 1 - данные
			HW_Status_IO = 1;		//Input/Output, направление передачи: 0 - от диска к хосту (Output), 1 - от хоста к диску (Input). Уточнить!
			HW_Status_REQ = 1;		//Request,		1 - диск готов к обмену данными
			in_buffer.clear();	//очищаем входной буффер
			drv_state = HDD_states::write_buffer; //выполняем команду
			data_left = 512;
			buffer_pointer = 0;
			return;
		}	
		
		//EXECUTE SECTOR BUFFER DIAGNOSTIC
		if (command == 0xE0)
		{
			//выставляем биты статуса
			HW_Status_IRQ = 1;		//IR request,	1 - контроллер ждет прерывания
			HW_Status_DRQ = 0;		//DMA reqest,	1 - контроллер ждет операции DMA
			HW_Status_BSY = 1;		//BUSY,			1 - контроллер выполняет команду, прием новых команд невозможен
			HW_Status_CD = 0;		//Control/DATA, тип данных: 0 - команда/статус, 1 - данные
			HW_Status_IO = 0;		//Input/Output, направление передачи: 0 - от диска к хосту (Output), 1 - от хоста к диску (Input). Уточнить!
			HW_Status_REQ = 1;		//Request,		1 - диск готов к обмену данными
			in_buffer.clear();	//очищаем входной буффер
			drv_state = HDD_states::read_result; //ожидаем чтения статуса от хоста
			return;
		}

		//EXECUTE CONTROllER DIAGNOSTIC
		if (command == 0xE4)
		{
			//выставляем биты статуса
			HW_Status_IRQ = 1;		//IR request,	1 - контроллер ждет прерывания
			HW_Status_DRQ = 0;		//DMA reqest,	1 - контроллер ждет операции DMA
			HW_Status_BSY = 1;		//BUSY,			1 - контроллер выполняет команду, прием новых команд невозможен
			HW_Status_CD = 0;		//Control/DATA, тип данных: 0 - команда/статус, 1 - данные
			HW_Status_IO = 0;		//Input/Output, направление передачи: 0 - от диска к хосту (Output), 1 - от хоста к диску (Input). Уточнить!
			HW_Status_REQ = 1;		//Request,		1 - диск готов к обмену данными
			in_buffer.clear();	//очищаем входной буффер
			drv_state = HDD_states::read_result; //ожидаем чтения статуса от хоста
			return;
		}

		cout << "HDD unknown command = " << (int)command << endl;
	}

	if (drv_state == HDD_states::read_result)
	{
		//cout << "Read results, ";
		//ждем считывания результатов команды
		if (IRQ_EN) {
			
			//cout << "HDD sent IRQ" << endl;
			drv_state == HDD_states::idle;
			HW_Status_IRQ = 1;		//IR request,	1 - контроллер ждет прерывания
			HW_Status_DRQ = 0;		//DMA reqest,	1 - контроллер ждет операции DMA
			HW_Status_BSY = 0;		//BUSY,			1 - контроллер выполняет команду, прием новых команд невозможен
			HW_Status_CD = 0;		//Control/DATA, тип данных: 0 - команда/статус, 1 - данные
			HW_Status_IO = 1;		//Input/Output, направление передачи: 0 - от диска к хосту (Output), 1 - от хоста к диску (Input). Уточнить!
			HW_Status_REQ = 1;		//Request,		1 - диск готов к обмену данными
			int_ctrl.request_IRQ(5); //запускаем IRQ5
			return;
		}
		else {
			//ждем разрешения прерываний
			
			//cout << "IRQ did not send"
		};
		//cout << " in_buf = " << (int)in_buffer.size();
		//cout << endl;
		return;
	}

	if (drv_state == HDD_states::enter_drive_params)
	{
		//ждем ввода данных в порт
		return;
	}

	if (drv_state == HDD_states::write_buffer)
	{
		//ждем ввода данных в буфер через DMA
		if (data_left && DMA_EN)
		{
			dma_ctrl.request_hw_dma(3);//запрашиваем транзакцию
			return; //ждем пока не кончится
		}

		if (data_left && !DMA_EN) return; //ждем разрешения DMA

		//если ввод окончен - возвращаем результат
		//выставляем биты статуса
		HW_Status_IRQ = 1;		//IR request,	1 - контроллер ждет прерывания
		HW_Status_DRQ = 0;		//DMA reqest,	1 - контроллер ждет операции DMA
		HW_Status_BSY = 1;		//BUSY,			1 - контроллер выполняет команду, прием новых команд невозможен
		HW_Status_CD = 1;		//Control/DATA, тип данных: 0 - команда/статус, 1 - данные
		HW_Status_IO = 1;		//Input/Output, направление передачи: 0 - от диска к хосту (Output), 1 - от хоста к диску (Input). Уточнить!
		HW_Status_REQ = 1;		//Request,		1 - диск готов к обмену данными
		out_buffer.push_back(0); //статус выполнения ОК
		int_ctrl.request_IRQ(5); //запрашиваем прерывание
		drv_state = HDD_states::read_result; //ожидаем чтения статуса от хоста
		return;
	}

	if (drv_state == HDD_states::read_sectors)
	{
		//ждем считывания данных в буфер через DMA
		if (data_left && DMA_EN)
		{
			dma_ctrl.request_hw_dma(3);//запрашиваем транзакцию
			return; //ждем пока не кончится
		}

		if (data_left && !DMA_EN) return; //ждем разрешения DMA

		//если чтения окончено - возвращаем результат
		//выставляем биты статуса
		HW_Status_IRQ = 1;		//IR request,	1 - контроллер ждет прерывания
		HW_Status_DRQ = 0;		//DMA reqest,	1 - контроллер ждет операции DMA
		HW_Status_BSY = 1;		//BUSY,			1 - контроллер выполняет команду, прием новых команд невозможен
		HW_Status_CD = 0;		//Control/DATA, тип данных: 0 - команда/статус, 1 - данные
		HW_Status_IO = 0;		//Input/Output, направление передачи: 0 - от диска к хосту (Output), 1 - от хоста к диску (Input). Уточнить!
		HW_Status_REQ = 1;		//Request,		1 - диск готов к обмену данными
		int_ctrl.request_IRQ(5); //запрашиваем прерывание
		drv_state = HDD_states::read_result; //ожидаем чтения статуса от хоста
		//если выбран диск #1 - выдаем ошибку
		if (selected_drive == 1)
		{
			out_buffer.push_back(0b00100010); //признак ошибки диска #1
			out_buffer.push_back(0b00000100); //код 04
			out_buffer.push_back(0b00100000); //диск #1
			out_buffer.push_back(0b00000000); //просто нули
			out_buffer.push_back(0b00000000); //просто нули
			//копируем в буфер статуса
			for (int i = 0; i < 5; i++) status_buffer_1.at(i) = out_buffer.at(i);
		}
		else status_buffer_0.at(0) = 0;
		
		if (log_to_console_HDD) cout << "Read sectors -> END" << endl;
		return;
	}

	if (drv_state == HDD_states::write_sectors)
	{
		//ждем записи данных через DMA (в буфер и сразу на диск)
		if (data_left && DMA_EN)
		{
			dma_ctrl.request_hw_dma(3);//запрашиваем транзакцию
			return; //ждем пока не кончится
		}

		if (data_left && !DMA_EN) return; //ждем разрешения DMA

		//если запись окончена - возвращаем результат
		//выставляем биты статуса
		HW_Status_IRQ = 1;		//IR request,	1 - контроллер ждет прерывания
		HW_Status_DRQ = 0;		//DMA reqest,	1 - контроллер ждет операции DMA
		HW_Status_BSY = 1;		//BUSY,			1 - контроллер выполняет команду, прием новых команд невозможен
		HW_Status_CD = 0;		//Control/DATA, тип данных: 0 - команда/статус, 1 - данные
		HW_Status_IO = 0;		//Input/Output, направление передачи: 0 - от диска к хосту (Output), 1 - от хоста к диску (Input). Уточнить!
		HW_Status_REQ = 1;		//Request,		1 - диск готов к обмену данными
		int_ctrl.request_IRQ(5); //запрашиваем прерывание
		drv_state = HDD_states::read_result; //ожидаем чтения статуса от хоста
		//если выбран диск #1 - выдаем ошибку
		if (selected_drive == 1)
		{
			out_buffer.push_back(0b00100010); //признак ошибки диска #1
			out_buffer.push_back(0b00000100); //код 04
			out_buffer.push_back(0b00100000); //диск #1
			out_buffer.push_back(0b00000000); //просто нули
			out_buffer.push_back(0b00000000); //просто нули
			//копируем в буфер статуса
			for (int i = 0; i < 5; i++) status_buffer_1.at(i) = out_buffer.at(i);
		}
		else status_buffer_0.at(0) = 0;
			

		//if (log_to_console_HDD) cout << "Write sectors -> END" << endl;

		//сброс данных в файл
		//flush_buffer();
		sectors_changed = 1; //выставляем флаг
		return;
	}


	cout << "HDD unknown state = " << (int)drv_state << endl;
	step_mode = 1;
}

void HDD_Ctrl::load_disk_data(uint32 address, uint8 data)
{
	if (address < MAX_Heads * MAX_Cylinders * MAX_Sectors * 512) sector_data[address] = data;
}

int HDD_Ctrl::get_drv() { return selected_drive; }
string HDD_Ctrl::get_CSH()
{ 
	return to_string(cylinder) + "/" + to_string(sector) + "/" + to_string(head);
}

string HDD_Ctrl::get_state()
{
	switch (drv_state)
	{
	case HDD_states::idle:
		return "idle";
		break;
	case HDD_states::receive_command:
		return "rec_cmd";
		break;
	case HDD_states::command_exec:
		return "exec " + to_string(command);
		break;
	case HDD_states::read_result:
		return "result";
		break;
	case HDD_states::seek:
		return "seek";
		break;
	case HDD_states::recalibrate:
		return "recal";
		break;
	case HDD_states::enter_drive_params:
		return "param";
		break;
	case HDD_states::write_buffer:
		return "WR_buf";
		break;
	case HDD_states::read_sectors:
		return "RD_sec";
		break;	
	case HDD_states::write_sectors:
		return "WR_sec";
		break;

	}
	return "unknown";
}
void HDD_Ctrl::flash_rom(uint32 address, uint8 data)
{
	rom[address] = data;
}
uint8 HDD_Ctrl::read_rom(uint32 address)
{
	return rom[address];
}