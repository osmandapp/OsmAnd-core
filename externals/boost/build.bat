@echo off

setlocal EnableDelayedExpansion
set SRCLOC=%~dp0

REM Cleanup environment
call "%SRCLOC%..\..\..\build\utils\functions.cmd" :cleanupEnvironment

REM Get and verify arguments
set buildNeeded=0
set targetOS=%1
set compiler=%2
set targetArch=%3
set validTargetOS=0
if "%targetOS%"=="windows" (
	set validTargetOS=1
)
if "%targetOS%"=="linux" (
	set validTargetOS=1
)
if "%targetOS%"=="macosx" (
	set validTargetOS=1
)
if "%targetOS%"=="ios" (
	set validTargetOS=1
)
if "%targetOS%"=="android" (
	call "%SRCLOC%build-android.bat"
	exit /B
)
if not "%validTargetOS%"=="1" (
	echo 'windows', 'linux', 'macosx', 'ios', 'android' are the only supported targets, while '%targetOS%' was specified
	exit /B 1
)
if not "%buildNeeded%"=="1" (
	echo Building Boost for '%targetOS%' is not needed
	exit /B 0
)

REM Quit from script
endlocal
exit /B
