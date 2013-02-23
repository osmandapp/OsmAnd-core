@echo off

REM Prepare environment
set PATH=%PATH%;%~dp0\tools.windows\bin

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

REM Perform build
(cd %~dp0upstream.patched.windows.%envArch% && ^
set QTBASE_CONFIGURATION=^
	-opensource -confirm-license ^
	-xplatform win32-msvc2012 ^
	-nomake examples -nomake demos -nomake tests -nomake docs ^
	-qt-sql-sqlite ^
	-no-widgets -no-opengl -no-openvg -no-accessibility -no-nis ^
	-c++11 -shared -debug-and-release)
(cd %~dp0upstream.patched.windows.%envArch% && cmd /C "configure.bat %QTBASE_CONFIGURATION%")
(cd %~dp0upstream.patched.windows.%envArch% && cmd /C "nmake")
