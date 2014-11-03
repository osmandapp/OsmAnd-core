#include "DefaultLogSink.h"

#include <cstdio>

OsmAnd::DefaultLogSink::DefaultLogSink()
{
}

OsmAnd::DefaultLogSink::~DefaultLogSink()
{
}

#if defined(OSMAND_TARGET_OS_android)

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

#elif defined(OSMAND_TARGET_OS_windows)

#include <io.h>
#include <fcntl.h>

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
        //NOTE: This ugly stuff is needed just because Win32 has no _getmode or fcntl API.
        const auto currentMode = _setmode(_fileno(stdout), _O_TEXT);
        if (currentMode != -1)
            _setmode(_fileno(stdout), currentMode);

        if (currentMode == -1 || (currentMode & (_O_U16TEXT | _O_U8TEXT | _O_WTEXT)) == 0)
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
        else
        {
            if (level == LogSeverityLevel::Error)
                wprintf(L"ERROR: ");
            else if (level == LogSeverityLevel::Info)
                wprintf(L"INFO: ");
            else if (level == LogSeverityLevel::Warning)
                wprintf(L"WARN: ");
            else
                wprintf(L"DEBUG: ");

            int len = vsnprintf(nullptr, 0, format, args);
            char* buffer = new char[len + 1];
            vsnprintf(buffer, len, format, args);
            buffer[len] = 0;
            wprintf(L"%hs", buffer);

            wprintf(L"\n");
        }
    }
}

void OsmAnd::DefaultLogSink::flush()
{
    if (!IsDebuggerPresent())
        fflush(stdout);
}

#endif
