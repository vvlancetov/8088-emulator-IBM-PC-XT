#include <conio.h>
#include <bitset>
#include <iostream>
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include "adlib.h"
#include "audio.h"

using namespace std::chrono;
using namespace std;

extern adlib_card soundcard;
extern Audio_mon_device Audio_monitor;

// Вспомогательная функция для получения реального значения множителя частоты
float get_mult_value(uint8 mult) {
	if (mult == 0) return 0.5f;
	if (mult >= 1 && mult <= 10) return (float)mult;
	if (mult == 11) return 10.0f;
	if (mult == 12 || mult == 13) return 12.0f;
	return 15.0f; // 14 и 15
}


// Теперь функция get_wave_sample должна принимать полную 18-битную фазу, а не готовый 10-битный индекс!
float adlib_card::get_wave_sample(uint32 full_phase_18bit, uint8 waveform) {
	if (!wave_select_enable) waveform = 0;

	// Извлекаем интерполированный синус
	float base_sine = get_interpolated_sine(full_phase_18bit);

	// Логика обрезки полуволн для waveform 1, 2, 3 остается прежней, 
	// но индексы для условий проверяем по старшим битам:
	uint32_t table_idx = (full_phase_18bit >> 8) & 1023;

	switch (waveform) {
	case 0: return base_sine;
	case 1: if (table_idx >= 512) return 0.0f; return base_sine;
	case 2: return std::abs(base_sine);
	case 3: if (table_idx >= 256 && table_idx < 512) return 0.0f;
		if (table_idx >= 768) return 0.0f;
		return std::abs(base_sine);
	default: return base_sine;
	}
}

void adlib_card::advance_envelope(uint8_t& stage, float& env_atten, float dt, uint8_t rate_a, uint8_t rate_d, uint8_t rate_s, uint8_t rate_r, bool eg_type)
{
	if (stage == 0) { // IDLE
		env_atten = 511.0f;
		return;
	}

	// Перевод параметра Sustain (0..15) в аппаратные шаги затухания OPL2.
	// Каждый шаг индекса Sustain — это примерно 3 дБ затухания (всего до 24-36 шагов)
	float target_sustain_atten = (float)rate_s * 3.0f;

	switch (stage) {
	case 1: // ATTACK
		// В фазе атаки затухание падает до 0 (звук нарастает)
		env_atten -= attack_steps[rate_a] * dt;
		if (env_atten <= 0.0f) {
			env_atten = 0.0f;
			stage = 2; // Переходим в DECAY
		}
		break;

	case 2: // DECAY
		// В децибелах звук падает = шаги затухания РАСТУТ
		env_atten += decay_release_steps[rate_d] * dt;
		if (env_atten >= target_sustain_atten) {
			env_atten = target_sustain_atten;
			stage = 3; // В SUSTAIN
		}
		break;

	case 3: // SUSTAIN
		if (eg_type == 0) { // Percussive режим (как наше пианино)
			// Звук продолжает затухать со скоростью Release
			env_atten += decay_release_steps[rate_r] * dt;
			if (env_atten >= 511.0f) {
				env_atten = 511.0f;
				stage = 0; // IDLE
			}
		}
		break;

	case 4: // RELEASE
		env_atten += decay_release_steps[rate_r] * dt;
		if (env_atten >= 511.0f) {
			env_atten = 511.0f;
			stage = 0; // IDLE
		}
		break;
	}
}

float adlib_card::get_interpolated_sine(uint32 phase_18bit) {
	// Старшие 10 бит идут в индекс таблицы
	uint32_t idx_curr = (phase_18bit >> 8) & 1023;
	uint32_t idx_next = (idx_curr + 1) & 1023;

	// Младшие 8 бит фазы (0..255) — это наша дробная часть (остаток)
	float frac = (float)(phase_18bit & 0xFF) / 256.0f;

	// Берем два соседних значения из синуса
	float s1 = sine_table[idx_curr];
	float s2 = sine_table[idx_next];

	// Линейно интерполируем между ними
	return s1 + (s2 - s1) * frac;
}

void adlib_card::set_index(uint8 reg_index)
{
	current_index = reg_index;
	//cout << "adlib: set_index = " << (int)reg_index << endl;
}

