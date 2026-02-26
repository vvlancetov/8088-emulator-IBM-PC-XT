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

//динамик
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
	//усредняющий массив
	avg_arr_ptr++;
	if (avg_arr_ptr == 16) avg_arr_ptr = 0;
	avg_arr[avg_arr_ptr] = sample * 32000 * volume / 100;

	//выравнивание частот таймера и сэмпла
	static float pos = 0;
	bool can_hear = 0;
	pos = pos + 0.0820;  // 0.0805 ОК
	if (pos < 1.0) return;
	else pos--;
	
	//расчет среднего по массиву
	int16_t agv_sample = 0;
	for (int i = 0; i < 16; i++) agv_sample = agv_sample + avg_arr[i]/16;
		
	//получение сэмпла от таймера
	if (timer_freq > 30 && timer_freq < 5000) can_hear = 1;
	if (step_mode || log_to_console) can_hear = 0;
	
	if (!wait_for_dispatch)
	{
		//пишем в следующий сэмпл
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
	}
	else
	{
		//пишем в текущий сэмпл (излишняя генерация)
		if (audio_stream.next_buffer_to_play == 1) //пишем сэмпл A
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

	}

	if (!wait_for_dispatch || next_byte_to_gen < 600) next_byte_to_gen++; //если wait_for_dispatch, то генерация не далее 600

	if (next_byte_to_gen == sample_size) 
	{
		wait_for_dispatch = 1; //ждем запроса сэмпла
		next_byte_to_gen = 0;
	}

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
	if (audio_stream.next_buffer_to_play == 0 && !wait_for_dispatch)
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
		Audio_monitor.diff = -empty_s;
		Audio_monitor.get_sample(Audio_monitor.diff);
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
		Audio_monitor.diff = next_byte_to_gen;
		Audio_monitor.get_sample(Audio_monitor.diff);
		//Audio_monitor.over_samples_A = next_byte_to_gen;
		//next_byte_to_gen = 0;//перезапуск с начала
		//cout << "order messed (block A)" << endl;
	}
	//wait_for_dispatch = 0; //снимаем блокировку генерации
}
void SoundMaker::change_to_B() //начинаем играть В
{
	//заканчиваем генерацию сэмпла B и переключаемся на генерацию A
	if (audio_stream.next_buffer_to_play == 1 && !wait_for_dispatch)
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
		Audio_monitor.diff = -empty_s;
		next_byte_to_gen = 0;
		Audio_monitor.get_sample(Audio_monitor.diff);
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
		Audio_monitor.diff = next_byte_to_gen;
		Audio_monitor.get_sample(Audio_monitor.diff);
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

void Audio_mon_device::get_sample(int16_t sample)
{
	//добавляем число в массив
	array_pointer_next_el++;

	//переходим на начало массива
	if (array_pointer_next_el == 1000) array_pointer_next_el = 0;

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

void Audio_mon_device::main_loop()
{
	//создаем окно
	main_window.create(sf::VideoMode(sf::Vector2u(window_size_x, window_size_y)), window_title, sf::Style::Titlebar, sf::State::Windowed);
	main_window.setPosition({ window_pos_x, window_pos_y });
	main_window.setFramerateLimit(60);
	main_window.setMouseCursorVisible(1);
	main_window.setKeyRepeatEnabled(0);
	main_window.setVerticalSyncEnabled(1);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	while (visible)
	{
		if (do_render)
		{
			//рисуем окно
			render();
			//события окна
			while (main_window.pollEvent()) {};
			do_render = 0;
		}
		else
		{
			//отдыхаем
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(5)); //задержка перед завершением
	main_window.close();
}
void Audio_mon_device::show()
{
	//признак видимости
	visible = 1;

	//создаем поток
	std::thread new_t(&Audio_mon_device::main_loop, this);
	t = std::move(new_t);
}
void Audio_mon_device::hide()
{
	//запоминаем координаты
	sf::Vector2i p = main_window.getPosition();
	window_pos_x = p.x;
	window_pos_y = p.y;

	//скрываем окно
	visible = 0;
	if (t.joinable()) t.join();
}
void Audio_mon_device::render()
{
	main_window.setActive(1);
	//синхронизация монитора (окно 2000х300)
	main_window.clear(sf::Color::Black);// очистка экрана

	sf::RectangleShape rectangle; //столбик
	rectangle.setSize(sf::Vector2f(2, 2));
	
	//выводим данные
	array_pointer_to_draw = array_pointer_next_el;
	
	for (int x = 1000; x >= 0; --x)
	{
		if (!array_pointer_to_draw) array_pointer_to_draw = 999;
		else array_pointer_to_draw--;
		rectangle.setFillColor(sf::Color(200, 200, 200));
		rectangle.setPosition(sf::Vector2f(x, 100 - sample_array[array_pointer_to_draw] / 10));
		main_window.draw(rectangle);
		rectangle.setFillColor(sf::Color(255, 0, 0));
		rectangle.setPosition(sf::Vector2f(x, 100));
		main_window.draw(rectangle);
	}
	//if (sample_array[0]) step_mode = 1;  //останов для отладки
	

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

	text.setString("diff = " + to_string(diff));
	text.setPosition(sf::Vector2f(600, 5));
	main_window.draw(text);
	main_window.display();
	main_window.setActive(0);
}
void Audio_mon_device::update(int new_elapsed_ms)
{
	do_render = 1;				//обновить
}