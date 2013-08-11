@echo off

REM Prepare environment
set PATH=%PATH%;%~dp0\tools.windows\bin
set QTBASE_CONFIGURATION=^
	-debug-and-release -opensource -confirm-license -c++11 -static -no-widgets -no-accessibility ^
	-qt-sql-sqlite -no-opengl -no-nis -no-iconv -no-inotify -largefile -no-fontconfig ^
	-qt-zlib -qt-pcre -no-icu -no-gif -qt-libpng -no-libjpeg -no-freetype -no-angle -no-openssl ^
	-no-dbus -no-audio-backend -no-qml-debug -no-directwrite -no-style-windows -no-style-windowsxp ^
	-no-style-windowsvista -no-style-fusion -no-style-windowsce -no-style-windowsmobile ^
	-nomake examples -nomake tools -no-vcproj -no-native-gestures

REM Determine target architecture of environment
for /f "tokens=9 delims= " %%l in ('cl 2^>^&1') do (
	if "%%l"=="x86" (
		set envArch=i686
	)
	if "%%l"=="x64" (
		set envArch=amd64
	)
	if "%%l"=="ARM" (
		set envArch=arm
	)
	goto envDetected
)
:envDetected:
	
REM Build for WinPhone(arm and x86) and WinRT(arm, x86 and x64)
set target=winphone
set arch=i686
set xPlatform=winphone-x86-msvc2012
call :BUILD

set target=winphone
set arch=arm
set xPlatform=winphone-arm-msvc2012
call :BUILD

set target=winrt
set arch=i686
set xPlatform=winrt-x86-msvc2012
call :BUILD

set target=winrt
set arch=amd64
set xPlatform=winrt-x64-msvc2012
call :BUILD

set target=winrt
set arch=arm
set xPlatform=winrt-arm-msvc2012
call :BUILD

goto :EOF

REM ### Function start : BUILD
:BUILD

REM Check if we have a build directory
if not exist "%~dp0upstream.patched.%target%.%arch%.static" (
	mkdir "%~dp0upstream.patched.%target%.%arch%.static"
	xcopy "%~dp0upstream.patched" "%~dp0upstream.patched.%target%.%arch%.static" /E
	(pushd %~dp0upstream.patched.%target%.%arch%.static && (cmd /C "configure.bat %QTBASE_CONFIGURATION% -xplatform %xPlatform%" & popd))
)

REM Perform build
(pushd %~dp0upstream.patched.%target%.%arch%.static && (cmd /C "nmake" & popd))

goto :EOF
REM ### Function end : BUILD
