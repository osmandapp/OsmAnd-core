@echo off
set ORIGINAL_DIR=%CD%
rmdir /S /Q "%~dp0\%~n0.baked"
mkdir "%~dp0\%~n0.baked"
chdir "%~dp0\%~n0.baked"
cmake -G "Visual Studio 11" -DCMAKE_TARGET_OS:STRING=windows -DCMAKE_TARGET_CPU_ARCH:STRING=x86 %~dp0.cmake
chdir %ORIGINAL_DIR%