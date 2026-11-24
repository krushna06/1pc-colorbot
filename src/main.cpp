#include "1pc_cb.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    EnableDpiAwareness();
    LoadConfig();

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = kWindowClass;

    if (!RegisterClassExW(&windowClass)) {
        return 1;
    }

    HWND window = CreateWindowExW(
        0, kWindowClass, kWindowTitle,
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
        100, 100, kWindowWidth, kWindowHeight,
        nullptr, nullptr, instance, nullptr);
    if (!window) {
        UnregisterClassW(kWindowClass, instance);
        return 1;
    }

    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);

    unsigned int threadId = 0;
    HANDLE worker = reinterpret_cast<HANDLE>(
        _beginthreadex(nullptr, 0, WorkerThread, nullptr, 0, &threadId));
    if (!worker) {
        DestroyWindow(window);
        UnregisterClassW(kWindowClass, instance);
        return 1;
    }

    bool running = true;
    bool menuVisible = true;
    bool insertPrevious = false;
    bool activatePrevious = false;
    MSG message{};

    while (running) {
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        if (!running) {
            break;
        }

        const bool insertDown = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
        if (!IsBindingActive() && insertDown && !insertPrevious) {
            menuVisible = !menuVisible;
            ShowWindow(window, menuVisible ? SW_SHOW : SW_HIDE);
        }
        insertPrevious = insertDown;

        const bool activateDown =
            (GetAsyncKeyState(static_cast<int>(GetLong(g_cfg.activateVk))) & 0x8000) != 0;
        if (!IsBindingActive()) {
            if (g_cfg.holdMode) {
                if (activateDown != g_cfg.enable) {
                    g_cfg.enable = activateDown;
                    SaveConfig();
                }
            } else if (activateDown && !activatePrevious) {
                g_cfg.enable = !g_cfg.enable;
                SaveConfig();
            }
        }
        activatePrevious = activateDown;

        ProcessKeyBindScan();
        UpdateUiState();

        SleepMilliseconds(menuVisible ? 1 : 10);
    }

    g_cfg.alive = false;
    WaitForSingleObject(worker, INFINITE);
    CloseHandle(worker);

    DestroyWindow(window);
    UnregisterClassW(kWindowClass, instance);
    return 0;
}
