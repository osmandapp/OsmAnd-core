#ifndef _OSMAND_CORE_I_LOG_SINK_H_
#define _OSMAND_CORE_I_LOG_SINK_H_

#include <OsmAndCore/stdlib_common.h>
#include <cstdarg>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Logging.h>

namespace OsmAnd
{
    class OSMAND_CORE_API ILogSink
    {
        Q_DISABLE_COPY_AND_MOVE(ILogSink)
    private:
    protected:
        ILogSink();
    public:
        virtual ~ILogSink();

#if !defined(SWIG)
        virtual void log(const LogSeverityLevel level, const char* format, va_list args) = 0;
#endif // !defined(SWIG)
        virtual void flush() = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_LOG_SINK_H_)
