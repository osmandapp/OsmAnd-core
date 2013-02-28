@echo off

REM Prepare environment
set PATH=%PATH%;%~dp0\tools.windows\bin
set QTBASE_CONFIGURATION=^
	-opensource -confirm-license ^
	-xplatform win32-msvc2012 ^
	-nomake examples -nomake demos -nomake tests -nomake docs ^
	-qt-sql-sqlite -opengl desktop ^
	-no-style-windowsvista ^
	-no-accessibility -no-nis ^
	-c++11 -shared -debug-and-release

REM Determine target processor (x86 or x64) of environment
for /f "tokens=9 delims= " %%l in ('cl 2^>^&1') do (
	if "%%l"=="x86" (
		set envArch=i686
	)
	if "%%l"=="x64" (
		set envArch=amd64
	)
	goto envDetected
)
:envDetected:

REM Check if we have a build directory
REM WORKAROUND: -no-style-windowsvista is needed when -no-accessibility is selected
if not exist "%~dp0upstream.patched.windows.%envArch%" (
	mkdir "%~dp0upstream.patched.windows.%envArch%"
	xcopy "%~dp0upstream.patched" "%~dp0upstream.patched.windows.%envArch%" /E
	(cd %~dp0upstream.patched.windows.%envArch% && cmd /C "configure.bat %QTBASE_CONFIGURATION%")
)

REM Perform build
(cd %~dp0upstream.patched.windows.%envArch% && cmd /C "nmake")
