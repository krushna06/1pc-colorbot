# 1pc-CB

`1pc-CB` is a standalone Win32 application that watches a small, centered
region of the desktop for a configured purple/magenta color range. When the
detection filter is satisfied, it sends the configured keyboard or mouse
input. It has no third-party runtime dependencies.

This project is for educational purposes only. Use it only where such
automation is permitted.

![1pc-CB menu](https://i.imgur.com/YI2HccU.png)

## Building

```bat
build.bat
```

Output: `1pc-CB.exe`. A Visual Studio C++ build tools installation is required.
The application uses only Win32, GDI, comctl32, winmm and the MSVC CRT.

## Settings

| Control | Default | Range | Effect |
|---|---:|---:|---|
| Master Enable | Off | on/off | Enables or disables the detection worker. |
| Hold to Activate | Off | on/off | Makes Activate follow the key instead of toggling. |
| Shoot | `O` (VK 79) | VK 1–255; mouse 1/2/4/5/6 | Input injected when firing. |
| Activate | Mouse5 (VK 5) | VK 1–255; mouse 1/2/4/5/6 | Key that toggles or holds Master Enable. |
| Shot delay | 400 ms | 1–1000 ms | Minimum time between fired inputs. |
| Human min | 30 ms | 1–500 ms | Lower bound of the pre-shot random delay. |
| Human max | 70 ms | 1–500 ms | Upper bound of the pre-shot random delay. |
| Tolerance | 25 | 0–50 | Lowers the required saturation and brightness. |
| Hue tolerance | 15 | 0–180 | Widens the accepted hue band. |
| Min pixels | 1 | 1–200 | Matching pixels required in one frame. |
| History frames | 1 | 1–20 | Number of frames remembered. |
| Required hits | 1 | 1–20 | Hits required in the remembered window. |
| Width (X) | 9 | 1–20 | Capture-region width in pixels. |
| Height (Y) | 9 | 1–20 | Capture-region height in pixels. |

Related pairs stay ordered automatically: `Human min <= Human max` and
`Required hits <= History frames`.

## How it works

1. A worker thread captures a centered patch of the desktop with `GetDC(NULL)`,
   `BitBlt` and `GetDIBits` into a 32-bit top-down DIB. The GDI resources are
   reused between frames.
2. Each pixel is converted to HSV and tested against:
   * Hue: `275 - Hue tolerance` to `320 + Hue tolerance`, with wraparound.
   * Saturation: `max(35 - Tolerance, 0)` to `100` percent.
   * Value: `max(85 - Tolerance, 0)` to `100` percent.
3. A frame counts as a hit when matching pixels are at least **Min pixels**.
4. A deque of **History frames** frame-results is kept; firing requires at
   least **Required hits** in a full history window and that the shot delay has
   elapsed.
5. Before firing, a uniform delay in `[Human min, Human max]` is applied, then
   the Shoot key/button is sent down, held for 15–30 ms, and released. Input is
   posted with `BroadcastSystemMessageW(0x92, ...)` with no target window.