void adlib_card::set_value(uint8 reg_value)
{
	//присваиваем значение индексу
	index_array[current_index] = reg_value;

	//cout << "ADLIB: set[0x" << hex << (int)current_index << "] = 0x" << hex << (int)reg_value << endl;

	if (current_index == 0x01) wave_select_enable = (reg_value & 0x20) != 0; // Бит 5 разрешает альтернативные волны

	if (current_index == 2)
	{
		timer_1 = reg_value * 80 / 1024 * 1000; //значение таймера 1
		return;
	}

	if (current_index == 3)
	{
		timer_2 = reg_value * 320 / 1024 * 1000; //значение таймера 2
		return;
	}

	if (current_index == 4)
	{
		// ИСПРАВЛЕНО: Бит 7 (сброс флагов) обрабатывается параллельно с масками, а не вместо них!
		if (reg_value & 0x80)
		{
			FT1 = 0;
			FT2 = 0;
			F_IRQ = 0;
		}

		// Маски и триггеры запуска обновляются ВСЕГДА при записи в регистр 04h
		timer_1_masked = ((reg_value >> 6) & 1);
		timer_2_masked = ((reg_value >> 5) & 1);

		timer_1_ON = (reg_value & 1);
		timer_2_ON = ((reg_value >> 1) & 1);

		return;
	}

	//установка параметров каналов
	uint8 group = current_index & 0xE0;  // Определяем группу регистров (20h, 40h, 60h...)
	uint8 offset = current_index & 0x1F; // Смещение внутри группы (00h..1Fh)

	// --- Группа 20h - 35h: Характеристики звука (Tremolo, Vibrato, Sustain, Mult) ---
	if (group == 0x20) {
		uint8 ch = reg_to_channel[offset];
		if (ch == 0xFF) return;

		if (!reg_to_operator[offset]) { // Модулятор (Op1)
			channels[ch].op1_mult = reg_value & 0x0F;
			channels[ch].op1_ksr = (reg_value & 0x10) != 0;
			channels[ch].op1_eg_type = (reg_value & 0x20) != 0;
			channels[ch].op1_vibrato = (reg_value & 0x40) != 0;
			channels[ch].op1_tremolo = (reg_value & 0x80) != 0;
		}
		else { // Носитель (Op2)
			channels[ch].op2_mult = reg_value & 0x0F;
			channels[ch].op2_ksr = (reg_value & 0x10) != 0;
			channels[ch].op2_eg_type = (reg_value & 0x20) != 0;
			channels[ch].op2_vibrato = (reg_value & 0x40) != 0;
			channels[ch].op2_tremolo = (reg_value & 0x80) != 0;
		}
	}

	// --- Группа 40h - 55h: Громкость (KSL / Total Level) ---
	else if (group == 0x40) {
		uint8 ch = reg_to_channel[offset];
		if (ch == 0xFF) return;

		if (!reg_to_operator[offset]) {
			channels[ch].op1_total_level = reg_value & 0x3F;
			channels[ch].op1_ksl = reg_value >> 6;
		}
		else {
			channels[ch].op2_total_level = reg_value & 0x3F;
			channels[ch].op2_ksl = reg_value >> 6;
		}
	}

	// --- Группа 60h - 75h: Атака и Спад (Attack / Decay) ---
	else if (group == 0x60) {
		uint8 ch = reg_to_channel[offset];
		if (ch == 0xFF) return;

		if (!reg_to_operator[offset]) {
			channels[ch].op1_attack = reg_value >> 4;
			channels[ch].op1_decay = reg_value & 0x0F;
		}
		else {
			channels[ch].op2_attack = reg_value >> 4;
			channels[ch].op2_decay = reg_value & 0x0F;
		}
	}

	// --- Группа 80h - 95h: Сустейн и Релиз (Sustain / Release) ---
	else if (group == 0x80) {
		uint8 ch = reg_to_channel[offset];
		if (ch == 0xFF) return;

		if (!reg_to_operator[offset]) {
			channels[ch].op1_sustain = reg_value >> 4;
			channels[ch].op1_release = reg_value & 0x0F;
		}
		else {
			channels[ch].op2_sustain = reg_value >> 4;
			channels[ch].op2_release = reg_value & 0x0F;
		}
	}

	// --- Группа A0h - A8h: Младший байт F-Number ---
	else if (current_index >= 0xA0 && current_index <= 0xA8) {
		uint8 ch = current_index - 0xA0;
		channels[ch].f_number = (channels[ch].f_number & 0x0300) | reg_value;
	}

	// --- Группа B0h - B8h: Старшие биты F-Number, Октава и KEY_ON ---
	else if (current_index >= 0xB0 && current_index <= 0xB8) {
		uint8 ch = current_index - 0xB0;

		channels[ch].f_number = (channels[ch].f_number & 0x00FF) | ((reg_value & 0x03) << 8);
		channels[ch].block = (reg_value >> 2) & 0x07;

		bool new_key_on = (reg_value & 0x20) != 0;

		if (new_key_on) {
			// Если нота включается (или обновляется частота при нажатой клавише)
			if (!channels[ch].key_on) { // Только если это первое нажатие
				channels[ch].op1_phase = 0;
				channels[ch].op2_phase = 0;
				channels[ch].op1_env_stage = 1; // ATTACK
				channels[ch].op2_env_stage = 1;
				channels[ch].op1_env_atten = 511.0f;
				channels[ch].op2_env_atten = 511.0f;
			}
		}
		else {
			// БЕЗУСЛОВНАЯ ЛОГИКА KEY_OFF: Если бит KEY_ON равен 0, 
			// мы ВСЕГДА отправляем работающие операторы в RELEASE(4)
			if (channels[ch].op1_env_stage >= 1 && channels[ch].op1_env_stage <= 3) {
				channels[ch].op1_env_stage = 4;
			}
			if (channels[ch].op2_env_stage >= 1 && channels[ch].op2_env_stage <= 3) {
				channels[ch].op2_env_stage = 4;
			}

			// Сброс фидбэка, чтобы исключить зависание частотной модуляции
			channels[ch].op1_old_out = 0.0f;
			channels[ch].op1_new_out = 0.0f;
		}
		channels[ch].key_on = new_key_on;
	}



	// --- Группа C0h - C8h: Режим синтеза и Обратная связь ---
	else if (current_index >= 0xC0 && current_index <= 0xC8) {
		uint8 ch = current_index - 0xC0;
		channels[ch].feedback_type = reg_value & 0x01;
		channels[ch].feedback_strength = (reg_value >> 1) & 0x07;
	}

	// --- Регистрация формы волны (E0h - F5h) ---
	else if (current_index >= 0xE0 && current_index <= 0xF5) {
		uint8 wave_offset = current_index & 0x1F;
		uint8 ch = reg_to_channel[wave_offset];
		if (ch == 0xFF) return;

		if (!reg_to_operator[wave_offset]) {
			channels[ch].op1_waveform = reg_value & 0x03;
		}
		else {
			channels[ch].op2_waveform = reg_value & 0x03;
		}
	}

	else if (current_index == 0xBD) {
		rhythm_mode = (reg_value & 0x20) != 0; // Бит 5: включение ритм-режима

		// Считываем триггеры ударов
		drum_bd = (reg_value & 0x10) != 0; // Bass Drum
		drum_sd = (reg_value & 0x08) != 0; // Snare Drum
		drum_tom = (reg_value & 0x04) != 0; // Tom-Tom
		drum_tc = (reg_value & 0x02) != 0; // Cymbal
		drum_hh = (reg_value & 0x01) != 0; // Hi-Hat

		// Логика KEY_ON для барабанов: если бит взвелся, переводим огибающие в ATTACK (stage 1)
		if (rhythm_mode) {
			if (drum_bd) { channels[6].op1_env_stage = 1; channels[6].op2_env_stage = 1; }
			if (drum_sd) { channels[7].op2_env_stage = 1; }
			if (drum_tom) { channels[8].op1_env_stage = 1; }
			if (drum_tc) { channels[8].op2_env_stage = 1; }
			if (drum_hh) { channels[7].op1_env_stage = 1; }
		}
	}
}

