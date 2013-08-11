@echo off

REM Prepare environment
set PATH=%PATH%;%~dp0\tools.windows\bin
set QTBASE_CONFIGURATION=^
	-xplatform win32-msvc2012 ^
	-debug-and-release -opensource -confirm-license -c++11 -no-gui -no-widgets -no-accessibility ^
	-qt-sql-sqlite -no-opengl -no-nis -no-iconv -no-inotify -no-eventfd -largefile -no-fontconfig ^
	-qt-zlib -qt-pcre -no-icu -no-gif -no-libpng -no-libjpeg -no-freetype -no-angle -no-openssl ^
	-no-dbus -no-audio-backend -no-qml-debug -no-directwrite -no-style-windows -no-style-windowsxp ^
	-no-style-windowsvista -no-style-fusion -no-style-windowsce -no-style-windowsmobile ^
	-nomake examples -nomake tools -no-vcproj -no-native-gestures

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

REM Check if we have a build directory (shared)
if not exist "%~dp0upstream.patched.windows.%envArch%.shared" (
	mkdir "%~dp0upstream.patched.windows.%envArch%.shared"
	xcopy "%~dp0upstream.patched" "%~dp0upstream.patched.windows.%envArch%.shared" /E
	(pushd %~dp0upstream.patched.windows.%envArch%.shared && (cmd /C "configure.bat -shared %QTBASE_CONFIGURATION%" & popd))
)

REM Perform build (shared)
(pushd %~dp0upstream.patched.windows.%envArch%.shared && (cmd /C "nmake" & popd))

REM Check if we have a build directory (static)
if not exist "%~dp0upstream.patched.windows.%envArch%.static" (
	mkdir "%~dp0upstream.patched.windows.%envArch%.static"
	xcopy "%~dp0upstream.patched" "%~dp0upstream.patched.windows.%envArch%.static" /E
	(pushd %~dp0upstream.patched.windows.%envArch%.static && (cmd /C "configure.bat -static %QTBASE_CONFIGURATION%" & popd))
)

REM Perform build (static)
(pushd %~dp0upstream.patched.windows.%envArch%.static && (cmd /C "nmake" & popd))
