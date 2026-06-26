@echo off
cls
echo ========================================
echo   Vehicle ECU Simulator - Build and Run
echo ========================================
echo.

echo [1/2] Building project...
mingw32-make clean >NUL 2>&1
mingw32-make all
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] BUILD FAILED!
    pause
    exit /b 1
)

echo.
echo [2/2] Running automated test suite...
echo 1 | ecu_sim.exe
echo.

echo ========================================
echo   Simulation complete!
echo   Detailed logs saved to log.txt
echo   Module sizes saved to lib_sizes.txt
echo ========================================
pause
