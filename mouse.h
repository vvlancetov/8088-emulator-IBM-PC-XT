#pragma once
#include <chrono>
#include <vector>
#include "custom_classes.h"

using namespace std;

class Mouse
{
private:
	sf::Vector2i position_new = { 0,0 }; //позиции в окне
	sf::Vector2i position_old = { 0,0 };
	uint8 buttons_new = 0; //состояния кнопок
	uint8 buttons_old = 0;
	bool send_answer = 0;  //требуется ответ на запрос

	bool power = 0;
	vector<uint8> out_buffer;

public:
	Mouse(); //конструктор
	uint8 read_port(uint16 address);
	void write_port(uint16 address, uint8 data);
	void sync();
};