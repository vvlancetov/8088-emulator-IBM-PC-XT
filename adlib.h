#pragma once
#include <SFML/Audio.hpp>
#include <chrono>
#include <vector>
#include <cmath>

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

class AdlibAudioStream : public sf::SoundStream
{
    bool onGetData(Chunk& data) override;
    void onSeek(sf::Time timeOffset) override;

public:

    //std::chrono::steady_clock::time_point timer_start; //для отслеживания времени выполнения
    //std::chrono::steady_clock::time_point timer_end;
    //uint32 duration = 0; //продолжительность проигрывания сэмпла
    AdlibAudioStream()
    {
        initialize(2, 48000, { sf::SoundChannel::FrontLeft, sf::SoundChannel::FrontRight });
        setLooping(0);
    }

    std::int16_t* s_buffer_C;		//ссылка на буфер для звука C
    std::int16_t* s_buffer_A;		//ссылка на буфер для звука A
    std::int16_t* s_buffer_B;		//ссылка на буфер для звука B
    int next_buffer_to_play = 0;	//следующий буфер 0 - A, 1 - B, 2 - C
    int sample_size = 0;			//размер сэмпла
    //bool buffer_changed = false;	//флаг смены буфера
};


struct OPL2_Channel {
    // === Общие параметры канала ===
    uint16   f_number = 0;       // 10 бит частоты (0..1023)
    uint8    block = 0;          // Октава (0..7)
    bool     key_on = false;     // Статус проигрывания ноты
    bool     feedback_type = 0;  // 0 = FM-синтез, 1 = Аддитивный
    uint8    feedback_strength = 0; // Сила обратной связи для Модулятора (0..7)

    // === Параметры ОПЕРАТОРА 1 (Модулятор) ===
    uint8 op1_mult = 0;        // Множитель частоты (0..15)
    bool    op1_ksr = false;     // Key Scale Rate (флаг)
    bool    op1_eg_type = false; // Тип огибающей: 0 = затухающая, 1 = сустейн-режим
    bool    op1_vibrato = false; // Включение вибрато
    bool    op1_tremolo = false; // Включение тремоло
    uint8   op1_total_level = 0; // Общая громкость оператора (0..63)
    uint8   op1_ksl = 0;         // Key Scale Level (0..3)
    uint8   op1_attack = 0;      // Скорость атаки (0..15)
    uint8   op1_decay = 0;       // Скорость спада (0..15)
    uint8   op1_sustain = 0;     // Уровень удержания (0..15)
    uint8   op1_release = 0;     // Скорость затухания (0..15)
    uint8   op1_waveform = 0;    // Форма волны (0..3)
    float op1_env_atten = 511.0f;

    // === Параметры ОПЕРАТОРА 2 (Носитель) ===
    uint8   op2_mult = 0;
    bool    op2_ksr = false;
    bool    op2_eg_type = false;
    bool    op2_vibrato = false;
    bool    op2_tremolo = false;
    uint8   op2_total_level = 0;
    uint8   op2_ksl = 0;
    uint8   op2_attack = 0;
    uint8   op2_decay = 0;
    uint8   op2_sustain = 0;
    uint8   op2_release = 0;
    uint8   op2_waveform = 0;
    float op2_env_atten = 511.0f;

    // === Внутреннее состояние синтеза (динамические переменные) ===
    uint32   op1_phase = 0;      // Фазовый аккумулятор модулятора
    uint32   op2_phase = 0;      // Фазовый аккумулятор носителя
    uint8    op1_env_stage = 0;  // Текущая фаза ADSR (0=IDLE, 1=ATTACK, 2=DECAY, 3=SUSTAIN, 4=RELEASE)
    uint8    op2_env_stage = 0;
    float    op1_env_vol = 0.0f; // Текущий коэффициент громкости огибающей (0.0 .. 1.0)
    float    op2_env_vol = 0.0f;
    
    // Внутренние буферы для обратной связи Оператора 1
    float op1_old_out = 0.0f;
    float op1_new_out = 0.0f;
};

class adlib_card
{
private:
	uint8 current_index = 0;
	uint8 index_array[256] = { 0 };
	//отдельный таймер для синхронизации реального времени
	std::chrono::high_resolution_clock Hi_Res_Clk;
    std::chrono::steady_clock::time_point timer_start; //для отслеживания времени выполнения
    std::chrono::steady_clock::time_point timer_start_timers; //для отслеживания времени выполнения таймеров
    std::chrono::steady_clock::time_point timer_end;
    std::chrono::steady_clock::time_point timer_start_sample;

