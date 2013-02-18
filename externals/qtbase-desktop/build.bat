@echo off

REM Prepare environment
set PATH=%PATH%;%~dp0\tools.windows\bin

REM Perform build
set ORIGINAL_DIR=%CD%
chdir %~dp0\upstream.patched.windows
configure.bat ^
	-opensource -confirm-license ^
	-nomake examples -nomake demos -nomake tests -nomake docs ^
	-qt-sql-sqlite ^
	-no-widgets -no-opengl -no-openvg -no-accessibility -no-nis ^
	-c++11 -shared -release
nmake
cd %ORIGINAL_DIR%