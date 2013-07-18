@echo off

REM Prepare environment
set PATH=%PATH%;%~dp0\tools.windows\bin
set QTBASE_CONFIGURATION=^
	-xplatform win32-msvc2012 ^
	-debug-and-release -opensource -confirm-license -c++11 -static -no-gui -no-widgets -no-accessibility ^
	-qt-sql-sqlite -no-opengl -no-nis -no-iconv -no-inotify -no-eventfd -largefile -no-fontconfig ^
	-qt-zlib -qt-pcre -no-icu -no-gif -no-libpng -no-libjpeg -no-freetype -no-angle -no-openssl ^
	-no-dbus -no-audio-backend -no-qml-debug -no-directwrite -no-style-windows -no-style-windowsxp ^
	-no-style-windowsvista -no-style-fusion -no-style-windowsce -no-style-windowsmobile ^
	-nomake examples -nomake tools ^
	-no-native-gestures

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
if not exist "%~dp0upstream.patched.windows.%envArch%" (
	mkdir "%~dp0upstream.patched.windows.%envArch%"
	xcopy "%~dp0upstream.patched" "%~dp0upstream.patched.windows.%envArch%" /E
	(pushd %~dp0upstream.patched.windows.%envArch% && (cmd /C "configure.bat %QTBASE_CONFIGURATION%" & popd))
)

REM Perform build
(pushd %~dp0upstream.patched.windows.%envArch% && (cmd /C "nmake" & popd))
