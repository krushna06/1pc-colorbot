@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 goto :failed

if not exist obj mkdir obj

cl /nologo /W3 /std:c++17 /EHsc /O2 /DUNICODE /D_UNICODE /utf-8 ^
   /Fo:obj\ /Fe:1pc-CB.exe src\*.cpp ^
   /link user32.lib gdi32.lib comctl32.lib winmm.lib /subsystem:windows
if errorlevel 1 goto :failed

del /q obj\*.obj >nul 2>&1
rmdir obj >nul 2>&1

echo.
echo Build succeeded: 1pc-CB.exe
exit /b 0

:failed
echo.
echo Build FAILED.
exit /b 1
