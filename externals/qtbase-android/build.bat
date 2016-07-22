@echo off

setlocal EnableDelayedExpansion
set SRCLOC=%~dp0

REM Cleanup environment
call "%SRCLOC%..\..\..\build\utils\functions.cmd" :cleanupEnvironment

REM Get and verify arguments
set targetOS=%1
set compiler=%2
set targetArch=%3
if not "%targetOS%"=="android" (
	echo 'android' is the only supported target, while '%targetOS%' was specified
	exit /B 1
)
set validCompiler=0
if "%compiler%"=="gcc" (
	set validCompiler=1
)
if not "%validCompiler%"=="1" (
	echo 'gcc' is the only supported compilers, while '%compiler%' was specified
	exit /B 1
)
set validArch=0
if "%targetArch%"=="armeabi" (
	set validArch=1
)
if "%targetArch%"=="armeabi-v7a" (
	set validArch=1
)
if "%targetArch%"=="x86" (
	set validArch=1
)
if "%targetArch%"=="mips" (
	set validArch=1
)
if not "%validArch%"=="1" (
	echo 'armeabi', 'armeabi-v7a', 'x86', 'mips' are the only supported target architectures, while '%targetArch%' was specified
	exit /B 1
)
echo Going to build embedded Qt for %targetOS%/%compiler%/%targetArch%

REM ---EXPERIMENTAL---
set EXPERIMENTAL_USE_MSVC=0
REM ---EXPERIMENTAL---

REM Verify environment
for %%E in (perl.exe) do (
	set %%E_FULL_NAME=%%~$PATH:E
	for %%F in (!%%E_FULL_NAME!) do set %%E_PATH=%%~dpF
)
if "%perl.exe_FULL_NAME%" == "" (
	echo Perl not found in PATH.
	exit /B 1
)
echo Using perl from %perl.exe_PATH%

set MAKE_TOOL=""
set SHELL_EXECUTE_COMMAND=""
set CONFIGURE_SCRIPT=""
if "%EXPERIMENTAL_USE_MSVC%"=="0" (
	if not exist "%MINGW_ROOT%" (
		echo MINGW_ROOT '%MINGW_ROOT%' does not point to a valid directory. Please install MinGW and compilers
		exit /B 1
	)
	echo Using MinGW '%MINGW_ROOT%'
	set "PATH=%perl.exe_PATH%;%MINGW_ROOT%\bin;%MINGW_ROOT%\mingw32\bin;%MINGW_ROOT%\msys\1.0\bin;%PATH%"
	set MAKE_TOOL=mingw32-make
	set SHELL_EXECUTE_COMMAND=sh.exe -c
	set CONFIGURE_SCRIPT=./configure
)
if not "%EXPERIMENTAL_USE_MSVC%"=="0" (
	call "%VS120COMNTOOLS%\VCVarsQueryRegistry.bat"
	call "%VCINSTALLDIR%\vcvarsall.bat" x86
	set MAKE_TOOL=nmake
	set SHELL_EXECUTE_COMMAND=cmd /C
	set CONFIGURE_SCRIPT=configure.bat
)

if not exist "%ANDROID_SDK%" (
	echo ANDROID_SDK '%ANDROID_SDK%' does not point to a valid directory
	exit /B 1
)
set ANDROID_SDK_ROOT=%ANDROID_SDK%
echo Using ANDROID_SDK '%ANDROID_SDK%'

if not exist "%ANDROID_NDK%" (
	echo ANDROID_NDK '%ANDROID_NDK%' does not point to a valid directory
	exit /B 1
)
set ANDROID_NDK_ROOT=%ANDROID_NDK%
echo Using ANDROID_NDK '%ANDROID_NDK%'

set ANDROID_NDK_HOST=""
if exist "%ANDROID_NDK%\prebuilt\windows-x86_64" (
	set ANDROID_NDK_HOST=windows-x86_64
)
if exist "%ANDROID_NDK%\prebuilt\windows-x86" (
	set ANDROID_NDK_HOST=windows-x86
)
if exist "%ANDROID_NDK%\prebuilt\windows" (
	set ANDROID_NDK_HOST=windows
)
if "ANDROID_NDK_HOST"=="" (
	echo ANDROID_NDK '%ANDROID_NDK%' contains no valid host prebuilt tools
	exit /B 1
)
echo Using ANDROID_NDK_HOST '%ANDROID_NDK_HOST%'

set ANDROID_NDK_PLATFORM=android-9
if not exist "%ANDROID_NDK%\platforms\%ANDROID_NDK_PLATFORM%" (
	echo Platform '%ANDROID_NDK%\platforms\%ANDROID_NDK_PLATFORM%' does not exist
	exit /B 1
)
echo Using ANDROID_NDK_PLATFORM '%ANDROID_NDK_PLATFORM%'

set ANDROID_TARGET_ARCH=%targetArch%
set targetArchFamily=""
if "%targetArch%"=="armeabi" (
	set targetArchFamily=arm
)
if "%targetArch%"=="armeabi-v7a" (
	set targetArchFamily=arm
)
if "%targetArch%"=="x86" (
	set targetArchFamily=x86
)
if "%targetArch%"=="mips" (
	set targetArchFamily=mips
)
if not exist "%ANDROID_NDK%\platforms\%ANDROID_NDK_PLATFORM%\arch-%targetArchFamily%" (
	echo Architecture headers '%ANDROID_NDK%\platforms\%ANDROID_NDK_PLATFORM%\arch-%targetArchFamily%' does not exist
	exit /B 1
)
echo Using ANDROID_TARGET_ARCH '%ANDROID_TARGET_ARCH%'

