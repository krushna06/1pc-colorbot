#include "1pc_cb.h"

#include <algorithm>
#include <cmath>

bool CaptureContext::Capture(int width, int height) {
    HDC screenDC = GetDC(nullptr);
    if (!screenDC) {
        return false;
    }

    if (!Ensure(width, height, screenDC)) {
        ReleaseDC(nullptr, screenDC);
        return false;
    }

    const int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    const int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = width;
    bi.bmiHeader.biHeight = -height;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    bool ok = false;
    if (BitBlt(memDC_, 0, 0, width, height, screenDC, x, y, SRCCOPY)) {
        const int lines = GetDIBits(memDC_, bitmap_, 0, height,
                                    pixels.data(), &bi, DIB_RGB_COLORS);
        ok = (lines == height);
    }

    ReleaseDC(nullptr, screenDC);
    return ok;
}

void CaptureContext::Shutdown() {
    if (memDC_ && oldBitmap_ && oldBitmap_ != HGDI_ERROR) {
        SelectObject(memDC_, oldBitmap_);
    }
    if (bitmap_) {
        DeleteObject(bitmap_);
    }
    if (memDC_) {
        DeleteDC(memDC_);
    }
    memDC_ = nullptr;
    bitmap_ = nullptr;
    oldBitmap_ = nullptr;
    width_ = 0;
    height_ = 0;
    pixels.clear();
}

CaptureContext::~CaptureContext() {
    Shutdown();
}

bool CaptureContext::Ensure(int width, int height, HDC screenDC) {
    if (!memDC_) {
        memDC_ = CreateCompatibleDC(screenDC);
    }
    if (!memDC_) {
        return false;
    }
    if (bitmap_ && width == width_ && height == height_) {
        return true;
    }

    if (oldBitmap_ && oldBitmap_ != HGDI_ERROR) {
        SelectObject(memDC_, oldBitmap_);
        oldBitmap_ = nullptr;
    }
    if (bitmap_) {
        DeleteObject(bitmap_);
        bitmap_ = nullptr;
    }

    bitmap_ = CreateCompatibleBitmap(screenDC, width, height);
    if (!bitmap_) {
        return false;
    }
    oldBitmap_ = SelectObject(memDC_, bitmap_);
    if (!oldBitmap_ || oldBitmap_ == HGDI_ERROR) {
        DeleteObject(bitmap_);
        bitmap_ = nullptr;
        return false;
    }

    width_ = width;
    height_ = height;
    pixels.assign(static_cast<size_t>(width) * static_cast<size_t>(height) * 4, 0);
    return true;
}

bool PixelMatches(const BYTE* pixel, float tolerance, float hueTolerance) {
    const float r = static_cast<float>(pixel[2]) / 255.0f;
    const float g = static_cast<float>(pixel[1]) / 255.0f;
    const float b = static_cast<float>(pixel[0]) / 255.0f;

    const float maxValue = std::max({r, g, b});
    const float minValue = std::min({r, g, b});
    const float delta = maxValue - minValue;

    float saturation = 0.0f;
    float hue = 0.0f;

    if (delta >= 0.00001f) {
        saturation = (delta / maxValue) * 100.0f;
        if (maxValue == r) {
            hue = std::fmod((g - b) / delta, 6.0f) * 60.0f;
        } else if (maxValue == g) {
            hue = ((b - r) / delta + 2.0f) * 60.0f;
        } else {
            hue = ((r - g) / delta + 4.0f) * 60.0f;
        }
        if (hue < 0.0f) {
            hue += 360.0f;
        }
    }

    const float value = maxValue * 100.0f;
    const float hueLow = 275.0f - hueTolerance;
    const float hueHigh = 320.0f + hueTolerance;

    bool hueOk;
    if (hueHigh >= 360.0f) {
        hueOk = (hue >= hueLow || hue <= hueHigh - 360.0f);
    } else {
        hueOk = (hue >= hueLow && hue <= hueHigh);
    }

    const float saturationLow = std::max(35.0f - tolerance, 0.0f);
    const float saturationHigh = std::min(tolerance + 100.0f, 100.0f);
    const float valueLow = std::max(85.0f - tolerance, 0.0f);

    return hueOk &&
           saturation >= saturationLow && saturation <= saturationHigh &&
           value >= valueLow && value <= saturationHigh;
}
