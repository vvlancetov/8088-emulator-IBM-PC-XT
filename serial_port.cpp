#include <iostream>
#include "serial_port.h"
#include "custom_classes.h"
#include "mouse.h"

using namespace std;

extern Mouse ms_mouse;
extern IC8259 int_ctrl;

extern bool step_mode;		//ждать ли нажатия пробела для выполнения команд
extern bool log_to_console;

SerialPort::SerialPort()
{
	//конструктор порта
	reg_int_ctrl = 0; //регистр управления прерываниями
	reg_int_ID = 1; //Регистр идентификации прерывания
	reg_line_ctrl = 0b00000011; //Регистр управления линией
	reg_modem_ctrl = 0; //Регистр управления модемом
	reg_line_state = 0b01100000; //Регистр состояния линии
	reg_modem_state = 0; //Регистр состояния модема
}

uint8 SerialPort::read_port(uint16 address)
{
	//cout << "read port " << hex << (int)address << endl;
	
	if (address == 0x3F8)
	{
		if ((reg_line_ctrl & 0x80) == 0)
		{
			//прием данных из буфера
			if (log_to_console) cout << "COM read data_byte_in <- " << (int)data_reg << endl;
			//сброс бита линии состояния
			reg_line_state = reg_line_state & 0b11111110;
			//сброс идентификации прерывания
			reg_int_ID = 1;
			uint8 out = data_reg;
			//data_reg = 0; //обнуление буфера
			in_ctrl_state = com_port_states::IN_data_POPED;
			return out;
		}
		else
		{
			//возвращаем младший байт делителя
			if (log_to_console) cout << "COM read speed_divider low_byte " << hex << (int)(speed_divider & 255) << endl;
			return speed_divider & 255;
		}
	}

	if (address == 0x3F9)
	{
		/*
		Регистр управления прерываниями: 1 - разрешить прерывание, 0 - запретить
		бит 0 – прерывание по приему символа
		бит 1 – прерывание по завершению передачи символа
		бит 2 – прерывание по разрыву соединения или обнаружению ошибки
		бит 3 – прерывание по изменению состояния модема (любой из линий CTS, DSR, RI и DCD)
		*/
		
		if ((reg_line_ctrl & 0x80) == 0)
		{
			//возвращаем регистр
			if (log_to_console) cout << "COM read reg_int_ctrl <- " << (int)reg_int_ctrl << endl;
			return reg_int_ctrl;
		}
		else
		{
			//возвращаем старший байт делителя
			if (log_to_console) cout << "COM read speed_divider high_byte " << hex << (int)(speed_divider >> 8) << endl;
			return speed_divider >> 8;
		}
	}


	if (address == 0x3FA)
	{
		/*
		Регистр идентификации прерывания
		бит 0 - есть отложенные прерывания (при = 0)
		биты 1-2
		11 = ошибка или обрыв линии; сбрасывается чтением регистра состояния линии (порт 3FDh)
		10 = принят символ; сбрасывается чтением приемника (порт 3F8h)
		01 = передан символ; сбрасывается записью символа в регистр передатчика (порт 3F8h)
		00 = изменение состояния модема(линий CTS, DSR, RI или DCD); сбрасывается чтением регистра состояния модема (порт 3FEh)
		*/

		if (log_to_console) cout << "COM read reg_int_ID <- " << (int)reg_int_ID << endl;
		return reg_int_ID;
	}

	if (address == 0x3FB)
	{
		if (log_to_console) cout << "COM read reg_line_ctrl <- " << (int)reg_line_ctrl << endl;
		return reg_line_ctrl;
	}


	if (address == 0x3FC)
	{
		if (log_to_console) cout << "COM read reg_modem_ctrl <- " << (int)reg_modem_ctrl << endl;
		return reg_modem_ctrl;
	}

	if (address == 0x3FD)
	{
		/*чтение состояния линии
		Формат регистра состояния линии(1 соответствует активному состоянию) :
		бит 0 — приёмник получил данные, и их можно прочитать из регистра данных (1 для чтения данных программой)
		бит 1 — потеря данных, новый байт был получен, а старый не прочитан из регистра данных, и новый байт заместил в регистре данных старый байт;
		бит 2 — произошла ошибка чётности;
		бит 3 — произошла ошибка синхронизации;
		бит 4 — обнаружен разрыв соединения;
		бит 5 — регистр переданных данных пуст, можно записать новый байт (1 для записи данных от программы)
		бит 6 — данные переданы приёмнику(сдвиговый регистр, куда помещается байт из регистра данных, пуст);
		бит 7 — устройству не удалось связаться с компьютером в установленный срок.
		*/

		if (log_to_console) cout << "COM read reg_line_state <- " << (int)reg_line_state << endl;

		uint8 out = reg_line_state;
		reg_line_state &= 0b11100001;//сброс битов ошибки
		return out;
	}

	if (address == 0x3FE)
	{
		if ((reg_modem_ctrl >> 4) & 1) return ms_mouse.get_modem_state(1); //loopback mode
		else return ms_mouse.get_modem_state(0);
		/*
		Регистр состояния модема:
		бит 0 – произошло изменение состояния линии CTS;
		бит 1 – произошло изменение состояния линии DSR;
		бит 2 – произошло изменение состояния линии IR;
		бит 3 – произошло изменение состояния линии DCD;
		бит 4 – состояние линии CTS;
		бит 5 – состояние линии DSR;
		бит 6 – состояние линии IR;
		бит 7 – состояние линии DCD;
		*/
	}
}
void SerialPort::write_port(uint16 address, uint8 data)
{
	//cout << "write port " << hex << (int)address << endl;

	//step_mode = 1;
	
	if (address == 0x3F8)
	{
		if ((reg_line_ctrl & 0x80) == 0)
		{
			//получаем байт для отправки
			
			data_reg = data;
			//сбрасываем пятый бит
			reg_line_state = reg_line_state & 0b10011111;
			out_ctrl_state = com_port_states::WAIT_RECEIVER;
			if ((reg_modem_ctrl >> 4) & 1) //установлен ли признак цикла
			{
				if (log_to_console) cout << "COM send byte to loop" << endl;
				receive_data(data); //отправляем данные на вход, если стоит тестовый режим
			}
			else
			{
				if (log_to_console) cout << "COM write data_byte_out" << (int)data << endl;
			}
		}
		else
		{
			//получаем младший байт делителя
			speed_divider = (speed_divider & 0xFF00) | (data);
			if (log_to_console)
			{
				if (speed_divider) cout << "COM update speed " << dec << (int)(115200 / speed_divider) << endl;
				else cout << "COM update speed NULL" << endl;
			}
		}
	}

	if (address == 0x3F9)
	{
		if ((reg_line_ctrl & 0x80) == 0)
		{
			//обновляем регистр
			if (log_to_console) cout << "COM write reg_int_ctrl = " << (int)data << endl;
			reg_int_ctrl = data & 0b00001111;
			//if (!reg_int_ctrl) reg_int_ID = 1;
		}
		else
		{
			//получаем старший байт делителя
			speed_divider = (speed_divider & 0x00FF) | (data * 256);
			if (log_to_console)
			{
				if (speed_divider) cout << "COM update speed " << dec << (int)(115200 / speed_divider) << endl;
				else cout << "COM update speed NULL" << endl;
			}
		}
	}

	if (address == 0x3FB)
	{
		if (log_to_console) cout << "COM write reg_line_ctrl = " << (int)data << endl;
		reg_line_ctrl = data;
	}

	if (address == 0x3FC)
	{
		if (log_to_console) cout << "COM write reg_modem_ctrl = " << (int)data << endl;
		reg_modem_ctrl = data & 0b00011111;
	}

	if (address == 0x3FF)
	{
		//cout << "COM write ext_reg at xxF (NOT PRESENT) = " << (int)data << endl;
	}

}

