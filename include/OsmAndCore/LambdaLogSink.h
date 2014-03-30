#ifndef _OSMAND_CORE_LAMBDA_LOG_SINK_H_
#define _OSMAND_CORE_LAMBDA_LOG_SINK_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/ILogSink.h>

namespace OsmAnd
{
    class OSMAND_CORE_API LambdaLogSink : public ILogSink
    {
        Q_DISABLE_COPY(LambdaLogSink);

    public:
        typedef std::function<void(LambdaLogSink* const sink, const LogSeverityLevel level, const char* format, va_list args)> WriteHandlerSignature;
        typedef std::function<void(LambdaLogSink* const sink)> FlushHandlerSignature;

    private:
        const WriteHandlerSignature _writeHandler;
        const FlushHandlerSignature _flushHandler;
    protected:
    public:
        LambdaLogSink(const WriteHandlerSignature writeHandler, const FlushHandlerSignature flushHandler);
        virtual ~LambdaLogSink();

        virtual void log(const LogSeverityLevel level, const char* format, va_list args);
        virtual void flush();
    };
}

#endif // !defined(_OSMAND_CORE_LAMBDA_LOG_SINK_H_)
