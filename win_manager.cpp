#include <iostream>
#include <io.h>
#include <fcntl.h>
#include <string>
#include "win_manager.h"
#include "video.h"
#include "audio.h"
#include "adlib.h"
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")

extern Monitor monitor;
extern std::atomic<bool> PC_power_on;
extern std::atomic<bool> winman_on;
extern void reset_registers();
extern uint8 speed_div; //делитель скорости
extern Monitor monitor; //объект монитора

extern bool step_mode;		//ждать ли нажатия пробела для выполнения команд
extern bool log_to_console; //логирование команд на консоль

//отладочные окна
extern Dev_mon_device debug_monitor;
extern FDD_mon_device FDD_monitor;
extern HDD_mon_device HDD_monitor;
extern Audio_mon_device Audio_monitor;
extern Mem_mon_device Mem_monitor;
extern EGA_mon_device EGA_monitor;
extern COM_mon_device COM_monitor;

extern FDD_Ctrl FDD_A; //дисковод

// Инициализация статического мьютекса
std::mutex EmuWindowManager::window_mutex;

// ИСПРАВЛЕНО: Глобальный указатель на активный экземпляр менеджера окон для WndProc
static EmuWindowManager* g_pWindowManager = nullptr;

void ShowAboutDialog(HWND parent); //объявление

extern SoundMaker speaker;
extern adlib_card soundcard;


// Global instance or external controls
static int g_press_drive = -1;
EmuWindowManager::EmuWindowManager() : hwndMain(NULL), hwndConsole(NULL), isConsoleVisible(true), running(false), custom_stream_buf(nullptr), old_cout_buf(nullptr)
{
    // Запоминаем текущий объект в глобальной переменной при создании
    g_pWindowManager = this;
}
EmuWindowManager::~EmuWindowManager() {
    if (!running.load() && hwndConsole == NULL) return;
    Shutdown();
}

