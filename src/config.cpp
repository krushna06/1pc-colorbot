#include "1pc_cb.h"

#include <cstdio>

Config g_cfg;

Config::Config()
    : enable(false),
      tolerance(25),
      hueTol(15),
      cooldownMs(400),
      minDelay(30),
      maxDelay(70),
      shootVk(79),
      activateVk(5),
      captureX(9),
      captureY(9),
      holdMode(false),
      minPixels(1),
      temporalFrames(1),
      temporalMin(1),
      alive(true) {}

void SetLong(volatile LONG& dst, LONG value) {
    InterlockedExchange(&dst, value);
}

LONG GetLong(const volatile LONG& src) {
    return static_cast<LONG>(src);
}

int ClampInt(int value, int lo, int hi) {
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}

void SleepMilliseconds(int ms) {
    if (ms <= 0) {
        return;
    }
    Sleep(static_cast<DWORD>(ms));
}

namespace {

std::wstring g_settingsPath;

void InitSettingsPath() {
    wchar_t modulePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    const std::wstring full(modulePath);
    const size_t slash = full.find_last_of(L"\\/");
    const std::wstring dir = (slash == std::wstring::npos) ? L"." : full.substr(0, slash + 1);
    g_settingsPath = dir + L"1pc-CB.ini";

    HANDLE test = CreateFileW(g_settingsPath.c_str(), GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (test != INVALID_HANDLE_VALUE) {
        CloseHandle(test);
        return;
    }

    wchar_t appData[MAX_PATH] = {};
    if (GetEnvironmentVariableW(L"APPDATA", appData, MAX_PATH) > 0) {
        const std::wstring folder = std::wstring(appData) + L"\\1pc-CB";
        CreateDirectoryW(folder.c_str(), nullptr);
        g_settingsPath = folder + L"\\settings.ini";
    }
}

}

void LoadConfig() {
    InitSettingsPath();

    constexpr wchar_t section[] = L"Settings";
    const auto read = [&](const wchar_t* key, int fallback) -> int {
        return static_cast<int>(GetPrivateProfileIntW(section, key, fallback,
                                                       g_settingsPath.c_str()));
    };

    g_cfg.enable = read(L"Enable", 0) != 0;
    SetLong(g_cfg.tolerance, ClampInt(read(L"Tolerance", 25), 0, 50));
    SetLong(g_cfg.hueTol, ClampInt(read(L"HueTolerance", 15), 0, 180));
    SetLong(g_cfg.cooldownMs, ClampInt(read(L"Cooldown", 400), 1, 1000));
    SetLong(g_cfg.minDelay, ClampInt(read(L"MinDelay", 30), 1, 500));
    SetLong(g_cfg.maxDelay, ClampInt(read(L"MaxDelay", 70), 1, 500));
    SetLong(g_cfg.shootVk, ClampInt(read(L"Shoot", 79), 1, 255));
    SetLong(g_cfg.activateVk, ClampInt(read(L"Activate", 5), 1, 255));
    SetLong(g_cfg.captureX, ClampInt(read(L"CaptureX", 9), 1, 20));
    SetLong(g_cfg.captureY, ClampInt(read(L"CaptureY", 9), 1, 20));
    g_cfg.holdMode = read(L"HoldMode", 0) != 0;
    SetLong(g_cfg.minPixels, ClampInt(read(L"MinPixels", 1), 1, 200));
    SetLong(g_cfg.temporalFrames, ClampInt(read(L"TemporalFrames", 1), 1, 20));
    SetLong(g_cfg.temporalMin, ClampInt(read(L"TemporalMin", 1), 1, 20));

    if (GetLong(g_cfg.minDelay) > GetLong(g_cfg.maxDelay)) {
        SetLong(g_cfg.maxDelay, GetLong(g_cfg.minDelay));
    }
    if (GetLong(g_cfg.temporalMin) > GetLong(g_cfg.temporalFrames)) {
        SetLong(g_cfg.temporalFrames, GetLong(g_cfg.temporalMin));
    }
    g_cfg.alive = true;
}

void SaveConfig() {
    if (g_settingsPath.empty()) {
        InitSettingsPath();
    }

    constexpr wchar_t section[] = L"Settings";
    const auto write = [&](const wchar_t* key, int value) {
        wchar_t buffer[32] = {};
        std::swprintf(buffer, 32, L"%d", value);
        WritePrivateProfileStringW(section, key, buffer, g_settingsPath.c_str());
    };

    write(L"Enable", g_cfg.enable ? 1 : 0);
    write(L"Tolerance", static_cast<int>(GetLong(g_cfg.tolerance)));
    write(L"HueTolerance", static_cast<int>(GetLong(g_cfg.hueTol)));
    write(L"Cooldown", static_cast<int>(GetLong(g_cfg.cooldownMs)));
    write(L"MinDelay", static_cast<int>(GetLong(g_cfg.minDelay)));
    write(L"MaxDelay", static_cast<int>(GetLong(g_cfg.maxDelay)));
    write(L"Shoot", static_cast<int>(GetLong(g_cfg.shootVk)));
    write(L"Activate", static_cast<int>(GetLong(g_cfg.activateVk)));
    write(L"CaptureX", static_cast<int>(GetLong(g_cfg.captureX)));
    write(L"CaptureY", static_cast<int>(GetLong(g_cfg.captureY)));
    write(L"HoldMode", g_cfg.holdMode ? 1 : 0);
    write(L"MinPixels", static_cast<int>(GetLong(g_cfg.minPixels)));
    write(L"TemporalFrames", static_cast<int>(GetLong(g_cfg.temporalFrames)));
    write(L"TemporalMin", static_cast<int>(GetLong(g_cfg.temporalMin)));
}