    int sample_time_corr = -1;
    float timer_1 = 0;
	float timer_2 = 0;
	bool FT1 = 0;
	bool FT2 = 0;
	bool F_IRQ = 0;
	bool timer_1_ON = 0;
	bool timer_2_ON = 0;
	bool timer_1_masked = 0;
	bool timer_2_masked = 0;
    bool wave_select_enable = false; // Флаг из регистра 01h, бит 5
    OPL2_Channel channels[9];

    // Массив трансляции смещения регистра (Reg & 0x1F) в номер канала (0..8).
    // Значение 0xFF означает, что это смещение не привязано к каналу.
    const uint8 reg_to_channel[32] = {
        0,  1,  2,  0,  1,  2, 0xFF, 0xFF, // 00h - 07h
        3,  4,  5,  3,  4,  5, 0xFF, 0xFF, // 08h - 0Fh
        6,  7,  8,  6,  7,  8, 0xFF, 0xFF, // 10h - 17h
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };

    // Массив определения оператора: true = Носитель (Op2), false = Модулятор (Op1)
    const bool reg_to_operator[32] = {
        false, false, false, true,  true,  true,  false, false,
        false, false, false, true,  true,  true,  false, false,
        false, false, false, true,  true,  true,  false, false,
        false, false, false, false, false, false, false, false
    };

    // Коэффициенты изменения амплитуды (из расчета на 1 микросекунду)
    // Индексы от 0 до 15. Значение 0 — фаза стоит на месте.
    const float attack_steps[16] = {
        0.0f, 0.00075f, 0.0015f, 0.003f, 0.006f, 
        0.012f, 0.024f, 0.048f, 0.096f, 0.192f,
        0.384f, 0.768f, 1.536f, 3.072f, 6.144f, 511.0f
    };

    const float decay_release_steps[16] = {
        0.0f, 0.00003f, 0.00006f, 0.00012f, 
        0.00024f, 0.00048f, 0.00096f, 0.00192f,
        0.00384f, 0.00768f, 0.01536f, 0.03072f, 
        0.06144f, 0.12288f, 0.24576f, 511.0f
    };




    // Перевод параметра Sustain (0..15) в целевой уровень громкости (1.0 .. 0.0)
    // 0 в OPL2 — это максимальный сустейн (громкость не падает, коэффициент 1.0)
    // 15 — минимальный сустейн (громкость падает почти до нуля)
    float get_sustain_level(uint8_t sustain_idx) {
        return (15.0f - sustain_idx) / 15.0f;
    }

    // Глобальная или статическая таблица синуса
    float sine_table[1024];

    //счетчик для генератора 
    int duration_count = 0;
    //int duration_corr = 0;

    //сэмплы для звука
    int sample_size = 4800;				//длина звукового сэмпла  - 1/20 секунды
    int16_t sound_sample_A[4800];		//массив для сэмплов (числа со знаком по модулю 32000)
    int16_t sound_sample_B[4800];		//второй массив
    int16_t sound_sample_C[4800];	    //третий массив
    int next_byte_to_gen = 0;			//позиция следующего байта для генерации
    int next_byte_to_gen_forward = 0;   //то же для генерации еще одного сэмпла
    int sample_overhead = 100;			//дополнительные сэмплы для генерации на опережение
    int overhead_counter = 0;			//счетчик "лишних" сгенерированных наперед сэмплов
    //int max_amplitude = 30000;		//максимальная амплитуда сигнала (потолок примерно 32000)
    uint8 volume = 0;					//громкость звука
    float old_sample = 0.0f;            //старый семпл для усреднения
    float tremolo_lfo_phase = 0.0f;
    float vibrato_lfo_phase = 0.0f;
    // Бит 5 регистра BDh - режим барабанов
    bool rhythm_mode = false;
    // Биты 0-4 регистра BDh (состояние триггеров KEY_ON для барабанов)
    bool drum_bd = false, drum_sd = false, drum_tom = false, drum_tc = false, drum_hh = false;

    // Аппаратный генератор псевдослучайного шума OPL2 (23-битный LFSR сдвиговый регистр)
    uint32_t noise_lfsr = 1;


public:
	adlib_card();
	void set_index(uint8 reg_index);
	void set_value(uint8 reg_value);
    float get_wave_sample(uint32 table_idx, uint8 waveform);
    void advance_envelope(uint8_t& stage, float& env_atten, float dt, uint8_t rate_a, uint8_t rate_d, uint8_t rate_s, uint8_t rate_r, bool eg_type);
    float get_interpolated_sine(uint32 phase_18bit);
    uint8 read_status();
	void sync();
    std::string get_channel_debug_info(uint8 ch_idx);
    AdlibAudioStream audio_stream;
    int change_to_A();
    int change_to_B();
    int change_to_C();
    void set_volume(uint8 vol);
    void volume_up();
    void volume_down();

};
