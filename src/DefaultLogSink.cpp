#include "DefaultLogSink.h"

#include <cstdio>

OsmAnd::DefaultLogSink::DefaultLogSink()
{
}

OsmAnd::DefaultLogSink::~DefaultLogSink()
{
}

#if defined(ANDROID) || defined(__ANDROID__)

#include <android/log.h>

void OsmAnd::DefaultLogSink::log(const LogSeverityLevel level, const char* format, va_list args)
{
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
    __android_log_vprint(androidLevel, "net.osmand.core:native", format, args);
}

void OsmAnd::DefaultLogSink::flush()
{
}

#elif defined(__APPLE__) || defined(__linux__)

void OsmAnd::DefaultLogSink::log(const LogSeverityLevel level, const char* format, va_list args)
{
    if (level == LogSeverityLevel::Error)
        printf("ERROR: ");
    else if (level == LogSeverityLevel::Info)
        printf("INFO: ");
    else if (level == LogSeverityLevel::Warning)
        printf("WARN: ");
    else
        printf("DEBUG: ");
    vprintf(format, args);
    printf("\n");
}

void OsmAnd::DefaultLogSink::flush()
{
    fflush(stdout);
}

#elif defined(WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void OsmAnd::DefaultLogSink::log(const LogSeverityLevel level, const char* format, va_list args)
{
    if (IsDebuggerPresent())
    {
        if (level == LogSeverityLevel::Error)
            OutputDebugStringA("ERROR: ");
        else if (level == LogSeverityLevel::Info)
            OutputDebugStringA("INFO: ");
        else if (level == LogSeverityLevel::Warning)
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
        if (level == LogSeverityLevel::Error)
            printf("ERROR: ");
        else if (level == LogSeverityLevel::Info)
            printf("INFO: ");
        else if (level == LogSeverityLevel::Warning)
            printf("WARN: ");
        else
            printf("DEBUG: ");
        vprintf(format, args);
        printf("\n");
    }
}

void OsmAnd::DefaultLogSink::flush()
{
    if (!IsDebuggerPresent())
        fflush(stdout);
}

#endif
