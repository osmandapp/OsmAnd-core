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
set validCompiler=0
if "%compiler%"=="msvc" (
	set validCompiler=1
)
if "%compiler%"=="gcc" (
	set validCompiler=1
)
if not "%validCompiler%"=="1" (
	echo 'msvc' and 'gcc' are the only supported compilers, while '%compiler%' was specified
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

for %%C in (g++.exe) do set %%C=%%~$PATH:C

if "%compiler%"=="msvc" (
	REM Prepare environment
	set "PATH=%PATH%;%~dp0\tools.windows\bin"
	set QTBASE_CONFIGURATION=^
		-xplatform win32-msvc2013 ^
		-debug-and-release -opensource -confirm-license -c++std c++11 -no-gui -no-widgets -no-accessibility ^
		-no-sql-sqlite -no-opengl -no-iconv -no-inotify -no-eventfd -largefile -no-fontconfig ^
		-qt-zlib -qt-pcre -no-icu -no-gif -no-libpng -no-libjpeg -no-freetype -no-angle -no-openssl ^
		-no-dbus -no-audio-backend -no-qml-debug -no-directwrite -no-style-windows -no-style-windowsxp ^
		-no-style-windowsvista -no-style-fusion -no-style-windowsce -no-style-windowsmobile ^
		-nomake examples -nomake tools -no-native-gestures
	set MAKE_CMD=nmake

	REM Build for x86
	if "%targetArch%"=="i686" (
		setlocal
		call :msvc_env x86
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
		call :msvc_env x86_amd64
		call :build
		if %ERRORLEVEL% neq 0 (
			echo Build failed with %ERRORLEVEL%
			exit /B %ERRORLEVEL%
		)
		endlocal
	)
)
if "%compiler%"=="gcc" (
	if "%g++.exe%" == "" (
		if "%MINGW_ROOT%"=="" (
			echo g++ was not found in PATH and MINGW_ROOT is not set. No compiler found
			exit /b 1
		)
		
		REM Use from MINGW_ROOT
		echo g++ was not found in PATH, will use from MINGW_ROOT "%MINGW_ROOT%"
		set "PATH=%PATH%;%MINGW_ROOT%\bin"
	)
	set MAKE_CMD=mingw32-make

	REM Build for x86
	if "%targetArch%"=="i686" (
		set QTBASE_CONFIGURATION=^
			-xplatform win32-g++-32 ^
			-debug-and-release -opensource -confirm-license -c++std c++11 -no-gui -no-widgets -no-accessibility ^
			-qt-sql-sqlite -no-opengl -no-iconv -no-inotify -no-eventfd -largefile -no-fontconfig ^
			-qt-zlib -qt-pcre -no-icu -no-gif -no-libpng -no-libjpeg -no-freetype -no-angle -no-openssl ^
			-no-dbus -no-audio-backend -no-qml-debug -no-directwrite -no-style-windows -no-style-windowsxp ^
			-no-style-windowsvista -no-style-fusion -no-style-windowsce -no-style-windowsmobile ^
			-nomake examples -nomake tools -no-native-gestures

		call :build
		if %ERRORLEVEL% neq 0 (
			echo Build failed with %ERRORLEVEL%
			exit /B %ERRORLEVEL%
		)
	)

	REM Build for amd64
	if "%targetArch%"=="amd64" (
		set QTBASE_CONFIGURATION=^
			-xplatform win32-g++-64 ^
			-debug-and-release -opensource -confirm-license -c++std c++11 -no-gui -no-widgets -no-accessibility ^
			-qt-sql-sqlite -no-opengl -no-iconv -no-inotify -no-eventfd -largefile -no-fontconfig ^
			-qt-zlib -qt-pcre -no-icu -no-gif -no-libpng -no-libjpeg -no-freetype -no-angle -no-openssl ^
			-no-dbus -no-audio-backend -no-qml-debug -no-directwrite -no-style-windows -no-style-windowsxp ^
			-no-style-windowsvista -no-style-fusion -no-style-windowsce -no-style-windowsmobile ^
			-nomake examples -nomake tools -no-vcproj -no-native-gestures
	
		call :build
		if %ERRORLEVEL% neq 0 (
			echo Build failed with %ERRORLEVEL%
			exit /B %ERRORLEVEL%
		)
	)
)
	
REM Quit from script
endlocal
exit /B

REM >>> 'msvc_env' function
:msvc_env

call "%VS120COMNTOOLS%\VCVarsQueryRegistry.bat"
call "%VCINSTALLDIR%\vcvarsall.bat" %1

exit /B
REM <<< 'msvc_env' function

REM >>> 'build' function
:build
setlocal

REM Check if we have a build directory (shared)
set "SHARED_BUILD_PATH=%~dp0upstream.patched.windows.%compiler%-%targetArch%.shared"
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
pushd %SHARED_BUILD_PATH% && (cmd /C "%MAKE_CMD%" & popd)
if %ERRORLEVEL% neq 0 (
	echo Build failed with %ERRORLEVEL%
	exit /B %ERRORLEVEL%
)

REM Check if we have a build directory (static)
set "STATIC_BUILD_PATH=%~dp0upstream.patched.windows.%compiler%-%targetArch%.static"
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
pushd %STATIC_BUILD_PATH% && (cmd /C "%MAKE_CMD%" & popd)
if %ERRORLEVEL% neq 0 (
	echo Build failed with %ERRORLEVEL%
	exit /B %ERRORLEVEL%
)

endlocal
exit /B
REM <<< 'build' function