void SerialPort::receive_data(uint8 byte)
{
	if ((get_DTR() && get_RTS()) || ((reg_modem_ctrl >> 4) & 1))  //DTR + RTS или loop
	{
		if (log_to_console) cout << "COM byte received " << (int)byte << endl;
		//принимаем данные
		data_reg = byte;

		//меняем регистр состояния линии
		reg_line_state = reg_line_state | 0b00000001; //бит 0 - данные приняты

		if (in_ctrl_state == com_port_states::IN_WAIT_HOST)
		{
			//если было ожидание - ставим ошибку - затерт байт
			reg_line_state = reg_line_state | 0b00000010;
		}

		//меняем регистр управления модемом
		//reg_modem_ctrl = reg_modem_ctrl & 0b11111101; //снимаем бит 1 - request to send, т.е. не слать биты

		if (reg_int_ctrl & 1) //проверка разрешения прерывания
		{
			//меняем регистр идентификации прерываний
			reg_int_ID = 0b00000100;//принят символ
		}

		in_ctrl_state = com_port_states::IN_WAIT_HOST;
	}
	else
	{
		if (log_to_console) cout << "COM byte rejected! (DTR = " << (int)get_DTR() << " RTS = " << (int)get_RTS() << ")" << endl;
	}
}

void SerialPort::sync()
{
	//проверка состояния прерываний
	if ((reg_int_ID & 1) == 0)  //есть прерывания
	{
		if (log_to_console) cout << "COM -> IRQ4 (" << int_to_bin(reg_int_ID) << ")" << endl;
		int_ctrl.request_IRQ(4);
		//if (!int_ctrl.request_IRQ(4)) reg_int_ID |= 1; //вызываем прерывание, если удачно - обнуляем регистр
	}
	
	//проверка состояния исходящего потока
	if (out_ctrl_state == com_port_states::OUT_IDLE)
	{
		//ждем байта для отправки
	}
	
	if (out_ctrl_state == com_port_states::WAIT_RECEIVER)
	{
		//ждем готовности получателя
		if ((reg_modem_ctrl >> 4) & 1)
		{
			//внутренний цикл проверки
			//устнавливаем 5 и 6 биты
			reg_line_state = reg_line_state | 0b01100000;
			out_ctrl_state = com_port_states::OUT_IDLE;
			if ((reg_int_ctrl >> 1) & 1)  //необходимость в прерывании
			{
				//устанавливаем идентификатор прерывания
				reg_int_ID = reg_int_ID | 2;
				reg_int_ID = reg_int_ID & 0b11111110;
			}
		}
		else
		{
			if (ms_mouse.get_CTS()) //проверяем готовность к приему
			{
				//отправляем данные в мышь
				ms_mouse.reseive_data(data_reg);
				data_reg = 0;
				//устнавливаем 5 и 6 биты
				reg_line_state = reg_line_state | 0b01100000;
				out_ctrl_state = com_port_states::OUT_IDLE;
				if ((reg_int_ctrl >> 1) & 1)  //необходимость в прерывании
				{
					//устанавливаем идентификатор прерывания
					reg_int_ID = reg_int_ID | 2;
					reg_int_ID = reg_int_ID & 0b11111110;
				}
			}
		}
	}

	//проверка состояния входящего потока
	if (in_ctrl_state == com_port_states::IN_IDLE)
	{
		//ждем байта для получения
	}

	if (in_ctrl_state == com_port_states::IN_WAIT_HOST)
	{
		//ждем пока заберут данные
	}

	if (in_ctrl_state == com_port_states::IN_data_POPED)
	{
		//данные забрали
		in_ctrl_state = com_port_states::IN_IDLE;
	}
}

