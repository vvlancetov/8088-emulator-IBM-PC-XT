#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <sstream>
#include <vector>
#include <conio.h>
#include <Windows.h>
#include <fstream>
#include "video.h"
#include "fdd.h"
#include "loader.h"

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

using namespace std;
extern HANDLE hConsole;
extern string path;

//текстура шрифта
extern sf::Texture font_texture;
extern sf::Sprite font_sprite;

//текстуры графической палитры
extern sf::Texture CGA_320_texture;
extern sf::Sprite CGA_320_palette_sprite;

extern uint8 memory_2[1024 * 1024]; //память 2.0

extern Video_device monitor;

extern Dev_mon_device debug_monitor;
extern FDD_mon_device FDD_monitor;

extern string filename_ROM;
extern string filename_test;
extern string filename_HDD;
extern string filename_v_rom;
extern string filename_FDD;

extern FDD_Ctrl FDD_A;

void loader(int argc, char* argv[])
{
	
	//путь к текущему каталогу
	path = argv[0];
	int l_symb = (int)path.find_last_of('\\');
	path.resize(++l_symb);
	cout << "path = " << path << endl;
	
	//загружаем ретро-шрифт в виде текстуры
	if (font_texture.loadFromFile(path + "videorom_CGA.png")) cout << "Font ROM loaded" << endl;
	font_sprite.setTexture(font_texture);
	font_texture.setSmooth(0);

	//загружаем палитры CGA
	if (CGA_320_texture.loadFromFile(path + "CGA_320_palette.png")) cout << "CGA 320 palette loaded" << endl;
	CGA_320_palette_sprite.setTexture(CGA_320_texture);
	CGA_320_texture.setSmooth(0);

	//загружаем обычный шрифт для служебных надписей
	if (!monitor.font.openFromFile(path + "ShareTechMonoRegular.ttf")) cout << "Error loading debug font" << endl;
	else cout << "font " << path + "ShareTechMonoRegular.ttf" << " loaded" << endl;

	if (!debug_monitor.font.openFromFile(path + "ShareTechMonoRegular.ttf")) cout << "DEBUG_MON: error loading font" << endl;
	else cout << "DEBUG_MON: font " << path + "ShareTechMonoRegular.ttf" << " loaded" << endl;

	if (!FDD_monitor.font.openFromFile(path + "arialnarrow.ttf")) cout << "FDD_MON: error loading font" << endl;
	else cout << "FDD_MON: font " << path + "arialnarrow.ttf" << " loaded" << endl;

	//загружаем ПЗУ
	fstream file(path + filename_ROM, ios::binary | ios::in);
	if (!file.is_open()) cout << "File " << filename_ROM << " not found!" << endl;
	else
	{
		//считываем данные из файла BIOS и записываем по адресу F0000
		int a = 0;
		char b;    //buffer
		while (file.read(&b, 1)) {
			// записываем виртуальный ROM
			memory_2[0xF0000 + a] = b;
			a++;
		};
		file.close();
		cout << "Loaded " << (int)(a) << " commands from ROM file" << endl;
	}

	//загружаем тест в память
	fstream file_test(path + filename_test, ios::binary | ios::in);
	if (!file_test.is_open()) cout << "File " << filename_test << " not found!" << endl;
	else
	{
		int a = 0; //начальный адрес
		char b;    //buffer
		while (file_test.read(&b, 1)) {
			// записываем виртуальный ROM
			//memory.write_2(a + 0x1000, b);
			a++;
		};
		file_test.close();
	}

	//загружаем видео ПЗУ
	fstream file_v_rom(path + filename_v_rom, ios::binary | ios::in);
	if (!file_v_rom.is_open()) cout << "File " << filename_v_rom << " not found!" << endl;
	else
	{
		int a = 0; //начальный адрес
		char b;    //buffer
		while (file_v_rom.read(&b, 1)) {
			// записываем виртуальный ROM
			memory_2[a] = b;
			a++;
		};
		file_v_rom.close();
		cout << "Загружено " << (int)(a) << " байт данных в видео ПЗУ" << endl;
	}

	//открываем файл HDD
	fstream file_HDD(path + filename_HDD, ios::binary | ios::in | ios::out);
	if (!file_HDD.is_open()) cout << "File " << filename_HDD << " not found!" << endl;
	else
	{
		//файл HDD открыт
		cout << "HDD file opened OK" << endl;
	}

	//открываем файл FDD
	
	fstream file_FDD(path + filename_FDD, ios::binary | ios::in | ios::out);
	if (!file_FDD.is_open()) cout << "File " << filename_FDD << " not found!" << endl;
	else
	{
		//файл FDD открыт
		int a = 0; //counter
		char b;    //buffer
		cout << "FDD file opened OK" << endl;
		uint8 track, sector, head;
		uint16 byte;
		for (track = 0; track < 80; track++)
		{
			for (sector = 0; sector < 9; sector++)
			{
				for (byte = 0; byte < 512; byte++)
				{
					for (head = 0; head < 2; head++)
					{
						if (file_FDD.read(&b, 1))
						{
							FDD_A.load_diskette(track, sector, head, byte, b);
							a++;
						}
					}
				}
			}
		}

		file_FDD.close();
		cout << "Загружено " << (int)(a) << " байт данных в FDD ";
		switch (a / 512)
		{
		case 2400:
			FDD_A.diskette_heads = 2;  // 1 or 2
			FDD_A.diskette_sectors = 15; //8, 9 or 15
			cout << " HEADS = 2 SECTORS = 15 CAPACITY = 1200" << endl;
			break;
		case 720:
			FDD_A.diskette_heads = 2;  // 1 or 2
			FDD_A.diskette_sectors = 9; //8, 9 or 15
			cout << " HEADS = 2 SECTORS = 9 CAPACITY = 360" << endl;
			break;
		case 640:
			FDD_A.diskette_heads = 2;  // 1 or 2
			FDD_A.diskette_sectors = 8; //8, 9 or 15
			cout << " HEADS = 2 SECTORS = 8 CAPACITY = 320" << endl;
			break;
		case 360:
			FDD_A.diskette_heads = 1;  // 1 or 2
			FDD_A.diskette_sectors = 9; //8, 9 or 15
			cout << " HEADS = 1 SECTORS = 9 CAPACITY = 180" << endl;
			break;
		case 320:
			FDD_A.diskette_heads = 1;  // 1 or 2
			FDD_A.diskette_sectors = 8; //8, 9 or 15
			cout << " HEADS = 1 SECTORS = 8 CAPACITY = 160" << endl;
			break;
		default:
			FDD_A.diskette_heads = 2;  // 1 or 2
			FDD_A.diskette_sectors = 9; //8, 9 or 15
			cout << " DEFAULT: HEADS = 2 SECTORS = 9 CAPACITY = 360" << endl;
			break;
		}
	}

	//проверяем аргументы командной строки
	cout << "Checking command string parameters..." << endl;
	
	for (int i = 0; i < argc; i++)
	{
		cout << (int)i << " " << argv[i] << endl;
	}
	
	/*
	for (int i = 1; i < argc; i++)
	{
		string s = argv[i];
		if (s.substr(0, 2) == "-f")
		{
			//найдено имя файла
			if (argc >= i + 2)
			{
				filename_ROM = argv[i + 1];// "Prog.txt";
				cout << "new filename = " << filename_ROM << endl;
			}
			else
			{
				filename_ROM = "Prog.txt";
				cout << "filename = " << filename_ROM << " (default)" << endl;
			}
		}
		if (s.substr(0, 3) == "-ru")
		{
			//использование русского языка
			RU_lang = true;
			cout << "set RU lang" << endl;
		}
		if (s.substr(0, 5) == "-step")
		{
			//пошаговое выполнение
			step_mode = true;
			cout << "set step mode ON" << endl;
		}
		if (s.substr(0, 4) == "-log")
		{
			//пошаговое выполнение
			log_to_console = true;
			cout << "set log to console ON" << endl;
		}
	} */
}
