@echo off
cls
echo ========================================
echo   Vehicle ECU Simulator - Determinism Check
echo ========================================
echo.

echo [1/3] Building fresh project...
mingw32-make clean >NUL 2>&1
mingw32-make all
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo [2/3] Executing two identical test runs...
echo 1 | ecu_sim.exe > _run1_full.tmp 2>&1
echo 1 | ecu_sim.exe > _run2_full.tmp 2>&1

rem Filter out timing-dependent lines
findstr /V /C:"[PERF]" /C:"CPU Time" /C:"cycles" _run1_full.tmp > _run1_logic.tmp
findstr /V /C:"[PERF]" /C:"CPU Time" /C:"cycles" _run2_full.tmp > _run2_logic.tmp
del /Q _run1_full.tmp _run2_full.tmp >NUL 2>&1
echo         Runs complete.

echo.
echo [3/3] Comparing Logic Outputs...
fc _run1_logic.tmp _run2_logic.tmp >NUL 2>&1

rem Use explicit GOTO to prevent logic fall-through
if %ERRORLEVEL% NEQ 0 goto :FAIL

:PASS
echo.
echo ****************************************************
echo   PASS: Logic Determinism Confirmed
echo   (Vehicle behavior is identical in both runs)
echo ****************************************************
rem Logic logs kept for inspection
goto :DONE

:FAIL
echo.
echo ####################################################
echo   FAIL: Non-Deterministic Logic Detected
echo ####################################################
echo Logic differs between runs. Check _run1_logic.tmp and _run2_logic.tmp.

:DONE
echo.
pause
