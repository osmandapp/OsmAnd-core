@echo off
set ORIGINAL_DIR=%CD%
rmdir /S /Q "%~dp0\..\baked\%~n0"
mkdir "%~dp0\..\baked\%~n0"
chdir "%~dp0\..\baked\%~n0"
"%WINDOWS_CMAKE%\bin\cmake.exe" -G "Visual Studio 11 Win64" -DCMAKE_TARGET_OS:STRING=windows -DCMAKE_TARGET_CPU_ARCH:STRING=amd64 %~dp0.cmake
chdir %ORIGINAL_DIR%