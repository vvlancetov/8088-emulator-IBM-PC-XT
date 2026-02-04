#include <conio.h>
#include <bitset>
#include <iostream>
#include <chrono>
#include "audio.h"

#define DEBUG

using namespace std;
#ifdef DEBUG
extern Audio_mon_device Audio_monitor;
#endif

extern bool step_mode;
extern bool log_to_console;
extern SoundMaker speaker;

template< typename T >
extern std::string int_to_bin(T i);

//звуковая карта и пищалка
void SoundMaker::sync()
{
	return;
	//if (beeping) Audio_monitor.beepping = "ON";
	//else Audio_monitor.beepping = "OFF";
	//timer_freq = 300;
	bool can_hear = (timer_freq > 50 && timer_freq < 4000);

	//считаем опережение генерации
	if (count_overhead)
	{
		if (audio_stream.buffer_changed == false) overhead_counter++;
		else
		{
			audio_stream.buffer_changed = false;  //снимаем флаг
#ifdef DEBUG 
			Audio_monitor.generation_overhead = overhead_counter;
#endif
			overhead_counter = 0;				//обнуляем счетчик
			count_overhead = false;				//остановка счетчика
		}
	}
	
	//поправка частоты звука
	if (timer_freq < 50) timer_freq = 50;
	if (timer_freq > 4000) timer_freq = 4000;

	//рассчет кол-ва сэмплов на один период колебаний
	int step = 22050 * 2 / timer_freq;
	
	//рассчет продолжительности генерации:	1/50 секунды + небольшой задел + округление до целого
	int samples_to_gen = ceil((22050 * 2 / 200 + sample_overhead) / step) * step;
	
	for (int i = 0; i < samples_to_gen; i++)
	{
		int mini_step = i % step;
		if (sample_to_gen == 0)
		{
			//пишем сэмпл A
			//if (beeping && can_hear) sound_sample_A[next_byte_to_gen] = max_amplitude * sin(mini_step * 2 * 3.1415926 / step);  //синус
			if (beeping && can_hear)
			{
				sound_sample_A[next_byte_to_gen] = max_amplitude *3 * sin(mini_step * 2 * 3.1415926 / step);  //резкий синус
				if (sound_sample_A[next_byte_to_gen] > max_amplitude) sound_sample_A[next_byte_to_gen] = max_amplitude;
				if (sound_sample_A[next_byte_to_gen] < -max_amplitude) sound_sample_A[next_byte_to_gen] = -max_amplitude;

			}
			if (beeping && can_hear)
			{
				//if (mini_step < step / 2) sound_sample_A[next_byte_to_gen] = max_amplitude;  //квадрат
				//else sound_sample_A[next_byte_to_gen] = -max_amplitude;
			}
			else sound_sample_A[next_byte_to_gen] = 0; //если звука нет, то = 0
			//передаем в отладчик
#ifdef DEBUG
			//Audio_monitor.get_sample(sound_sample_A[next_byte_to_gen]);
#endif
		}
		else
		{
			//пишем сэмпл B
			//if (beeping && can_hear) sound_sample_B[next_byte_to_gen] = max_amplitude * sin(mini_step * 2 * 3.1415926 / step);
			if (beeping && can_hear)
			{
				sound_sample_B[next_byte_to_gen] = max_amplitude * 3 * sin(mini_step * 2 * 3.1415926 / step);  //резкий синус
				if (sound_sample_B[next_byte_to_gen] > max_amplitude) sound_sample_B[next_byte_to_gen] = max_amplitude;
				if (sound_sample_B[next_byte_to_gen] < -max_amplitude) sound_sample_B[next_byte_to_gen] = -max_amplitude;

			}
			if (beeping && can_hear)
			{
				//if (mini_step < step / 2) sound_sample_B[next_byte_to_gen] = max_amplitude;
				//else sound_sample_B[next_byte_to_gen] = -max_amplitude;
			}
			else sound_sample_B[next_byte_to_gen] = 0; //если звука нет, то = 0
			//передаем в отладчик
#ifdef DEBUG
			//Audio_monitor.get_sample(sound_sample_B[next_byte_to_gen]);
#endif
		}

		next_byte_to_gen++;
		if (next_byte_to_gen == sample_size)
		{
			next_byte_to_gen = 0; //переход к началу
			//передаем ссылку на готовый массив сэмплов
			if (sample_to_gen == 0)
			{
				audio_stream.next_buffer_to_play = 0;
				sample_to_gen = 1;		//смена сэмпла генерации
#ifdef DEBUG
				Audio_monitor.curr_buffer = "A";
#endif
			}
			else 
			{
				audio_stream.next_buffer_to_play = 1;
				sample_to_gen = 0;		//смена сэмпла генерации
#ifdef DEBUG
				Audio_monitor.curr_buffer = "B";
#endif
			}
			count_overhead = true; //считаем "лишние" сэмплы
			//также это сигнал того, что другой сэмпл готов
		}
	}
}
void SoundMaker::beep_on()     //сигнал ВКЛ
{
	beeping = true;
}
void SoundMaker::beep_off()    //сигнал ВЫКЛ
{
	beeping = false;
}
void SoundMaker::put_sample(int16_t sample)
{
	avg_arr_ptr++;
	if (avg_arr_ptr == spacer) avg_arr_ptr = 0;
	avg_arr[avg_arr_ptr] = sample;
	if (avg_arr_ptr != 0) return;

	int16_t agv_sample = sample * max_amplitude;
	//int16_t agv_sample = sample * max_amplitude;
	//for (int i = 0; i < 13; i++) agv_sample += avg_arr[i];
	//agv_sample = (agv_sample / 13.0) * max_amplitude;

	//получение сэмпла от таймера
	bool can_hear = (timer_freq > 0 && timer_freq < 5000);
	can_hear = 1;
	if (step_mode || log_to_console) can_hear = 0;
	if (sample_to_gen == 0)
	{
		//пишем сэмпл A
		//if (beeping && can_hear) sound_sample_A[next_byte_to_gen] = max_amplitude * sin(mini_step * 2 * 3.1415926 / step);  //синус
		if (beeping && can_hear)
		{
			sound_sample_A[next_byte_to_gen] = agv_sample;
			//cout << agv_sample << " ";
			//if (sound_sample_A[next_byte_to_gen] > max_amplitude) sound_sample_A[next_byte_to_gen] = max_amplitude;
			//if (sound_sample_A[next_byte_to_gen] < -max_amplitude) sound_sample_A[next_byte_to_gen] = -max_amplitude;
		}
		else sound_sample_A[next_byte_to_gen] = 0; //если звука нет, то = 0
		//передаем в отладчик
#ifdef DEBUG
		Audio_monitor.get_sample(sound_sample_A[next_byte_to_gen]);
#endif
	}
	else
	{
		//пишем сэмпл B
		//if (beeping && can_hear) sound_sample_B[next_byte_to_gen] = max_amplitude * sin(mini_step * 2 * 3.1415926 / step);
		if (beeping && can_hear)
		{
			sound_sample_B[next_byte_to_gen] = agv_sample;
			//cout << agv_sample << " ";
			//if (sound_sample_B[next_byte_to_gen] > max_amplitude) sound_sample_B[next_byte_to_gen] = max_amplitude;
			//if (sound_sample_B[next_byte_to_gen] < -max_amplitude) sound_sample_B[next_byte_to_gen] = -max_amplitude;
		}
		else sound_sample_B[next_byte_to_gen] = 0; //если звука нет, то = 0
		//передаем в отладчик
#ifdef DEBUG
		Audio_monitor.get_sample(sound_sample_B[next_byte_to_gen]);
#endif
	}

	next_byte_to_gen++;
	if (next_byte_to_gen == sample_size)
	{
		timer_end = chrono::steady_clock::now(); //считываем время
		raw_duration[raw_duration_ptr] = chrono::duration_cast<chrono::microseconds>(timer_end - timer_start).count();
		raw_duration_ptr++;
		if (raw_duration_ptr == 8) raw_duration_ptr = 0;
		duration = 0;
		for (int y = 0; y < 8; y++) duration += raw_duration[y] / 8;
		if (duration < 45000 && spacer > 5) spacer--;
		if (duration < 55000 && spacer < 15) spacer++;

		next_byte_to_gen = 0; //переход к началу
		//передаем ссылку на готовый массив сэмплов
		if (sample_to_gen == 0)
		{
			audio_stream.next_buffer_to_play = 0;
			sample_to_gen = 1;		//смена сэмпла генерации
#ifdef DEBUG
			Audio_monitor.curr_buffer = "A";
#endif
		}
		else
		{
			audio_stream.next_buffer_to_play = 1;
			sample_to_gen = 0;		//смена сэмпла генерации
#ifdef DEBUG
			Audio_monitor.curr_buffer = "B";
#endif
		}
		count_overhead = true; //считаем "лишние" сэмплы
		timer_start = chrono::steady_clock::now(); //засекаем заново
		//также это сигнал того, что другой сэмпл готов
	}

}

