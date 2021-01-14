#ifndef _OSMAND_LOGGING_H_
#define _OSMAND_LOGGING_H_

#include "CommonCollections.h"

namespace OsmAnd {
enum class LogSeverityLevel { Error = 1, Warning, Debug, Info };

void LogPrintf(LogSeverityLevel level, const char* format, ...);
}  // namespace OsmAnd

#endif	// _OSMAND_LOGGING_H_
