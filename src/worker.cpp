#include "1pc_cb.h"

#include <algorithm>
#include <chrono>
#include <deque>

unsigned WINAPI WorkerThread(LPVOID) {
    using Clock = std::chrono::steady_clock;

    timeBeginPeriod(1);

    std::mt19937 rng(static_cast<uint32_t>(std::random_device{}()));
    auto lastShot = Clock::now();

    std::deque<unsigned char> history;
    int historySize = -1;
    CaptureContext capture;

    while (g_cfg.alive) {
        if (!g_cfg.enable) {
            SleepMilliseconds(50);
            continue;
        }

        const auto now = Clock::now();
        const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastShot).count();
        if (elapsedMs < static_cast<long long>(GetLong(g_cfg.cooldownMs))) {
            SleepMilliseconds(1);
            continue;
        }

        const int width = ClampInt(static_cast<int>(GetLong(g_cfg.captureX)), 1, 20);
        const int height = ClampInt(static_cast<int>(GetLong(g_cfg.captureY)), 1, 20);

        if (!capture.Capture(width, height)) {
            SleepMilliseconds(2);
            continue;
        }

        const float tolerance = static_cast<float>(GetLong(g_cfg.tolerance));
        const float hueTolerance = static_cast<float>(GetLong(g_cfg.hueTol));

        int matchCount = 0;
        for (int y = 0; y < height; ++y) {
            const BYTE* row = capture.pixels.data() + static_cast<size_t>(y) * width * 4;
            for (int x = 0; x < width; ++x) {
                if (PixelMatches(row + static_cast<size_t>(x) * 4, tolerance, hueTolerance)) {
                    ++matchCount;
                }
            }
        }

        const bool frameHit = matchCount >= static_cast<int>(GetLong(g_cfg.minPixels));

        int frames = std::max(1, static_cast<int>(GetLong(g_cfg.temporalFrames)));
        int required = std::max(1, static_cast<int>(GetLong(g_cfg.temporalMin)));
        if (required > frames) {
            required = frames;
        }

        if (historySize != frames) {
            history.clear();
            historySize = frames;
        }
        history.push_back(frameHit ? 1 : 0);
        while (static_cast<int>(history.size()) > frames) {
            history.pop_front();
        }

        const int hits = static_cast<int>(std::count(history.begin(), history.end(), 1));
        const bool shouldFire = (static_cast<int>(history.size()) == frames && hits >= required);

        if (shouldFire && GetForegroundWindow() != nullptr) {
            const int lo = static_cast<int>(GetLong(g_cfg.minDelay));
            const int hi = static_cast<int>(GetLong(g_cfg.maxDelay));

            int delay = lo;
            if (lo < hi) {
                std::uniform_int_distribution<int> distribution(lo, hi);
                delay = distribution(rng);
            }
            SleepMilliseconds(delay);

            InjectKeyOrButton(static_cast<DWORD>(GetLong(g_cfg.shootVk)), rng);
            lastShot = Clock::now();
        }

        if (!frameHit) {
            SleepMilliseconds(1);
        }
    }

    capture.Shutdown();
    timeEndPeriod(1);
    return 0;
}
