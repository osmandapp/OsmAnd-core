#include "Logging.h"

#include <stdio.h>
#include <stdarg.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel level, const char* format, ...)
{
    va_list args;
	va_start(args, format);
    int len = vsnprintf(nullptr, 0, format, args);
    char* buffer = new char[len + 1];
    vsnprintf(buffer, len, format, args);
    buffer[len] = 0;
    OutputDebugStringA(buffer);
	delete[] buffer;
	va_end(args);
}