#include "Logging.h"

#include <stdio.h>
#include <stdarg.h>

#include <android/log.h>

void OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel level, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	int androidLevel;
	switch(level)
    {
    case LogSeverityLevel::Error:
        androidLevel = ANDROID_LOG_ERROR;
        break;
    case LogSeverityLevel::Warning:
        androidLevel = ANDROID_LOG_WARN;
        break;
    case LogSeverityLevel::Info:
        androidLevel = ANDROID_LOG_INFO;
        break;
    case LogSeverityLevel::Debug:
        androidLevel = ANDROID_LOG_DEBUG;
        break;
    }
    __android_log_vprint(androidLevel, "net.osmand:native", format, args);
	va_end(args);
}