uint8 adlib_card::read_status()
{
	// Перед чтением статуса обязательно делаем sync, чтобы таймеры успели обновиться
	sync();

	// Аппаратная логика OPL2: флаг прерывания IRQ (бит 7) взводится ТОЛЬКО в том случае,
	// если сработал таймер, который в данный момент НЕ замаскирован игрой!
	bool effective_ft1 = FT1 && !timer_1_masked;
	bool effective_ft2 = FT2 && !timer_2_masked;

	F_IRQ = effective_ft1 || effective_ft2;

	uint8 status = 0;
	if (F_IRQ)         status |= 0x80; // Бит 7: Общий IRQ
	if (effective_ft1) status |= 0x40; // Бит 6: Флаг Таймера 1
	if (effective_ft2) status |= 0x20; // Бит 5: Флаг Таймера 2

	// Биты 0-4 на OPL2 жестко зануляются (в отличие от некоторых других чипов)
	return status;
}

void adlib_card::sync()
{
	//синхронизация
	timer_end = Hi_Res_Clk.now(); //считываем время

	//проверка для таймеров
	int duration_timers = duration_cast<nanoseconds>(timer_end - timer_start_timers).count();

	if (duration_timers)
	{
		//обновляем счетчики и выставляем флаги

		if (timer_1_ON) //счетчик 1 включен
		{
			timer_1 = timer_1 + (duration_timers >> 10) + 10; //вместо 1000 делим на 1024
			if (timer_1 >= 20480 / 1024 * 1000)
			{
				if (!timer_1_masked)
				{
					FT1 = 1; //флаг
					F_IRQ = 1;
				}
				timer_1 = 0; //перезапуск
			}
		}

		if (timer_2_ON) //счетчик 2 включен
		{
			timer_2 = timer_2 + (duration_timers >> 10) + 10;
			if (timer_2 >= 81920 / 1024 * 1000)
			{
				if (!timer_2_masked)
				{
					FT2 = 1; //флаг
					F_IRQ = 1;
				}
				timer_2 = 0; //перезапуск
			}
		}

		timer_start_timers = timer_end; //засекаем заново
	}


	//проверка для генерации сэмплов
	int duration = duration_cast<microseconds>(timer_end - timer_start).count();

	if (duration < (1000000.0f / 48000.0f + sample_time_corr)) return; //выход, если времени слишком мало

	//проверка синхронизации с середине сэмпла семпл 4800 = 50 000 мкс
	if (next_byte_to_gen == 1200)
	{
		int duration_sample = duration_cast<microseconds>(timer_end - timer_start_sample).count();
		if (duration_sample > 12500) sample_time_corr = -2;
		if (duration_sample > 15000) sample_time_corr = -5;
		if (duration_sample < 12000) sample_time_corr = -1;
	}

	if (next_byte_to_gen == 2400)
	{
		int duration_sample = duration_cast<microseconds>(timer_end - timer_start_sample).count();
		if (duration_sample > 25000) sample_time_corr = -3;
		if (duration_sample > 30000) sample_time_corr = -8;
		if (duration_sample < 24500) sample_time_corr = -1;
	}

	if (next_byte_to_gen == 3600)
	{
		int duration_sample = duration_cast<microseconds>(timer_end - timer_start_sample).count();
		if (duration_sample > 37500) sample_time_corr = -3;
		if (duration_sample > 45000) sample_time_corr = -10;
		if (duration_sample < 36500) sample_time_corr = -1;
	}

	timer_start = timer_end; // засекаем заново

	//добавляем очередной сэмпл в массив
	//Частота дискретизации нашего вывода (родная для OPL2)
	const float sample_rate = 49716.0f;
	//const float sample_rate = 48000.0f;  //заменил на нормальную

	float mix_sample = 0.0f;

	// Продвигаем фазы глобальных LFO (частота dt = 20.114 мкс на сэмпл) 3.7 Гц для тремоло
	tremolo_lfo_phase += 0.0000037f * duration;
	// 6.4 Гц для вибрато
	vibrato_lfo_phase += 0.0000064f * duration;

	// Генерируем значения модуляции
	// Тремоло дает колебание затухания от 0 до +24 шагов (примерно 4.8 дБ)
	float current_tremolo_atten = (std::sin(tremolo_lfo_phase * 2.0f * 3.1415926f) + 1.0f) * 12.0f;

	// Вибрато дает небольшое качание частоты (коэффициент от 0.993 до 1.007, то есть ~0.7%)
	float current_vibrato_factor = 1.0f + std::sin(vibrato_lfo_phase * 2.0f * 3.1415926f) * 0.007f;

	// Продвигаем 23-битный LFSR генератор шума OPL2
	// На каждом сэмпле берем биты 22 и 17, делаем XOR и сдвигаем
	uint32_t bit = ((noise_lfsr >> 22) ^ (noise_lfsr >> 17)) & 1;
	noise_lfsr = (noise_lfsr << 1) | bit;
	float current_noise = (noise_lfsr & 1) ? 1.0f : -1.0f; // Результат шума: -1.0 или 1.0


		// Проходим по всем 9 каналам
	for (int ch = 0; ch < 9; ++ch) {
		OPL2_Channel& channel = channels[ch];

		float channel_output = 0.0f; // Результат текущего канала

		// =================================================================
		// БЕЗУСЛОВНАЯ ЗАЩИТА ОГИБАЮЩЕЙ (Для всех 9 каналов без исключения!)
		// =================================================================
		// Если игра отпустила KEY_ON (key_on == false), но огибающие 
		// всё еще находятся в стадиях ATTACK(1), DECAY(2) или SUSTAIN(3),
		// мы ОБЯЗАНЫ принудительно перевести их в стадию RELEASE(4)
		if (!channel.key_on) {
			if (channel.op1_env_stage >= 1 && channel.op1_env_stage <= 3) channel.op1_env_stage = 4;
			if (channel.op2_env_stage >= 1 && channel.op2_env_stage <= 3) channel.op2_env_stage = 4;
		}

		// Если оба оператора аппаратно затихли (IDLE), полностью обнуляем канал и идем дальше
		if (channel.op1_env_stage == 0 && channel.op2_env_stage == 0) {
			channel.op1_old_out = 0.0f;
			channel.op1_new_out = 0.0f;
			continue;
		}

		// 1. ПРОДВИГАЕМ ОГИБАЮЩИЕ ВПЕРЕД ПО ВРЕМЕНИ
		advance_envelope(channel.op1_env_stage, channel.op1_env_atten, 20.1143f,
			channel.op1_attack, channel.op1_decay, channel.op1_sustain, channel.op1_release,
			channel.op1_eg_type);

		advance_envelope(channel.op2_env_stage, channel.op2_env_atten, 20.1143f,
			channel.op2_attack, channel.op2_decay, channel.op2_sustain, channel.op2_release,
			channel.op2_eg_type);

		// 2. Расчет базового шага частоты фазы
		float base_step = (float)channel.f_number * std::pow(2.0f, (float)channel.block - 1.0f);

		// Эффект вибрато применяется только в обычном режиме (не к барабанам)
		bool is_drum_channel = false;
		if (rhythm_mode) {
			if (ch == 6 && drum_bd) is_drum_channel = true; // Бочка на 6-м канале
			if (ch == 7 && (drum_sd || drum_hh)) is_drum_channel = true; // Малый/Хэт на 7-м канале
			if (ch == 8 && (drum_tc || drum_tom)) is_drum_channel = true; // Тарелка/Том на 8-м канале
		}

		float op1_vib = (channel.op1_vibrato && !is_drum_channel) ? current_vibrato_factor : 1.0f;
		float op2_vib = (channel.op2_vibrato && !is_drum_channel) ? current_vibrato_factor : 1.0f;

		uint32_t op1_step = static_cast<uint32_t>(base_step * get_mult_value(channel.op1_mult) * op1_vib);
		uint32_t op2_step = static_cast<uint32_t>(base_step * get_mult_value(channel.op2_mult) * op2_vib);

		// Обновляем фазовые аккумуляторы (18-битная маска OPL2)
		channel.op1_phase = (channel.op1_phase + op1_step) & 0x3FFFF;
		channel.op2_phase = (channel.op2_phase + op2_step) & 0x3FFFF;

		// --- ВЫЧИСЛЕНИЕ АМПЛИТУД С УЧЕТОМ ТРЕМОЛО ---
		float op1_amp = 0.0f;
		if (channel.op1_env_stage != 0) {
			float op1_trem = (channel.op1_tremolo && !is_drum_channel) ? current_tremolo_atten : 0.0f;
			float op1_total_atten = (float)channel.op1_total_level + channel.op1_env_atten + op1_trem;
			if (op1_total_atten < 511.0f) op1_amp = std::pow(2.0f, -op1_total_atten / 16.0f);
		}

		float avg_feedback = (channel.op1_old_out + channel.op1_new_out) * 0.5f;

		// ИСПРАВЛЕНО: Аппаратно-точный логарифмический сдвиг обратной связи OPL2.
		// Вместо линейного умножения сдвигаем фазу на основе avg_feedback, 
		// переведенного в разрядность 18-битного аккумулятора (<< 8).
		int32_t op1_fb_shift = 0;
		if (channel.feedback_strength > 0) {
			// Сила фидбэка OPL2 масштабируется экспоненциально (от сдвига на 1/32 периода до 4 периодов)
			op1_fb_shift = static_cast<int32_t>(avg_feedback * (1 << (channel.feedback_strength + 1))) << 2;
		}

		// Применяем фидбэк ДО сдвига индекса таблицы синуса!
		uint32_t op1_full_phase = static_cast<uint32_t>(int32_t(channel.op1_phase) + op1_fb_shift) & 0x3FFFF;
		uint32_t op1_idx = (op1_full_phase >> 8) & 1023;

		channel.op1_old_out = channel.op1_new_out;
		channel.op1_new_out = get_wave_sample(op1_full_phase, channel.op1_waveform) * op1_amp;

		float op2_amp = 0.0f;
		if (channel.op2_env_stage != 0) {
			float op2_trem = (channel.op2_tremolo && !is_drum_channel) ? current_tremolo_atten : 0.0f;
			float op2_total_atten = (float)channel.op2_total_level + channel.op2_env_atten + op2_trem;
			if (op2_total_atten < 511.0f) op2_amp = std::pow(2.0f, -op2_total_atten / 16.0f);
		}


		// =================================================================
		// РАЗДЕЛЕНИЕ РЕЖИМОВ: ОБЫЧНЫЙ (FM/ADD) ИЛИ БАРАБАНЫ
		// =================================================================
		if (!is_drum_channel) {
			// >>> ОБЫЧНЫЙ МЕЛОДИЧЕСКИЙ РЕЖИМ (СИНТЕЗ ДВИГАТЕЛЯ) <<<

			float avg_feedback = (channel.op1_old_out + channel.op1_new_out) * 0.5f;

			// Ограничиваем фидбэк Модулятора для защиты от хаоса
			if (avg_feedback > 1.0f)  avg_feedback = 1.0f;
			if (avg_feedback < -1.0f) avg_feedback = -1.0f;

			// Исправленный логарифмический фидбэк OPL2
			int32_t op1_fb_shift = 0;
			if (channel.feedback_strength > 0) {
				op1_fb_shift = static_cast<int32_t>(avg_feedback * (1 << (channel.feedback_strength + 1))) << 2;
			}

			// Применяем фидбэк к полной фазе
			uint32_t op1_full_phase = static_cast<uint32_t>(int32_t(channel.op1_phase) + op1_fb_shift) & 0x3FFFF;

			channel.op1_old_out = channel.op1_new_out;
			channel.op1_new_out = get_wave_sample(op1_full_phase, channel.op1_waveform) * op1_amp;

			if (channel.feedback_type == 0) {
				// Режим FM-синтеза
				int32_t fm_phase_modulation = static_cast<int32_t>(channel.op1_new_out * 1024.0f);
				uint32_t op2_full_phase = static_cast<uint32_t>(int32_t(channel.op2_phase) + (fm_phase_modulation << 8) + 0x40000) & 0x3FFFF;
				channel_output = get_wave_sample(op2_full_phase, channel.op2_waveform) * op2_amp;
			}
			else {
				// Режим аддитивного синтеза (Именно здесь поет двигатель F-15!)
				// Подставляем полную интерполированную фазу Носителя
				float op2_out = get_wave_sample(channel.op2_phase, channel.op2_waveform) * op2_amp;
				float op1_active_out = (channel.op1_env_stage != 0) ? channel.op1_new_out : 0.0f;

				// Складываем сигналы Модулятора и Носителя
				channel_output = (op1_active_out + op2_out) * 0.5f;
			}
		}
		else {
			// >>> НАСТОЯЩИЙ РИТМ-РЕЖИМ БАРАБАНОВ (Только при ударах!) <<<

			// КАНАЛ 6: BASS DRUM
			if (ch == 6) {
				uint32_t op1_idx = (channel.op1_phase >> 8) & 1023;
				float op1_out = get_wave_sample(channel.op1_phase, channel.op1_waveform) * op1_amp;

				uint32_t fm_mod = static_cast<uint32_t>(op1_out * 1024.0f);
				uint32_t op2_idx = ((channel.op2_phase >> 8) + fm_mod) & 1023;
				float bd_output = get_wave_sample(channel.op2_phase, channel.op2_waveform) * op2_amp;
				channel_output = bd_output * 4.5f;
			}

			// КАНАЛ 7: CLOSED HI-HAT & SNARE DRUM
			if (ch == 7) {
				if (drum_hh) {
					float hh_output = current_noise * op1_amp * 2.0f;
					mix_sample += hh_output;
				}

				if (drum_sd) {
					uint32_t op2_idx = (channel.op2_phase >> 8) & 1023;
					if (noise_lfsr & 1) op2_idx = (op2_idx + 512) & 1023;
					float sd_output = get_wave_sample(channel.op2_phase, channel.op2_waveform) * op2_amp;
					channel_output = sd_output * 3.8f;
				}
			}

			// КАНАЛ 8: TOM-TOM & CRASH CYMBAL
			if (ch == 8) {
				if (drum_tom) {
					float tom_output = get_wave_sample(channel.op1_phase, channel.op1_waveform) * op1_amp;
					mix_sample += tom_output * 2.5f;
				}

				if (drum_tc) {
					uint32_t op2_idx = (channel.op2_phase >> 8) & 1023;
					if (noise_lfsr & 2) op2_idx = (op2_idx + 256) & 1023;
					float cymbal_output = get_wave_sample(channel.op2_phase, channel.op2_waveform) * op2_amp;
					channel_output = cymbal_output * 2.2f;
				}
			}
		}

		// Микшируем чистый выход канала
		mix_sample += channel_output;
	}



	// Ограничиваем жестко (Clamping), чтобы звук не хрипел, сохраняя громкость
	if (mix_sample > 1.0f) mix_sample = 1.0f;
	if (mix_sample < -1.0f) mix_sample = -1.0f;

	//Audio_monitor.get_adlib_sample(mix_sample * 30000); //передаем данные в монитор для отладки
	//if (mix_sample > 0.000001) cout << (float)mix_sample << endl;

	// Записываем финальный микшированный семпл в выходной буфер и перемещаем указатель буфера
	if (audio_stream.next_buffer_to_play == 0)
	{
		if (next_byte_to_gen < sample_size)
		{
			//Audio_monitor.set_adlib_msg("Gen A(" + to_string(next_byte_to_gen) + ")");
			sound_sample_A[next_byte_to_gen] = (mix_sample + old_sample) * (300.0 / 2.0 * volume);	//левый канал
			next_byte_to_gen++;
			sound_sample_A[next_byte_to_gen] = (mix_sample + old_sample) * (300.0 / 2.0 * volume);	//правый канал
			next_byte_to_gen++;
		}
		else
		{
			//перезапускаем таймер сэмпла
			if (next_byte_to_gen_forward == 0) {
				timer_start_sample = timer_start; sample_time_corr = -1;
			}

			//достигнут конец сэмпла
			//генерируем следующий
			if (next_byte_to_gen_forward < 2000)
			{
				//Audio_monitor.set_adlib_msg("Gen A+");
				sound_sample_B[next_byte_to_gen_forward] = (mix_sample + old_sample) * (300.0 / 2.0 * volume);	//левый канал
				next_byte_to_gen_forward++;
				sound_sample_B[next_byte_to_gen_forward] = (mix_sample + old_sample) * (300.0 / 2.0 * volume);	//правый канал
				next_byte_to_gen_forward++;
				if (next_byte_to_gen_forward == 500) sample_time_corr = 0;
			}
			//если сэмплов >500 они отбрасываются
		}
	}
	if (audio_stream.next_buffer_to_play == 1)
	{
		if (next_byte_to_gen < sample_size)
		{
			//Audio_monitor.set_adlib_msg("Gen B");
			sound_sample_B[next_byte_to_gen] = (mix_sample + old_sample) * (300.0 / 2.0 * volume);	//левый канал
			next_byte_to_gen++;
			sound_sample_B[next_byte_to_gen] = (mix_sample + old_sample) * (300.0 / 2.0 * volume);	//правый канал
			next_byte_to_gen++;
		}
		else
		{
			//перезапускаем таймер сэмпла
			if (next_byte_to_gen_forward == 0) {
				timer_start_sample = timer_start; sample_time_corr = -1;
			}

			//достигнут конец сэмпла
			//генерируем следующий
			if (next_byte_to_gen_forward < 2000)
			{
				//Audio_monitor.set_adlib_msg("Gen B+");
				sound_sample_C[next_byte_to_gen_forward] = (mix_sample + old_sample) * (300.0 / 2.0 * volume);	//левый канал
				next_byte_to_gen_forward++;
				sound_sample_C[next_byte_to_gen_forward] = (mix_sample + old_sample) * (300.0 / 2.0 * volume);	//правый канал
				next_byte_to_gen_forward++;
				if (next_byte_to_gen_forward == 500) sample_time_corr = 0;
			}
		}
	}
	if (audio_stream.next_buffer_to_play == 2)
	{
		if (next_byte_to_gen < sample_size)
		{
			sound_sample_C[next_byte_to_gen] = (mix_sample + old_sample) * (300.0 / 2.0 * volume);	//левый канал
			next_byte_to_gen++;
			sound_sample_C[next_byte_to_gen] = (mix_sample + old_sample) * (300.0 / 2.0 * volume);	//правый канал
			next_byte_to_gen++;
		}
		else
		{
			//перезапускаем таймер сэмпла
			if (next_byte_to_gen_forward == 0) {
				timer_start_sample = timer_start; sample_time_corr = -1;
			}

			//достигнут конец сэмпла
			//генерируем следующий
			if (next_byte_to_gen_forward < 2000)
			{
				sound_sample_A[next_byte_to_gen_forward] = (mix_sample + old_sample) * (300.0 / 2.0 * volume);	//левый канал
				next_byte_to_gen_forward++;
				sound_sample_A[next_byte_to_gen_forward] = (mix_sample + old_sample) * (300.0 / 2.0 * volume);	//правый канал
				next_byte_to_gen_forward++;
				if (next_byte_to_gen_forward == 500) sample_time_corr = 0;
			}
		}
	}

	old_sample = mix_sample;
}

