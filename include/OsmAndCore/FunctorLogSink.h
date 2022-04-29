#ifndef _OSMAND_CORE_FUNCTOR_LOG_SINK_H_
#define _OSMAND_CORE_FUNCTOR_LOG_SINK_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/ILogSink.h>

namespace OsmAnd
{
    class OSMAND_CORE_API FunctorLogSink : public ILogSink
    {
        Q_DISABLE_COPY_AND_MOVE(FunctorLogSink);

    public:
        OSMAND_CALLABLE(WriteCallback, void, FunctorLogSink* sink, LogSeverityLevel level, const char* format, va_list args);
        OSMAND_CALLABLE(FlushCallback, void, FunctorLogSink* sink);

    private:
        const WriteCallback _writeFunctor;
        const FlushCallback _flushFunctor;
    protected:
    public:
        FunctorLogSink(const WriteCallback writeFunctor, const FlushCallback flushFunctor);
        virtual ~FunctorLogSink();

#if !defined(SWIG)
        virtual void log(const LogSeverityLevel level, const char* format, va_list args);
#endif // !defined(SWIG)
        virtual void flush();
    };
}

#endif // !defined(_OSMAND_CORE_FUNCTOR_LOG_SINK_H_)
