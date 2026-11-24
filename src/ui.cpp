#include "1pc_cb.h"

#include <cstdio>
#include <cwchar>

namespace {

constexpr int IDC_MASTER = 1001;
constexpr int IDC_HOLD = 1002;
constexpr int IDC_BTN_SHOOT = 1003;
constexpr int IDC_BTN_ACTIVATE = 1004;

constexpr int IDC_SLIDER_FIRST = 1100;
constexpr int SLIDER_COUNT = 10;

enum SliderIndex {
    SL_COOLDOWN = 0,
    SL_MINDELAY,
    SL_MAXDELAY,
    SL_TOLERANCE,
    SL_HUETOL,
    SL_MINPIXELS,
    SL_FRAMES,
    SL_REQUIRED,
    SL_CAPTUREX,
    SL_CAPTUREY,
};

struct SliderInfo {
    int minValue;
    int maxValue;
    const wchar_t* label;
    int y;
};

constexpr SliderInfo kSliders[SLIDER_COUNT] = {
    {   1, 1000, L"Shot delay",      204 },
    {   1,  500, L"Human min",       234 },
    {   1,  500, L"Human max",       264 },
    {   0,   50, L"Tolerance",       326 },
    {   0,  180, L"Hue tolerance",   356 },
    {   1,  200, L"Min pixels",      386 },
    {   1,   20, L"History frames",  416 },
    {   1,   20, L"Required hits",   446 },
    {   1,   20, L"Width (X)",       508 },
    {   1,   20, L"Height (Y)",      538 },
};

HWND g_sliderHwnd[SLIDER_COUNT] = {};
HWND g_valueHwnd[SLIDER_COUNT] = {};
HWND g_masterHwnd = nullptr;
HWND g_holdHwnd = nullptr;
HWND g_shootHwnd = nullptr;
HWND g_activateHwnd = nullptr;
HWND g_statusHwnd = nullptr;

int g_bindMode = 0;
bool g_bindWaitingRelease = false;

int SliderValue(SliderIndex index) {
    switch (index) {
    case SL_COOLDOWN: return static_cast<int>(GetLong(g_cfg.cooldownMs));
    case SL_MINDELAY: return static_cast<int>(GetLong(g_cfg.minDelay));
    case SL_MAXDELAY: return static_cast<int>(GetLong(g_cfg.maxDelay));
    case SL_TOLERANCE: return static_cast<int>(GetLong(g_cfg.tolerance));
    case SL_HUETOL: return static_cast<int>(GetLong(g_cfg.hueTol));
    case SL_MINPIXELS: return static_cast<int>(GetLong(g_cfg.minPixels));
    case SL_FRAMES: return static_cast<int>(GetLong(g_cfg.temporalFrames));
    case SL_REQUIRED: return static_cast<int>(GetLong(g_cfg.temporalMin));
    case SL_CAPTUREX: return static_cast<int>(GetLong(g_cfg.captureX));
    case SL_CAPTUREY: return static_cast<int>(GetLong(g_cfg.captureY));
    }
    return 0;
}

void SetSliderValue(SliderIndex index, int value) {
    value = ClampInt(value, kSliders[index].minValue, kSliders[index].maxValue);
    switch (index) {
    case SL_COOLDOWN:
        SetLong(g_cfg.cooldownMs, value);
        break;
    case SL_MINDELAY:
        SetLong(g_cfg.minDelay, value);
        if (value > GetLong(g_cfg.maxDelay)) {
            SetLong(g_cfg.maxDelay, value);
        }
        break;
    case SL_MAXDELAY:
        SetLong(g_cfg.maxDelay, value);
        if (value < GetLong(g_cfg.minDelay)) {
            SetLong(g_cfg.minDelay, value);
        }
        break;
    case SL_TOLERANCE:
        SetLong(g_cfg.tolerance, value);
        break;
    case SL_HUETOL:
        SetLong(g_cfg.hueTol, value);
        break;
    case SL_MINPIXELS:
        SetLong(g_cfg.minPixels, value);
        break;
    case SL_FRAMES:
        SetLong(g_cfg.temporalFrames, value);
        break;
    case SL_REQUIRED:
        SetLong(g_cfg.temporalMin, value);
        if (value > GetLong(g_cfg.temporalFrames)) {
            SetLong(g_cfg.temporalFrames, value);
        }
        break;
    case SL_CAPTUREX:
        SetLong(g_cfg.captureX, value);
        break;
    case SL_CAPTUREY:
        SetLong(g_cfg.captureY, value);
        break;
    }
}

void SetTextIfChanged(HWND hwnd, const wchar_t* text) {
    if (!hwnd) {
        return;
    }
    wchar_t current[256] = {};
    const int length = GetWindowTextW(hwnd, current, 256);
    if (length != static_cast<int>(std::wcslen(text)) || std::wcscmp(current, text) != 0) {
        SetWindowTextW(hwnd, text);
    }
}

void RefreshUiText() {
    wchar_t buffer[128] = {};

    std::swprintf(buffer, 128, L"Shoot: %s",
                  VkDisplayName(static_cast<DWORD>(GetLong(g_cfg.shootVk))).c_str());
    SetTextIfChanged(g_shootHwnd, buffer);

    std::swprintf(buffer, 128, L"Activate: %s",
                  VkDisplayName(static_cast<DWORD>(GetLong(g_cfg.activateVk))).c_str());
    SetTextIfChanged(g_activateHwnd, buffer);

    if (g_bindMode != 0) {
        SetTextIfChanged(g_statusHwnd, L"Listening for input... (Esc cancels)");
    } else {
        SetTextIfChanged(g_statusHwnd, L"Press Insert to hide the menu.");
    }
}

void RefreshValues() {
    wchar_t buffer[64] = {};
    for (int i = 0; i < SLIDER_COUNT; ++i) {
        const int value = SliderValue(static_cast<SliderIndex>(i));
        if (g_sliderHwnd[i] &&
            static_cast<int>(SendMessageW(g_sliderHwnd[i], TBM_GETPOS, 0, 0)) != value) {
            SendMessageW(g_sliderHwnd[i], TBM_SETPOS, TRUE, value);
        }
        if (g_valueHwnd[i]) {
            if (i == SL_COOLDOWN || i == SL_MINDELAY || i == SL_MAXDELAY) {
                std::swprintf(buffer, 64, L"%d ms", value);
            } else {
                std::swprintf(buffer, 64, L"%d", value);
            }
            SetTextIfChanged(g_valueHwnd[i], buffer);
        }
    }
}

HWND AddStatic(HWND parent, const wchar_t* text, int x, int y, int width, int height,
               bool bold = false) {
    HWND hwnd = CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE | SS_LEFT,
                                x, y, width, height, parent, nullptr, nullptr, nullptr);
    if (hwnd && bold) {
        LOGFONTW fontInfo{};
        fontInfo.lfHeight = -16;
        fontInfo.lfWeight = FW_BOLD;
        wcscpy_s(fontInfo.lfFaceName, L"Segoe UI");
        HFONT font = CreateFontIndirectW(&fontInfo);
        SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }
    return hwnd;
}

