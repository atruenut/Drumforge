@echo off
REM ============================================================================
REM  DrumForge - one-click Windows build  (produces DrumForge.vst3 = the DLL)
REM
REM  Needs ONE of:
REM    A) MinGW-w64 g++ on PATH   (MSYS2:  pacman -S mingw-w64-ucrt-x86_64-gcc)  <- used first
REM    B) Visual Studio 2019/2022 with "Desktop development with C++"
REM       (run this from "x64 Native Tools Command Prompt for VS")
REM
REM  Also needs the Steinberg VST3 SDK in third_party\vst3sdk:
REM    git clone --recurse-submodules https://github.com/steinbergmedia/vst3sdk.git third_party\vst3sdk
REM
REM  Usage:   build_windows.bat            build only
REM           build_windows.bat install    build + copy to C:\Program Files\Common Files\VST3 (run as Admin)
REM ============================================================================
setlocal enabledelayedexpansion
cd /d "%~dp0"

if not exist "third_party\vst3sdk\pluginterfaces" (
  echo [!] VST3 SDK not found in third_party\vst3sdk
  echo     git clone --recurse-submodules https://github.com/steinbergmedia/vst3sdk.git third_party\vst3sdk
  exit /b 1
)

set SDK=third_party\vst3sdk
set OUT=build\win
set BUNDLE=%OUT%\DrumForge.vst3\Contents\x86_64-win
if not exist "%BUNDLE%" mkdir "%BUNDLE%"

REM sources shared with CMake
set SRCS=
for /f "usebackq delims=" %%L in ("build\sources.txt") do set SRCS=!SRCS! %%L
set SRCS=%SRCS% %SDK%\public.sdk\source\main\dllmain.cpp %SDK%\public.sdk\source\common\threadchecker_win32.cpp src\plugin\editor_win.cpp

where g++ >nul 2>nul
if %errorlevel%==0 goto :mingw
where cl >nul 2>nul
if %errorlevel%==0 goto :msvc
echo [!] No compiler found. Install MSYS2 MinGW-w64 (g++) or run from the VS x64 Native Tools prompt.
exit /b 1

:mingw
echo [*] Building with MinGW g++ ...
g++ -std=c++17 -O2 -fno-math-errno -DRELEASE=1 -DUNICODE -D_UNICODE -DNOMINMAX -I%SDK% -Isrc -shared %SRCS% ^
    -static -static-libgcc -static-libstdc++ -luser32 -lgdi32 -lole32 -lshell32 -lcomdlg32 ^
    -o "%BUNDLE%\DrumForge.vst3"
if errorlevel 1 goto :fail
goto :done

:msvc
echo [*] Building with MSVC ...
set CPPS=
for %%F in (%SRCS%) do set CPPS=!CPPS! %%F
cl /nologo /std:c++17 /O2 /EHsc /MT /LD /wd4244 /wd4267 /DRELEASE=1 /DUNICODE /D_UNICODE /DNOMINMAX /I%SDK% /Isrc %CPPS% ^
   /Fo"%OUT%\\" /Fe"%BUNDLE%\DrumForge.vst3" /link user32.lib gdi32.lib ole32.lib shell32.lib comdlg32.lib
if errorlevel 1 goto :fail
goto :done

:done
echo.
echo [OK] Built %BUNDLE%\DrumForge.vst3
if /i "%1"=="install" (
  set DEST=%CommonProgramFiles%\VST3
  xcopy /e /i /y "%OUT%\DrumForge.vst3" "!DEST!\DrumForge.vst3\" >nul
  if errorlevel 1 ( echo [!] Install failed - run this prompt as Administrator ) else ( echo [OK] Installed to !DEST!\DrumForge.vst3 )
) else (
  echo     Copy the folder %OUT%\DrumForge.vst3 to  C:\Program Files\Common Files\VST3\
  echo     ^(or run: build_windows.bat install  as Administrator^), then rescan plugins in your DAW.
)
exit /b 0

:fail
echo [!] Build failed.
exit /b 1
