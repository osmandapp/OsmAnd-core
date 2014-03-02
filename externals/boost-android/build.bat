@echo off

setlocal

if "%ANDROID_SDK%"=="" (echo ANDROID_SDK is not set)
if not exist %ANDROID_SDK% (echo ANDROID_SDK is set incorrectly)
if "%ANDROID_NDK%"=="" (echo ANDROID_NDK is not set)
if not exist %ANDROID_NDK% (echo ANDROID_NDK is set incorrectly)

REM Initialize VisualC variables (for bootstrapper)
call "%VS110COMNTOOLS%\VCVarsQueryRegistry.bat"
set PATH=%PATH%;%VCINSTALLDIR%\bin

REM Prepare environment
set ANDROID_SDK_ROOT=%ANDROID_SDK%
set ANDROID_NDK_ROOT=%ANDROID_NDK%
for /D %%d in ("%ANDROID_NDK%\toolchains\*-4.8") DO (set ANDROID_NDK_TOOLCHAIN_VERSION=4.8)
if "%ANDROID_NDK%"=="" (
	for /D %%d in ("%ANDROID_NDK%\toolchains\*-4.7") DO (set ANDROID_NDK_TOOLCHAIN_VERSION=4.7)
)
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
if exist "%PROGRAMFILES(X86)%" (
	for /D %%d in ("%ANDROID_NDK%\prebuilt\windows-x86_64") DO (set ANDROID_NDK_HOST=windows-x86_64)
) else (
	for /D %%d in ("%ANDROID_NDK%\prebuilt\windows-x86") DO (set ANDROID_NDK_HOST=windows-x86)
)
if "%ANDROID_NDK_HOST%"=="" (
	for /D %%d in ("%ANDROID_NDK%\prebuilt\windows-x86") DO (set ANDROID_NDK_HOST=windows-x86)
)
if "%ANDROID_NDK_HOST%"=="" (
	for /D %%d in ("%ANDROID_NDK%\prebuilt\windows") DO (set ANDROID_NDK_HOST=windows)
)

REM Build for ARMv5
setlocal
set buildArch=armeabi
set configName=armv5
set ANDROID_NDK_PLATFORM=android-8
call :build
endlocal

REM Build for ARMv7a
setlocal
set buildArch=armeabi-v7a
set configName=armv7a
set ANDROID_NDK_PLATFORM=android-8
call :build
endlocal

REM Build for ARMv7a-neon
setlocal
set buildArch=armeabi-v7a-neon
set configName=armv7a-neon
set ANDROID_NDK_PLATFORM=android-8
call :build
endlocal

REM Build for x86
setlocal
set buildArch=x86
set configName=x86
set ANDROID_NDK_PLATFORM=android-9
call :build
endlocal

REM Build for MIPS
setlocal
set buildArch=mips
set configName=mips
set ANDROID_NDK_PLATFORM=android-9
call :build
endlocal

REM Quit from script
endlocal
exit /B

REM >>> 'build' function
:build
setlocal

REM Check if we have a build directory
if not exist "%~dp0upstream.patched.%buildArch%.static" (
	mkdir "%~dp0upstream.patched.%buildArch%.static"
	xcopy "%~dp0upstream.patched" "%~dp0upstream.patched.%buildArch%.static" /E /Q
	xcopy "%~dp0%configName%.jam" "%~dp0upstream.patched.%buildArch%.static\tools\build\v2\user-config.jam" /Q /Y
	(pushd %~dp0upstream.patched.%buildArch%.static && (cmd /C "bootstrap.bat" & popd))
)

REM Perform build
(pushd %~dp0upstream.patched.%buildArch%.static && (cmd /C "b2 %BOOST_CONFIGURATION%" & popd))

endlocal
exit /B
REM <<< 'build' function
