#ifndef _OSMAND_CORE_LOGGING_H_
#define _OSMAND_CORE_LOGGING_H_

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

#endif // _OSMAND_CORE_LOGGING_H_
