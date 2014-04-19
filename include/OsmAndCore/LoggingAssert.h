#ifndef _OSMAND_CORE_LOGGING_ASSERT_H_
#define _OSMAND_CORE_LOGGING_ASSERT_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/Logging.h>

#if OSMAND_DEBUG
#   define OSMAND_ASSERT(condition, message)                                                                \
    do {                                                                                                    \
        if (! (condition))                                                                                  \
        {                                                                                                   \
            OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Assertion '" #condition "' failed in "      \
                ( Q_FUNC_INFO )"(" __FILE__ ":" QT_STRINGIFY(__LINE__) ": %s", qPrintable(message));        \
            assert((condition));                                                                            \
        }                                                                                                   \
    } while (false)
#else
#   define OSMAND_ASSERT(condition, message)
#endif

#endif // !defined(_OSMAND_CORE_LOGGING_ASSERT_H_)