#pragma once

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

public:
	void start_meazure();
	uint8 get_input();
};