//функции для работы с дисководом
static bool pick_floppy_image(HWND owner, std::string& out_ansi)
{
    wchar_t buf[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = L"Floppy images (*.img;*.ima;*.dsk;*.flp;*.vfd)\0*.img;*.ima;*.dsk;*.flp;*.vfd\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;  // не менять текущий каталог процесса
    if (!GetOpenFileNameW(&ofn)) return false;

    char ansi[MAX_PATH * 2] = "";
    BOOL lossy = FALSE;
    WideCharToMultiByte(CP_ACP, 0, buf, -1, ansi, sizeof(ansi), "?", &lossy);
    if (lossy) { MessageBoxW(owner, L"The path contains characters outside the system code page.", L"Floppy", MB_OK | MB_ICONWARNING); return false; }
    out_ansi = ansi;
    return true;
}

static int panel_drive_at(HWND hWnd, LPARAM lParam)
{
    RECT rc; GetClientRect(hWnd, &rc);
    if (rc.right <= 0) return -1;
    float s = rc.right / 1040.0f;                       // та же шкала, что mouse_scale
    return SidePanel::hit_drive(GET_X_LPARAM(lParam) / s, GET_Y_LPARAM(lParam) / s);
}

static void on_floppy_button(HWND hWnd, int drive)
{
    if (FDD_A.diskette_in[drive]) { fdd_post_request({ drive, false, "" }); return; }   // EJECT
    std::string file;
    if (pick_floppy_image(hWnd, file)) fdd_post_request({ drive, true, file });         // INSERT
}


// Entry point to spin up the independent thread
void EmuWindowManager::Start(HINSTANCE hInstance, int nCmdShow) {
    running.store(true);
    // Launch window in a separate thread context
    window_thread = std::thread(&EmuWindowManager::WindowThreadFunc, this, hInstance, nCmdShow);
    // Detach it so it lives independently from the main CPU emulation loop
    //window_thread.detach();
}

void EmuWindowManager::WindowThreadFunc(HINSTANCE hInstance, int nCmdShow) {
    if (!Initialize(hInstance, nCmdShow)) {
        running.store(false);
        return;
    }

    // Independent Window Message Loop
    MSG msg;
    while (running.load()) {
        // Use GetMessage or PeekMessage. Since it's a separate thread, 
        // we can safely use GetMessage to not consume 100% of a CPU core on idle.
        if (GetMessageW(&msg, NULL, 0, 0)) {   //ожидаем сообщение от системы
            TranslateMessage(&msg);             //Виртуальный транслятор клавиатурных кодов
            DispatchMessageW(&msg);             //вызов GlobalWndProc для обработки сообщения
        }
        else {
            //пришло сообщение WM_QUIT после уничтожения окна
            running.store(false);
        }
    }
    Shutdown();
    //std::cout << "End of WindowThreadFunc" << std::endl;
}

bool EmuWindowManager::Initialize(HINSTANCE hInstance, int nCmdShow) {
    
    // Выделяем консоль строго ВНУТРИ потока окна
    AllocConsole();
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);
    std::ios::sync_with_stdio(true);

    hwndConsole = GetConsoleWindow();
    
    // РЕГИСТРАЦИЯ КОНСОЛЬНОГО ПЕРЕХВАТЧИКА В WINDOWS
    if (hwndConsole) {
    // ДЕЛАЕМ КРЕСТИК КОНСОЛИ СЕРЫМ И НЕАКТИВНЫМ
    // Извлекаем системное меню окна консоли
        HMENU hConsoleMenu = GetSystemMenu(hwndConsole, FALSE);
        if (hConsoleMenu) {
            // Удаляем команду SC_CLOSE (Закрыть). Крестик станет заблокированным!
            DeleteMenu(hConsoleMenu, SC_CLOSE, MF_BYCOMMAND);
        }

        // На старте скрываем консоль
        ShowWindow(hwndConsole, SW_HIDE);
        isConsoleVisible = false;
    }

    // Перехватываем стандартный cout процессора и связываем его с нашим потокобезопасным буфером!
    custom_stream_buf = new SafeStringStream();
    old_cout_buf = std::cout.rdbuf(custom_stream_buf);
    // С этого момента ЛЮБОЙ вызов cout << в любом потоке эмулятора процессора 
    // будет автоматически и безопасно перенаправляться в printf() потока окна.
    std::cout << "[СИСТЕМА] Потокобезопасный мост для отладочных логов успешно возведен!\n";

    // Вычисляем размеры экрана
    int screen_width = GetSystemMetrics(SM_CXSCREEN); // Физическая ширина экрана
    int screen_height = GetSystemMetrics(SM_CYSCREEN); // Физическая высота экрана

    // НАСТРОЙКА РАЗМЕРОВ ВНУТРЕННЕЙ (КЛИЕНТСКОЙ) ЧАСТИ ОКНА
    long target_client_width = 800+200+40;  //1040
    long target_client_height = 600 + 40;   //640
    monitor.resize(target_client_width, target_client_height); //передаем данные на монитор  (общие размеры 1056*699)

    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = GlobalWndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"x86EmuCoreWindowClass";

    if (!RegisterClassExW(&wc)) return false;

    DWORD window_style = WS_OVERLAPPEDWINDOW | WS_THICKFRAME;  //ВЫБОР СТИЛЯ РАМОК

    RECT wr = { 0, 0, target_client_width, target_client_height }; //размер внутренней области
    AdjustWindowRect(&wr, window_style, TRUE);

    // Вычисляем координаты для центрирования окна на реальном экране монитора
    int window_w = wr.right - wr.left;
    int window_h = wr.bottom - wr.top;
    int pos_x = (screen_width - window_w) / 2;
    int pos_y = (screen_height - window_h) / 2;

    hwndMain = CreateWindowExW(
        0, L"x86EmuCoreWindowClass", L"IBM PC/XT Emulator",
        window_style,
        pos_x, pos_y, // Центрировано на экране
        window_w, window_h,
        NULL,
        CreateMenuSystem(), // Создаем главное меню
        hInstance,
        this
    );

    if (!hwndMain) return false;

    //CreateMenuSystem();// Создаем и подключаем меню к нашему окну
    SetWindowPos(hwndMain, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    ShowWindow(hwndMain, nCmdShow);// Показываем основное окно
    UpdateWindow(hwndMain);
    
    return true;
}

void EmuWindowManager::Shutdown() {
    std::cout << "Shutdown()" << std::endl;
    std::lock_guard<std::mutex> lock(window_mutex);
        // Восстанавливаем оригинальный буфер cout перед закрытием, чтобы избежать падения памяти
        if (old_cout_buf) {
            std::cout.rdbuf(old_cout_buf);
            old_cout_buf = nullptr;
        }

        if (custom_stream_buf) {
            delete custom_stream_buf;
            custom_stream_buf = nullptr;
        }

        //уничтожаем указатели и освобождаем консоль
        if (hwndMain) {
            //PostMessageW(hwndMain, WM_CLOSE, 0, 0);
            std::cout << "hwndMain != NULL" << std::endl;
            hwndMain = NULL;
        }

        if (hwndConsole) {
            std::cout << "FreeConsole()" << std::endl;
            FreeConsole();
            hwndConsole = NULL;
        }

    //отключаем глобальные переменные
    //PC_power_on = false; 
    //winman_on = false;
}

void EmuWindowManager::ToggleConsole() {

    // Захватываем мьютекс, полностью изолируя этот шаг от SFML-рендерера графики
    std::lock_guard<std::mutex> lock(window_mutex);

    std::cout << "[ОТЛАДКА МЕНЮ] Запрос переключения консоли. Текущее состояние видимости: "
        << (isConsoleVisible ? "ВИДИМА" : "СКРЫТА") << "\n";

    if (!hwndConsole) {
        hwndConsole = GetConsoleWindow();
        std::cout << "[ОТЛАДКА МЕНЮ] Повторный запрос GetConsoleWindow(). HWND: " << std::hex << hwndConsole << std::dec << "\n";
    }

    if (hwndConsole) {
        isConsoleVisible = !isConsoleVisible;

        // Выполняем нативное переключение видимости окна кодом Windows
        BOOL res = ShowWindow(hwndConsole, isConsoleVisible ? SW_SHOW : SW_HIDE);

        std::cout << "[ОТЛАДКА МЕНЮ] ShowWindow выполнен. Результат Win32 API: " << (res ? "TRUE" : "FALSE (Окно уже было в этом состоянии)") << "\n";

        // Обязательно обновляем фокус ввода, чтобы клавиатура возвращалась в игру, а не застревала в консоли
        if (isConsoleVisible) {
            SetForegroundWindow(hwndConsole);
        }
        else {
            SetForegroundWindow(hwndMain);
        }

        // ДИНАМИЧЕСКОЕ ОБНОВЛЕНИЕ ТЕКСТА МЕНЮ
        if (hDebugMenu) {
            const wchar_t* new_text = isConsoleVisible ? L"Скрыть консоль" : L"Показать консоль";

            // Модифицируем пункт меню по его ID (IDM_DEBUG_SHOWCON)
            ModifyMenuW(hDebugMenu, IDM_DEBUG_SHOWCON, MF_BYCOMMAND | MF_STRING, IDM_DEBUG_SHOWCON, new_text);

            // Принудительно заставляем Windows перерисовать верхнюю панель меню, чтобы изменения применились визуально
            DrawMenuBar(hwndMain);

            std::wcout << L"[ОТЛАДКА МЕНЮ] Текст пункта изменен на: " << new_text << L"\n";
        }
    }
    else {
        std::cerr << "[ОШИБКА МЕНЮ] Критическая ошибка: Системный HWND консоли равен NULL!\n";
    }
}

LRESULT CALLBACK GlobalWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    
    case WM_INITMENUPOPUP:  //обновление всех пунктов меню
        // HIWORD(lParam) != 0 — это системное меню окна, его не трогаем
        if (!HIWORD(lParam) && g_pWindowManager)
            g_pWindowManager->UpdateMenuState();
        break;

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);

        switch (wmId) {
        case IDM_FILE_EXIT:
            PostMessageW(hWnd, WM_CLOSE, 0, 0);
            //DestroyWindow(hWnd); // Уничтожаем окно Win32
            break;
        
        case IDM_DEBUG_SHOWCON:
            if (g_pWindowManager) {
                g_pWindowManager->ToggleConsole();
            }
            else {
                std::cerr << "[ОШИБКА МЕНЮ] Указатель g_pWindowManager равен NULL!\n";
            }
            break;

        case IDM_RUN_POWERON:   
            if (g_pWindowManager) {
                g_pWindowManager->TogglePower();
            }
            else {
                std::cerr << "[ОШИБКА МЕНЮ] Указатель g_pWindowManager равен NULL!\n";
            }
            break;

        case IDM_RUN_RESET:
            if (g_pWindowManager) {
                g_pWindowManager->ResetPower();
            }
            else {
                std::cerr << "[ОШИБКА МЕНЮ] Указатель g_pWindowManager равен NULL!\n";
            }
            break;

        case IDM_DEBUG_STEP:
            if (g_pWindowManager) {
                g_pWindowManager->ToggleStepMode();
            }
            else {
                std::cerr << "[ОШИБКА МЕНЮ] Указатель g_pWindowManager равен NULL!\n";
            }
            break;
        case IDM_DEBUG_LOGCON:  
            if (g_pWindowManager) {
                g_pWindowManager->ToggleLogToConsole();
            }
            else {
                std::cerr << "[ОШИБКА МЕНЮ] Указатель g_pWindowManager равен NULL!\n";
            }
            break;
        case IDM_DEBUG_WINDOWS:     std::cout << "[STUB CMD] Debug -> Debug Windows\n"; break;
        //case IDM_OPTIONS_VIDEO:     std::cout << "[STUB CMD] Options -> Video Card\n"; break;
        case IDM_OPTIONS_SOUND:     std::cout << "[STUB CMD] Options -> Sound Card\n"; break;
        case IDM_OPTIONS_FLOPPY:    std::cout << "[STUB CMD] Options -> Floppy drives\n"; break;
        case IDM_OPTIONS_HARDDISK:  std::cout << "[STUB CMD] Options -> Hard Disks\n"; break;
        case IDM_HELP_ABOUT:        ShowAboutDialog(hWnd); break;

        //новые окна
        case IDM_CPU_8088:   speed_div = 3; std::cout << "SET speed_vid = 3" << std::endl; break;
        case IDM_CPU_80286:  speed_div = 2; std::cout << "SET speed_vid = 2" << std::endl; break;
        case IDM_CPU_80386:  speed_div = 1; std::cout << "SET speed_vid = 1" << std::endl; break;

        case IDM_VIDEO_MDA:  if (!PC_power_on) monitor.set_card_type(videocard_type::MDA); break;
        case IDM_VIDEO_CGA:  if (!PC_power_on) monitor.set_card_type(videocard_type::CGA); break;
        case IDM_VIDEO_EGA:  if (!PC_power_on) monitor.set_card_type(videocard_type::EGA); break;

        case IDM_DEBUG_SCR_REGS: 
            if (!debug_monitor.is_visible()) debug_monitor.show();
            else debug_monitor.hide();
            break;

        case IDM_DEBUG_SCR_FDD:
            if (!FDD_monitor.is_visible()) FDD_monitor.show();
            else FDD_monitor.hide();
            break;

        case IDM_DEBUG_SCR_HDD:
            if (!HDD_monitor.is_visible()) HDD_monitor.show();
            else HDD_monitor.hide();
            break;

        case IDM_DEBUG_SCR_SOUND:
            if (!Audio_monitor.is_visible()) Audio_monitor.show();
            else Audio_monitor.hide();
            break;

        case IDM_DEBUG_SCR_MEM:
            if (!Mem_monitor.is_visible()) Mem_monitor.show();
            else Mem_monitor.hide();
            break;

        case IDM_DEBUG_SCR_EGA:
            if (!EGA_monitor.is_visible()) EGA_monitor.show();
            else EGA_monitor.hide();
            break;

        case IDM_DEBUG_SCR_COM:
            if (!COM_monitor.is_visible()) COM_monitor.show();
            else COM_monitor.hide();
            break;

        default:
            return DefWindowProcW(hWnd, message, wParam, lParam);
        }
        break;
    }
    case WM_CLOSE:
        //обрабатываем нажатие на крестик окна
        std::cout << "[СИСТЕМА] Перехвачено закрытие основного окна (крестик).\n";
        PC_power_on = false;     // просим главный поток остановиться, окно не трогаем
        winman_on = false;
        //DestroyWindow(hWnd); // Уничтожаем окно Win32
        break;

    case WM_APP + 1:             // команда от главного потока: теперь можно
        DestroyWindow(hWnd);
        break;

    case WM_DESTROY:
        //обрабатываем уничтожение самого окна
        std::cout << "[СИСТЕМА] Окно уничтожено. Отправка сигнала выхода...\n";
        PostQuitMessage(0); //сообщение WM_QUIT для WindowThreadFunc
        break;

    case WM_SIZE: {
        // Проверяем, что окно не свернуто в панель задач
        if (wParam != SIZE_MINIMIZED) {
            uint16_t new_client_width = LOWORD(lParam);     // Извлекаем новую ширину и высоту КЛИЕНТСКОЙ области окна из lParam
            uint16_t new_client_height = HIWORD(lParam);
            //SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            monitor.resize(new_client_width, new_client_height);
            //std::cout << "[ВИНДОУС] Размер окна изменен. Новая клиентская область: " << new_client_width << "x" << new_client_height << "\n";
        }
    }
        break;

    case WM_SIZING: {
        // Указатель на структуру RECT с будущими внешними габаритами окна
        PRECT pRect = (PRECT)lParam;

        // Вычисляем текущую ширину и высоту рамок окна Windows (заголовок, меню, границы)
        // Нам нужно вычесть их, чтобы сохранить пропорции именно ВНУТРЕННЕЙ (клиентской) области
        RECT rcWindow = { 0, 0, 0, 0 };
        DWORD style = GetWindowLongPtrW(hWnd, GWL_STYLE);
        AdjustWindowRect(&rcWindow, style, TRUE); // TRUE - учитываем наличие меню

        int frame_width = (rcWindow.right - rcWindow.left) - 0; // Суммарная ширина рамок
        int frame_height = (rcWindow.bottom - rcWindow.top) - 0; // Суммарная высота рамок

        // Желаемое соотношение сторон внутренней области (например, 1.6 для 16:10)
        double target_ratio = 1.625;

        // Вычисляем будущую ширину и высоту внутренней области, которую хочет пользователь
        int current_w = (pRect->right - pRect->left) - frame_width;
        int current_h = (pRect->bottom - pRect->top) - frame_height;

        // Проверяем, за какой именно край или угол пользователь тянет окно
        switch (wParam) {
        case WMSZ_LEFT:
        case WMSZ_RIGHT:
            // Пользователь меняет ШИРИНУ — подстраиваем ВЫСОТУ под пропорции
            current_h = static_cast<int>(current_w / target_ratio);
            pRect->bottom = pRect->top + current_h + frame_height;
            break;

        case WMSZ_TOP:
        case WMSZ_BOTTOM:
            // Пользователь меняет ВЫСОТУ — подстраиваем ШИРИНУ под пропорции
            current_w = static_cast<int>(current_h * target_ratio);
            pRect->right = pRect->left + current_w + frame_width;
            break;

        case WMSZ_TOPLEFT:
        case WMSZ_TOPRIGHT:
        case WMSZ_BOTTOMLEFT:
        case WMSZ_BOTTOMRIGHT:
            // Пользователь тянет за УГОЛ — корректируем высоту на основе новой ширины
            current_h = static_cast<int>(current_w / target_ratio);
            pRect->bottom = pRect->top + current_h + frame_height;
            break;
        }

        // Сохраняем скорректированные значения в глобальный менеджер для последующей инициализации
        if (g_pWindowManager) {
            // Менеджер запоминает точные размеры для видеокарты
            // g_pWindowManager->UpdateCachedSizes(current_w, current_h);
        }

        return TRUE; // Сообщение обработано
    }

    case WM_GETMINMAXINFO: {
        // Указатель на структуру, управляющую размерами максимизации и границами окна
        MINMAXINFO* pMMI = (MINMAXINFO*)lParam;

        // 1. Получаем размеры РАБОЧЕЙ ОБЛАСТИ экрана (сразу за вычетом Панели Задач / Taskbar)
        RECT rcWorkArea;
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &rcWorkArea, 0);

        int max_avail_w = rcWorkArea.right - rcWorkArea.left;
        int max_avail_h = rcWorkArea.bottom - rcWorkArea.top;

        // 2. Рассчитываем размеры рамок текущего стиля окна (заголовок, меню, границы)
        RECT rcWindow = { 0, 0, 0, 0 };
        DWORD style = GetWindowLongPtrW(hWnd, GWL_STYLE);
        //DWORD style = WS_OVERLAPPEDWINDOW | WS_THICKFRAME;
        AdjustWindowRect(&rcWindow, style, TRUE); // TRUE - учитываем верхнее меню

        int frame_width = (rcWindow.right - rcWindow.left);
        int frame_height = (rcWindow.bottom - rcWindow.top);

        // 3. Вычисляем максимально возможные пропорциональные размеры КЛИЕНТСКОЙ области
        double target_ratio = 1.625; // Заданные вами строгие пропорции (например, 1040/640)

        // Предполагаем, что растягиваем по максимуму ширины рабочей области
        int ideal_client_w = max_avail_w - frame_width;
        int ideal_client_h = static_cast<int>(ideal_client_w / target_ratio);

        // Если по высоте мы вылезаем за пределы рабочей области (с учетом панели задач)
        if ((ideal_client_h + frame_height) > max_avail_h) {
            // Пересчитываем в обратную сторону — отталкиваясь от доступной высоты
            ideal_client_h = max_avail_h - frame_height;
            ideal_client_w = static_cast<int>(ideal_client_h * target_ratio);
        }

        // Финальные ВНЕШНИЕ размеры окна, которые разрешено занять при максимизации
        int final_window_w = ideal_client_w + frame_width;
        int final_window_h = ideal_client_h + frame_height;

        // 4. Расчет координат для идеального ЦЕНТРИРОВАНИЯ относительно рабочей области экрана
        int pos_x = rcWorkArea.left + (max_avail_w - final_window_w) / 2;
        int pos_y = rcWorkArea.top + (max_avail_h - final_window_h) / 2;

        // 5. Записываем скорректированные параметры в структуру Windows
        pMMI->ptMaxSize.x = final_window_w; // Максимальная ширина окна
        pMMI->ptMaxSize.y = final_window_h; // Максимальная высота окна

        pMMI->ptMaxPosition.x = pos_x;       // Координата X левого угла распахнутого окна
        pMMI->ptMaxPosition.y = pos_y;       // Координата Y верхнего угла распахнутого окна

        // Ограничиваем также минимальные размеры (например, не меньше оригинального CGA 320x200 + рамки)
        pMMI->ptMinTrackSize.x = 520 + frame_width;
        pMMI->ptMinTrackSize.y = 320 + frame_height;

        return 0; // Сообщение успешно обработано
    }
    
    case WM_LBUTTONDOWN: {
        g_press_drive = panel_drive_at(hWnd, lParam);
        break;
    }
    case WM_LBUTTONUP: {
        int pressed = g_press_drive;
        g_press_drive = -1;
        int drive = panel_drive_at(hWnd, lParam);
        if (drive >= 0 && drive == pressed) on_floppy_button(hWnd, drive);   // клик = нажали и отпустили на той же кнопке
        break;
    }

    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

