@echo off

set SRCLOC=%CD%
set PROTOC_EXECUTABLE=""
if defined PROTOC (
	set PROTOC_EXECUTABLE=%PROTOC%
) else (
	if [%1] == [] (
		set PROTOC_EXECUTABLE=protoc.exe
	) else (
		set PROTOC_EXECUTABLE=%1
	)
)
echo Using %PROTOC_EXECUTABLE%

%PROTOC_EXECUTABLE% --cpp_out="%SRCLOC%" --proto_path="%SRCLOC%\..\..\resources\protos" "%SRCLOC%\..\..\resources\protos\OBF.proto"
