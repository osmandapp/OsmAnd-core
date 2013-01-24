@echo off
set ORIGINAL_DIR=%CD%
rmdir /S /Q "%~dp0\%~n0.baked"
mkdir "%~dp0\%~n0.baked"
chdir "%~dp0\%~n0.baked"
"%WINDOWS_CMAKE%\bin\cmake.exe" -G "Visual Studio 11" -DCMAKE_TARGET_OS:STRING=windows -DCMAKE_TARGET_CPU_ARCH:STRING=amd64 %~dp0.cmake
chdir %ORIGINAL_DIR%