HMENU EmuWindowManager::CreateMenuSystem() {
    HMENU hMenu = CreateMenu();

    HMENU hFileMenu = CreatePopupMenu();
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_EXIT, L"&Exit");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"&File");

    hRunMenu = CreatePopupMenu();
    AppendMenuW(hRunMenu, MF_STRING, IDM_RUN_POWERON, L"&Power On");
    AppendMenuW(hRunMenu, MF_STRING, IDM_RUN_RESET, L"&Reset");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hRunMenu, L"&Run");

    // Сохраняем указатель на меню Debug, чтобы динамически менять текст
    hDebugMenu = CreatePopupMenu();
    AppendMenuW(hDebugMenu, MF_STRING, IDM_DEBUG_STEP, L"&Step mode (Ctrl+F9)");
    AppendMenuW(hDebugMenu, MF_STRING, IDM_DEBUG_LOGCON, L"&Log OPs to console (Ctrl+F10)");

    // Изначально консоль скрыта, значит пункт меню предлагает её ПОКАЗАТЬ
    AppendMenuW(hDebugMenu, MF_STRING, IDM_DEBUG_SHOWCON, L"Show console");

    //AppendMenuW(hDebugMenu, MF_STRING, IDM_DEBUG_WINDOWS, L"Debug &Windows");
    hDebugScrMenu = CreatePopupMenu();
    AppendMenuW(hDebugScrMenu, MF_STRING, IDM_DEBUG_SCR_REGS, L"Registers, peripherals\tCtrl+1");
    AppendMenuW(hDebugScrMenu, MF_STRING, IDM_DEBUG_SCR_FDD, L"FDD logs\tCtrl+2");
    AppendMenuW(hDebugScrMenu, MF_STRING, IDM_DEBUG_SCR_HDD, L"HDD logs\tCtrl+3");
    AppendMenuW(hDebugScrMenu, MF_STRING, IDM_DEBUG_SCR_SOUND, L"Sound devices\tCtrl+4");
    AppendMenuW(hDebugScrMenu, MF_STRING, IDM_DEBUG_SCR_MEM, L"Memory monitor\tCtrl+5");
    AppendMenuW(hDebugScrMenu, MF_STRING, IDM_DEBUG_SCR_EGA, L"EGA registers\tCtrl+6");
    AppendMenuW(hDebugScrMenu, MF_STRING, IDM_DEBUG_SCR_COM, L"COM port registers\tCtrl+7");
    AppendMenuW(hDebugMenu, MF_POPUP, (UINT_PTR)hDebugScrMenu, L"Debug &screens");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hDebugMenu, L"&Debug");

    hOptionsMenu = CreatePopupMenu();

    // --- Video card ---
    hVideoMenu = CreatePopupMenu();
    AppendMenuW(hVideoMenu, MF_STRING, IDM_VIDEO_MDA, L"MDA");
    AppendMenuW(hVideoMenu, MF_STRING, IDM_VIDEO_CGA, L"CGA");
    AppendMenuW(hVideoMenu, MF_STRING, IDM_VIDEO_EGA, L"EGA");
    AppendMenuW(hOptionsMenu, MF_POPUP, (UINT_PTR)hVideoMenu, L"&Video Card");

    // --- CPU speed ---
    hCpuMenu = CreatePopupMenu();
    AppendMenuW(hCpuMenu, MF_STRING, IDM_CPU_8088, L"8088");
    AppendMenuW(hCpuMenu, MF_STRING, IDM_CPU_80286, L"80286");
    AppendMenuW(hCpuMenu, MF_STRING, IDM_CPU_80386, L"80386");
    AppendMenuW(hOptionsMenu, MF_POPUP, (UINT_PTR)hCpuMenu, L"&CPU Speed");

    AppendMenuW(hOptionsMenu, MF_STRING, IDM_OPTIONS_SOUND, L"&Sound Card");
    AppendMenuW(hOptionsMenu, MF_STRING, IDM_OPTIONS_FLOPPY, L"&Floppy drives");
    AppendMenuW(hOptionsMenu, MF_STRING, IDM_OPTIONS_HARDDISK, L"&Hard Disks");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hOptionsMenu, L"&Options");

    HMENU hHelpMenu = CreatePopupMenu();
    AppendMenuW(hHelpMenu, MF_STRING, IDM_HELP_ABOUT, L"&About");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, L"&Help");

    //предустановленные значения
    if (log_to_console) CheckMenuItem(hDebugMenu, IDM_DEBUG_LOGCON, MF_BYCOMMAND | MF_CHECKED);
    if (step_mode) CheckMenuItem(hDebugMenu, IDM_DEBUG_STEP, MF_BYCOMMAND | MF_CHECKED);
    //предустановка скорости
    switch (speed_div)
    {
    case 1:
        CheckMenuItem(hCpuMenu, IDM_CPU_80386, MF_BYCOMMAND | MF_CHECKED);
        break;
    case 2:
        CheckMenuItem(hCpuMenu, IDM_CPU_80286, MF_BYCOMMAND | MF_CHECKED);
        break;
    case 3:
        CheckMenuItem(hCpuMenu, IDM_CPU_8088, MF_BYCOMMAND | MF_CHECKED);
        break;
    }
    //предустановка видеокарты
    switch (monitor.get_card_type())
    {
    case videocard_type::MDA:
        CheckMenuItem(hVideoMenu, IDM_VIDEO_MDA, MF_BYCOMMAND | MF_CHECKED);
        break;
    case videocard_type::CGA:
        CheckMenuItem(hVideoMenu, IDM_VIDEO_CGA, MF_BYCOMMAND | MF_CHECKED);
        break;
    case videocard_type::EGA:
        CheckMenuItem(hVideoMenu, IDM_VIDEO_EGA, MF_BYCOMMAND | MF_CHECKED);
        break;
    }
    
    return hMenu;
}

