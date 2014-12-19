@echo off

setlocal EnableDelayedExpansion
set "SRCLOC=%~dp0"

set "PROJECTS_ROOT=%SRCLOC%..\..\.."
set BUILD_TYPE=%1
set validBuildType=0
if "%BUILD_TYPE%"=="debug" (
	set validBuildType=1
)
if "%BUILD_TYPE%"=="release" (
	set validBuildType=1
)
if not "%validBuildType%"=="1" (
	echo None of build types (debug, release) were specified as first argument, going to build all

	%0 debug && %0 release
	exit /B %ERRORLEVEL%
)
echo Build type: %BUILD_TYPE%

call "%VS120COMNTOOLS%\VCVarsQueryRegistry.bat"
call "%VCINSTALLDIR%\vcvarsall.bat" x86

set "arch=armeabi"
call :buildArch
if %ERRORLEVEL% neq 0 (
	echo Arch failed %ERRORLEVEL%
	exit /B %ERRORLEVEL%
)

set "arch=armeabi-v7a"
call :buildArch
if %ERRORLEVEL% neq 0 (
	echo Arch failed %ERRORLEVEL%
	exit /B %ERRORLEVEL%
)

set "arch=x86"
call :buildArch
if %ERRORLEVEL% neq 0 (
	echo Arch failed %ERRORLEVEL%
	exit /B %ERRORLEVEL%
)

set "arch=mips"
call :buildArch
if %ERRORLEVEL% neq 0 (
	echo Arch failed %ERRORLEVEL%
	exit /B %ERRORLEVEL%
)

REM Quit from script
endlocal
exit /B
goto :EOF

REM ### Function start : buildArch
:buildArch
setlocal

echo Building for %arch%

set "BAKED_DIR=%PROJECTS_ROOT%\baked\%arch%-android-gcc-%BUILD_TYPE%.nmake"
if exist "%BAKED_DIR%" (
	echo Found baked in '%BAKED_DIR%'
)
if not exist "%BAKED_DIR%" (
	"%PROJECTS_ROOT%\build\%arch%-android-gcc.cmd" %BUILD_TYPE%
	if %ERRORLEVEL% neq 0 (
		echo Failed to bake %ERRORLEVEL%
		exit /B %ERRORLEVEL%
	)
)

pushd "%BAKED_DIR%" && (cmd /C "nmake OsmAndCoreWithJNI" & popd)

endlocal
goto :EOF
REM ### Function end : buildArch
