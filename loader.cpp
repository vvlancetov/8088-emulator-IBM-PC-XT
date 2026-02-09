#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <sstream>
#include <vector>
#include <conio.h>
#include <Windows.h>
#include <fstream>
#include "video.h"
#include "audio.h"
#include "fdd.h"
#include "hdd.h"
#include "loader.h"

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

using namespace std;
extern HANDLE hConsole;
extern string path;

//текстуры шрифта
extern sf::Texture font_texture_40;
extern sf::Sprite font_sprite_40;
extern sf::Texture font_texture_80;
extern sf::Sprite font_sprite_80;
extern sf::Texture font_texture_80_MDA;
extern sf::Sprite font_sprite_80_MDA;

//текстуры графической палитры
extern sf::Texture CGA_320_texture;
extern sf::Sprite CGA_320_palette_sprite;

extern uint8 memory_2[1024 * 1024 + 1024 * 1024]; //память 2.0

extern Monitor monitor;

extern Dev_mon_device debug_monitor;
extern FDD_mon_device FDD_monitor;
extern HDD_mon_device HDD_monitor;
extern Audio_mon_device Audio_monitor;
extern Mem_mon_device Mem_monitor;

string filename_ROM;
string filename_HDD_ROM;
string filename_HDD;
string filename_v_rom;


extern FDD_Ctrl FDD_A;
extern HDD_Ctrl HDD;
extern SoundMaker speaker;

uint32 ROM_address = 0; //адрес загрузки файла ПЗУ
extern uint16 Instruction_Pointer;
extern uint16* CS;

extern uint8 MB_switches;
extern enum class videocard_type;

void load_diskette(uint8 drive, std::string filename_FDD)
{
	//открываем файл FDD
	fstream file_FDD(path + filename_FDD, ios::binary | ios::in | ios::out);
	if (!file_FDD.is_open()) cout << "File FDD #" << (int)drive << " " << filename_FDD << " not found!" << endl;
	else
	{
		//файл FDD открыт
		int a = 0; //counter
		char b;    //buffer
		cout << "FDD #" << (int)drive << " file opened OK" << endl;

		//грузим сектора последовательно
		while (file_FDD.read(&b, 1) && a<2880*1024)
		{
			FDD_A.load_diskette((drive - 1) * 2880 * 1024 + a, b);
			a++;
		}

		file_FDD.close();
		FDD_A.file_size[drive - 1] = a; //записываем размер файла
		cout << "Загружено " << (int)(a) << " байт данных в FDD #" << (int)drive << " ";
		
		if (a)
		{
			//если загружены байты, обновляем данные 
			FDD_A.diskette_in[drive - 1] = 1;
			FDD_A.filename_FDD.at(drive - 1) = filename_FDD;
		}

		switch (a / 512)
		{
			//дискеты на 3.5"
		case 5760:
			FDD_A.diskette_heads[drive - 1] = 2;  // 1 or 2
			FDD_A.diskette_sectors[drive - 1] = 36; //8, 9, 15, 18
			FDD_A.diskette_cylinders[drive - 1] = 80;
			cout << "3.5\" HEADS = 2 SECTORS = 9 CYL = 80 CAPACITY = 2880" << endl;
			break;
		case 2880:
			FDD_A.diskette_heads[drive - 1] = 2;  // 1 or 2
			FDD_A.diskette_sectors[drive - 1] = 18; //8, 9, 18
			FDD_A.diskette_cylinders[drive - 1] = 80;
			FDD_A.set_MFM(drive - 1, 1); //дискета двойной плотности
			cout << "3.5\" HEADS = 2 SECTORS = 9 CYL = 80 CAPACITY = 1440" << endl;
			break;
		case 1440:
			FDD_A.diskette_heads[drive - 1] = 2;  // 1 or 2
			FDD_A.diskette_sectors[drive - 1] = 9; //8, 9, 15, 18
			FDD_A.diskette_cylinders[drive - 1] = 80;
			cout << "3.5\" HEADS = 2 SECTORS = 9 CYL = 80 CAPACITY = 720" << endl;
			break;

			//дискеты на 5.25"
		case 2400:
			FDD_A.diskette_heads[drive - 1] = 2;  // 1 or 2
			FDD_A.diskette_sectors[drive - 1] = 15; //8, 9 or 15
			FDD_A.diskette_cylinders[drive - 1] = 80;
			FDD_A.set_MFM(drive - 1, 1); //дискета двойной плотности
			cout << "5.25\" HEADS = 2 SECTORS = 15 CYL = 80 CAPACITY = 1200" << endl;
			break;
		case 720:
			FDD_A.diskette_heads[drive - 1] = 2;  // 1 or 2
			FDD_A.diskette_sectors[drive - 1] = 9; //8, 9 or 15
			FDD_A.diskette_cylinders[drive - 1] = 40;
			cout << "5.25\" HEADS = 2 SECTORS = 9 CYL = 40 CAPACITY = 360" << endl;
			break;
		case 640:
			FDD_A.diskette_heads[drive - 1] = 2;  // 1 or 2
			FDD_A.diskette_sectors[drive - 1] = 8; //8, 9 or 15
			FDD_A.diskette_cylinders[drive - 1] = 40;
			cout << "5.25\" HEADS = 2 SECTORS = 8 CYL = 40 CAPACITY = 320" << endl;
			break;
		case 360:
			FDD_A.diskette_heads[drive - 1] = 1;  // 1 or 2
			FDD_A.diskette_sectors[drive - 1] = 9; //8, 9 or 15
			FDD_A.diskette_cylinders[drive - 1] = 40;
			cout << "5.25\" HEADS = 1 SECTORS = 9 CYL = 40 CAPACITY = 180" << endl;
			break;
		case 320:
			FDD_A.diskette_heads[drive - 1] = 1;  // 1 or 2
			FDD_A.diskette_sectors[drive - 1] = 8; //8, 9 or 15
			FDD_A.diskette_cylinders[drive - 1] = 40;
			cout << "5.25\" HEADS = 1 SECTORS = 8 CYL = 40 CAPACITY = 160" << endl;
			break;
		default:  //360
			FDD_A.diskette_heads[drive - 1] = 2;  // 1 or 2
			FDD_A.diskette_sectors[drive - 1] = 9; //8, 9 or 15
			FDD_A.diskette_cylinders[drive - 1] = 40;
			cout << " DEFAULT(5.25\"): HEADS = 2 SECTORS = 9 CYL = 40 CAPACITY = 360" << endl;
			break;
		}
	}



}
void load_hdd(std::string filename_HDD)
{
	//открываем файл HDD

	fstream file_HDD(path + filename_HDD, ios::binary | ios::in);
	if (!file_HDD.is_open()) cout << "File HDD " << filename_HDD << " not found!" << endl;
	else
	{
		//файл HDD открыт
		int a = 0;
		char b;    //buffer
		while (!file_HDD.eof()) 
		{
			file_HDD.read(&b, 1);
			//записываем виртуальный HDD
			HDD.load_disk_data(a, b);
			a++;
		};
		file_HDD.close();
		HDD.filename = filename_HDD;//запоминаем имя файла
		cout << "Загружено " << (int)(a - 1) << " байт данных в образ HDD" << endl;
	}

	
	
	
	return;

	//запись образа для тестов
	
	file_HDD.open(path + "test.dat", ios::binary | ios::out);
	int a = 0;
	char b;    //buffer
	
	for(int i = 0; i < 256; i++)
	{
		for (int u = 0; u < 512; u++)
		{
			char b = i;
			file_HDD.write(&b, 1);
		}
		//записываем виртуальный HDD
	};
	file_HDD.close();
	

}

