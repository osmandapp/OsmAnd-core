#include "Logging.h"

#include "Common.h"
#include "ILogSink.h"

OsmAnd::Logger::Logger()
{
}

OsmAnd::Logger::~Logger()
{
}

QSet< std::shared_ptr<OsmAnd::ILogSink> > OsmAnd::Logger::getCurrentLogSinks() const
{
    QReadLocker scopedLocker(&_sinksLock);

    return detachedOf(_sinks);
}

void OsmAnd::Logger::addLogSink(const std::shared_ptr<ILogSink>& logSink)
{
    QWriteLocker scopedLocker(&_sinksLock);

    _sinks.insert(logSink);
}

void OsmAnd::Logger::removeLogSink(const std::shared_ptr<ILogSink>& logSink)
{
    QWriteLocker scopedLocker(&_sinksLock);

    _sinks.remove(logSink);
}

void OsmAnd::Logger::removeAllLogSinks()
{
    QWriteLocker scopedLocker(&_sinksLock);

    _sinks.clear();
}

void OsmAnd::Logger::log(const LogSeverityLevel level, const char* format, va_list args)
{
    QReadLocker scopedLocker1(&_sinksLock);
    QMutexLocker scopedLocker2(&_logMutex); // To avoid mixing of lines

    for(const auto& sink : constOf(_sinks))
        sink->log(level, format, args);
}

void OsmAnd::Logger::log(const LogSeverityLevel level, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    log(level, format, args);
    va_end(args);
}

void OsmAnd::Logger::flush()
{
    QReadLocker scopedLocker(&_sinksLock);

    for(const auto& sink : constOf(_sinks))
        sink->flush();
}

const std::shared_ptr<OsmAnd::Logger>& OsmAnd::Logger::get()
{
    static const std::shared_ptr<Logger> instance(new Logger());
    return instance;
}
