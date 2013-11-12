#include "Logging.h"

#include <stdio.h>
#include <stdarg.h>

#if defined(ANDROID) || defined(__ANDROID__)

#include <android/log.h>

OSMAND_CORE_API void OSMAND_CORE_CALL OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel level, const char* format, ...)
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

OSMAND_CORE_API void OSMAND_CORE_CALL OsmAnd::LogFlush()
{
}

#elif defined(__APPLE__) || defined(__linux__)

OSMAND_CORE_API void OSMAND_CORE_CALL OsmAnd::LogFlush()
{
    fflush(stdout);
}

OSMAND_CORE_API void OSMAND_CORE_CALL OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel level, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    if(level == LogSeverityLevel::Error)
        printf("ERROR: ");
    else if(level == LogSeverityLevel::Info)
        printf("INFO: ");
    else if(level == LogSeverityLevel::Warning)
        printf("WARN: ");
    else
        printf("DEBUG: ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}


#elif defined(WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

OSMAND_CORE_API void OSMAND_CORE_CALL OsmAnd::LogFlush()
{
    if(!IsDebuggerPresent())
        fflush(stdout);
}

OSMAND_CORE_API void OSMAND_CORE_CALL OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel level, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    
    if(IsDebuggerPresent())
    {
        if(level == LogSeverityLevel::Error)
            OutputDebugStringA("ERROR: ");
        else if(level == LogSeverityLevel::Info)
            OutputDebugStringA("INFO: ");
        else if(level == LogSeverityLevel::Warning)
            OutputDebugStringA("WARN: ");
        else
            OutputDebugStringA("DEBUG: ");

        int len = vsnprintf(nullptr, 0, format, args);
        char* buffer = new char[len + 1];
        vsnprintf(buffer, len, format, args);
        buffer[len] = 0;
        OutputDebugStringA(buffer);
        delete[] buffer;

        OutputDebugStringA("\n");
    }
    else
    {
        if(level == LogSeverityLevel::Error)
            printf("ERROR: ");
        else if(level == LogSeverityLevel::Info)
            printf("INFO: ");
        else if(level == LogSeverityLevel::Warning)
            printf("WARN: ");
        else
            printf("DEBUG: ");
        vprintf(format, args);
        printf("\n");
    }
    
    va_end(args);
}

#endif


