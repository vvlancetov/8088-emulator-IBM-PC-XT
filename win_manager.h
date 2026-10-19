#pragma once
#include <mutex> // Подключаем мьютекс

#ifndef EMU_WINDOW_H
#define EMU_WINDOW_H

#include <Windows.h>
#include <commdlg.h>
#include <windowsx.h>
#include <thread>
#include <atomic>
#include <string>
#include <sstream>
#include <mutex>
#include "video.h"
#include "fdd.h"

//константы

#ifndef WIN_CONSTANTS_H
#define WIN_CONSTANTS_H

// Core Win32 Menu and UI Command Component Identifiers
#define IDM_FILE_EXIT         1001

#define IDM_RUN_POWERON       2001
#define IDM_RUN_RESET         2002

#define IDM_DEBUG_STEP        3001
#define IDM_DEBUG_LOGCON      3002
#define IDM_DEBUG_SHOWCON     3003
#define IDM_DEBUG_WINDOWS     3004

#define IDM_OPTIONS_VIDEO     4001
#define IDM_OPTIONS_SOUND     4002
#define IDM_OPTIONS_FLOPPY    4003
#define IDM_OPTIONS_HARDDISK  4004

#define IDM_HELP_ABOUT        5001

// Debug screens (Ctrl+1..Ctrl+7)
#define IDM_DEBUG_SCR_BASE    3010
#define IDM_DEBUG_SCR_REGS    3011   // registers and peripherals
#define IDM_DEBUG_SCR_FDD     3012   // FDD logs
#define IDM_DEBUG_SCR_HDD     3013   // HDD logs
#define IDM_DEBUG_SCR_SOUND   3014   // sound devices
#define IDM_DEBUG_SCR_MEM     3015   // memory monitor
#define IDM_DEBUG_SCR_EGA     3016   // EGA registers
#define IDM_DEBUG_SCR_COM     3017   // COM port registers

// CPU speed
#define IDM_CPU_8088          4201
#define IDM_CPU_80286         4202
#define IDM_CPU_80386         4203

// Video card
#define IDM_VIDEO_MDA         4101
#define IDM_VIDEO_CGA         4102
#define IDM_VIDEO_EGA         4103

extern const int BASE_EMULATOR_WIDTH;
extern const int BASE_EMULATOR_HEIGHT;

static const wchar_t* ABOUT_URL = L"https://github.com/vvlancetov/8088-emulator-IBM-PC-XT";
#define IDC_ABOUT_OPEN 1101

#endif // WIN_CONSTANTS_H

// Кастомный буфер для перехвата и безопасной передачи std::cout между потоками
class SafeStringStream : public std::streambuf {
private:
    std::string buffer;
    std::mutex stream_mutex;

protected:
    // Перехватываем каждый символ, отправленный в cout <<
    virtual int overflow(int ch) override {
        if (ch != EOF) {
            std::lock_guard<std::mutex> lock(stream_mutex);
            buffer += static_cast<char>(ch);

            // Если дошли до конца строки (перенос строки \n)
            if (ch == '\n') {
                // Выводим накопленную строку в реальный stdout потока окна
                // В потоке окна этот дескриптор CONOUT$ активен и полностью валиден!
                printf("%s", buffer.c_str());
                buffer.clear();
            }
        }
        return ch;
    }
};

class EmuWindowManager {
private:
    HWND hwndMain;
    HWND hwndConsole;
    HMENU hDebugMenu;           // Сохраняем хэндл меню для изменения текста пунктов
    HMENU hRunMenu;
    HMENU hOptionsMenu;
    HMENU hDebugScrMenu = NULL;
    HMENU hCpuMenu = NULL;
    HMENU hVideoMenu = NULL;
    HACCEL hAccel = NULL;
    bool isConsoleVisible;
    std::atomic<bool> running;  // Потокобезопасный флаг работы окна
    std::thread window_thread;

    SafeStringStream* custom_stream_buf; // Наш кастомный буфер
    std::streambuf* old_cout_buf;        // Ссылка для восстановления оригинального буфера

    HMENU CreateMenuSystem();

public:

    // Глобальный мьютекс для синхронизации потока окна и потока рендеринга видеокарты
    static std::mutex window_mutex;

    EmuWindowManager();
    ~EmuWindowManager();

    bool Initialize(HINSTANCE hInstance, int nCmdShow);
    void WindowThreadFunc(HINSTANCE hInstance, int nCmdShow);   // Функция потока
    void Start(HINSTANCE hInstance, int nCmdShow);              // Запуск и detach
    void UpdateMenuState();                                     // синхронизирует меню с глобальными переменными
    void ToggleConsole();
    void TogglePower();
    void ResetPower();
    void ToggleLogToConsole();
    void ToggleStepMode();
    void Shutdown();                                            // Уничтожение перед выходом
    void RequestClose() { if (hwndMain) PostMessageW(hwndMain, WM_APP + 1, 0, 0); }
    void Join() { if (window_thread.joinable()) window_thread.join(); }
    HWND GetHWND() const { return hwndMain; }
    bool IsRunning() const { return running.load(); }
    std::string GetApplicationDirectory();
};

LRESULT CALLBACK GlobalWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif // EMU_WINDOW_H
