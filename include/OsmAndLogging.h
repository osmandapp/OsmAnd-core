#ifndef _OSMAND_LOGGING_H_
#define _OSMAND_LOGGING_H_

#include <OsmAndCore.h>

namespace OsmAnd
{
    enum class LogSeverityLevel
    {
        Error = 1,
        Warning,
        Debug,
        Info
    };

    OSMAND_CORE_API void OSMAND_CORE_CALL LogPrintf(LogSeverityLevel level, const char* format, ...);
	OSMAND_CORE_API void OSMAND_CORE_CALL LogFlush();
}

#endif // _OSMAND_LOGGING_H_
