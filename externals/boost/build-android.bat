@echo off

setlocal EnableDelayedExpansion
set SRCLOC=%~dp0

echo Going to build Boost for %targetOS%/%compiler%/%targetArch%

set validTargetOS=1
set buildNeeded=1

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

REM Prepare environment
call "%VS120COMNTOOLS%\VCVarsQueryRegistry.bat"
call "%VCINSTALLDIR%\vcvarsall.bat" x86
set BOOST_CONFIGURATION=^
	--layout=versioned ^
	--with-thread ^
	toolset=gcc-android ^
	target-os=linux ^
	threading=multi ^
	link=static ^
	runtime-link=shared ^
	variant=release ^
	threadapi=pthread ^
	stage

REM Check if we have a build directory (static)
set "STATIC_BUILD_PATH=%~dp0upstream.patched.%targetOS%.%compiler%-%targetArch%.static"
if not exist "%STATIC_BUILD_PATH%" (
	mkdir "%STATIC_BUILD_PATH%"
	xcopy "%~dp0upstream.patched" "%STATIC_BUILD_PATH%" /E /Q
	
	pushd %STATIC_BUILD_PATH% && (cmd /C "bootstrap.bat" & popd)
	if %ERRORLEVEL% neq 0 (
		echo Configure failed with %ERRORLEVEL%
		exit /B %ERRORLEVEL%
	)
	
	echo Using '%targetOS%.%compiler%-%targetArch%.jam'
	copy "%~dp0targets\%targetOS%.%compiler%-%targetArch%.jam" "%STATIC_BUILD_PATH%\project-config.jam" /Y
)

REM Perform build (static)
pushd %STATIC_BUILD_PATH% && (cmd /C "b2 %BOOST_CONFIGURATION% -j %NUMBER_OF_PROCESSORS%" & popd)
if %ERRORLEVEL% neq 0 (
	echo Build failed with %ERRORLEVEL%
	exit /B %ERRORLEVEL%
)

REM Quit from script
endlocal
exit /B
