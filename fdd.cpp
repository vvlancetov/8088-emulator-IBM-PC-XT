//#include <SFML/Graphics.hpp>
//#include <SFML/Audio.hpp>

#include <iostream>
#include <bitset>
#include <Windows.h>
#include <conio.h>
#include <string>
#include "custom_classes.h"
#include "fdd.h"
#include "video.h"

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

using namespace std;

extern HANDLE hConsole;

extern bool log_to_console_FDD;
extern bool step_mode;
extern bool log_to_console;

extern FDD_mon_device FDD_monitor;
extern IC8259 int_ctrl;
extern IC8237 dma_ctrl;


//FDD controller
uint8 FDD_Ctrl::read_port(uint16 port)
{
	SetConsoleTextAttribute(hConsole, 12);
	//cout << "FDD READ " << (int) port;
	SetConsoleTextAttribute(hConsole, 7);
	if (port == 0x3F0)
	{
		if (log_to_console_FDD) cout << "READ from port 3f0" << endl;
		//step_mode = 1; log_to_console = 1;
	}

	if (port == 0x3F1)
	{
		uint8 out = 0;
		out = out | ((motors_pin_enabled & 3) | ((motors_pin_enabled >> 2) & 3)); //���� �������
		out = out | ((selected_drive & 1) << 5);
		out = out | (3 << 6);
		SetConsoleTextAttribute(hConsole, 11);
		if (log_to_console_FDD) cout << "FDD read STATUS REG B <- " << (bitset<8>)(out) << endl;
		SetConsoleTextAttribute(hConsole, 7);
		return out;
	}

	if (port == 0x3F2)
	{
		//Digital out register
		Digital_output_register = (motors_pin_enabled << 4) | (DMA_enabled << 3) | selected_drive;
		SetConsoleTextAttribute(hConsole, 11);
		if (log_to_console_FDD) cout << "READ DOR: MOTORS[" << (bitset<4>)(motors_pin_enabled) << "] DMA = " << (int)DMA_enabled << " SELDRV = " << (int)selected_drive << endl;
		SetConsoleTextAttribute(hConsole, 7);
		return Digital_output_register;
	}

	if (port == 0x3F3)
	{
		//Tape drive register
		SetConsoleTextAttribute(hConsole, 11);
		if (log_to_console_FDD) cout << "READ TDR: " << (int)(Tape_drive_register & 3) << endl;
		SetConsoleTextAttribute(hConsole, 7);
		return Tape_drive_register & 3;
	}

	if (port == 0x3F4)
	{
		//main status register
		Main_status_register = (RQM * 128) | (DIO * 64) | (DMA_ON * 32) | (CMD_BUSY * 16) | (FDD_busy_bits & 15); // 32 - non DMA mode
		SetConsoleTextAttribute(hConsole, 11);
		//if (log_to_console_FDD) cout << "READ STATUS RQM=" << (int)RQM << " DIO=" << (int)DIO << " DMA_ON=" << (int)DMA_ON << " CMD_BUSY="<< (int)CMD_BUSY << " DRV_BUSY=" << (bitset<4>)FDD_busy_bits << endl;
		SetConsoleTextAttribute(hConsole, 7);
		return Main_status_register;
	}

	//FIFO port
	if (port == 0x3F5)
	{
		//data FIFO
		SetConsoleTextAttribute(hConsole, 11);

		//������ ����������� ��������
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
					DIO = 0; //������� ������
					CMD_BUSY = 0; //����������� ��������
				}
				return out;
			}
			else
			{
				if (log_to_console_FDD) cout << "FDD data buffer READ - EMPTY!" << endl;
				//sense_int_buffer.push_back(0b11000000 | selected_drive);
				return 0;
			}
		}

		//������ ������ ������� READ ��� DMA (� ����������)
		if (drv_state == FDD_states::command_exec && command == 62)
		{
			if (out_buffer.size() > 0)
			{
				//������ ����� � ����
				uint8 out = out_buffer.at(0);
				out_buffer.erase(out_buffer.begin());
				//������������� ������ ��� ����������
				sense_int_buffer.push_back(0b00100000);
				if (log_to_console_FDD) cout << "FIFO read out buffer " << (int)out << endl;
				return out;
			}
			else
			{
				//������������� ������ ������ ��� INT
				if (log_to_console_FDD) cout << "FIFO EMPTY " << endl;
				//sense_int_buffer.push_back(0b01100000 | selected_drive);
				return 0;
			}

		}
		SetConsoleTextAttribute(hConsole, 7);
		return 0;
	}

	if (port == 0x3F7)
	{
		//digital input register
		SetConsoleTextAttribute(hConsole, 11);
		if (log_to_console_FDD) cout << "FDD digital input REG <- " << (int)Digital_input_register << endl;
		SetConsoleTextAttribute(hConsole, 7);
		return Digital_input_register;
	}
}
void FDD_Ctrl::write_port(uint16 port, uint8 data)
{
	//SetConsoleTextAttribute(hConsole, 12);
	//if (log_to_console_FDD || 1) FDD_monitor.log("FDD WRITE " + to_string(port));
	//SetConsoleTextAttribute(hConsole, 7);

	if (port == 0x3F2)
	{
		SetConsoleTextAttribute(hConsole, 11);
		//if (log_to_console_FDD) cout << "WRITE DOR MOTORS[" << (bitset<4>)(data >> 4) << "] DMA_EN=" << (int)((data >> 3) & 1) << " RESET=" << (int)((data >> 2) & 1) << " SELECT_DRV=" << (int)(data & 3) << endl;
		SetConsoleTextAttribute(hConsole, 7);

		Digital_output_register = data; //����� ������

		selected_drive = data & 3;
		DMA_enabled = (data >> 3) & 1;
		motors_pin_enabled = (data >> 4);


		//������������ � RESET
		if (drv_state == FDD_states::idle || drv_state == FDD_states::read_result)
		{
			if (((data >> 2) & 1) == 0)
			{
				drv_state = FDD_states::reset;
				if (log_to_console_FDD) cout << "FDD: state -> RESET";
			}
			return;
		}

		//��������� ��������� ������������
		if (drv_state == FDD_states::reset)
		{
			//���� ��� ������ �� ���������, �� �������
			if (!((data >> 2) & 1)) return;
			if (log_to_console_FDD)
			{
				FDD_monitor.log("RESET Ctrl");
				cout << "FDD RESET complete";
			}

			//���������� ������������
			Tape_drive_register = 0;
			Main_status_register = 0;
			Data_fifo.clear();
			out_buffer.clear();
			Digital_input_register = 0;
			selected_drive = 0;
			RQM = 1;
			IC = 0;
			SE = 0;
			//��������� ������ ��� senceINT
			sense_int_buffer.clear();
			sense_int_buffer.push_back(0b11000000);
			sense_int_buffer.push_back(0b11000001);
			sense_int_buffer.push_back(0b11000010);
			sense_int_buffer.push_back(0b11000011);
			DIO = 0; //������� ������
			//��������� ����������
			drv_state = FDD_states::command_exec;
			CMD_BUSY = 1;
			command = 255;		//�������� ��� ��������
			sleep_counter = 3;	//��������
			return;
		}

		//����� ������ ��������� ������ �� ������
	}

	if (port == 0x3F3)
	{
		if (drv_state == FDD_states::idle) Tape_drive_register = data;
	}

	if (port == 0x3F4)
	{
		SetConsoleTextAttribute(hConsole, 11);
		//if (log_to_console_FDD) cout << "WR DSR softres=" << (int)((data >> 7) & 1) << " lowPWR=" << (int)((data >> 6) & 1) << " precomp=" << (int)((data >> 2) & 7) << " D_rate=" << (int)((data) & 3) << endl;
		SetConsoleTextAttribute(hConsole, 7);

		Data_rate_select_register = data & 0b01111111;
		data_rate_sel = data & 3; //��������� ���������� ��� ��������

		if ((data >> 7) & 1) //���������� ������������
		{
			//�� �� �����, ��� � ������������, ������ ��� ����� ���������
			Tape_drive_register = 0;
			Main_status_register = 0;
			Data_fifo.clear();
			Digital_input_register = 0;
			selected_drive = 0;
			RQM = 1;
			CMD_BUSY = 1;
			IC = 0;
			SE = 0;
			//sense_int_buffer.push_back(0b11000000|selected_drive); //������������� ���� ������ ?
			out_buffer.clear();
			sense_int_buffer.clear();
			sense_int_buffer.push_back(0b11000000);
			sense_int_buffer.push_back(0b11000001);
			sense_int_buffer.push_back(0b11000010);
			sense_int_buffer.push_back(0b11000011);
			drv_state = FDD_states::command_exec;
			command = 255; //�������� ��� ��������
			sleep_counter = 3;
			return;
		}
	}

	if (port == 0x3F5)
	{
		//data FIFO
		SetConsoleTextAttribute(hConsole, 11);
		//if (log_to_console_FDD) cout << "FDD data buffer(FIFO) WRITE " << (int)data << endl;
		SetConsoleTextAttribute(hConsole, 7);

		if (drv_state == FDD_states::idle)
		{
			//���������� �������
			if ((data & 31) == 3)
			{
				//������� Specify
				command = 3;
				drv_state = FDD_states::enter_param;//������ �� ���������
				//cout << "FDD command 4  -> WAIT PARAMs" << endl;
				//DMA_ON = false;
				CMD_BUSY = 1;
				params_left = 2;
				return;
			}

			if ((data & 31) == 4)
			{
				//������� Sense drive status
				command = 4;
				drv_state = FDD_states::enter_param;//������ �� ���������
				//cout << "FDD command 4  -> WAIT PARAMs" << endl;
				//DMA_ON = false;
				CMD_BUSY = 1;
				params_left = 1;
				return;
			}


			if ((data & 31) == 6)
			{
				//READ
				command = 6;
				drv_state = FDD_states::enter_param;//������ �� ���� ����������
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
				drv_state = FDD_states::enter_param;//������ �� ���� ����������
				//cout << "FDD command 7  -> WAIT PARAMs" << endl;
				CMD_BUSY = 1;
				//DMA_ON = false;
				params_left = 1;
				return;
			}

			if ((data & 31) == 8)
			{
				//������� Sense interrupt status
				command = 8;
				drv_state = FDD_states::command_exec;//������ �� ���������
				//cout << "FDD command 8  -> RUN" << endl;
				//IC = 3;
				//DMA_ON = false;
				//SE = 0;
				CMD_BUSY = 1;
				return;
			}

			if ((data & 31) == 15)
			{
				//Seek
				command = 15;
				drv_state = FDD_states::enter_param;//������ �� ���� ����������
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
			//���� ����������
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

		//������ ����� ����� �� ����������
		if (drv_state == FDD_states::seek)
		{
			if ((data & 31) == 8)  //����������� SEEK + ���������� �������� SIS
			{
				command = 8; //������������ �� Sense interrupt status
				IC = 1; //������� ���������� Seek �����������
				SE = 1;
				sense_int_buffer.push_back(0b11000000 | selected_drive);
				CMD_BUSY = 1;
				int_ctrl.request_IRQ(6); //�� ����, ��� �����, ���������
				if (log_to_console_FDD) FDD_monitor.log("SEEK interrupted by SIS. Error set.");
				drv_state = FDD_states::command_exec;
				return;
			}
		}

		//������ ����� ����� �� ����������
		if (drv_state == FDD_states::recalibrate)
		{
			if ((data & 31) == 8)  //���������� �������� SIS
			{
				command = 8; //������������ �� Sense interrupt status
				IC = 1; //������� ���������� Seek �����������
				SE = 1;
				sense_int_buffer.push_back(0b11000000 | selected_drive);
				CMD_BUSY = 1;
				int_ctrl.request_IRQ(6); //�� ����, ��� �����, ���������
				if (log_to_console_FDD) FDD_monitor.log("RECALIBRATE interrupted by SIS. Error set.");
				drv_state = FDD_states::command_exec;
				return;
			}
		}

	}
	if (port == 0x3F7)
	{
		//digital input register
		SetConsoleTextAttribute(hConsole, 11);
		if (log_to_console_FDD) cout << "FDD: write digital input REG = " << (int)data << endl;
		SetConsoleTextAttribute(hConsole, 7);
		Digital_input_register = data;
	}

}
void FDD_Ctrl::sync()
{
	if (sleep_counter != 0)
	{
		//�������� �������
		sleep_counter--;
		return;
	}

	//FDD_busy_bits = 0; //������� ���� ���������

	if (drv_state == FDD_states::command_exec)
	{
		//������������ �������
		//� ������ FIFO ������ ���������!
		//CMD_BUSY = 0;

		if (command == 3)  //Specify
		{
			//byte 0 = SRT + HUD
			//byte 1 = HLT + ND
			if (Data_fifo.at(1) & 1) DMA_ON = false;
			else DMA_ON = true;
			Data_fifo.clear();
			//sleep_counter = 10; //������ ���������
			//command = 255;
			CMD_BUSY = 0;
			DIO = 0; //������� ������
			drv_state = FDD_states::idle;
			SetConsoleTextAttribute(hConsole, 11);
			//if (log_to_console_FDD) cout << "CMD 3 [Specify] DMA_ON=" << to_string(DMA_ON) << endl;
			SetConsoleTextAttribute(hConsole, 7);
			return;
		}

		if (command == 4)    //Sense drive status
		{
			uint8 param = Data_fifo.at(0);
			uint8 drv = param & 3;
			uint8 head = (param >> 2) & 1;
			SetConsoleTextAttribute(hConsole, 11);
			if (log_to_console_FDD) cout << "CMD 4 (Sense drive status) drv=" << (int)drv << " head= " << (int)head << endl;
			SetConsoleTextAttribute(hConsole, 7);
			//������ ����������
			bool tr_0 = false;
			if (C_cylinder == 0) tr_0 = true; //������� �� �������� 0 
			uint8 result = 0 * 128 | 0 * 64 | 1 * 32 | tr_0 * 16 | 1 * 8 | H_head * 4 | drv;  //�������� ����� ���������� ������� ��������� ������
			Data_fifo.clear();
			Data_fifo.push_back(result);
			drv_state = FDD_states::read_result;
			results_left = 1;
			//DMA_enabled = 0;
			DIO = 1; //������� ������
			return;
		}

		if (command == 6)    //READ
		{
			//cout << endl;
			H_head = (Data_fifo.at(0) >> 2) & 1;
			selected_drive = (Data_fifo.at(0)) & 3;

			C_cylinder = Data_fifo.at(1);

			H_head = Data_fifo.at(2); //������-�� ������ ��� �������

			R_sector = Data_fifo.at(3); //����� ������� ��� ������

			SectorSizeCode = Data_fifo.at(4); //���-�� ���� � ������� (��. �������)
			Sector_size = pow(2, SectorSizeCode) * 128;
			if (Sector_size > 16384) Sector_size = 16384;

			uint8 EOT = Data_fifo.at(5); //����� ���������� ������� �� �������
			uint8 GPL = Data_fifo.at(6); //��� 3 ����� ���������
			uint8 DTL = Data_fifo.at(7); //����������� ������ ������� ���� Sector_size = 128 (���� < 128), ����� �����
			if (Sector_size == 128 && DTL != 0) Sector_size = DTL;
			if (log_to_console_FDD) FDD_monitor.log("CMD 6 [READ] head=" + to_string(H_head) + " CYL=" + to_string(C_cylinder) + " DRV=" + to_string(selected_drive) + " SEC_begin = " + to_string(R_sector) + " SEC_size = " + to_string(Sector_size) + " MT = " + to_string(MT) + " EOT = " + to_string(EOT));

			Data_fifo.clear();

			//������ �������� 

			if (DMA_ON && DMA_enabled) //�������� � ������ DMA
			{
				//step_mode = 1;
				//if (log_to_console_FDD) FDD_monitor.log("DMA ON...  ");

				//���������� ������
				//��������� ������ �������
				if (!R_sector) //�� ������ ���� ����
				{
					R_sector = 1;
					cout << "R_sector = 0!!!";
				}

				uint32 start = 0;

				if (MT)
				{
					//������������ ������ - ���� �� ������������
					start = C_cylinder * 9216 + (R_sector - 1) * 512; //������� 2
					for (int j = 0; j < Sector_size; j += 2)
					{
						int addr = start + j; //����� ����� �� ����� 512 - ������ ������� �� ���������
						out_buffer.push_back(sector_data[addr]);
						addr += 9 * 512 * H_head;
						out_buffer.push_back(sector_data[addr + 1]);
					}
				}
				else
				{
					//��������� �������
					// diskette_heads = 1;  // 1 or 2          ���-�� �������
					// diskette_sectors = 8; //8, 9 or 15	  ���-�� ��������


					//������� ������ head 0 - 1 �������� ��� ��������� �������
					start = C_cylinder * 512 * diskette_sectors * diskette_heads + diskette_sectors * 512 * H_head * (diskette_heads - 1) + (R_sector - 1) * 512; //������� 2
					start_sec = C_cylinder * diskette_sectors * diskette_heads + H_head * diskette_sectors * (diskette_heads - 1) + R_sector - 1;
					end_sec = C_cylinder * diskette_sectors * diskette_heads + H_head * diskette_sectors * (diskette_heads - 1) + EOT - 1;
					//if (log_to_console_FDD) FDD_monitor.log("File sector # " + to_string(C_cylinder * 18 + H_head * 9 + (R_sector - 1)) + " to # " + to_string(C_cylinder * 18 + H_head * 9 + (EOT - 1)));
					//if (log_to_console_FDD) FDD_monitor.log("Read " + to_string(EOT - R_sector + 1) + " sectors to buffer");
					for (int j = 0; j < Sector_size * (EOT - R_sector + 1); ++j)
					{
						int addr = start + j; //����� ����� �� ����� 512 - ������ ������� �� ���������
						out_buffer.push_back(sector_data[addr]);
					}
					R_sector = EOT;
				}

				//out_buffer.pop_back();
				//if (log_to_console_FDD) FDD_monitor.log("DMA buffer ready (size = " + to_string(out_buffer.size())+ ")");

				//���� ��������� ������
				command = 61; //������ � ���
				RQM = 0; //������ ��������
				DIO = 1; //������� ������ 
				CMD_BUSY = 1;
				FDD_busy_bits = (1 << selected_drive); //������ ������� ���������
				return;
			}
			else
			{
				//��� ����� �� ������ ��������
				//�������� � ������� ������ ������
				if (log_to_console_FDD) FDD_monitor.log("DMA OFF... load buffer ");
				//���� ��������� ������
				command = 62; //������ ��� ���

				for (int i = R_sector; i < EOT; ++i)
				{
					//������ ������ �������� � �����
					for (int j = 0; j < Sector_size; ++j)
					{
						for (int h = 0; h < 2; ++h)
						{
							uint32 addr = (C_cylinder * 9 * 2 + i * 2) * 512 + j * 2 + h; //����� ����� �� ����� 512 - ������ ������� �� ���������
							out_buffer.push_back(sector_data[addr]);
						}
					}
				}
				R_sector++;
				DIO = 1; //������� �������
				CMD_BUSY = 1;
				FDD_busy_bits = FDD_busy_bits | (1 << selected_drive); //������ ������� ���������
				if (log_to_console_FDD) cout << " read BUFFER ready" << endl;
				step_mode = 1;
				return;
			}
		}

		if (command == 61)    //READ + DMA
		{
			if (out_buffer.size())
			{
				//if (log_to_console_FDD) FDD_monitor.log("DMA Req Ch12(FDD)");
				if (dma_ctrl.request_hw_dma(2))
				{
					//if (log_to_console_FDD) FDD_monitor.log("no DMA: out_buffer = " + to_string(out_buffer.size()/512) + " sectors left)");
					R_sector -= out_buffer.size() / 512; //���������� ������� ������
					end_sec -= out_buffer.size() / 512;
					//int_ctrl.request_IRQ(6);
					out_buffer.clear(); //�������� DMA ��� ������ ������� �����
				}
				//��������, �������� ������ ��� ������� ������
			}
			else
			{
				if (log_to_console_FDD) FDD_monitor.log("Fact read sectors from " + to_string(start_sec) + " to " + to_string(end_sec));
				//if (end_sec == 164) step_mode = 1;
				//���������� ����� �������� � FIFO � ����������� ������
				Data_fifo.push_back(0b00000000 | (H_head << 2) | selected_drive); // ST0
				//Data_fifo.push_back(0b11000000 | selected_drive); //error
				Data_fifo.push_back(0b00000000); // ST1
				Data_fifo.push_back(0b00000000); // ST2
				Data_fifo.push_back(C_cylinder); // C
				Data_fifo.push_back(H_head); // H
				Data_fifo.push_back(R_sector); // R
				Data_fifo.push_back(SectorSizeCode); // N ��������
				results_left = 7;
				RQM = 1;
				DIO = 1; //������� ������
				CMD_BUSY = 1;
				FDD_busy_bits = FDD_busy_bits & ~(1 << selected_drive); //������� ������� ���������
				int_ctrl.request_IRQ(6);
				//cout << " INT 6"; step_mode = 1;
				drv_state = FDD_states::read_result;
			}
			return;
		}

		if (command == 62)    //READ polling
		{
			//�������� ���
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
			}
			else
			{
				data_out = 0b10000000 | selected_drive; //������ ������� SENSE
			}

			Data_fifo.push_back(data_out); //ST0    80?
			//if (log_to_console_FDD) FDD_monitor.log("Sense INT -> FIFO PUSH (" + to_string(Data_fifo.at(Data_fifo.size() - 1)) + ") ");
			IC = 0; //���������� ��� ����������
			SE = 0;
			Data_fifo.push_back(C_cylinder); //PCN - � �������� ��� ��������
			//if (log_to_console_FDD) FDD_monitor.log("(" + to_string(Data_fifo.at(Data_fifo.size() - 1)) + ") ");
			//if (log_to_console_FDD) FDD_monitor.log("CMD 8 (Sense INT status) -> " + to_string(Data_fifo.at(Data_fifo.size() - 2)) + " & " + to_string(Data_fifo.at(Data_fifo.size() - 1)));
			//SetConsoleTextAttribute(hConsole, 11);
			if (log_to_console_FDD) cout << "CMD 8 (Sense INT status) -> " << (int)Data_fifo.at(Data_fifo.size() - 2) << " & " << (int)Data_fifo.at(Data_fifo.size() - 1) << endl;
			//SetConsoleTextAttribute(hConsole, 7);
			drv_state = FDD_states::read_result;
			results_left = 2;
			//DMA_enabled = 0;
			DIO = 1; //������� ������
			//int_ctrl.request_IRQ(6);
			return;
		}

		if (command == 7)
		{
			selected_drive = Data_fifo.at(0) & 3;
			SetConsoleTextAttribute(hConsole, 11);
			if (log_to_console_FDD) cout << "CMD 7 (Recalibrate) drv=" << (int)(selected_drive & 3);
			SetConsoleTextAttribute(hConsole, 7);
			Data_fifo.clear();
			C_cylinder = 0; //������� �� ������� 0
			H_head = 0;
			drv_state = FDD_states::recalibrate;
			DIO = 0; //������� ������
			CMD_BUSY = 0;
			FDD_busy_bits = (1 << selected_drive); //������ ������� ��������� ��������
			sleep_counter = 10; //������ ���������
			//DMA_enabled = 0;
			//int_ctrl.request_INT(6);
			return;
		}

		if (command == 15)  //Seek
		{
			selected_drive = Data_fifo.at(0) & 3;    //����� �����
			C_cylinder = Data_fifo.at(1);            //��������� �� ������ �������
			H_head = ((Data_fifo.at(0) >> 2) & 1);   //������ �������
			//Data_fifo.pop_back();
			SetConsoleTextAttribute(hConsole, 11);
			if (log_to_console_FDD) FDD_monitor.log("CMD SEEK drv=" + to_string(selected_drive) + " head= " + to_string(H_head) + " cyl=" + to_string(C_cylinder));
			SetConsoleTextAttribute(hConsole, 7);
			Data_fifo.clear();
			drv_state = FDD_states::seek;
			DIO = 0; //������� ������
			CMD_BUSY = 0;
			FDD_busy_bits = (1 << selected_drive); //������ ������� ���������
			sleep_counter = 10; //������ ���������
			//DMA_enabled = 0;
			//int_ctrl.request_INT(6);
			return;
		}

		//������� ��� ������ �� �������� ����� ������������
		if (command == 255)
		{
			SetConsoleTextAttribute(hConsole, 11);
			cout << "FDD: cmd 255" << endl;
			SetConsoleTextAttribute(hConsole, 7);
			CMD_BUSY = 0;
			DIO = 0; //������� ������
			drv_state = FDD_states::idle; //������� � ��������� �������
			DMA_ON = DMA_enabled;
			int_ctrl.request_IRQ(6);  //������ ������ ���� �� ��������
			return;
		}

		//��������� ����� �������
		if (log_to_console_FDD) cout << "FDD new command! (" << (int)command << ")" << endl;
		step_mode = 1; log_to_console = 1;
	}

	if (drv_state == FDD_states::recalibrate)
	{
		//���� �� ���� ���������� ��������� ���������
		IC = 0; //������� ����������
		SE = 1;
		sense_int_buffer.clear();
		sense_int_buffer.push_back(0b00100000 | selected_drive); //������ ����������, �����
		//CMD_BUSY = 0;
		FDD_busy_bits = 0; //������� ������� ���������
		if (log_to_console_FDD) cout << "Recalibrate complete" << endl;
		drv_state = FDD_states::idle;
		int_ctrl.request_IRQ(6); //���������� �����
		return;
	}

	if (drv_state == FDD_states::seek)
	{
		IC = 0; //������� ����������
		SE = 1;
		if (C_cylinder < 80)
		{
			//��� ��
			sense_int_buffer.push_back(0b00100000 | selected_drive); //������ ����������, �����
		}
		else
		{
			//������
			C_cylinder = 79;
			sense_int_buffer.push_back(0b01100000 | selected_drive); //������ ����������, �����
		}

		//CMD_BUSY = 0;
		FDD_busy_bits = 0; //������ ������� ��������� = 0
		if (log_to_console_FDD) cout << "Seek complete" << endl;
		drv_state = FDD_states::idle;
		int_ctrl.request_IRQ(6); //���������� �����
		return;
	}


}
void FDD_Ctrl::load_diskette(uint8 track, uint8 sector, bool head, uint16 byte_N, uint8 data)
{
	if (track > 79) return;
	if (sector > 9) return;
	uint32 addr = (track * 9 * 2 + sector * 2) * 512 + byte_N * 2 + head; //����� ����� �� �����
	//cout << "write byte addr = " << (int)addr << " data = " << (int)data << endl;
	sector_data[addr] = data;
}
uint8 FDD_Ctrl::read_sector(uint8 track, uint8 sector, bool head)
{
	if (track > 79) return 0;
	if (sector > 9) return 0;
	//uint16 raw_data = sector_data[track][sector];
	//if (head) return raw_data & 255;
	//else return raw_data >> 8;
}
string FDD_Ctrl::get_state(uint8* ptr)
{
	//��������� ������ �� ������
	*ptr = CMD_BUSY;
	*(ptr + 1) = selected_drive; // selected_drive;
	*(ptr + 2) = DMA_enabled;
	*(ptr + 3) = DMA_ON;
	*(ptr + 4) = motors_pin_enabled;
	*(ptr + 5) = FDD_busy_bits;

	//���������� ��������
	*(ptr + 6) = C_cylinder;	//������� �������
	*(ptr + 7) = R_sector;		//������� ������
	*(ptr + 8) = H_head;		//������� ������� �������
	*(ptr + 9) = DIR_control;	//����������� ������ 1 - � ������
	*(ptr + 10) = MFM_mode;		//0 - ��������� ���������, 1 - �������
	*(ptr + 11) = MT;			//multi-track

	//������ ���������
	switch (drv_state)
	{
	case FDD_states::idle:
		return "IDLE   ";
	case FDD_states::reset:
		return "RESET  ";
	case FDD_states::enter_param:
		return "PAR_ENT";
	case FDD_states::command_exec:
		return "CMD_EXE";
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
string FDD_Ctrl::get_command()
{
	switch (command)
	{
	case 3:
		return "[3]Specify";
	case 4:
		return "[4]Sense drive status";
	case 6:
		return "[6]Read";
	case 7:
		return "[7]Recalibrate";
	case 8:
		return "[8]Sense interrupt status";
	case 15:
		return "[15]Seek";
	default:
		return "Unknown";
	}
	return "";
}
uint8 FDD_Ctrl::get_DMA_data()
{
	//������ ����� �� ������ ����������� DMA
	uint8 out = 0;
	if (out_buffer.size())
	{
		out = out_buffer.at(0);
		out_buffer.erase(out_buffer.begin());
		//cout << "BUFF out(left " << out_buffer.size() <<") = " << hex << (int)out << endl;
	}
	return out;
}
void FDD_Ctrl::put_DMA_data(uint8 data)
{
	//���������� ������ � ����� FDD



}