std::string EmuWindowManager::GetApplicationDirectory()
{
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);

    std::string exepath(buffer);
    size_t pos = exepath.find_last_of("\\/");
    if (pos != std::string::npos) {
        return exepath.substr(0, pos + 1); // Вернет "C:\Games\Emulator"
    }
    return "";
}

void EmuWindowManager::TogglePower()
{
    if (PC_power_on)
    {
        //выключение
        ModifyMenuW(hRunMenu, IDM_RUN_POWERON, MF_BYCOMMAND | MF_STRING, IDM_RUN_POWERON, L"Power On");
        speaker.stop();
        soundcard.stop();
        PC_power_on = false;
    }
    else
    {
        //включение
        ModifyMenuW(hRunMenu, IDM_RUN_POWERON, MF_BYCOMMAND | MF_STRING, IDM_RUN_POWERON, L"Power Off");
        //делаем сброс регистров 
        reset_registers();
        speaker.start();
        soundcard.start();
        PC_power_on = true;
    }
}

void EmuWindowManager::ResetPower()
{
        //делаем сброс регистров 
        reset_registers();
}

void EmuWindowManager::ToggleLogToConsole() {
    // 1. Захватываем мьютекс для потокобезопасности
    std::lock_guard<std::mutex> lock(window_mutex);

    // 2. Инвертируем состояние глобального флага логирования
    log_to_console = !log_to_console;

    // 3. УПРАВЛЕНИЕ ГАЛОЧКОЙ В МЕНЮ
    if (hDebugMenu) { // Проверяем, что хэндл меню Debug сохранен
        // Если флаг стал true — ставим галочку, если false — снимаем [^1]
        UINT check_flag = log_to_console ? MF_CHECKED : MF_UNCHECKED;

        CheckMenuItem(hDebugMenu, IDM_DEBUG_LOGCON, MF_BYCOMMAND | check_flag);

        // Принудительно перерисовываем верхнюю плашку меню
        DrawMenuBar(hwndMain);
    }
}
void EmuWindowManager::ToggleStepMode() {
    // 1. Захватываем мьютекс для потокобезопасности
    std::lock_guard<std::mutex> lock(window_mutex);

    // 2. Инвертируем состояние глобального флага логирования
    step_mode = !step_mode;

    // 3. УПРАВЛЕНИЕ ГАЛОЧКОЙ В МЕНЮ
    if (hDebugMenu) { // Проверяем, что хэндл меню Debug сохранен
        // Если флаг стал true — ставим галочку, если false — снимаем [^1]
        UINT check_flag = step_mode ? MF_CHECKED : MF_UNCHECKED;

        CheckMenuItem(hDebugMenu, IDM_DEBUG_STEP, MF_BYCOMMAND | check_flag);

        // Принудительно перерисовываем верхнюю плашку меню
        DrawMenuBar(hwndMain);
    }
}

