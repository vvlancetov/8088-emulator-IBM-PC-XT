//#include <SFML/Graphics.hpp>
//#include <SFML/Audio.hpp>

#include <iostream>
#include <bitset>
#include <Windows.h>
#include <conio.h>
#include <thread>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <chrono>
#include "custom_classes.h"
#include "fdd.h"
#include "video.h"

//#define DEBUG

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

using namespace std;

extern HANDLE hConsole;

extern bool log_to_console_FDD;
extern bool step_mode;
extern bool log_to_console;

#ifdef DEBUG 
extern FDD_mon_device FDD_monitor;
#endif
extern IC8259 int_ctrl;
extern IC8237 dma_ctrl;

//данные файла дискеты
extern string path;
extern Monitor monitor;

FDD_Ctrl::FDD_Ctrl()
{
	sector_data = (uint8*)calloc(2880 * 1024 * 4, 1); //максимальный объем дискет
	filename_FDD.resize(4);
}
FDD_Ctrl::~FDD_Ctrl()
{
	free(sector_data);
}

uint32 FDD_Ctrl::real_address(uint8 selected_drive, bool head, uint8 cylinder, uint8 sector, uint16 sector_size)
{
	//рассчитываем адрес байта в массиве h=0 C= 0  S = 1      M_S = 9  M_H = 2
	//return cylinder * sector_size * diskette_sectors[selected_drive] * diskette_heads[selected_drive] + diskette_sectors[selected_drive] * sector_size * head * (diskette_heads[selected_drive] - 1) + (sector - 1) * sector_size + selected_drive * 2880 * 1024;
	return (cylinder * diskette_sectors[selected_drive] * diskette_heads[selected_drive] + diskette_sectors[selected_drive] * head + (sector - 1)) * sector_size + selected_drive * 2880 * 1024;
}

bool FDD_Ctrl::write_to_disk(uint32 address, uint8 data)
{
	//запись на диск с проверкой
	if ((address - selected_drive * 2880 * 1024) < file_size[selected_drive])
	{
		sector_data[address] = data; //пишем в буфер
		return 0;
	}
	else
	{
		if (log_to_console_FDD) cout << "FDD #" << (int)(selected_drive + 1) << " ERROR: adress > file_size" << endl;
		return 1;
	}
}

void FDD_Ctrl::flush_buffer()
{

	//сбрасываем данные на диск
	fstream file_FDD(path + filename_FDD.at(selected_drive), ios::binary | ios::out);

	if (!file_FDD.is_open())
	{
		if (log_to_console_FDD) cout << "File " << filename_FDD.at(selected_drive) << " not found!" << endl;
		return;
	}

	for (int i = 0; i < file_size[selected_drive]; i++)
	{
		file_FDD.write((char*)&sector_data[i + selected_drive * 2880 * 1024], 1);
	}
	file_FDD.flush();
	file_FDD.close();
	//if (log_to_console_FDD) FDD_monitor.log("Buffer flushed to Disk#" + to_string(selected_drive + 1) + "(" + filename_FDD.at(selected_drive) + ")");
}

