@echo off
REM --- Перехід до кореня проєкту ---
cd /d "%~dp0\.."

REM --- Пошук BIN файлу у build ---
set "SRC="
for %%f in (build\*.bin) do (
    set "SRC=%%f"
)

REM --- Перевірка і копіювання ---
if defined SRC (
    setlocal enabledelayedexpansion
    set "DST=%~n1"
    set "DST=!SRC:build\=!"
    set "DST=!DST:.ino.bin=.bin!"
    copy /Y "%SRC%" "!DST!"
    echo The BIN file has been copied as "!DST!"!
    endlocal
) else (
    echo BIN file not found!
)