bool MyAudioStream::onGetData(Chunk& data)
{
	timer_end = chrono::steady_clock::now(); //считываем время
	duration = chrono::duration_cast<chrono::microseconds>(timer_end - timer_start).count();
	if (next_buffer_to_play == 0) data.samples = s_buffer_A;
	else data.samples = s_buffer_B;
	if (step_mode) data.samples = speaker.empty_sound_sample;
	data.sampleCount = sample_size;
	buffer_changed = true;		//ставим флаг что буфер заменен
	timer_start = chrono::steady_clock::now(); //засекаем заново
	return true;
}
void MyAudioStream::onSeek(sf::Time timeOffset)
{
	return;
}

SoundMaker::SoundMaker() // конструктор класса
{
	//audio_stream.buffer_ready = false;
	audio_stream.sample_size = sample_size;		//передаем внутрь данные о размере сэмпла
	audio_stream.s_buffer = empty_sound_sample; //ссылка на пустой массив
	audio_stream.s_buffer_A = sound_sample_A;	//ссылки на массивы генерации
	audio_stream.s_buffer_B = sound_sample_B;
	audio_stream.setLooping(0);
	audio_stream.play();
	timer_start = steady_clock::now();
	timer_end = steady_clock::now();
}

void Audio_mon_device::sync()
{

	//return;

	//синхронизация монитора (окно 2000х300)
	main_window.clear(sf::Color::Black);// очистка экрана
	
	sf::RectangleShape rectangle; //столбик
	rectangle.setSize(sf::Vector2f(2, 2));
	rectangle.setFillColor(sf::Color(200, 200, 200));
	//выводим данные
	array_pointer_to_draw = array_pointer_next_el;
	/*
	for (int x = 2000; x >= 0; --x)
	{
		if (!array_pointer_to_draw) array_pointer_to_draw = 1999;
		else array_pointer_to_draw--;
		rectangle.setPosition(sf::Vector2f(x, 100 - sample_array[array_pointer_to_draw] / 200));
		main_window.draw(rectangle);
	}
	//if (sample_array[0]) step_mode = 1;  //останов для отладки
	*/
	
	//выводим надписи
	
	sf::Text text(font);				
	text.setCharacterSize(25);
	text.setFillColor(sf::Color::White);
	text.setOutlineColor(sf::Color::Black);
	text.setOutlineThickness(3);
	text.setString("play duration = " + to_string(speaker.audio_stream.duration) + " \ngen duration = " + to_string(speaker.duration) + "\npinout = " + pinout_11);
	text.setPosition(sf::Vector2f(10, 5));
	main_window.draw(text);
		
	
	main_window.display();
}
void Audio_mon_device::get_sample(int16_t sample)
{
	//добавляем число в массив
	array_pointer_next_el++;

	//переходим на начало массива
	if (array_pointer_next_el == 5000) array_pointer_next_el = 0;

	sample_array[array_pointer_next_el] = sample;
}

void Audio_mon_device::set_pinout(uint8 data)
{
	pinout_11 = int_to_bin(data);
}

void SoundMaker::set_volume(uint8 vol)
{
	volume = vol;
	if (volume > 100) volume = 100;
}