//FDD controller
uint8 FDD_Ctrl::read_port(uint16 port)
{
	//if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 12);
	//cout << "FDD READ " << (int) port;
	//if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 7);
	if (port == 0x3F0)
	{
		if (log_to_console_FDD) cout << "FDD READ from port 3f0" << endl;
		//step_mode = 1; log_to_console = 1;
	}

	if (port == 0x3F1)
	{
		uint8 out = 0;
		out = out | ((motors_pin_enabled & 3) | ((motors_pin_enabled >> 2) & 3)); //биты моторов
		out = out | ((selected_drive & 1) << 5);
		out = out | (3 << 6);
		if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 11);
		if (log_to_console_FDD) cout << "FDD read STATUS REG B <- " << (bitset<8>)(out) << endl;
		if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 7);
		return out;
	}

	if (port == 0x3F2)
	{
		//Digital out register
		Digital_output_register = (motors_pin_enabled << 4) | (DMA_enabled << 3) | selected_drive;
		if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 11);
		if (log_to_console_FDD) cout << "READ DOR: MOTORS[" << (bitset<4>)(motors_pin_enabled) << "] DMA = " << (int)DMA_enabled << " SELDRV = " << (int)selected_drive << endl;
		if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 7);
		return Digital_output_register;
	}

	if (port == 0x3F3)
	{
		//Tape drive register
		if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 11);
		if (log_to_console_FDD) cout << "READ TDR: " << (int)(Tape_drive_register & 3) << endl;
		if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 7);
		return Tape_drive_register & 3;
	}

	if (port == 0x3F4)
	{
		//main status register
		Main_status_register = (RQM * 128) | (DIO * 64) | (DMA_ON * 32) | (CMD_BUSY * 16) | (FDD_busy_bits & 15); // 32 - non DMA mode
		//if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 11);
		//if (log_to_console_FDD) cout << "READ STATUS RQM=" << (int)RQM << " DIO=" << (int)DIO << " DMA_ON=" << (int)DMA_ON << " CMD_BUSY="<< (int)CMD_BUSY << " DRV_BUSY=" << (bitset<4>)FDD_busy_bits << endl;
		//if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 7);
		return Main_status_register;
	}

	//FIFO port
	if (port == 0x3F5)
	{

		//чтение результатов операции
		if (drv_state == FDD_states::read_result)
		{
			if (Data_fifo.size())
			{
				//if (log_to_console_FDD) cout << "FDD data buffer READ " << (int)Data_fifo.at(0) << " remain=" << (int)(results_left - 1) <<  endl;
				uint8 out = Data_fifo.at(0);
				Data_fifo.erase(Data_fifo.begin());
				results_left--;
				if (results_left == 0)
				{
					drv_state = FDD_states::idle;
					DIO = 0; //требуем записи
					CMD_BUSY = 0; //констроллер свободен
					//if (log_to_console_FDD) cout << "READ RESULT -> IDLE" << endl;
				}
				if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 7);
				return out;
			}
			else
			{
				//if (log_to_console_FDD) cout << "FDD data buffer READ - EMPTY! STATE -> IDLE" << endl;
				DIO = 0; //требуем записи
				CMD_BUSY = 0; //констроллер свободен
				drv_state = FDD_states::idle;
				if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 7);
				return 0;
			}
		}

		//чтения данных команды READ без DMA (в разработке)
		if (drv_state == FDD_states::command_exec && command == 62)
		{
			if (out_buffer.size() > 0)
			{
				//выдаем число в порт
				uint8 out = out_buffer.at(0);
				out_buffer.erase(out_buffer.begin());
				//устанавливаем статус для прерывания
				sense_int_buffer.push_back(0b00100000);
				if (log_to_console_FDD) cout << "FIFO read out buffer " << (int)out << endl;
				if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 7);
				return out;
			}
			else
			{
				//устанавливаем статус ошибки для INT
				if (log_to_console_FDD) cout << "FIFO EMPTY " << endl;
				//sense_int_buffer.push_back(0b01100000 | selected_drive);
				if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 7);
				return 0;
			}

		}
		if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 7);
		return 0;
	}

	if (port == 0x3F7)
	{
		//digital input register
		if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 11);
		if (log_to_console_FDD) cout << "FDD digital input REG <- " << (int)Digital_input_register << endl;
		if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 7);
		return Digital_input_register;
	}
}
void FDD_Ctrl::write_port(uint16 port, uint8 data)
{
	//cout << "WRITE port " << hex << (int)port << " data " << (int)data << endl;
	//SetConsoleTextAttribute(hConsole, 12);
	//if (log_to_console_FDD || 1) FDD_monitor.log("FDD WRITE " + to_string(port));
	//SetConsoleTextAttribute(hConsole, 7);

	//выбор привода, разрешение DMA, управление моторами, перезагрузка
	if (port == 0x3F2)
	{
		//if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 11);
		//if (log_to_console_FDD) cout << "WRITE DOR MOTORS[" << (bitset<4>)(data >> 4) << "] DMA_EN=" << (int)((data >> 3) & 1) << " RESET=" << (int)((data >> 2) & 1) << " SELECT_DRV=" << (int)(data & 3) << endl;
		//if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 7);

		Digital_output_register = data; //можно убрать

		selected_drive = data & 3;
		DMA_enabled = (data >> 3) & 1;
		motors_pin_enabled = (data >> 4);
		if (log_to_console_FDD) cout << "FDD: SELECT DRIVE #" << (int)selected_drive << " DMA_EN=" << (int)DMA_enabled << " motor_pins=" << (bitset<4>(motors_pin_enabled)) << endl;
		//переключение в RESET
		if (drv_state == FDD_states::idle || drv_state == FDD_states::read_result)
		{
			if (((data >> 2) & 1) == 0)
			{
				drv_state = FDD_states::reset;
				if (log_to_console_FDD) cout << "FDD: state -> RESET" << endl;
			}
			return;
		}

		//обработка состояния перезагрузки
		if (drv_state == FDD_states::reset)
		{
			//если бит сброса не поменялся, то выходим
			if (!((data >> 2) & 1)) return;
			if (log_to_console_FDD)
			{
#ifdef DEBUG
				FDD_monitor.log("RESET Ctrl");
#endif
				cout << "FDD RESET complete" << endl;
			}

			//производим перезагрузку
			Tape_drive_register = 0;
			Main_status_register = 0;
			Data_fifo.clear();
			out_buffer.clear();
			Digital_input_register = 0;
			selected_drive = 0;
			RQM = 1;
			IC = 0;
			SE = 0;
			//установка буфера для sence INT
			sense_int_buffer.clear();
			sense_int_buffer.push_back(0b11000000);
			sense_int_buffer.push_back(0b11000001);
			sense_int_buffer.push_back(0b11000010);
			sense_int_buffer.push_back(0b11000011);
			DIO = 0; //требуем записи
			//установка переменных
			drv_state = FDD_states::command_exec;
			CMD_BUSY = 1;
			command = 255;		//заглушка для задержки
			sleep_counter = 3;	//задержка
			return;
		}

		//любые другие состояния ничего не меняют
	}

	//Enhanced floppy mode 2. Непонятное назначение.
	if (port == 0x3F3)
	{
		if (drv_state == FDD_states::idle) Tape_drive_register = data;
	}

	//Data rate + soft reset
	if (port == 0x3F4)
	{
		//if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 11);
		//if (log_to_console_FDD) cout << "WR DSR softres=" << (int)((data >> 7) & 1) << " lowPWR=" << (int)((data >> 6) & 1) << " precomp=" << (int)((data >> 2) & 7) << " D_rate=" << (int)((data) & 3) << endl;
		//if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 7);

		Data_rate_select_register = data & 0b01111111;
		data_rate_sel = data & 3; //отдельная переменная для скорости

		if ((data >> 7) & 1) //програмная перезагрузка
		{
			//то же самое, что и перезагрузка, только без смены состояний
			if (log_to_console_FDD) cout << "Soft reset" << endl;
			Tape_drive_register = 0;
			Main_status_register = 0;
			Data_fifo.clear();
			Digital_input_register = 0;
			selected_drive = 0;
			RQM = 1;
			CMD_BUSY = 1;
			IC = 0;
			SE = 0;
			//sense_int_buffer.push_back(0b11000000|selected_drive); //перезагружаем один привод ?
			out_buffer.clear();
			sense_int_buffer.clear();
			sense_int_buffer.push_back(0b11000000);
			sense_int_buffer.push_back(0b11000001);
			sense_int_buffer.push_back(0b11000010);
			sense_int_buffer.push_back(0b11000011);
			drv_state = FDD_states::idle;
			return;
			//прошлая версия
			drv_state = FDD_states::command_exec;
			command = 255; //заглушка для задержки
			sleep_counter = 3;
			return;
		}
	}

	//FIFO порт для ввода команд
	if (port == 0x3F5)
	{
		//data FIFO
		//if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 11);
		//if (log_to_console_FDD) cout << "FDD data buffer(FIFO) WRITE " << (int)data << endl;
		//if (log_to_console_FDD) cout << "FDD STATE = " << (int)drv_state << endl;
		//if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 7);

		if (drv_state == FDD_states::idle)
		{
			//распознаем команду
			if ((data & 31) == 3)
			{
				//команда Specify
				command = 3;
				drv_state = FDD_states::enter_param;//перекл на выполение
				//cout << "FDD command 3(Specify)  -> WAIT PARAMs" << endl;
				//DMA_ON = false;
				CMD_BUSY = 1;
				params_left = 2;
				return;
			}

			if ((data & 31) == 4)
			{
				//команда Sense drive status
				command = 4;
				drv_state = FDD_states::enter_param;//перекл на выполение
				//cout << "FDD command 4  -> WAIT PARAMs" << endl;
				//DMA_ON = false;
				CMD_BUSY = 1;
				params_left = 1;
				return;
			}

			if ((data & 31) == 5)
			{
				//WRITE
				command = 5;
				drv_state = FDD_states::enter_param;//перекл на ввод параметров
				//cout << "FDD command 15  -> WAIT PARAMs" << endl;
				CMD_BUSY = 1;
				//DMA_ON = false;
				params_left = 8;
				return;
			}

			if ((data & 31) == 6)
			{
				//READ
				command = 6;
				MFM_mode[selected_drive] = (data >> 6) & 1;
				MT[selected_drive] = (data >> 7) & 1;
				drv_state = FDD_states::enter_param;//перекл на ввод параметров
				//cout << "FDD command 15  -> WAIT PARAMs" << endl;
				CMD_BUSY = 1;
				//DMA_ON = false;
				params_left = 8;
				return;
			}

			if ((data & 31) == 7)
			{
				//Recalibrate
				command = 7;
				drv_state = FDD_states::enter_param;//перекл на ввод параметров
				//cout << "FDD command 7  -> WAIT PARAMs" << endl;
				CMD_BUSY = 1;
				//DMA_ON = false;
				params_left = 1;
				return;
			}

			if ((data & 31) == 8)
			{
				//команда Sense interrupt status
				command = 8;
				drv_state = FDD_states::command_exec;//перекл на выполение
				//cout << "FDD command 8  -> RUN" << endl;
				//IC = 3;
				//DMA_ON = false;
				//SE = 0;
				CMD_BUSY = 1;
				return;
			}

			if ((data & 31) == 13)
			{
				//FORMAT A TRACK
				command = 13;
				MFM_mode[selected_drive] = (data >> 6) & 1;
				drv_state = FDD_states::enter_param;//перекл на ввод параметров
				//cout << "FDD command 15  -> WAIT PARAMs" << endl;
				CMD_BUSY = 1;
				//DMA_ON = false;
				params_left = 5;
				return;
			}

			if ((data & 31) == 15)
			{
				//Seek
				command = 15;
				drv_state = FDD_states::enter_param;//перекл на ввод параметров
				//cout << "FDD command 15  -> WAIT PARAMs" << endl;
				CMD_BUSY = 1;
				//DMA_ON = false;
				params_left = 2;
				return;
			}



			if (log_to_console_FDD) cout << "FDD new command = " << (int)data << endl;
			step_mode = 1; log_to_console = 1;
		}

		if (drv_state == FDD_states::enter_param)
		{
			//ввод параметров
			//cout << "FDD cmd= " << (int)command << " params IN left=" << (int)(params_left-1) << endl;
			Data_fifo.push_back(data);
			params_left--;
			if (!params_left)
			{
				drv_state = FDD_states::command_exec;
				//if (log_to_console_FDD) cout << "FDD params END->RUN command " << (int)command << endl;
				return;
			}
		}

		if (drv_state == FDD_states::command_exec && command == 13)  //получение параметров в процессе форматирования
		{
			//добавляем полученные данные в буфер
			Data_fifo.push_back(data);
		}

		//скорее всего этого не произойдет
		if (drv_state == FDD_states::seek)
		{
			if ((data & 31) == 8)  //выполняется SEEK + прерывание командой SIS
			{
				command = 8; //переключение на Sense interrupt status
				IC = 1; //признак завершения Seek прерыванием
				SE = 1;
				sense_int_buffer.push_back(0b11000000 | selected_drive);
				CMD_BUSY = 1;
				int_ctrl.request_IRQ(6); //не факт, что нужно, проверить
#ifdef DEBUG
				if (log_to_console_FDD) FDD_monitor.log("SEEK interrupted by SIS. Error set.");
#endif
				drv_state = FDD_states::command_exec;
				return;
			}
		}

		//скорее всего этого не произойдет
		if (drv_state == FDD_states::recalibrate)
		{
			if ((data & 31) == 8)  //прерывание командой SIS
			{
				command = 8; //переключение на Sense interrupt status
				IC = 1; //признак завершения Seek прерыванием
				SE = 1;
				sense_int_buffer.push_back(0b11000000 | selected_drive);
				CMD_BUSY = 1;
				int_ctrl.request_IRQ(6); //не факт, что нужно, проверить
#ifdef DEBUG
				if (log_to_console_FDD) FDD_monitor.log("RECALIBRATE interrupted by SIS. Error set.");
#endif
				drv_state = FDD_states::command_exec;
				return;
			}
		}

	}
	if (port == 0x3F7)
	{
		//digital input register
		if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 11);
		if (log_to_console_FDD) cout << "FDD: write digital input REG = " << (int)data << endl;
		if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 7);
		Digital_input_register = data;
	}

}
void FDD_Ctrl::sync()
{
	if (sleep_counter != 0)
	{
		//задержка команды
		sleep_counter--;
		return;
	}

	if (drv_state == FDD_states::command_exec)
	{
		//обрабатываем команду
		//в буфере FIFO только параметры!
		//CMD_BUSY = 0;

		if (command == 3)  //Specify
		{
			//из нужны параметров задает только использование DMA
			//фазы отдачи результатов нет
			if (Data_fifo.at(1) & 1) DMA_ON = false;
			else DMA_ON = true;
			Data_fifo.clear();
			RQM = 1; //хост может читать или писать
			DIO = 0; //требуем записи
			CMD_BUSY = 0;//сразу ставим статус свободен
			drv_state = FDD_states::idle;
			FDD_monitor.log("CMD 3 [Specify] DMA_ON=" + to_string(DMA_ON));
			return;
		}

		if (command == 4)    //Sense drive status
		{
			//отдает признак запрета записи и признак нулевой дорожки
			uint8 param = Data_fifo.at(0);
			//параметры запроса
			uint8 drv = param & 3;
			uint8 head = (param >> 2) & 1;
			//определяем результат
			bool tr_0 = false;
			if (C_cylinder[drv] == 0) tr_0 = true; //головка на цилиндре 0 
			uint8 result = 0 * 128 | write_protect[drv] * 64 | 1 * 32 | tr_0 * 16 | 1 * 8 | H_head[drv] * 4 | drv;  //доделать чтобы отдавались статусы отдельных дисков - DONE
			FDD_monitor.log("CMD 4 [Sense drv stat] drv=" + to_string(drv) + " head= " + to_string(H_head[drv]) + " WR_protect = " + to_string(write_protect[drv]) + " TRC0 = " + to_string(tr_0));
			Data_fifo.clear();
			Data_fifo.push_back(result);
			drv_state = FDD_states::read_result;
			results_left = 1;
			RQM = 1; //хост может читать или писать
			DIO = 1; //требуем чтения
			return;
		}

		if (command == 5)    //WRITE
		{
			selected_drive = (Data_fifo.at(0)) & 3;
			//H_head[selected_drive] = (Data_fifo.at(0) >> 2) & 1;
			C_cylinder[selected_drive] = Data_fifo.at(1);
			H_head[selected_drive] = Data_fifo.at(2); //почему-то второй раз указано
			R_sector[selected_drive] = Data_fifo.at(3); //номер сектора для чтения

			SectorSizeCode[selected_drive] = Data_fifo.at(4); //кол-во байт в секторе (см. таблицу)
			Sector_size[selected_drive] = pow(2, SectorSizeCode[selected_drive]) * 128;
			if (Sector_size[selected_drive] > 16384) Sector_size[selected_drive] = 16384;

			EOT = Data_fifo.at(5); //номер последнего сектора на дорожке
			GPL = Data_fifo.at(6); //гэп 3 между секторами
			DTL = Data_fifo.at(7); //специальный размер сектора если Sector_size = 128 (факт < 128), иначе игнор
			if (Sector_size[selected_drive] == 128 && DTL != 0) Sector_size[selected_drive] = DTL;
			FDD_monitor.log("CMD 5 [WRITE] head=" + to_string(H_head[selected_drive]) + " CYL=" + to_string(C_cylinder[selected_drive]) + " DRV=" + to_string(selected_drive) + " SEC_begin = " + to_string(R_sector[selected_drive]) + " SEC_size = " + to_string(Sector_size[selected_drive]) + " MT = " + to_string(MT[selected_drive]) + " EOT = " + to_string(EOT));
			Data_fifo.clear();

			//делаем выгрузку 

			if (DMA_ON && DMA_enabled) //передача в режиме DMA
			{
				//step_mode = 1;
				//if (log_to_console_FDD) FDD_monitor.log("DMA ON...  ");

				//заполнение буфера
				//начальный индекс массива
				if (!R_sector[selected_drive]) //не должно быть ноля
				{
					R_sector[selected_drive] = 1;
					if (log_to_console_FDD) cout << "R_sector = 0!!!";
				}

				write_ptr = real_address(selected_drive, H_head[selected_drive], C_cylinder[selected_drive], R_sector[selected_drive], Sector_size[selected_drive]); //начало записи
				write_ptr_end = write_ptr + Sector_size[selected_drive] * (EOT - R_sector[selected_drive] + 1);
				if (log_to_console_FDD) FDD_monitor.log("write drv#" + to_string(selected_drive) + " addresses from " + to_string(write_ptr % (2880 * 1024)) + " to " + to_string(write_ptr_end % (2880 * 1024)));

				//ждем завешения чтения
				command = 51; //запись с ДМА
				RQM = 0; //доступ запрещен
				DIO = 1; //требуем чтения 
				CMD_BUSY = 1;
				FDD_busy_bits = (1 << selected_drive); //ставим признак занятости
				EOP = 0; //сброс сигнала конца процесс (на всякий)
				return;
			}
			else
			{
				//эта ветка не должна работать
				//передача в обычном режиме опроса

				//скопирована с чтения
#ifdef DEBUG
				if (log_to_console_FDD) FDD_monitor.log("DMA OFF... load buffer ");
#endif
				//ждем завешения чтения
				command = 52; //запись без ДМА

				for (int i = R_sector[selected_drive]; i < EOT; ++i)
				{
					//грузим данные секторов в буфер
					for (int j = 0; j < Sector_size[selected_drive]; ++j)
					{
						for (int h = 0; h < 2; ++h)
						{
							uint32 addr = (C_cylinder[selected_drive] * 9 * 2 + i * 2) * 512 + j * 2 + h; //адрес байта на диске 512 - размер сектора по умолчанию
							//out_buffer.push_back(sector_data[addr]);
						}
					}
				}
				R_sector[selected_drive]++;
				DIO = 1; //требуем четения
				CMD_BUSY = 1;
				FDD_busy_bits = FDD_busy_bits | (1 << selected_drive); //ставим признак занятости
				//if (log_to_console_FDD) cout << " write BUFFER ready" << endl;
				step_mode = 1;
			}
			return;
		}

		if (command == 51)    //WRITE + DMA
		{
			if ((in_buffer.size() < (write_ptr_end - write_ptr)) && !EOP)
			{
				//if (log_to_console_FDD) FDD_monitor.log("DMA Req Ch12(FDD)");
				uint8 dma_res = dma_ctrl.request_hw_dma(2);
				//если ответ "1", значит ошибка контроллера DMA
				//if (log_to_console_FDD) FDD_monitor.log("no DMA: out_buffer = " + to_string(out_buffer.size()/512) + " sectors left (" + to_string(out_buffer.size()) + " bytes)");
				//R_sector -= out_buffer.size() / 512; //откатываем текущий сектор
				//end_sec -= out_buffer.size() / 512;
				//int_ctrl.request_IRQ(6);
				//out_buffer.clear(); //вызываем DMA при отказе очищаем буфер
				//if (log_to_console_FDD) FDD_monitor.log("DMA ret 0");

				//возможно, добавить ошибку при очистке буфера
			}
			else
			{
				if (EOP)
				{
					if (log_to_console_FDD) cout << "FDD WRITE, EOP from DMA" << endl;
					//окончание за счет конца слов в DMA
					EOP = 0; //сброс флага конца процесса
				}
				//пересчет секторов
				//R_sector -= out_buffer.size() / 512; 
				end_sec = R_sector[selected_drive] + in_buffer.size() / Sector_size[selected_drive] - 1;//пересчитываем конечный сектор
				//if (log_to_console_FDD) FDD_monitor.log("end_sector calc = " + to_string((write_ptr + in_buffer.size()) / Sector_size) + " buf_size = " + to_string(in_buffer.size()));
				//if (log_to_console_FDD) FDD_monitor.log("cleared buffer = " + to_string(out_buffer.size() / 512) + " sectors left (" + to_string(out_buffer.size()) + " bytes)");
				//out_buffer.clear(); //очищаем буфер

				if (log_to_console_FDD) cout << "FDD WRITE ptr from " << dec << (int)write_ptr << " to " << (int)write_ptr_end << " buff_size =" << (int)in_buffer.size() << endl;
				if (log_to_console_FDD) FDD_monitor.log("Writing " + to_string(in_buffer.size() / Sector_size[selected_drive]) + " sectors from " + to_string((write_ptr - selected_drive * 2880 * 1024) / Sector_size[selected_drive]) + " to " + to_string(((write_ptr - selected_drive * 2880 * 1024) + in_buffer.size() - 1) / Sector_size[selected_drive]));
				
				//пишем данные из буфера на дискету
				while ((write_ptr < write_ptr_end) && (in_buffer.size() != 0))
				{
					sector_data[write_ptr] = in_buffer.at(0); //переносим данные из буфера
					write_ptr++;
					in_buffer.erase(in_buffer.begin());
				}
				in_buffer.clear(); //стираем буфер
				//if (end_sec == 164) step_mode = 1;
				//записываем итоги операции в FIFO и запрашиваем чтение
				Data_fifo.push_back(0b00000000 | (H_head[selected_drive] << 2) | selected_drive); // ST0
				//Data_fifo.push_back(0b11000000 | selected_drive); //error
				Data_fifo.push_back(0b00000000); // ST1
				Data_fifo.push_back(0b00000000); // ST2
				Data_fifo.push_back(C_cylinder[selected_drive]); // C
				Data_fifo.push_back(H_head[selected_drive]); // H
				Data_fifo.push_back(R_sector[selected_drive]); // R
				Data_fifo.push_back(SectorSizeCode[selected_drive]); // N заглушка
				results_left = 7;
				RQM = 1;
				DIO = 1; //требуем чтения
				CMD_BUSY = 1;
				FDD_busy_bits = FDD_busy_bits & ~(1 << selected_drive); //снимаем признак занятости
				int_ctrl.request_IRQ(6);
				//cout << " INT 6"; step_mode = 1;
				drv_state = FDD_states::read_result;
				flush_buffer(); //скидываем данные на диск
			}
			return;
		}

		if (command == 52)    //WRITE polling
		{
			//добавить код
			if (log_to_console_FDD) FDD_monitor.log("command 52, polling mode");
			int_ctrl.request_IRQ(6);
			return;
		}

		if (command == 6)    //READ
		{
			//cout << "READ!" << endl;
			//step_mode = 1;
			selected_drive = (Data_fifo.at(0)) & 3;
			//H_head[selected_drive] = (Data_fifo.at(0) >> 2) & 1;
			C_cylinder[selected_drive] = Data_fifo.at(1);
			H_head[selected_drive] = Data_fifo.at(2); //почему-то второй раз указано
			R_sector[selected_drive] = Data_fifo.at(3); //номер сектора для чтения

			SectorSizeCode[selected_drive] = Data_fifo.at(4); //кол-во байт в секторе (см. таблицу)
			Sector_size[selected_drive] = pow(2, SectorSizeCode[selected_drive]) * 128;
			if (Sector_size[selected_drive] > 16384) Sector_size[selected_drive] = 16384;

			uint8 EOT = Data_fifo.at(5); //номер последнего сектора на дорожке base 0
			uint8 GPL = Data_fifo.at(6); //гэп 3 между секторами
			uint8 DTL = Data_fifo.at(7); //специальный размер сектора если Sector_size = 128 (факт < 128), иначе игнор
			if (Sector_size[selected_drive] == 128 && DTL != 0) Sector_size[selected_drive] = DTL;
#ifdef DEBUG
			if (log_to_console_FDD) FDD_monitor.log("CMD 6 [READ] head=" + to_string(H_head[selected_drive]) + " CYL=" + to_string(C_cylinder[selected_drive]) + " DRV=" + to_string(selected_drive) + " SEC_begin = " + to_string(R_sector[selected_drive]) + " SEC_size = " + to_string(Sector_size[selected_drive]) + " MT = " + to_string(MT[selected_drive]) + " MFM = " + to_string(MFM_mode[selected_drive]) + " EOT = " + to_string(EOT));
			//if (log_to_console_FDD) cout << ("CMD 6 [READ] head=" + to_string(H_head[selected_drive]) + " CYL=" + to_string(C_cylinder[selected_drive]) + " DRV=" + to_string(selected_drive) + " SEC_begin = " + to_string(R_sector[selected_drive]) + " SEC_size = " + to_string(Sector_size[selected_drive]) + " MT = " + to_string(MT[selected_drive]) + " MFM = " + to_string(MFM_mode[selected_drive]) + " EOT = " + to_string(EOT)) << endl;
#endif
			//log_to_console = 1;
			Data_fifo.clear();

			//делаем выгрузку 

			if (DMA_ON && DMA_enabled) //передача в режиме DMA
			{
				//step_mode = 1;
				//if (log_to_console_FDD) FDD_monitor.log("DMA ON...  ");

				//заполнение буфера
				//начальный индекс массива
				if (!R_sector[selected_drive]) //не должно быть ноля
				{
					R_sector[selected_drive] = 1;
					if (log_to_console_FDD) cout << "R_sector = 0!!!";
				}

				//рассчитываем адреса точек начала и конца
				read_start = real_address(selected_drive, H_head[selected_drive], C_cylinder[selected_drive], R_sector[selected_drive], Sector_size[selected_drive]);
				read_end = real_address(selected_drive, H_head[selected_drive], C_cylinder[selected_drive], diskette_sectors[selected_drive], Sector_size[selected_drive]) + Sector_size[selected_drive];
				start_sec = (read_start - 2880 * 1024 * selected_drive) / Sector_size[selected_drive] + 1;
				end_sec = (read_end - 2880 * 1024 * selected_drive) / Sector_size[selected_drive];

				if (log_to_console_FDD) cout << "FDD READ: out_buffer data from " << (int)(read_start) << " to " << (int)read_end << " sec(" << (int)start_sec << " to " << (int)end_sec << ")" << endl;

				//ждем завешения чтения
				command = 61; //чтение с ДМА
				//if (log_to_console_FDD) cout << " -> command 61" << endl;
				RQM = 0; //доступ запрещен
				DIO = 1; //требуем чтения 
				CMD_BUSY = 1;
				FDD_busy_bits = (1 << selected_drive); //ставим признак занятости
				EOP = 0; //сброс сигнала конца процесс (на всякий)
				return;
			}
			else
			{
				//эта ветка не должна работать
				//передача в обычном режиме опроса
#ifdef DEBUG
				if (log_to_console_FDD) FDD_monitor.log("DMA OFF... load buffer ");
#endif
				//ждем завешения чтения
				command = 62; //чтение без ДМА

				for (int i = R_sector[selected_drive]; i < EOT; ++i)
				{
					//грузим данные секторов в буфер
					for (int j = 0; j < Sector_size[selected_drive]; ++j)
					{
						for (int h = 0; h < 2; ++h)
						{
							uint32 addr = (C_cylinder[selected_drive] * 9 * 2 + i * 2) * 512 + j * 2 + h; //адрес байта на диске 512 - размер сектора по умолчанию
							out_buffer.push_back(sector_data[selected_drive * 2880 * 1024 + addr]);
						}
					}
				}
				R_sector[selected_drive]++;
				DIO = 1; //требуем четения
				CMD_BUSY = 1;
				FDD_busy_bits = FDD_busy_bits | (1 << selected_drive); //ставим признак занятости
				//if (log_to_console_FDD) cout << " read BUFFER ready" << endl;
				step_mode = 1;
				return;
			}
		}

		if (command == 61)    //READ + DMA
		{
			
			//monitor.debug_mess_1 = "st=" + to_string(read_start) + " en=" + to_string(read_end) + " EOP " + to_string(EOP);
			//замедление
			if (step_mode) std::this_thread::sleep_for(std::chrono::milliseconds(300));
			if ((read_start < read_end) && !EOP)
			{
				//в буфере есть данные и DMA их принимает
				uint8 dma_res = dma_ctrl.request_hw_dma(2);
			}
			else
			{
				
				if (EOP)
				{
					//окончание за счет конца передачи DMA
					//if (log_to_console_FDD) cout << "FDD EOP, bytes left " << (int)(read_end - read_start) << endl;
					EOP = 0; //сброс флага конца процесса
				}
				//else cout << "FDD all bytes read!" << endl;
				
				//пересчет секторов
				R_sector[selected_drive] -= out_buffer.size() / 512; //откатываем текущий сектор
				end_sec -= out_buffer.size() / 512;
				//if (log_to_console_FDD) FDD_monitor.log("cleared buffer = " + to_string(out_buffer.size() / 512) + " sectors left (" + to_string(out_buffer.size()) + " bytes)");

				if (log_to_console_FDD) FDD_monitor.log("Finished read sectors from " + to_string(start_sec) + " to " + to_string(end_sec));
				//cout << ("Finished read sectors from " + to_string(start_sec) + " to " + to_string(end_sec)) << endl;

				//if (end_sec == 164) step_mode = 1;
				//записываем итоги операции в FIFO и запрашиваем чтение
				Data_fifo.push_back(0b00000000 | (H_head[selected_drive] << 2) | selected_drive); // ST0
				//Data_fifo.push_back(0b11000000 | selected_drive); //error
				Data_fifo.push_back(0b00000000); // ST1
				Data_fifo.push_back(0b00000000); // ST2
				Data_fifo.push_back(C_cylinder[selected_drive]); // C
				Data_fifo.push_back(H_head[selected_drive]); // H
				Data_fifo.push_back(R_sector[selected_drive]); // R
				Data_fifo.push_back(SectorSizeCode[selected_drive]); // N заглушка
				results_left = 7;
				RQM = 1; //хост может читать или писать
				DIO = 1; //требуем чтения от хоста
				CMD_BUSY = 1;
				DMA_ON = 0; //отключаем DMA для выдачи результатов через  порт
				FDD_busy_bits = FDD_busy_bits & ~(1 << selected_drive); //снимаем признак занятости
				out_buffer.clear();
				//cout << " INT 6 after read" << endl;
				//step_mode = 1;
				int_ctrl.request_IRQ(6);
				drv_state = FDD_states::read_result;
			}
			return;
		}

		if (command == 62)    //READ polling
		{
			//добавить код
			if (log_to_console_FDD) FDD_monitor.log("command 62, polling mode");
			int_ctrl.request_IRQ(6);
			return;
		}

		if (command == 8)  //Sense INT status
		{
			if (drv_state == FDD_states::idle) CMD_BUSY = 0;
			Data_fifo.clear();
			uint8 data_out;
			if (sense_int_buffer.size())
			{
				data_out = sense_int_buffer.at(0);
				sense_int_buffer.erase(sense_int_buffer.begin());
				selected_drive = data_out & 3;
			}
			else
			{
				data_out = 0b11000000 | selected_drive; //ошибка запроса SENSE
			}

			Data_fifo.push_back(data_out); //ST0    80?
			//if (log_to_console_FDD) FDD_monitor.log("Sense INT -> FIFO PUSH (" + to_string(Data_fifo.at(Data_fifo.size() - 1)) + ") ");
			IC = 0; //сбрасываем код завершения
			SE = 0;
			Data_fifo.push_back(C_cylinder[selected_drive]); //PCN - № цилиндра под головкой
			//if (log_to_console_FDD) FDD_monitor.log("(" + to_string(Data_fifo.at(Data_fifo.size() - 1)) + ") ");
			//if (log_to_console_FDD) FDD_monitor.log("CMD 8 (Sense INT status) -> " + to_string(Data_fifo.at(Data_fifo.size() - 2)) + " & " + to_string(Data_fifo.at(Data_fifo.size() - 1)));
			FDD_monitor.log("CMD 8 [Sense INT status] -> " + int_to_bin(Data_fifo.at(Data_fifo.size() - 2)) + " & " + int_to_bin(Data_fifo.at(Data_fifo.size() - 1)) + " drv# " + to_string(selected_drive) + " left(" + to_string(sense_int_buffer.size()) + ")");
			drv_state = FDD_states::read_result;
			results_left = 2;
			//DMA_enabled = 0;
			DIO = 1; //требуем чтения
			//int_ctrl.request_IRQ(6);
			return;
		}

		if (command == 7) //Recalibrate
		{
			//перемещаемся на дорожку 0 указанного диска
			selected_drive = Data_fifo.at(0) & 3;
			if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 11);
			FDD_monitor.log("CMD 7 [Recalibrate] drv=" + to_string((selected_drive & 3)));
			if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 7);
			Data_fifo.clear();
			C_cylinder[selected_drive] = 0; //перейти на цилиндр 0
			H_head[selected_drive] = 0;
			drv_state = FDD_states::recalibrate;
			DIO = 0; //требуем записи
			CMD_BUSY = 1;
			FDD_busy_bits = (1 << selected_drive); //ставим признак занятости приводов
			sleep_counter = 5; //период занятости
			//DMA_enabled = 0;
			//int_ctrl.request_INT(6);
			return;
		}

		if (command == 13)  //format
		{
			//step_mode = 1;
			static bool f_started = 0;//признак начала процесса форматирования
			static uint8 datapattern = 0;
			static uint16 sectors_to_format = 0;

			if (f_started == 0)  //начало процесса
			{
				selected_drive = Data_fifo.at(0) & 3;    //выбор диска
				H_head[selected_drive] = ((Data_fifo.at(0) >> 2) & 1);   //меняем головку
				SectorSizeCode[selected_drive] = Data_fifo.at(1); //кол-во байт в секторе (см. таблицу)
				Sector_size[selected_drive] = pow(2, SectorSizeCode[selected_drive]) * 128;
				if (Sector_size[selected_drive] > 16384) Sector_size[selected_drive] = 16384;

				sectors_to_format = Data_fifo.at(2); //кол-во секторов для форматирования
				datapattern = Data_fifo.at(4); //шаблон для заполнения

				Data_fifo.erase(Data_fifo.begin());  //затираем данные
				Data_fifo.erase(Data_fifo.begin());
				Data_fifo.erase(Data_fifo.begin());
				Data_fifo.erase(Data_fifo.begin());
				Data_fifo.erase(Data_fifo.begin());
				DIO = 0; //требуем записи
				CMD_BUSY = 0;
				RQM = 1;
				FDD_monitor.log("CMD D [FORMAT] drv = " + to_string(selected_drive) + " head = " + to_string(H_head[selected_drive]) + " sec_size = " + to_string(Sector_size[selected_drive]) + " MFM=" + to_string(MFM_mode[selected_drive]) + " SC = " + to_string(sectors_to_format) + " patt = " + to_string(datapattern));
				f_started = 1;
			}

			if (f_started)
			{
				//идет процесс форматирования
				//cout << "Input buf(" << (int)in_buffer.size() << ")";
				//for (int y = 0; y < Data_fifo.size(); y++) cout << (int)Data_fifo.at(y) << " - ";
				//cout << endl;
				//std::this_thread::sleep_for(std::chrono::milliseconds(100));

				//получаем данные от хоста по секторам
				if (in_buffer.size() >= 4)  //в буфере есть данные
				{
					SectorSizeCode[selected_drive] = in_buffer.at(3); //кол-во байт в секторе (см. таблицу)
					Sector_size[selected_drive] = pow(2, SectorSizeCode[selected_drive]) * 128;
					if (Sector_size[selected_drive] > 16384) Sector_size[selected_drive] = 16384;
					uint32 address = real_address(selected_drive, in_buffer.at(1), in_buffer.at(0), in_buffer.at(2), Sector_size[selected_drive]);//реальный адрес
					//перезаписываем сектор
					cout << "format sector address " << dec << (int)address << " to " << (int)(address + 512) << " S_size = " << (int)Sector_size[selected_drive] << " H=" << (int)in_buffer.at(1) << " Cyl=" << (int)in_buffer.at(0) << " Sec=" << (int)in_buffer.at(2) << endl;
					bool write_error = 0; //флаг оштбки
					for (int j = 0; j < Sector_size[selected_drive]; j++)
					{
						if (write_to_disk(address + j, datapattern))
						{
							write_error = 1;  //запись на диск + флаг ошибки
							sectors_to_format = 1; //обнуляем сектора
							break;
						}
					}
					if (log_to_console_FDD) FDD_monitor.log("sector C/S/H(" + to_string(in_buffer.at(0)) + "/" + to_string(in_buffer.at(2)) + "/" + to_string(in_buffer.at(1)) + ") formatted");
					if (log_to_console_FDD && write_error) FDD_monitor.log("write error!");
					sectors_to_format--;
					in_buffer.erase(in_buffer.begin());  //затираем данные
					in_buffer.erase(in_buffer.begin());
					in_buffer.erase(in_buffer.begin());
					in_buffer.erase(in_buffer.begin());

					if (sectors_to_format == 0)
					{
						f_started = 0;
						//форматирование закончено
						if (log_to_console_FDD) FDD_monitor.log("track formatted");
						//записываем итоги операции в FIFO и запрашиваем чтение
						uint8 out = 0b00000000 | (H_head[selected_drive] << 2) | selected_drive;
						if (write_error) out = out || 0b01000000; //добавляем бит ошибки
						Data_fifo.push_back(out); // ST0
						out = 0b00000000;
						if (write_error) out = out || 0b00000010; //добавляем бит ошибки
						//Data_fifo.push_back(0b11000000 | selected_drive); //error
						Data_fifo.push_back(out); // ST1
						Data_fifo.push_back(0b00000000); // ST2
						Data_fifo.push_back(0); // заглушка
						Data_fifo.push_back(0); // заглушка
						Data_fifo.push_back(0); // заглушка
						Data_fifo.push_back(0); // заглушка
						results_left = 7;
						RQM = 1;
						DIO = 1; //требуем чтения
						CMD_BUSY = 1;
						DMA_ON = 0; //отключаем DMA
						FDD_busy_bits = FDD_busy_bits & ~(1 << selected_drive); //снимаем признак занятости
						int_ctrl.request_IRQ(6);
						drv_state = FDD_states::read_result;
						//if (log_to_console_FDD) cout << "FORMAT -> read result" << endl;
						flush_buffer(); //сбрасываем данные на реальный накопитель
					}
				}
				else
				{
					if (DMA_enabled && DMA_ON) uint8 dma_res = dma_ctrl.request_hw_dma(2);
				}
			}
			return;
		}

		if (command == 15)  //Seek
		{
			selected_drive = Data_fifo.at(0) & 3;    //выбор диска
			C_cylinder[selected_drive] = Data_fifo.at(1);            //переходим на нужный цилиндр
			H_head[selected_drive] = ((Data_fifo.at(0) >> 2) & 1);   //меняем головку
			FDD_monitor.log("CMD F [SEEK] drv=" + to_string(selected_drive) + " head= " + to_string((int)H_head) + " cyl=" + to_string((int)C_cylinder));
			Data_fifo.clear();
			drv_state = FDD_states::seek;
			DIO = 0; //требуем записи
			CMD_BUSY = 1;
			FDD_busy_bits = (1 << selected_drive); //ставим признак занятости
			sleep_counter = 10; //период занятости
			//DMA_enabled = 0;
			//int_ctrl.request_INT(6);
			return;
		}

		//команда для выхода из задержки после перезагрузки
		if (command == 255)
		{
			if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 11);
			if (log_to_console_FDD) cout << "FDD: cmd 255 (wait)" << endl;
			if (log_to_console_FDD) SetConsoleTextAttribute(hConsole, 7);
			CMD_BUSY = 0;
			DIO = 0; //требуем записи
			drv_state = FDD_states::idle; //возврат в состояние простоя
			DMA_ON = DMA_enabled;
			int_ctrl.request_IRQ(6);  //выдает ошибку если не включить
			return;
		}

		//появление новой команды
		cout << "FDD new command! (" << (int)command << ")" << endl;
		step_mode = 1; log_to_console = 1;
	}

	if (drv_state == FDD_states::recalibrate)
	{
		//если не было прерывания завершаем корректно
		SE = 1;
		sense_int_buffer.clear();
		if (selected_drive >= active_drives || file_size[selected_drive] == 0)
		{
			//если выбран неподключенный дисковод
			IC = 1;
			sense_int_buffer.push_back(0b10100000 | selected_drive); //статус с ошибкой
		}
		else
		{
			IC = 0; //признак завершения
			sense_int_buffer.push_back(0b00100000 | selected_drive); //статус прерывания, нужно
		}
		//CMD_BUSY = 0;
		FDD_busy_bits = 0; //снимаем признак занятости
		if (log_to_console_FDD) cout << "FDD#" << (int)selected_drive << " Recalibrate complete" << endl;
		drv_state = FDD_states::idle;
		int_ctrl.request_IRQ(6); //прерывание нужно
		return;
	}

	if (drv_state == FDD_states::seek)
	{
		IC = 0; //признак завершения
		SE = 1; //seek end (для операции seek или recalibrate)
		if (selected_drive < active_drives && file_size[selected_drive] > 0)
		{
			if (C_cylinder[selected_drive] < diskette_cylinders[selected_drive])
			{
				//все ок
				sense_int_buffer.push_back(0b00100000 | selected_drive); //статус прерывания, нужно
				IC = 0;
			}
			else
			{
				//ошибка
				cout << "too many cyl = " << dec << (int)C_cylinder[selected_drive] << endl;
				C_cylinder[selected_drive] = diskette_cylinders[selected_drive] - 1;
				sense_int_buffer.push_back(0b10100000 | selected_drive); //статус прерывания, нужно
				IC = 2;
			}
		}
		else
		{
			//выбран несуществующий дисковод
			cout << "FDD not exist" << endl;
			IC = 1; //ошибка
			sense_int_buffer.push_back(0b10100000 | selected_drive); //статус прерывания, нужно
			IC = 2;
		}

		//CMD_BUSY = 0;
		FDD_busy_bits = 0; //ставим признак занятости = 0
		if (log_to_console_FDD) cout << "drv #" << (int)selected_drive << " Seek complete cyl=" << dec << (int)C_cylinder[selected_drive] << endl;
		drv_state = FDD_states::idle;
		int_ctrl.request_IRQ(6); //прерывание нужно
		return;
	}

	if (drv_state == FDD_states::read_result)
	{
		//CMD_BUSY = 0; //снимаем бит занятости после выгрузки результатов
	}

	if (drv_state == FDD_states::idle)
	{
		RQM = 1; //хост может читать или писать
		DIO = 0; //требуем запись от хоста
		CMD_BUSY = 0;
		//DMA_ON = 0; //отключаем DMA
	}

}
void FDD_Ctrl::load_diskette(uint32 address, uint8 data)
{
	//загружаем данные в массив
	sector_data[address] = data;
}
string FDD_Ctrl::get_state(uint8* ptr)
{
	//заполняем массив по ссылке
	*ptr = CMD_BUSY;
	*(ptr + 1) = selected_drive; // selected_drive;
	*(ptr + 2) = DMA_enabled;
	*(ptr + 3) = DMA_ON;
	*(ptr + 4) = motors_pin_enabled;
	*(ptr + 5) = FDD_busy_bits;

	//управление головкой
	*(ptr + 6) = C_cylinder[selected_drive];	//текущий цилиндр
	*(ptr + 7) = R_sector[selected_drive];		//текущий сектор
	*(ptr + 8) = H_head[selected_drive];		//текущая сторона дискеты
	*(ptr + 9) = DIR_control[selected_drive];	//направление поиска 1 - к центру
	*(ptr + 10) = MFM_mode[selected_drive];		//0 - одинарная плотность, 1 - двойная
	*(ptr + 11) = MT[selected_drive];			//multi-track

	//отдаем состояние
	switch (drv_state)
	{
	case FDD_states::idle:
		return "IDLE   ";
	case FDD_states::reset:
		return "RESET  ";
	case FDD_states::enter_param:
		return "PAR_ENT";
	case FDD_states::command_exec:
		return "CMD[" + to_string(command) + "]";
	case FDD_states::read_result:
		return "RD_RES ";
	case FDD_states::seek:
		return "SEEK   ";
	case FDD_states::recalibrate:
		return "RECAL  ";
	default:
		return "";
	}
}

uint8 FDD_Ctrl::read_DMA_data()
{
	//выдаем число из буфера контроллеру DMA
	uint8 out = 0;
	if (drv_state == FDD_states::command_exec && command == 61)
	{
		//если идет чтение с дискеты
		out = sector_data[read_start];
		read_start++;
	}
	else
	{
	
		if (out_buffer.size())
		{
			out = out_buffer.at(0);
			out_buffer.erase(out_buffer.begin());
		}
	}
	return out;
}
void FDD_Ctrl::write_DMA_data(uint8 data)
{
	//записываем данные в буфер FDD
	if (drv_state == FDD_states::command_exec && command == 13) //данные для форматирования
	{
		//cout << "DMA receive " << (int)data << endl;
	}
	in_buffer.push_back(data);
}
void FDD_Ctrl::set_active_drives(uint8 d)
{
	//задаем количество дисководов
	active_drives = d;
	if (d == 0) active_drives = 1;
	if (d > 4)  active_drives = 4;
}
uint8 FDD_Ctrl::get_active_drives()
{
	return active_drives;
}

void FDD_Ctrl::set_MFM(uint8 drive, bool MFM)
{
	MFM_mode[drive] = MFM;
}