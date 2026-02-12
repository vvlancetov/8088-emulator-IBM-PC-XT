#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/System.hpp>
#include "video.h"
#include "custom_classes.h"
#include <string>
#include <chrono>

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

using namespace std::chrono;


class MyAudioStream : public sf::SoundStream
{
	bool onGetData(Chunk& data) override;
	void onSeek(sf::Time timeOffset) override;

public:

	std::chrono::steady_clock::time_point timer_start; //для отслеживания времени выполнения
	std::chrono::steady_clock::time_point timer_end;
	uint32 duration; //продолжительность проигрывания сэмпла
	MyAudioStream()
	{
		initialize(2, 48000, {sf::SoundChannel::FrontLeft, sf::SoundChannel::FrontRight});
		setLooping(0);
		timer_start = steady_clock::now();
		timer_end = steady_clock::now();
	}

	std::int16_t* s_buffer;			//ссылка на текущий буфер для звука
	std::int16_t* s_buffer_A;		//ссылка на буфер для звука A
	std::int16_t* s_buffer_B;		//ссылка на буфер для звука B
	int next_buffer_to_play = 0;	//следующий буфер 0 - A, 1 - B
	int sample_size;				//размер сэмпла
	bool buffer_changed = false;	//флаг смены буфера
};

class SoundMaker //класс звуковой карты
{
private:

	int sample_size = 4800;				//длина звукового сэмпла  - 1/20 секунды
	int16_t sound_sample_A[4800];		//массив для сэмплов (числа со знаком по модулю 32000)
	int16_t sound_sample_B[4800];		//второй массив
	
	int16_t avg_arr[16];					//усредняющий массив
	uint8 avg_arr_ptr = 0;				//указатель следующего элемента в массиве
	int next_byte_to_gen = 0;			//позиция следующего байта для генерации
	//int sample_to_gen = 0;				//текущий сэмпл 0 - A, 1 - B.
	int sample_overhead = 100;			//дополнительные сэмплы для генерации на опережение
	int max_amplitude = 30000;			//максимальная амплитуда сигнала (потолок примерно 32000)
	int overhead_counter = 0;			//счетчик "лишних" сгенерированных наперед сэмплов
	bool count_overhead = 0;			//считать "лишние" сэмплы
	std::chrono::steady_clock::time_point timer_start; //для отслеживания времени выполнения
	std::chrono::steady_clock::time_point timer_end;
	uint8 spacer = 10;
	uint8 volume = 30; //громкость звука
	

public:
	int16_t empty_sound_sample[4800];	//массив-заглушка
	MyAudioStream audio_stream;
	SoundMaker();				//декларируем конструктор
	bool beeping = false;		//издает ли звук пищалка
	//bool freq_changed = true;  // частота таймера изменилась
	uint16 timer_freq = 0;		//частота звука, запрограммированная на таймере
	void sync();				//синхронизация - генерация сэмплов
	void beep_on();     //сигнал ВКЛ
	void beep_off();    //сигнал ВЫКЛ
	void put_sample(int16_t sample); //подаем сигнал от таймера
	uint32 duration; //средняя продолжительность создания сэмпла
	uint32 raw_duration[8] = { 0 };
	uint8 raw_duration_ptr = 0;
	void set_volume(uint8 vol);
	void change_to_A();	//подготовить сэмпл А
	void change_to_B(); //подготовить сэмпл В
	bool wait_for_dispatch = 0;
};

//класс аудио_монитора
class Audio_mon_device : public Dev_mon_device
{
	int16_t sample_array[5000] = { 0 };
	uint16 array_pointer_next_el = 0;
	int array_pointer_to_draw = 0;
	std::string pinout_11;

public:
	using Dev_mon_device::Dev_mon_device;
	void sync();
	void get_sample(int16_t sample);
	int generation_overhead = 999;
	std::string curr_buffer = "--";
	std::string beepping = "--";
	void set_pinout(uint8 data); //сигналы PPI
	int empty_samples_A;
	int empty_samples_B;
	int over_samples_A;
	int over_samples_B;
};