HWND AddSlider(HWND parent, SliderIndex index, int controlId) {
    const SliderInfo& info = kSliders[index];
    AddStatic(parent, info.label, 20, info.y, 150, 20);

    HWND track = CreateWindowExW(
        0, TRACKBAR_CLASSW, L"",
        WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS,
        175, info.y - 4, 320, 22,
        parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(controlId)), nullptr, nullptr);

    if (track) {
        SendMessageW(track, TBM_SETRANGE, TRUE, MAKELONG(info.minValue, info.maxValue));
        SendMessageW(track, TBM_SETPOS, TRUE, SliderValue(index));
    }

    g_valueHwnd[index] = AddStatic(parent, L"", 510, info.y, 100, 20);
    return track;
}

}

void EnableDpiAwareness() {
    HMODULE user32 = LoadLibraryW(L"user32.dll");
    if (user32) {
        using SetContextFn = decltype(&SetProcessDpiAwarenessContext);
        SetContextFn setContext = reinterpret_cast<SetContextFn>(
            GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (setContext) {
            setContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            FreeLibrary(user32);
            return;
        }
        FreeLibrary(user32);
    }

    HMODULE shcore = LoadLibraryW(L"shcore.dll");
    if (shcore) {
        using SetAwarenessFn = HRESULT(WINAPI*)(int);
        SetAwarenessFn setAwareness = reinterpret_cast<SetAwarenessFn>(
            GetProcAddress(shcore, "SetProcessDpiAwareness"));
        if (setAwareness) {
            setAwareness(2);
            FreeLibrary(shcore);
            return;
        }
        FreeLibrary(shcore);
    }
    SetProcessDPIAware();
}

bool IsBindingActive() {
    return g_bindMode != 0;
}

void UpdateUiState() {
    Button_SetCheck(g_masterHwnd, g_cfg.enable ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(g_holdHwnd, g_cfg.holdMode ? BST_CHECKED : BST_UNCHECKED);
    RefreshUiText();
}

void ProcessKeyBindScan() {
    if (g_bindMode == 0) {
        return;
    }

    if (g_bindWaitingRelease) {
        bool anyDown = false;
        for (int vk = 1; vk < 256; ++vk) {
            if (GetAsyncKeyState(vk) & 0x8000) {
                anyDown = true;
                break;
            }
        }
        if (!anyDown) {
            g_bindWaitingRelease = false;
        }
        return;
    }

    for (int vk = 1; vk < 256; ++vk) {
        if (GetAsyncKeyState(vk) & 0x8000) {
            if (vk != VK_ESCAPE) {
                if (g_bindMode == 1) {
                    SetLong(g_cfg.shootVk, vk);
                } else {
                    SetLong(g_cfg.activateVk, vk);
                }
            }
            g_bindMode = 0;
            RefreshUiText();
            SaveConfig();
            return;
        }
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        INITCOMMONCONTROLSEX commonControls{};
        commonControls.dwSize = sizeof(commonControls);
        commonControls.dwICC = ICC_BAR_CLASSES;
        InitCommonControlsEx(&commonControls);

        AddStatic(hwnd, L"1pc-CB Status", 20, 12, 300, 20, true);
        g_masterHwnd = CreateWindowExW(0, L"BUTTON", L"Master Enable",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            20, 42, 210, 22, hwnd,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_MASTER)), nullptr, nullptr);
        g_holdHwnd = CreateWindowExW(0, L"BUTTON", L"Hold to Activate",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            240, 42, 220, 22, hwnd,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_HOLD)), nullptr, nullptr);

        AddStatic(hwnd, L"Keybinds", 20, 82, 300, 20, true);
        g_shootHwnd = CreateWindowExW(0, L"BUTTON", L"Shoot: O",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20, 110, 285, 26, hwnd,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_SHOOT)), nullptr, nullptr);
        g_activateHwnd = CreateWindowExW(0, L"BUTTON", L"Activate: Mouse5",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            315, 110, 285, 26, hwnd,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_ACTIVATE)), nullptr, nullptr);
        g_statusHwnd = AddStatic(hwnd, L"Press Insert to hide the menu.", 20, 144, 580, 20);

        AddStatic(hwnd, L"Timing", 20, 180, 300, 20, true);
        AddStatic(hwnd, L"Detection", 20, 302, 300, 20, true);
        AddStatic(hwnd, L"Capture Area", 20, 484, 300, 20, true);

        for (int i = 0; i < SLIDER_COUNT; ++i) {
            g_sliderHwnd[i] = AddSlider(hwnd, static_cast<SliderIndex>(i), IDC_SLIDER_FIRST + i);
        }

        UpdateUiState();
        RefreshValues();
        return 0;
    }

    case WM_COMMAND: {
        const int id = LOWORD(wParam);
        const WORD code = HIWORD(wParam);
        if (code == BN_CLICKED) {
            switch (id) {
            case IDC_MASTER:
                g_cfg.enable = (Button_GetCheck(g_masterHwnd) == BST_CHECKED);
                break;
            case IDC_HOLD:
                g_cfg.holdMode = (Button_GetCheck(g_holdHwnd) == BST_CHECKED);
                break;
            case IDC_BTN_SHOOT:
                g_bindMode = 1;
                g_bindWaitingRelease = true;
                RefreshUiText();
                break;
            case IDC_BTN_ACTIVATE:
                g_bindMode = 2;
                g_bindWaitingRelease = true;
                RefreshUiText();
                break;
            default:
                break;
            }
            if (id == IDC_MASTER || id == IDC_HOLD) {
                SaveConfig();
            }
        }
        return 0;
    }

    case WM_HSCROLL: {
        HWND track = reinterpret_cast<HWND>(lParam);
        const int id = GetDlgCtrlID(track);
        if (id >= IDC_SLIDER_FIRST && id < IDC_SLIDER_FIRST + SLIDER_COUNT) {
            const int index = id - IDC_SLIDER_FIRST;
            const int position = static_cast<int>(SendMessageW(track, TBM_GETPOS, 0, 0));
            SetSliderValue(static_cast<SliderIndex>(index), position);
            RefreshValues();
            SaveConfig();
        }
        return 0;
    }

    case WM_CLOSE:
        SaveConfig();
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}