void loader(int argc, char* argv[])
{
	
	//путь к текущему каталогу
	path = argv[0];
	int l_symb = (int)path.find_last_of('\\');
	path.resize(++l_symb);
	cout << "Working catalog = " << path << endl;
	
	//загружаем CGA шрифт в виде текстуры
	if (font_texture_40.loadFromFile(path + "videorom_CGA_40.png")) cout << "CGA font ROM(40) loaded" << endl;
	font_sprite_40.setTexture(font_texture_40);
	font_texture_40.setSmooth(0);
	font_sprite_40.setScale(sf::Vector2f(1, 1.2));

	if (font_texture_80.loadFromFile(path + "videorom_CGA_80.png")) cout << "CGA font ROM(80) loaded" << endl;
	font_sprite_80.setTexture(font_texture_80);
	font_texture_80.setSmooth(0);
	font_sprite_80.setScale(sf::Vector2f(1, 1.2));

	//загружаем палитры CGA
	if (CGA_320_texture.loadFromFile(path + "CGA_320_palette.png")) cout << "CGA 320 palette loaded" << endl;
	CGA_320_palette_sprite.setTexture(CGA_320_texture);
	CGA_320_texture.setSmooth(0);

	//загружаем MDA шрифт в виде текстуры
	if (font_texture_80_MDA.loadFromFile(path + "videorom_MDA.png")) cout << "MDA Font ROM(80) loaded" << endl;
	font_sprite_80_MDA.setTexture(font_texture_80_MDA);
	font_texture_80_MDA.setSmooth(0);
	font_sprite_80_MDA.setScale(sf::Vector2f(1.0, 1.0));

	//загружаем обычный шрифт для служебных надписей
	if (!monitor.font.openFromFile(path + "CousineR.ttf")) cout << "Error loading debug font" << endl;
	else cout << "font " << path + "CousineR.ttf" << " loaded" << endl;

	//загружаем шрифты для окон отладки
#ifdef DEBUG
	if (!debug_monitor.font.openFromFile(path + "ShareTechMonoRegular.ttf")) cout << "DEBUG_MON: error loading font" << endl;
	else cout << "DEBUG_MON: font " << path + "ShareTechMonoRegular.ttf" << " loaded" << endl;

	if (!FDD_monitor.font.openFromFile(path + "arialnarrow.ttf")) cout << "FDD_MON: error loading font" << endl;
	else cout << "FDD_MON: font " << path + "arialnarrow.ttf" << " loaded" << endl;

	if (!HDD_monitor.font.openFromFile(path + "arialnarrow.ttf")) cout << "HDD_MON: error loading font" << endl;
	else cout << "HDD_MON: font " << path + "arialnarrow.ttf" << " loaded" << endl;

	if (!Audio_monitor.font.openFromFile(path + "arialnarrow.ttf")) cout << "Audio_MON: error loading font" << endl;
	else cout << "Audio_MON: font " << path + "arialnarrow.ttf" << " loaded" << endl;

	if (!Mem_monitor.font.openFromFile(path + "CousineR.ttf")) cout << "MEM_MON: error loading font" << endl;
	else cout << "MEM_MON: font " << path + "CousineR.ttf" << " loaded" << endl;

#endif

	//обрабатываем файл конфигурации
	fstream conf_file(path + "config.ini", ios::binary | ios::in);
	if (!conf_file.is_open()) cout << "Configuration file config.ini not found! Using default values." << endl;
	else
	{
		cout << "Configuration file OK. Reading..." << endl;
		std::string line;
		// Читаем файл построчно
		while (std::getline(conf_file, line))
		{
			//пропускаем комментарии
			if (line.find('#') == 0) continue;
			
			// Ищем символ '=' в строке
			size_t equal_pos = line.find('=');

			// Если символ '=' найден и он не является первым символом (т.е. есть что-то до него)
			if (equal_pos != std::string::npos && equal_pos > 0) {
				// Извлекаем параметр (часть до '=')
				std::string parameter = line.substr(0, equal_pos);

				// Извлекаем значение (часть после '=')
				std::string value = line.substr(equal_pos + 1);

				// Удаление ведущих пробелов из параметра
				size_t  first_char_param = parameter.find_first_not_of(" \t\n\r");
				if (first_char_param == std::string::npos) {
					parameter = ""; // Строка состоит только из пробелов
				}
				else {
					parameter = parameter.substr(first_char_param);
				}

				// Удаление замыкающих пробелов из параметра
				size_t  last_char_param = parameter.find_last_not_of(" \t\n\r");
				if (last_char_param != std::string::npos) {
					parameter = parameter.substr(0, last_char_param + 1);
				}

				// Удаление ведущих пробелов из значения
				size_t first_char_value = value.find_first_not_of(" \t\n\r");
				if (first_char_value == std::string::npos) {
					value = ""; // Строка состоит только из пробелов
				}
				else {
					value = value.substr(first_char_value);
				}

				// Удаление замыкающих пробелов из значения
				size_t last_char_value = value.find_last_not_of(" \t\n\r");
				if (last_char_value != std::string::npos) {
					value = value.substr(0, last_char_value + 1);
				}
			
				//обрабатываем все параметры по одному

				if (parameter == "BIOS") //имя файла БИОС
				{
					if (value != "") filename_ROM = value;
					cout << "BIOS file = " << filename_ROM << endl;
				}

				if (parameter == "BIOS_ADDRESS") //адрес загрузки биоса
				{
					if (value != "" && stoi(value, 0, 16)) ROM_address = stoi(value, 0, 16);
					cout << "ROM address = 0x" << hex << (int)ROM_address << endl;
				}

				if (parameter == "IP") //начальный адрес выполнения кода
				{
					if (value != "" && stoi(value, 0, 16)) Instruction_Pointer = stoi(value, 0, 16);
					cout << "Instruction_Pointer = 0x" << hex << (int)Instruction_Pointer << endl;
				}

				if (parameter == "CS") //начальный сегмент кода
				{
					if (value != "" && stoi(value, 0, 16)) *CS = stoi(value, 0, 16);
					cout << "segment CS = 0x" << hex << (int)*CS << endl;
				}

				if (parameter == "FDD_QUANTITY") //количество дисководов
				{
					if (value != "" && stoi(value, 0, 10)) FDD_A.set_active_drives(stoi(value, 0, 10));
					cout << "FDD drives = " << (int)FDD_A.get_active_drives() << endl;
					MB_switches = (MB_switches & 0b00111111) | ((~FDD_A.get_active_drives()) << 6);  //корректируем переключатели
				}

				if (parameter == "FDD1_FILE") //диск в дисководе 1
				{
					//загружаем дискету в массив
					if (value != "") load_diskette(1, value);
					//FDD_A.diskette_in[0] = 1;
					//FDD_A.filename_FDD.at(0) = value;
				}

				if (parameter == "FDD2_FILE") //диск в дисководе 2
				{
					//загружаем дискету в массив
					if (value != "") load_diskette(2, value);
					//FDD_A.diskette_in[1] = 1;
					//FDD_A.filename_FDD[1] = value;
				}

				if (parameter == "FDD3_FILE") //диск в дисководе 3
				{
					//загружаем дискету в массив
					if (value != "") load_diskette(3, value);
					//FDD_A.diskette_in[2] = 1;
					//FDD_A.filename_FDD[2] = value;
				}

				if (parameter == "FDD4_FILE") //диск в дисководе 4
				{
					//загружаем дискету в массив
					if (value != "") load_diskette(4, value);
					//FDD_A.diskette_in[3] = 1;
					//FDD_A.filename_FDD[3] = value;
				}

				if (parameter == "HDD_FILE") //
				{
					//загружаем дискету в массив
					if (value != "") load_hdd(value);
				}

				if (parameter == "HDD_ROM_FILE") //
				{
					//загружаем ПЗУ HDD
					fstream file_HDD_ROM(path + value, ios::binary | ios::in);
					if (!file_HDD_ROM.is_open()) cout << "File HDD-ROM " << value << " not found!" << endl;
					else
					{
						//считываем данные из файла BIOS и записываем по адресу C8000
						int a = 0;
						uint8 checksum = 0;
						char b;    //buffer
						while (file_HDD_ROM.read(&b, 1)) {
							// записываем виртуальный ROM
							memory_2[0xC8000 + a] = b;
							a++;
							checksum += b;
						};
						file_HDD_ROM.close();
						cout << "Loaded " << dec << (int)(a) << " commands from HDD_ROM file (checksum = " << hex << (int)checksum  << ")" << endl;
					}
				}

				if (parameter == "SPEAKER_VOLUME") //громкость спикера
				{
					//устанавливаем громкость
					if (value != "" && stoi(value, 0, 10)) speaker.set_volume(stoi(value, 0, 10));
				}

				if (parameter == "VIDEO") //видеокарта
				{
					//выбираем тип видео
					cout << "SET video to " << value << endl;
					if (value == "CGA")
					{
						cout << "============CGA===========" <<endl;
						monitor.set_card_type(videocard_type::CGA);
						MB_switches = (MB_switches & 0xCF) | (0b00100000); //80 col CGA

					}
					if (value == "MDA")
					{
						cout << "============MDA===========" << endl;
						monitor.set_card_type(videocard_type::MDA);
						MB_switches = (MB_switches & 0xCF) | (0b00110000); //MDA
					}
					if (value == "EGA")
					{
						cout << "============EGA===========" << endl;
						monitor.set_card_type(videocard_type::EGA);
						MB_switches = (MB_switches & 0xCF) | (0b00000000); //EGA
					}
				}
				
				if (parameter == "EGA_VIDEO_ROM_FILE") //ROM от EGA карты
				{
					//выбираем видео BIOS
					if (value != "") filename_v_rom = value;
				}
			}
		}
		conf_file.close();
	}

	//загружаем ПЗУ
	fstream file(path + filename_ROM, ios::binary | ios::in);
	if (!file.is_open()) cout << "BIOS file " << filename_ROM << " not found!" << endl;
	else
	{
		//считываем данные из файла BIOS и записываем по адресу F0000
		int a = 0;
		uint8 sum = 0;
		char b;    //buffer
		while (file.read(&b, 1)) {
			// записываем виртуальный ROM
			memory_2[ROM_address + a] = b;
			sum += b;
			a++;
		};
		file.close();
		cout << "Loaded " << (int)(a) << " BIOS from ROM file (checksum = " << (int)sum << ")" << endl;
	}

	//загружаем видео ПЗУ
	if (filename_v_rom != "")
	{
		fstream file_v_rom(path + filename_v_rom, ios::binary | ios::in);
		if (!file_v_rom.is_open()) cout << "File " << filename_v_rom << " not found!" << endl;
		else
		{
			int a = 0xC0000; //начальный адрес
			char b;    //buffer
			while (file_v_rom.read(&b, 1)) {
				// записываем виртуальный ROM
				memory_2[a] = b;
				a++;
			};
			file_v_rom.close();
			cout << "Загружено " << (int)(a) << " байт данных в видео ПЗУ с адреса 0xC0000" << endl;
		}
	}
}

