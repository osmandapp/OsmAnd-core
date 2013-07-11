#include "OsmAndCore/Logging.h"

#include <stdio.h>
#include <stdarg.h>

#if defined(ANDROID) || defined(__ANDROID__)

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

void OsmAnd::LogFlush()
{
}

#elif defined(__APPLE__) || defined(__linux__)
void OsmAnd::LogFlush()
{
    fflush(stdout);
}

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


#elif defined(WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
void OsmAnd::LogFlush()
{
}
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

#endif