void EmuWindowManager::UpdateMenuState()
{
    auto en = [](bool c) { return (UINT)(MF_BYCOMMAND | (c ? MF_ENABLED : MF_GRAYED)); };    //пункт on/off
    auto ck = [](bool c) { return (UINT)(MF_BYCOMMAND | (c ? MF_CHECKED : MF_UNCHECKED)); }; //галочка on/off

    // ---- Run ----
    // ModifyMenu сбрасывает состояние пункта, поэтому текст меняем до Enable/Check
    ModifyMenuW(hRunMenu, IDM_RUN_POWERON, MF_BYCOMMAND | MF_STRING,
        IDM_RUN_POWERON, PC_power_on ? L"Power Off" : L"Power On");
    EnableMenuItem(hRunMenu, IDM_RUN_RESET, en(PC_power_on));       // Reset только при включённом ПК

    // ---- Debug ----
    CheckMenuItem(hDebugMenu, IDM_DEBUG_STEP, ck(step_mode));
    CheckMenuItem(hDebugMenu, IDM_DEBUG_LOGCON, ck(log_to_console));


    // Отладочные экраны: открыть можно только при включённом ПК (show() иначе всё равно откажет),
    // а закрыть окно, которое уже висит на экране, нужно уметь всегда.
    auto dbg = [&](UINT id, bool visible, bool card_ok = true)
    {
        CheckMenuItem(hDebugScrMenu, id, ck(visible));
        EnableMenuItem(hDebugScrMenu, id, en(visible || (PC_power_on && card_ok)));
    };

    dbg(IDM_DEBUG_SCR_REGS, debug_monitor.is_visible());
    dbg(IDM_DEBUG_SCR_FDD, FDD_monitor.is_visible());
    dbg(IDM_DEBUG_SCR_HDD, HDD_monitor.is_visible());
    dbg(IDM_DEBUG_SCR_SOUND, Audio_monitor.is_visible());
    dbg(IDM_DEBUG_SCR_MEM, Mem_monitor.is_visible());
    dbg(IDM_DEBUG_SCR_EGA, EGA_monitor.is_visible(), monitor.get_card_type() == videocard_type::EGA);
    dbg(IDM_DEBUG_SCR_COM, COM_monitor.is_visible());


    // экран регистров EGA имеет смысл только при выбранной EGA
    //EnableMenuItem(hDebugScrMenu, IDM_DEBUG_SCR_EGA, en(monitor.get_card_type() == videocard_type::EGA));

    // ---- Speed (по speed_div) ----
    int cpu = (speed_div == 3) ? IDM_CPU_8088 :
        (speed_div == 2) ? IDM_CPU_80286 : IDM_CPU_80386;
    CheckMenuRadioItem(hCpuMenu, IDM_CPU_8088, IDM_CPU_80386, cpu, MF_BYCOMMAND);

    // ---- Video card ----
    int vid = IDM_VIDEO_MDA;
    switch (monitor.get_card_type()) {
    case videocard_type::MDA: vid = IDM_VIDEO_MDA; break;
    case videocard_type::CGA: vid = IDM_VIDEO_CGA; break;
    case videocard_type::EGA: vid = IDM_VIDEO_EGA; break;
    }
    CheckMenuRadioItem(hVideoMenu, IDM_VIDEO_MDA, IDM_VIDEO_EGA, vid, MF_BYCOMMAND);

    // менять карту можно только при выключенном ПК
    EnableMenuItem(hVideoMenu, IDM_VIDEO_MDA, en(!PC_power_on));
    EnableMenuItem(hVideoMenu, IDM_VIDEO_CGA, en(!PC_power_on));
    EnableMenuItem(hVideoMenu, IDM_VIDEO_EGA, en(!PC_power_on));

    // ---- Options: привода и диски тоже обычно меняют при выключенном ПК ----
    EnableMenuItem(hOptionsMenu, IDM_OPTIONS_FLOPPY, en(!PC_power_on));
    EnableMenuItem(hOptionsMenu, IDM_OPTIONS_HARDDISK, en(!PC_power_on));
}

