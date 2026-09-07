#pragma once

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

//класс дл€ создани€ платы расширени€ пам€ти типа EMS
class EMS_board
{
private:
	uint8 page_selectors[4] = { 0,0,0,0 };
	bool board_enable = 0;
	uint8 config_register_1 = 0;
	uint8 config_register_2 = 0;
	uint8 board_mem[2 * 1024 * 1024] = { 0 };

public:
	void page_0_select(uint8 page);
	void page_1_select(uint8 page);
	void page_2_select(uint8 page);
	void page_3_select(uint8 page);
	void write_ctrl_reg(uint8 data);
	uint8 read_status();
	void write_config_1(uint8 data);
	void write_config_2(uint8 data);
	uint8 read_config_1();
	uint8 read_config_2();
	void write_mem(uint16 address, uint8 data);
	uint8 read_mem(uint16 address);
};
