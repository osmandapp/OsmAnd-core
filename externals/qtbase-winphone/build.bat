@echo off

setlocal EnableDelayedExpansion
set SRCLOC=%~dp0

REM Cleanup environment
call "%SRCLOC%..\..\..\build\utils\functions.cmd" :cleanupEnvironment

REM Get and verify arguments
set targetOS=%1
set compiler=%2
set targetArch=%3
if not "%targetOS%"=="winphone" (
	echo 'winphone' is the only supported target, while '%targetOS%' was specified
	exit /B 1
)
set validCompiler=0
if "%compiler%"=="msvc" (
	set validCompiler=1
)
if not "%validCompiler%"=="1" (
	echo 'msvc' is the only supported compiler, while '%compiler%' was specified
	exit /B 1
)
set validArch=0
if "%targetArch%"=="i686" (
	set validArch=1
)
if "%targetArch%"=="arm" (
	set validArch=1
)
if not "%validArch%"=="1" (
	echo 'i686' and 'arm' are the only supported target architectures, while '%targetArch%' was specified
	exit /B 1
)
echo Going to build embedded Qt for %targetOS%/%compiler%/%targetArch%

if "%compiler%"=="msvc" (
	REM Prepare environment
	set "PATH=%PATH%;%~dp0\tools.windows\bin"
	set QTBASE_CONFIGURATION=^
		-debug-and-release -opensource -confirm-license -static -no-widgets -no-gui -no-accessibility  ^
		-qt-sql-sqlite -no-opengl -no-nis -no-iconv -no-inotify -largefile -no-fontconfig ^
		-qt-zlib -qt-pcre -no-icu -no-gif -qt-libpng -no-libjpeg -no-freetype -no-openssl ^
		-no-dbus -no-audio-backend -no-qml-debug -no-directwrite -no-style-windows -no-style-windowsxp ^
		-no-style-windowsvista -no-style-fusion -no-style-windowsce -no-style-windowsmobile ^
		-nomake examples -nomake tools -no-native-gestures
	set MAKE_CMD=nmake

	REM Build for x86
	if "%targetArch%"=="i686" (
		setlocal
		call :msvc_env x86
		call :build x86
		if %ERRORLEVEL% neq 0 (
			echo Build failed with %ERRORLEVEL%
			exit /B %ERRORLEVEL%
		)
		endlocal
	)

	REM Build for arm
	if "%targetArch%"=="arm" (
		setlocal
		call :msvc_env x86
		call :build arm
		if %ERRORLEVEL% neq 0 (
			echo Build failed with %ERRORLEVEL%
			exit /B %ERRORLEVEL%
		)
		endlocal
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

REM Check if we have a build directory (static)
set "STATIC_BUILD_PATH=%~dp0upstream.patched.winphone.%compiler%-%targetArch%.static"
if not exist "%STATIC_BUILD_PATH%" (
	mkdir "%STATIC_BUILD_PATH%"
	xcopy "%~dp0upstream.patched" "%STATIC_BUILD_PATH%" /E /Q
	pushd %STATIC_BUILD_PATH% && (cmd /C "configure.bat -static %QTBASE_CONFIGURATION% -xplatform winphone-%1-msvc2013 -prefix %STATIC_BUILD_PATH%" & popd)
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
