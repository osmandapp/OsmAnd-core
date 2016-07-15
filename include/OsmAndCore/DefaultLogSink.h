#ifndef _OSMAND_CORE_DEFAULT_LOG_SINK_H_
#define _OSMAND_CORE_DEFAULT_LOG_SINK_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/ILogSink.h>

namespace OsmAnd
{
    class OSMAND_CORE_API DefaultLogSink : public ILogSink
    {
        Q_DISABLE_COPY_AND_MOVE(DefaultLogSink)
    private:
    protected:
    public:
        DefaultLogSink();
        virtual ~DefaultLogSink();

#if !defined(SWIG)
        virtual void log(const LogSeverityLevel level, const char* format, va_list args);
#endif // !defined(SWIG)
        virtual void flush();
    };
}

#endif // !defined(_OSMAND_CORE_DEFAULT_LOG_SINK_H_)
