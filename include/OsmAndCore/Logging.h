#ifndef _OSMAND_CORE_LOGGING_H_
#define _OSMAND_CORE_LOGGING_H_

#include <OsmAndCore/QtExtensions.h>
#include <QIODevice>

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

    OSMAND_CORE_API void OSMAND_CORE_CALL SaveLogsTo(const std::shared_ptr<QIODevice>& outputDevice, const bool autoClose = false);
    OSMAND_CORE_API void OSMAND_CORE_CALL StopSavingLogs();
}

#endif // !defined(_OSMAND_CORE_LOGGING_H_)
