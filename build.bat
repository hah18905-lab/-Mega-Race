@echo off
rem Build with the official raylib installer (C:\raylib, includes w64devkit)
set RL=C:\raylib
set PATH=%RL%\w64devkit\bin;%PATH%
g++ main.cpp -o MegaRace.exe -O2 -std=c++17 -I %RL%\raylib\src -L %RL%\raylib\src -lraylib -lopengl32 -lgdi32 -lwinmm
if errorlevel 1 (
  echo.
  echo BUILD FAILED. Check that raylib is installed in C:\raylib
  pause
  exit /b 1
)
echo Build OK. Starting the game...
start MegaRace.exe