set ANDROID_NDK_TOOLCHAIN_VERSION=0
if "%compiler%"=="gcc" (
	set ANDROID_NDK_TOOLCHAIN_VERSION=4.9
)
echo Using ANDROID_NDK_TOOLCHAIN_VERSION '%ANDROID_NDK_TOOLCHAIN_VERSION%'

set TOOLCHAIN_PATH=""
if "%targetArch%"=="armeabi" (
	set TOOLCHAIN_PATH=%ANDROID_NDK%\toolchains\arm-linux-androideabi-%ANDROID_NDK_TOOLCHAIN_VERSION%
)
if "%targetArch%"=="armeabi-v7a" (
	set TOOLCHAIN_PATH=%ANDROID_NDK%\toolchains\arm-linux-androideabi-%ANDROID_NDK_TOOLCHAIN_VERSION%
)
if "%targetArch%"=="x86" (
	set TOOLCHAIN_PATH=%ANDROID_NDK%\toolchains\x86-%ANDROID_NDK_TOOLCHAIN_VERSION%
)
if "%targetArch%"=="mips" (
	set TOOLCHAIN_PATH=%ANDROID_NDK%\toolchains\mipsel-linux-android-%ANDROID_NDK_TOOLCHAIN_VERSION%
)
if not exist "%TOOLCHAIN_PATH%" (
	echo Toolchain at '%TOOLCHAIN_PATH%' not found
	exit /B 1
)
echo Using toolchain '%TOOLCHAIN_PATH%'

REM Prepare configuration
set QTBASE_CONFIGURATION=^
	-release -opensource -confirm-license -c++std c++11 -no-accessibility -qt-sql-sqlite ^
	-no-qml-debug -qt-zlib -no-gif -no-libpng -no-libjpeg -no-openssl -qt-pcre ^
	-nomake examples -nomake tools -no-gui -no-widgets -no-nis -no-cups -no-iconv -no-icu -no-dbus ^
	-no-opengl -no-evdev
if "%compiler%"=="gcc" (
	set QTBASE_CONFIGURATION=^
		-xplatform android-g++ ^
		%QTBASE_CONFIGURATION%
)
if "%targetArch%"=="mips" (
	set QTBASE_CONFIGURATION=^
		%QTBASE_CONFIGURATION% ^
		-no-use-gold-linker
)
if "%EXPERIMENTAL_USE_MSVC%"=="0" (
	set QTBASE_CONFIGURATION=^
		-platform win32-g++ ^
		%QTBASE_CONFIGURATION%
)
if not "%EXPERIMENTAL_USE_MSVC%"=="0" (
	set QTBASE_CONFIGURATION=^
		%QTBASE_CONFIGURATION% ^
		-no-inotify -no-eventfd
)

REM Check if we have a build directory (shared)
set "SHARED_BUILD_PATH=%~dp0upstream.patched.android.%compiler%-%targetArch%.shared"
if not exist "%SHARED_BUILD_PATH%" (
	mkdir "%SHARED_BUILD_PATH%"
	xcopy "%~dp0upstream.patched" "%SHARED_BUILD_PATH%" /E /Q
	if "%EXPERIMENTAL_USE_MSVC%"=="0" (
		pushd %SHARED_BUILD_PATH% && (%SHELL_EXECUTE_COMMAND% "%CONFIGURE_SCRIPT% -shared %QTBASE_CONFIGURATION% -prefix %SHARED_BUILD_PATH:\=/%" & popd)
	)
	if not "%EXPERIMENTAL_USE_MSVC%"=="0" (
		pushd %SHARED_BUILD_PATH% && (%SHELL_EXECUTE_COMMAND% "%CONFIGURE_SCRIPT% -shared %QTBASE_CONFIGURATION% -prefix %SHARED_BUILD_PATH%" & popd)
	)
	if %ERRORLEVEL% neq 0 (
		echo Configure failed with %ERRORLEVEL%
		exit /B %ERRORLEVEL%
	)
)

REM Perform build (shared)
pushd %SHARED_BUILD_PATH% && (%SHELL_EXECUTE_COMMAND% "%MAKE_TOOL%" & popd)
if %ERRORLEVEL% neq 0 (
	echo Build failed with %ERRORLEVEL%
	exit /B %ERRORLEVEL%
)

REM Check if we have a build directory (static)
set "STATIC_BUILD_PATH=%~dp0upstream.patched.android.%compiler%-%targetArch%.static"
if not exist "%STATIC_BUILD_PATH%" (
	mkdir "%STATIC_BUILD_PATH%"
	xcopy "%~dp0upstream.patched" "%STATIC_BUILD_PATH%" /E /Q
	if "%EXPERIMENTAL_USE_MSVC%"=="0" (
		pushd %STATIC_BUILD_PATH% && (%SHELL_EXECUTE_COMMAND% "%CONFIGURE_SCRIPT% -static %QTBASE_CONFIGURATION% -prefix %STATIC_BUILD_PATH:\=/%" & popd)
	)
	if not "%EXPERIMENTAL_USE_MSVC%"=="0" (
		pushd %STATIC_BUILD_PATH% && (%SHELL_EXECUTE_COMMAND% "%CONFIGURE_SCRIPT% -static %QTBASE_CONFIGURATION% -prefix %STATIC_BUILD_PATH%" & popd)
	)
	if %ERRORLEVEL% neq 0 (
		echo Configure failed with %ERRORLEVEL%
		exit /B %ERRORLEVEL%
	)
)

REM Perform build (static)
pushd %STATIC_BUILD_PATH% && (%SHELL_EXECUTE_COMMAND% "%MAKE_TOOL%" & popd)
if %ERRORLEVEL% neq 0 (
	echo Build failed with %ERRORLEVEL%
	exit /B %ERRORLEVEL%
)

REM Quit from script
endlocal
exit /B
