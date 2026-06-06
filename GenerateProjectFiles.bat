@echo off
setlocal

where python3 >nul 2>&1
if errorlevel 1 (
	echo python3 is not installed or not on PATH.
	exit /b 1
)

set "SCRIPT_PATH=%~dp0Scripts\regen.py"
if not exist "%SCRIPT_PATH%" (
	echo Script file not found: %SCRIPT_PATH%
	exit /b 1
)

python3 "%SCRIPT_PATH%" --gen
exit /b %errorlevel%