std::string SerialPort::get_debug_data(uint8 id)
{
	//возвращаем данные для окна отладки
	if (id == 1) return "BUFFER = 0x" + int_to_hex(data_reg, 2);
	if (id == 2) return "BUFFER = 0x" + int_to_hex(data_reg, 2);
	if (id == 3)
	{
		if (speed_divider) return "Speed = " + to_string(115200/ speed_divider) + " bod";
		else return "Speed N/A";
	}
	if (id == 4) return "INT Control   = 0b" + int_to_bin(reg_int_ctrl);
	if (id == 5) return "INT ID        = 0b" + int_to_bin(reg_int_ID);
	if (id == 6) return "Line control  = 0b" + int_to_bin(reg_line_ctrl);
	if (id == 7) return "Modem control = 0b" + int_to_bin(reg_modem_ctrl);
	if (id == 8) return "Line state    = 0b" + int_to_bin(reg_line_state);
	if (id == 9) return "Modem state   = 0b" + int_to_bin(reg_modem_state);
	if (id == 10) return "DTR (Data terminal ready) = " + to_string(((reg_modem_ctrl >> 0) & 1));
	if (id == 11) return "RTS (Request to send) = " + to_string(((reg_modem_ctrl >> 1) & 1));
	
	if (id == 12) return "INT on receive  = " + to_string((reg_int_ctrl >> 0) & 1);
	if (id == 13) return "INT after send  = " + to_string((reg_int_ctrl >> 1) & 1);
	if (id == 14) return "INT on error    = " + to_string((reg_int_ctrl >> 2) & 1);
	if (id == 15) return "INT on ch state = " + to_string((reg_int_ctrl >> 3) & 1);

	if (id == 16)
	{
		if (reg_int_ID & 1) return "No INT";
		else return "Has INT";
	}

	if (id == 17)
	{
		if (reg_int_ID & 1) return "Reason: NA";
		else
		{
			switch ((reg_int_ID >> 1) & 3)
			{
			case 0:
				return "Reason: change CTS, DSR, RI or DCD";
			case 1:
				return "Byte sent";
			case 2:
				return "Byte received";
			case 3:
				return "Error";
			}
		}
	}

	if (id == 18)
	{
		std::string out = "Bits = ";
		switch (reg_line_ctrl & 3)
		{
		case 0:
			out += "5";
			break;
		case 1:
			out += "6";
			break;
		case 2:
			out += "7";
			break;
		case 3:
			out += "8";
			break;
		}
		out += " Stop-bits = ";
		switch (reg_line_ctrl & 7)
		{
		case 0:
			out += "1";
			break;
		case 4:
			out += "1.5";
			break;
		default:
			out += "2";
		}
		out += " Parity = ";
		switch ((reg_line_ctrl >> 3) & 3)
		{
		case 0:
		case 2:
			out += "no";
			break;
		case 3:
			out += "even";
			break;
		case 1:
			out += "odd";
			break;
		}
		
		return out;
	}

	if (id == 19)
	{
		std::string out = "";
		if ((reg_line_ctrl >> 6) & 1) out += "[zero] ";
		if ((reg_line_ctrl >> 7) & 1) out += "[DLAB]";
		return out;
	}

	if (id == 20)
	{
		std::string out = "";
		if ((reg_modem_ctrl >> 0) & 1) out += "DTR  = 1";
		else out += "DTR  = 0";

		if ((reg_modem_ctrl >> 1) & 1) out += "   RTS  = 1\n";
		else out += "   RTS  = 0\n";

		if ((reg_modem_ctrl >> 2) & 1) out += "OUT1 = 1";
		else out += "OUT1 = 0";

		if ((reg_modem_ctrl >> 3) & 1) out += "   OUT2 = 1\n";
		else out += "   OUT2 = 0\n";

		if ((reg_modem_ctrl >> 4) & 1) out += "LOOP = 1\n";
		else out += "LOOP = 0\n";

		return out;
	}

	if (id == 21)
	{
		std::string out = "";
		if ((reg_line_state >> 0) & 1) out += "1 - data received\n";
		else out += "0 - no data received\n";
		if ((reg_line_state >> 1) & 1) out += "1 - data lost\n";
		else out += "0 - ok\n";
		if ((reg_line_state >> 2) & 1) out += "1 - parity error\n";
		else out += "0 - ok\n";
		if ((reg_line_state >> 3) & 1) out += "1 - stop-bit error\n";
		else out += "0 - ok\n";
		if ((reg_line_state >> 4) & 1) out += "1 - line cut\n";
		else out += "0 - ok\n";
		if ((reg_line_state >> 5) & 1) out += "1 - out buffer empty\n";
		else out += "0 - out buffer busy\n";
		if ((reg_line_state >> 6) & 1) out += "1 - out shifter empty";
		else out += "0 - out shifter busy";
			
		return out;
	}

	if (id == 22)
	{
		std::string out = "";
		if ((reg_modem_state >> 0) & 1) out += "1 - CTS changed\n";
		else out += "0\n";
		if ((reg_modem_state >> 1) & 1) out += "1 - DSR changed\n";
		else out += "0\n";
		if ((reg_modem_state >> 2) & 1) out += "1 - RI changed\n";
		else out += "0\n";
		if ((reg_modem_state >> 3) & 1) out += "1 - DCD changed\n";
		else out += "0\n";
		if ((reg_modem_state >> 4) & 1) out += "CTS = 1\n";
		else out += "CTS = 0\n";
		if ((reg_modem_state >> 5) & 1) out += "DSR = 1\n";
		else out += "DSR = 0\n";
		if ((reg_modem_state >> 6) & 1) out += "RI = 1\n";
		else out += "RI = 0\n";
		if ((reg_modem_state >> 6) & 1) out += "DCD = 1";
		else out += "DCD = 0";

		return out;
	}


	return "no data";
}