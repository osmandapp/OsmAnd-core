#include "Logging.h"

#include <stdio.h>
#include <stdarg.h>

extern void OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel level, const char* msg, ...)
{
	va_list args;
	va_start( args, msg);
	if(level == LogSeverityLevel::Error) {
		printf("ERROR: ");
	} else if(level == LogSeverityLevel::Info) {
		printf("INFO: ");
	} else if(level == LogSeverityLevel::Warning) {
		printf("WARN: ");
	} else {
		printf("DEBUG: ");
	}
	vprintf(msg, args);
	printf("\n");
	va_end(args);
}
