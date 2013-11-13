#ifndef __LOGGING_H_
#define __LOGGING_H_

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    STRONG_ENUM(LogSeverityLevel)
    {
        Error = 1,
        Warning,
        Debug,
        Info
    };

    OSMAND_CORE_API void OSMAND_CORE_CALL LogPrintf(LogSeverityLevel level, const char* format, ...);
    OSMAND_CORE_API void OSMAND_CORE_CALL LogFlush();
}

#endif // __LOGGING_H_
