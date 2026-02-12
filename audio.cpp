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
	//выравнивание частот таймера и сэмпла
	static float pos = 0;
	bool can_hear = 0;
	if (wait_for_dispatch) return;
	pos = pos + 0.06;  // 0.0805 ОК
	if (pos < 1.0) return;
	else pos--;
    
	avg_arr_ptr++;
	if (avg_arr_ptr == 16) avg_arr_ptr = 0;
	if (next_byte_to_gen >= 10 && next_byte_to_gen <= (sample_size - 10)) avg_arr[avg_arr_ptr] = sample * 32000 * volume / 100;
	else avg_arr[avg_arr_ptr] = 0;
	
	int16_t agv_sample = 0;
	for (int i = 0; i < 16; i++) agv_sample = agv_sample + avg_arr[i]/16;
		
	//получение сэмпла от таймера
	if (timer_freq > 10 && timer_freq < 5000) can_hear = 1;
	if (step_mode || log_to_console) can_hear = 0;
	
	if (audio_stream.next_buffer_to_play == 0) //пишем сэмпл A
	{
		if (beeping && can_hear) sound_sample_A[next_byte_to_gen] = agv_sample;
		else sound_sample_A[next_byte_to_gen] = 0; //если звука нет, то = 0
		//Audio_monitor.get_sample(sound_sample_A[next_byte_to_gen]);
	}
	else  //пишем сэмпл B
	{
		if (beeping && can_hear) sound_sample_B[next_byte_to_gen] = agv_sample;
		else sound_sample_B[next_byte_to_gen] = 0; //если звука нет, то = 0
		//Audio_monitor.get_sample(sound_sample_B[next_byte_to_gen]);
	}

	next_byte_to_gen++;
	if (next_byte_to_gen == sample_size) wait_for_dispatch = 1; //ждем запроса сэмпла

}

bool MyAudioStream::onGetData(Chunk& data)
{
	if (next_buffer_to_play == 0)
	{
		//cout << "stream: change to A" << endl;
		speaker.change_to_A();
		//cout << "stream: play A" << endl;
		data.samples = s_buffer_A;
		next_buffer_to_play = 1;
		speaker.wait_for_dispatch = 0; //снимаем блокировку генерации
	}
	else
	{
		//cout << "stream: change to B" << endl;
		speaker.change_to_B();
		//cout << "stream: play B" << endl;
		data.samples = s_buffer_B;
		next_buffer_to_play = 0;
		speaker.wait_for_dispatch = 0; //снимаем блокировку генерации
	}
	data.sampleCount = sample_size;
	return true;
}

void SoundMaker::change_to_A()  //начинаем играть А
{
	//заканчиваем генерацию сэмпла A и переключаемся на генерацию B
	if (audio_stream.next_buffer_to_play == 0)
	{
		
		//cout << "end gen A, wait_for_dispatch = " << (int)wait_for_dispatch << " left_to_gen = " << (int)(sample_size - next_byte_to_gen) << endl;
		int empty_s = 0;
		while (next_byte_to_gen < sample_size)
		{
			sound_sample_A[next_byte_to_gen] = 0;
			next_byte_to_gen++;
			empty_s++;
		}
		
		//sample_to_gen = 1; //на начало сэмпла B
		next_byte_to_gen = 0;
		// обновляем таймер генерации
		//timer_end = chrono::steady_clock::now(); //считываем время
		//raw_duration[raw_duration_ptr] = chrono::duration_cast<chrono::microseconds>(timer_end - timer_start).count();
		//raw_duration_ptr++;
		//if (raw_duration_ptr == 8) raw_duration_ptr = 0;
		//duration = 0;
		//for (int y = 0; y < 8; y++) duration += raw_duration[y] / 8;
		//Audio_monitor.curr_buffer = "B";
		//timer_start = chrono::steady_clock::now(); //засекаем заново
		//Audio_monitor.empty_samples_A = empty_s;
	}
	else
	{
		//если уже генеруем В, ничего не делаем
		//Audio_monitor.empty_samples_A = 0;
		//Audio_monitor.over_samples_A = next_byte_to_gen;
		//next_byte_to_gen = 0;//перезапуск с начала
		//cout << "order messed (block A)" << endl;
	}
	//wait_for_dispatch = 0; //снимаем блокировку генерации
}

void SoundMaker::change_to_B() //начинаем играть В
{
	//заканчиваем генерацию сэмпла B и переключаемся на генерацию A
	if (audio_stream.next_buffer_to_play == 1)
	{
		//cout << "end gen B, wait_for_dispatch = " << (int)wait_for_dispatch << " left_to_gen = " << (int)(sample_size - next_byte_to_gen) << endl;
		int empty_s = 0;
		while (next_byte_to_gen < sample_size)
		{
			sound_sample_B[next_byte_to_gen] = 0;
			next_byte_to_gen++;
			empty_s++;
		}
		
		//sample_to_gen = 0;//на начало сэмпла A
		next_byte_to_gen = 0;
		// обновляем таймер генерации
		/*
		timer_end = chrono::steady_clock::now(); //считываем время
		raw_duration[raw_duration_ptr] = chrono::duration_cast<chrono::microseconds>(timer_end - timer_start).count();
		raw_duration_ptr++;
		if (raw_duration_ptr == 8) raw_duration_ptr = 0;
		duration = 0;
		for (int y = 0; y < 8; y++) duration += raw_duration[y] / 8;
		Audio_monitor.curr_buffer = "A";
		timer_start = chrono::steady_clock::now(); //засекаем заново
		*/
		//Audio_monitor.empty_samples_B = empty_s;
		
	}
	else
	{
		//если уже генеруем В, ничего не делаем
		//Audio_monitor.empty_samples_B = 0;
		//Audio_monitor.over_samples_B = next_byte_to_gen;
		//next_byte_to_gen = 0; //перезапуск с начала
		//cout << "order messed (block B)" << endl;
	}
	//wait_for_dispatch = 0; //снимаем блокировку генерации
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
	text.setString("buffer = " + curr_buffer + "\nplay duration = " + to_string(speaker.audio_stream.duration) + " \ngen duration = " + to_string(speaker.duration) + "\npinout = " + pinout_11 + "\nfreq = " + to_string(speaker.timer_freq) + "\nbeep_on = " + to_string(speaker.beeping));
	text.setPosition(sf::Vector2f(10, 5));
	main_window.draw(text);

	text.setString("empty_s_A = " + to_string(empty_samples_A) + "\nempty_s_B = " + to_string(empty_samples_B) + "\nover_s_A = " + to_string(over_samples_A) + "\nover_s_B = " + to_string(over_samples_B) + "\nwait_for_dispatch = " + to_string((int)speaker.wait_for_dispatch));
	text.setPosition(sf::Vector2f(300, 5));
	main_window.draw(text);
	
	main_window.display();
	while (main_window.pollEvent()) {};
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