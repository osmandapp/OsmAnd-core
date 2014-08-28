@echo off

setlocal EnableDelayedExpansion
set SRCLOC=%~dp0

REM Cleanup environment
call "%SRCLOC%..\..\..\build\utils\functions.cmd" :cleanupEnvironment

REM Get and verify arguments
set targetOS=%1
set compiler=%2
set targetArch=%3
if not "%targetOS%"=="windows" (
	echo 'windows' is the only supported target, while '%targetOS%' was specified
	exit /B 1
)
if not "%compiler%"=="msvc" (
	echo 'msvc' is the only supported compiler, while '%compiler%' was specified
	exit /B 1
)
set validArch=0
if "%targetArch%"=="i686" (
	set validArch=1
)
if "%targetArch%"=="amd64" (
	set validArch=1
)
if not "%validArch%"=="1" (
	echo 'i686' and 'amd64' are the only supported target architectures, while '%targetArch%' was specified
	exit /B 1
)
echo Going to build embedded Qt for %targetOS%/%compiler%/%targetArch%

REM Prepare environment
set "PATH=%PATH%;%~dp0\tools.windows\bin"
set QTBASE_CONFIGURATION=^
	-xplatform win32-msvc2013 ^
	-debug-and-release -opensource -confirm-license -c++11 -no-gui -no-widgets -no-accessibility ^
	-qt-sql-sqlite -no-opengl -no-nis -no-iconv -no-inotify -no-eventfd -largefile -no-fontconfig ^
	-qt-zlib -qt-pcre -no-icu -no-gif -no-libpng -no-libjpeg -no-freetype -no-angle -no-openssl ^
	-no-dbus -no-audio-backend -no-qml-debug -no-directwrite -no-style-windows -no-style-windowsxp ^
	-no-style-windowsvista -no-style-fusion -no-style-windowsce -no-style-windowsmobile ^
	-nomake examples -nomake tools -no-vcproj -no-native-gestures

REM Initialize VisualC variables
call "%VS120COMNTOOLS%\VCVarsQueryRegistry.bat"

REM Build for x86
if "%targetArch%"=="i686" (
	setlocal
	call "%VCINSTALLDIR%\vcvarsall.bat" x86
	call :build
	if %ERRORLEVEL% neq 0 (
		echo Build failed with %ERRORLEVEL%
		exit /B %ERRORLEVEL%
	)
	endlocal
)

REM Build for amd64
if "%targetArch%"=="amd64" (
	setlocal
	call "%VCINSTALLDIR%\vcvarsall.bat" x86_amd64
	call :build
	if %ERRORLEVEL% neq 0 (
		echo Build failed with %ERRORLEVEL%
		exit /B %ERRORLEVEL%
	)
	endlocal
)
	
REM Quit from script
endlocal
exit /B

REM >>> 'build' function
:build
setlocal

REM Determine target architecture (x86 or x64) of environment
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
echo Building for msvc-%envArch%

REM Check if we have a build directory (shared)
set "SHARED_BUILD_PATH=%~dp0upstream.patched.windows.msvc-%envArch%.shared"
if not exist "%SHARED_BUILD_PATH%" (
	mkdir "%SHARED_BUILD_PATH%"
	xcopy "%~dp0upstream.patched" "%SHARED_BUILD_PATH%" /E /Q
	pushd %SHARED_BUILD_PATH% && (cmd /C "configure.bat -shared %QTBASE_CONFIGURATION% -prefix %SHARED_BUILD_PATH%" & popd)
	if %ERRORLEVEL% neq 0 (
		echo Configure failed with %ERRORLEVEL%
		exit /B %ERRORLEVEL%
	)
)

REM Perform build (shared)
pushd %SHARED_BUILD_PATH% && (cmd /C "nmake" & popd)
if %ERRORLEVEL% neq 0 (
	echo Build failed with %ERRORLEVEL%
	exit /B %ERRORLEVEL%
)

REM Check if we have a build directory (static)
set "STATIC_BUILD_PATH=%~dp0upstream.patched.windows.msvc-%envArch%.static"
if not exist "%STATIC_BUILD_PATH%" (
	mkdir "%STATIC_BUILD_PATH%"
	xcopy "%~dp0upstream.patched" "%STATIC_BUILD_PATH%" /E /Q
	pushd %STATIC_BUILD_PATH% && (cmd /C "configure.bat -static %QTBASE_CONFIGURATION% -prefix %STATIC_BUILD_PATH%" & popd)
	if %ERRORLEVEL% neq 0 (
		echo Configure failed with %ERRORLEVEL%
		exit /B %ERRORLEVEL%
	)
)

REM Perform build (static)
pushd %STATIC_BUILD_PATH% && (cmd /C "nmake" & popd)
if %ERRORLEVEL% neq 0 (
	echo Build failed with %ERRORLEVEL%
	exit /B %ERRORLEVEL%
)

endlocal
exit /B
REM <<< 'build' function
