#include "1pc_cb.h"

#include <cstdio>

namespace {

void PostInputMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    DWORD recipients = kBroadcastRecipients;
    BroadcastSystemMessageW(kBroadcastFlags, &recipients, message, wParam, lParam);
}

}

void InjectKeyOrButton(DWORD vk, std::mt19937& rng) {
    const UINT holdMs = static_cast<UINT>((rng() >> 28) + 15);  // 15..30 ms

    POINT point{};
    GetCursorPos(&point);
    const LPARAM cursor = MAKELPARAM(static_cast<WORD>(point.x), static_cast<WORD>(point.y));

    UINT downMessage = 0;
    UINT upMessage = 0;
    WPARAM downWParam = 0;
    WPARAM upWParam = 0;

    switch (vk) {
    case 1:
        downMessage = WM_LBUTTONDOWN; upMessage = WM_LBUTTONUP;
        downWParam = MK_LBUTTON;      upWParam = 0;
        break;
    case 2:
        downMessage = WM_RBUTTONDOWN; upMessage = WM_RBUTTONUP;
        downWParam = MK_RBUTTON;      upWParam = 0;
        break;
    case 4:
        downMessage = WM_MBUTTONDOWN; upMessage = WM_MBUTTONUP;
        downWParam = MK_MBUTTON;      upWParam = 0;
        break;
    case 5:
        downMessage = WM_XBUTTONDOWN; upMessage = WM_XBUTTONUP;
        downWParam = MK_XBUTTON1 | XBUTTON1;
        upWParam = XBUTTON1;
        break;
    case 6:
        downMessage = WM_XBUTTONDOWN; upMessage = WM_XBUTTONUP;
        downWParam = MK_XBUTTON1 | XBUTTON2;
        upWParam = XBUTTON2;
        break;
    default: {
        const UINT scancode = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
        const LPARAM downLParam = static_cast<LPARAM>((scancode << 16) | 1u);
        const LPARAM upLParam = static_cast<LPARAM>((scancode << 16) | 0xC0000001u);
        PostInputMessage(WM_KEYDOWN, static_cast<WPARAM>(vk), downLParam);
        SleepMilliseconds(holdMs);
        PostInputMessage(WM_KEYUP, static_cast<WPARAM>(vk), upLParam);
        return;
    }
    }

    PostInputMessage(downMessage, downWParam, cursor);
    SleepMilliseconds(holdMs);
    PostInputMessage(upMessage, upWParam, cursor);
}

std::wstring VkDisplayName(DWORD vk) {
    switch (vk) {
    case 1: return L"Mouse1";
    case 2: return L"Mouse2";
    case 4: return L"Mouse4";
    case 5: return L"Mouse5";
    case 6: return L"Mouse6";
    case VK_ESCAPE: return L"Esc";
    case VK_INSERT: return L"Insert";
    case VK_SPACE: return L"Space";
    case VK_RETURN: return L"Enter";
    case VK_TAB: return L"Tab";
    case VK_SHIFT: return L"Shift";
    case VK_CONTROL: return L"Ctrl";
    case VK_MENU: return L"Alt";
    case VK_BACK: return L"Backspace";
    case VK_CAPITAL: return L"CapsLock";
    case VK_LSHIFT: return L"LShift";
    case VK_RSHIFT: return L"RShift";
    case VK_LCONTROL: return L"LCtrl";
    case VK_RCONTROL: return L"RCtrl";
    case VK_LMENU: return L"LAlt";
    case VK_RMENU: return L"RAlt";
    default: break;
    }

    const UINT ch = MapVirtualKeyW(vk, MAPVK_VK_TO_CHAR);
    if (ch != 0 && (ch & 0x80000000u) == 0) {
        return std::wstring(1, static_cast<wchar_t>(ch));
    }

    wchar_t buffer[64] = {};
    const UINT scancode = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    if (scancode != 0 && GetKeyNameTextW(static_cast<LONG>(scancode << 16), buffer, 64) > 0) {
        return std::wstring(buffer);
    }
    std::swprintf(buffer, 64, L"VK_%02X", static_cast<unsigned>(vk));
    return std::wstring(buffer);
}
