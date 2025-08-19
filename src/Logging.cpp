#include "Logging.h"

#include "QtExtensions.h"
#include "QtCommon.h"

#include "Common.h"
#include "ILogSink.h"

OsmAnd::Logger::Logger()
    : _severifyLevelThreshold(
#if OSMAND_DEBUG
        static_cast<int>(LogSeverityLevel::Debug))
#else
        static_cast<int>(LogSeverityLevel::Info))
#endif // OSMAND_DEBUG
{
}

OsmAnd::Logger::~Logger()
{
}

OsmAnd::LogSeverityLevel OsmAnd::Logger::getSeverityLevelThreshold() const
{
    return static_cast<LogSeverityLevel>(_severifyLevelThreshold.loadAcquire());
}

OsmAnd::LogSeverityLevel OsmAnd::Logger::setSeverityLevelThreshold(const LogSeverityLevel newThreshold)
{
    return static_cast<LogSeverityLevel>(_severifyLevelThreshold.fetchAndStoreOrdered(static_cast<int>(newThreshold)));
}

QSet< std::shared_ptr<OsmAnd::ILogSink> > OsmAnd::Logger::getCurrentLogSinks() const
{
    QReadLocker scopedLocker(&_sinksLock);

    return detachedOf(_sinks);
}

bool OsmAnd::Logger::addLogSink(const std::shared_ptr<ILogSink>& logSink)
{
    QWriteLocker scopedLocker(&_sinksLock);

    if (!logSink)
        return false;

    _sinks.insert(logSink);

    return true;
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
    if (static_cast<int>(level) < _severifyLevelThreshold.loadAcquire())
        return;

    QReadLocker scopedLocker1(&_sinksLock);
    QMutexLocker scopedLocker2(&_logMutex); // To avoid mixing of lines

    va_list args_;
    for(const auto& sink : constOf(_sinks))
    {
        va_copy(args_, args);
        sink->log(level, format, args_);
        va_end(args_);
    }
        
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
