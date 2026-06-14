@echo off
echo ===== Smart Desk OTA Server =====
echo.

REM ?? firmware.bin
copy /Y "%PROOT%\.pio\build\esp32-s3-devkitc-1\firmware.bin" "%~dp0firmware.bin" >nul
if %errorlevel% equ 0 (
    echo [OK] firmware.bin ???
) else (
    echo [FAIL] ?????pio run?
    pause
    exit /b
)

REM ???????
set /p VER=<"%~dp0version.txt"
echo [OK] ?????: %VER%

REM ?? HTTP ???
cd /d "%~dp0"
echo.
echo ===== ???: http://localhost:8080 =====
echo  ????? version.txt ?????
echo  Ctrl+C ??
echo.
python -m http.server 8080 --bind 0.0.0.0
if %errorlevel% neq 0 (
    echo [FAIL] Python ????
)
pause
