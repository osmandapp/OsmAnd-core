#include "Logging.h"

#include <stdio.h>
#include <stdarg.h>

void OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel level, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	//TODO: Do something here
	va_end(args);
}