#pragma once
#include "custom_classes.h"

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;

//управление джойстиком
class game_controller
{
private:
	bool countdown = 0;
	int counter = 0;
	int counter_X = 0;
	int counter_Y = 0;

	uint8 joystick_id = 0;
	bool joystick_active = 0;

	uint32 check_state_timer = 0; //таймер проверки состояний
	uint32 calibr_timer = 0; //таймер калибровки



public:
	game_controller();
	void start_meazure();
	uint8 get_input();
	void sync(uint32 elapsed_us);
	uint8 central_point = 30;
};