adlib_card::adlib_card()
{
	timer_start = Hi_Res_Clk.now();

	// Генерируем классическую синусоиду от -1.0 до 1.0
	for (int i = 0; i < 1024; i++)
	{
		double angle = (2.0 * 3.1415926 * i) / 1024;
		sine_table[i] = std::sin(angle);
	}

	//подготовка звукового устройства
	audio_stream.sample_size = sample_size;		//передаем внутрь данные о размере сэмпла
	audio_stream.s_buffer_C = sound_sample_C;	//ссылки на массивы генерации
	audio_stream.s_buffer_A = sound_sample_A;	//ссылки на массивы генерации
	audio_stream.s_buffer_B = sound_sample_B;
	audio_stream.setLooping(0);
	audio_stream.play();
}

void AdlibAudioStream::onSeek(sf::Time timeOffset)
{
	return;
}

bool AdlibAudioStream::onGetData(Chunk& data)
{
	//data.sampleCount = soundcard.get_samples_qty();
	if (next_buffer_to_play == 0)
	{
		data.sampleCount = soundcard.change_to_A();
		data.samples = s_buffer_A;
		next_buffer_to_play = 1;
		goto end_label;
	}
	if (next_buffer_to_play == 1)
	{
		data.sampleCount = soundcard.change_to_B();
		data.samples = s_buffer_B;
		next_buffer_to_play = 2;
		goto end_label;
	}
	if (next_buffer_to_play == 2)
	{
		data.sampleCount = soundcard.change_to_C();
		data.samples = s_buffer_C;
		next_buffer_to_play = 0;
		goto end_label;
	}
end_label:
	return true;
}

