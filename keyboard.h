#pragma once
#include "custom_classes.h"
#include <vector>

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;

enum class KBD_key {
	F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9, Num0,
	Q, W, E, R, T, Y, U, I, O, P, A, S, D, F, G, H, J, K, L, Z, X, C, V, B, N, M, ENTER, SPACE, Grave, Hyphen, Equal, Backspace, Escape, Tab, LShift, RShift,
	LControl, LAlt, RAlt, RControl, Backslash, LBracket, RBracket, Semicolon, Apostrophe, Comma, Period, Slash, Up, Down, Left, Right, Delete, Insert, PageUp, PageDown,Home, End, 
	PrintScreen, CapsLock, NumLock, ScrLock, Pause, NumpadDivide, NumpadMultiply, NumpadMinus, NumpadPlus, NumpadEnter, NumpadDecimal, Numpad0, Numpad1, Numpad2, Numpad3, Numpad4, Numpad5, Numpad6, Numpad7, Numpad8, Numpad9
};

class KBD //класс клавиатуры
{
private:
	bool pressed_keys[256] = { 0 };
	uint32 pressed_time_us[256] = { 0 };
	std::vector<uint8> out_buffer;
	uint8 sleep_timer = 0;

public:
	KBD(){};
	bool CLK_high = false;
	bool data_line_enabled = false;
	void poll_keys(uint32 elapsed_us, bool has_focus);			//синхронизация клавиатуры
	uint8 read_scan_code();	 //чтение скан-кода клавиатуры
	uint8 get_buf_size();	 //проверка объема буфера ввода
	void set_CLK_low();
	void set_CLK_high();
	void sync(); //процедура синхронизации и вызова прерывания #1
};