//окно ABOUT
static WORD* DlgAlign(WORD* p) { return (WORD*)(((ULONG_PTR)p + 3) & ~(ULONG_PTR)3); }
static WORD* DlgText(WORD* p, const wchar_t* s) { while ((*p++ = (WORD)*s++) != 0) {} return p; }

static WORD* DlgItem(WORD* p, WORD cls, DWORD style, short x, short y, short cx, short cy,
    WORD id, const wchar_t* text)
{
    p = DlgAlign(p);                                   // каждый элемент выровнен по DWORD
    DLGITEMTEMPLATE* it = (DLGITEMTEMPLATE*)p;
    it->style = style | WS_CHILD | WS_VISIBLE;
    it->dwExtendedStyle = 0;
    it->x = x; it->y = y; it->cx = cx; it->cy = cy; it->id = id;
    p = (WORD*)(it + 1);
    *p++ = 0xFFFF; *p++ = cls;                         // 0x0080 button, 0x0081 edit, 0x0082 static
    p = DlgText(p, text);
    *p++ = 0;                                          // данных создания нет
    return p;
}

static INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        SetFocus(GetDlgItem(hDlg, IDOK));
        return FALSE;                                  // фокус выставили сами
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, 0);
            return TRUE;
        case IDC_ABOUT_OPEN:
            ShellExecuteW(hDlg, L"open", ABOUT_URL, NULL, NULL, SW_SHOWNORMAL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void ShowAboutDialog(HWND parent)
{
    alignas(4) WORD buf[512] = {};
    DLGTEMPLATE* dt = (DLGTEMPLATE*)buf;
    dt->style = DS_MODALFRAME | DS_CENTER | DS_SETFONT | WS_POPUP | WS_CAPTION | WS_SYSMENU;
    dt->dwExtendedStyle = 0;
    dt->cdit = 4;                                      // число элементов
    dt->x = 0; dt->y = 0; dt->cx = 220; dt->cy = 86;

    WORD* p = (WORD*)(dt + 1);
    *p++ = 0;                                          // без меню
    *p++ = 0;                                          // стандартный класс диалога
    p = DlgText(p, L"About");
    *p++ = 9;                                          // шрифт (DS_SETFONT)
    p = DlgText(p, L"Segoe UI");

    p = DlgItem(p, 0x0082, SS_LEFT, 10, 10, 200, 12, (WORD)-1, L"IBM PC/XT emulator");
    p = DlgItem(p, 0x0081, ES_READONLY | ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP,
        10, 28, 200, 14, 1102, ABOUT_URL);
    p = DlgItem(p, 0x0080, BS_PUSHBUTTON | WS_TABSTOP, 10, 58, 90, 16, IDC_ABOUT_OPEN, L"Open in browser");
    p = DlgItem(p, 0x0080, BS_DEFPUSHBUTTON | WS_TABSTOP, 140, 58, 70, 16, IDOK, L"OK");

    DialogBoxIndirectParamW(GetModuleHandleW(NULL), dt, parent, AboutDlgProc, 0);
}