int adlib_card::change_to_A()
{
	//заканчиваем генерацию сэмпла A и переключаемся на генерацию B
	//сэмпл A подается на воспроизведение

	//добиваем сэмпл нулями, если он не полон
	if (next_byte_to_gen_forward) Audio_monitor.set_adlib_msg("forward = " + to_string(next_byte_to_gen_forward) + " cor = " + to_string(sample_time_corr)); //сколько сэмплов не использовано
	else Audio_monitor.set_adlib_msg("late for = " + to_string(sample_size - next_byte_to_gen) + " cor = " + to_string(sample_time_corr)); //сколько сэмплов не использовано
	//for (int i = 0; i < 20; i++) Audio_monitor.get_adlib_sample(sample_time_corr * 20);
	if (next_byte_to_gen < sample_size) sample_time_corr = -3; //уменьшаем время на каждый сэмпл для ускорения генерации

	while (next_byte_to_gen < sample_size)
	{
		if (next_byte_to_gen) sound_sample_A[next_byte_to_gen] = sound_sample_A[next_byte_to_gen - 1];
		next_byte_to_gen++;
	}

	//перезапускаем таймер сэмпла
	if (next_byte_to_gen_forward == 0)  timer_start_sample = Hi_Res_Clk.now();

	next_byte_to_gen = next_byte_to_gen_forward;
	if (next_byte_to_gen_forward > 500) sample_time_corr = -1;
	if (next_byte_to_gen_forward < 100) sample_time_corr = -2;
	next_byte_to_gen_forward = 0;
	return 4800;
}
int adlib_card::change_to_B()
{
	//заканчиваем генерацию сэмпла B и переключаемся на генерацию A
	//сэмпл B подается на воспроизведение

	//добиваем сэмпл нулями, если он не полон
	if (next_byte_to_gen_forward) Audio_monitor.set_adlib_msg("forward = " + to_string(next_byte_to_gen_forward) + " cor = " + to_string(sample_time_corr)); //сколько сэмплов не использовано
	else Audio_monitor.set_adlib_msg("late for = " + to_string(sample_size - next_byte_to_gen) + " cor = " + to_string(sample_time_corr)); //сколько сэмплов не использовано
	//for (int i = 0; i < 20; i++) Audio_monitor.get_adlib_sample(sample_time_corr * 20);
	if (next_byte_to_gen < sample_size) sample_time_corr = -3; //уменьшаем время на каждый сэмпл для ускорения генерации

	while (next_byte_to_gen < sample_size)
	{
		if (next_byte_to_gen) sound_sample_A[next_byte_to_gen] = sound_sample_A[next_byte_to_gen - 1];
		next_byte_to_gen++;
	}

	//перезапускаем таймер сэмпла
	if (next_byte_to_gen_forward == 0)  timer_start_sample = Hi_Res_Clk.now();

	next_byte_to_gen = next_byte_to_gen_forward;
	if (next_byte_to_gen_forward > 500) sample_time_corr = 0;
	if (next_byte_to_gen_forward < 100) sample_time_corr = -2;
	next_byte_to_gen_forward = 0;
	return 4800;
}
int adlib_card::change_to_C()
{
	//заканчиваем генерацию сэмпла B и переключаемся на генерацию C
	//сэмпл B подается на воспроизведение

	//добиваем сэмпл нулями, если он не полон
	if (next_byte_to_gen_forward) Audio_monitor.set_adlib_msg("forward = " + to_string(next_byte_to_gen_forward) + " cor = " + to_string(sample_time_corr)); //сколько сэмплов не использовано
	else Audio_monitor.set_adlib_msg("late for = " + to_string(sample_size - next_byte_to_gen) + " cor = " + to_string(sample_time_corr)); //сколько сэмплов не использовано
	//for (int i = 0; i<20;i++) Audio_monitor.get_adlib_sample(sample_time_corr * 20);
	if (next_byte_to_gen < sample_size) sample_time_corr = -3; //уменьшаем время на каждый сэмпл для ускорения генерации

	while (next_byte_to_gen < sample_size)
	{
		if (next_byte_to_gen) sound_sample_A[next_byte_to_gen] = sound_sample_A[next_byte_to_gen - 1];
		next_byte_to_gen++;
	}

	//перезапускаем таймер сэмпла
	if (next_byte_to_gen_forward == 0)  timer_start_sample = Hi_Res_Clk.now();

	next_byte_to_gen = next_byte_to_gen_forward;
	if (next_byte_to_gen_forward > 500) sample_time_corr = 0;
	if (next_byte_to_gen_forward < 100) sample_time_corr = -2;
	next_byte_to_gen_forward = 0;
	return 4800;
}
void adlib_card::set_volume(uint8 vol)
{
	volume = vol;
	if (volume > 100) volume = 100;
	//cout << "ADLIB: volume set to " << (int)volume << endl;
}
void adlib_card::volume_up()
{
	if (volume < 98) volume += 3;
	//cout << "ADLIB: volume " << (int)volume << endl;
}
void adlib_card::volume_down()
{
	if (volume > 2) volume -= 3;
	//cout << "ADLIB: volume " << (int)volume << endl;
}

