#pragma once
#include <vector>

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;

enum class KBD_key {
	F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9, Num0,
	Q, W, E, R, T, Y, U, I, O, P, A, S, D, F, G, H, J, K, L, Z, X, C, V, B, N, M, ENTER, SPACE, Grave, Hyphen, Equal, Backspace, Escape, Tab, LShift, RShift,
	LControl, LAlt, Backslash, LBracket, Semicolon, Apostrophe, Comma, Period, Slash, Up, Down, Left, Right
};

class KBD //класс клавиатуры
{
private:
	bool pressed_keys[256] = { 0 };
	uint32 pressed_time_us[256] = { 0 };
	std::vector<uint8> out_buffer;

public:
	KBD(){};
	bool CLK_high = false;
	bool enabled = false;
	void sync(uint32 elapsed_us);			//синхронизация клавиатуры
	uint8 read_scan_code();  //чтение скан-кода клавиатуры
	void set_CLK_low();
	void set_CLK_high();
};
