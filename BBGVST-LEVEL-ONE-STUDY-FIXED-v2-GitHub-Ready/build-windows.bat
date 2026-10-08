@echo off
REM ============================================================================
REM BEGINNER BUILD SCRIPT
REM ============================================================================
REM Double-click this file from a Windows developer machine with CMake.
REM It creates a build folder, asks CMake to configure the project, then builds
REM the Release version.
REM
REM If you are learning: the actual build rules are in CMakeLists.txt. This
REM file is just a convenient button for the same commands.
REM ============================================================================

cmake -B build -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1

cmake --build build --config Release --parallel
if errorlevel 1 exit /b 1

echo.
echo BBGVST build finished.
pause