std::string adlib_card::get_channel_debug_info(uint8 ch_idx)
{
	std::ostringstream ss;
	const char stage_chars[] = { 'I', 'A', 'D', 'S', 'R' };

	// --- ВЫВОД ШАПКИ ТАБЛИЦЫ (ИНДЕКС = 9) ---
	if (ch_idx == 9) {
		ss << "RHYTHM MODE: " << (rhythm_mode ? "ON" : "OFF") << "\n";
		ss << std::left
			<< std::setw(3) << "CH"
			<< std::setw(4) << "KEY"
			<< std::setw(7) << "BASE_F"
			<< std::setw(5) << "MODE"
			<< std::setw(3) << "FB"
			<< " | "
			<< std::setw(6) << "DRUM"
			<< " | "
			<< std::setw(3) << "ST1"
			<< std::setw(3) << "M1"
			<< std::setw(8) << "FIN_F1"
			<< std::setw(6) << "AMP1"
			<< " | "
			<< std::setw(3) << "ST2"
			<< std::setw(3) << "M2"
			<< std::setw(8) << "FIN_F2"
			<< std::setw(6) << "AMP2"
			<< " | "
			<< std::setw(8) << "CH_OUT"
			<< "\n";
		ss << std::string(96, '=') << "\n";
		return ss.str();
	}

	if (ch_idx >= 9) return "Error: Invalid index\n";

	const OPL2_Channel& ch = channels[ch_idx];

	// 1. Расчет базовой частоты канала (Гц)
	double base_hz = 0.0;
	if (ch.block > 0) {
		base_hz = ((double)ch.f_number * 49716.0 * (double)(1 << (ch.block - 1))) / 262144.0;
	}
	else {
		base_hz = ((double)ch.f_number * 49716.0 * 0.5) / 262144.0;
	}

	// 2. Расчет финальных частот каждого оператора с учетом Multiplier
	double fin_hz1 = base_hz * get_mult_value(ch.op1_mult);
	double fin_hz2 = base_hz * get_mult_value(ch.op2_mult);

	// 3. Перевод логарифмического затухания в понятную линейную шкалу AMP (0.00..1.00)
	float amp1 = 0.0f;
	if (ch.op1_env_stage != 0) {
		float total_atten1 = (float)ch.op1_total_level + ch.op1_env_atten;
		if (total_atten1 < 511.0f) amp1 = std::pow(2.0f, -total_atten1 / 16.0f);
	}
	float amp2 = 0.0f;
	if (ch.op2_env_stage != 0) {
		float total_atten2 = (float)ch.op2_total_level + ch.op2_env_atten;
		if (total_atten2 < 511.0f) amp2 = std::pow(2.0f, -total_atten2 / 16.0f);
	}

	// 4. Определение активного типа барабана для каналов 6, 7, 8
	std::string drum_name = "NONE";
	if (rhythm_mode) {
		if (ch_idx == 6 && drum_bd) drum_name = "B_DRM";
		if (ch_idx == 7 && (drum_sd || drum_hh)) drum_name = "SD/HH";
		if (ch_idx == 8 && (drum_tc || drum_tom)) drum_name = "TC/TM";
	}

	// Имитируем выход канала для таблицы (берем значение из sync)
	float current_ch_out = 0.0f;
	uint32_t op2_idx = 0;
	if (!(rhythm_mode && ch_idx >= 6)) { // Мелодический режим
		if (ch.feedback_type == 0) {
			int32_t fm_mod = static_cast<int32_t>(ch.op1_new_out * 1024.0f);
			op2_idx = static_cast<uint32_t>((int32_t(ch.op2_phase >> 8) + fm_mod)) & 1023;
			current_ch_out = const_cast<adlib_card*>(this)->get_wave_sample(ch.op2_phase, ch.op2_waveform) * amp2;
		}
		else {
			current_ch_out = (ch.op1_new_out + (const_cast<adlib_card*>(this)->get_wave_sample(ch.op2_phase, ch.op2_waveform) * amp2)) * 0.5f;
		}
	}

	// --- СБОРКА СТРОКИ ---
	ss << std::left
		<< std::setw(3) << (int)ch_idx + 1
		<< std::setw(4) << (ch.key_on ? "ON" : "OFF")
		<< std::setw(7) << std::fixed << std::setprecision(0) << base_hz
		<< std::setw(5) << (ch.feedback_type ? "ADD" : "FM")
		<< std::setw(3) << (int)ch.feedback_strength
		<< " | "
		<< std::setw(6) << drum_name
		<< " | "
		// Модулятор
		<< std::setw(3) << stage_chars[ch.op1_env_stage]
		<< std::setw(3) << (int)ch.op1_mult
		<< std::setw(8) << std::fixed << std::setprecision(0) << fin_hz1
		<< std::setw(6) << std::fixed << std::setprecision(3) << amp1
		<< " | "
		// Носитель
		<< std::setw(3) << stage_chars[ch.op2_env_stage]
		<< std::setw(3) << (int)ch.op2_mult
		<< std::setw(8) << std::fixed << std::setprecision(0) << fin_hz2
		<< std::setw(6) << std::fixed << std::setprecision(3) << amp2
		<< " | "
		<< std::setw(8) << std::fixed << std::setprecision(4) << current_ch_out
		<< "\n";

	return ss.str();
}

