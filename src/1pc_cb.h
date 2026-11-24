#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <mmsystem.h>
#include <commctrl.h>
#include <windowsx.h>
#include <process.h>

#include <random>
#include <string>
#include <vector>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "winmm.lib")

inline constexpr wchar_t kWindowClass[] = L"1pc-CB_Menu";
inline constexpr wchar_t kWindowTitle[] = L"1pc-CB";
inline constexpr int kWindowWidth = 650;
inline constexpr int kWindowHeight = 640;

inline constexpr DWORD kBroadcastFlags = 0x92;
inline constexpr DWORD kBroadcastRecipients = 8;

struct Config {
    volatile bool  enable;          // Master Enable        (false)
    volatile LONG  tolerance;       // Tolerance            (25,  0..50)
    volatile LONG  hueTol;          // Hue tolerance        (15,  0..180)
    volatile LONG  cooldownMs;      // Shot delay           (400, 1..1000 ms)
    volatile LONG  minDelay;        // Human min            (30,  1..500 ms)
    volatile LONG  maxDelay;        // Human max            (70,  1..500 ms)
    volatile LONG  shootVk;         // Shoot key/button     (79 = O)
    volatile LONG  activateVk;      // Activate key/button  (5 = X1 / Mouse5)
    volatile LONG  captureX;        // Width (X)            (9,   1..20)
    volatile LONG  captureY;        // Height (Y)           (9,   1..20)
    volatile bool  holdMode;        // Hold to Activate     (false)
    volatile LONG  minPixels;       // Min pixels           (1,   1..200)
    volatile LONG  temporalFrames;  // History frames       (1,   1..20)
    volatile LONG  temporalMin;     // Required hits        (1,   1..20)
    volatile bool  alive;           // Worker loop flag     (true)

    Config();
};

extern Config g_cfg;

void SetLong(volatile LONG& dst, LONG value);
LONG GetLong(const volatile LONG& src);
int ClampInt(int value, int lo, int hi);
void SleepMilliseconds(int ms);

void LoadConfig();
void SaveConfig();

class CaptureContext {
public:
    std::vector<BYTE> pixels;

    bool Capture(int width, int height);
    void Shutdown();
    ~CaptureContext();

private:
    bool Ensure(int width, int height, HDC screenDC);

    HDC memDC_ = nullptr;
    HBITMAP bitmap_ = nullptr;
    HGDIOBJ oldBitmap_ = nullptr;
    int width_ = 0;
    int height_ = 0;
};

bool PixelMatches(const BYTE* pixel, float tolerance, float hueTolerance);

void InjectKeyOrButton(DWORD vk, std::mt19937& rng);
std::wstring VkDisplayName(DWORD vk);

unsigned WINAPI WorkerThread(LPVOID parameter);

void EnableDpiAwareness();
bool IsBindingActive();
void ProcessKeyBindScan();
void UpdateUiState();
LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
