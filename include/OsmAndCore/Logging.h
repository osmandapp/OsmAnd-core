#ifndef _OSMAND_CORE_LOGGING_H_
#define _OSMAND_CORE_LOGGING_H_

#include <OsmAndCore/stdlib_common.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <cstdarg>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QSet>
#include <QReadWriteLock>
#include <QMutex>
#include <QIODevice>
#include <QAtomicInt>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    enum class LogSeverityLevel
    {
        Verbose = 0,
        Debug,
        Info,
        Warning,
        Error
    };

    class ILogSink;

    class OSMAND_CORE_API Logger Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(Logger)
    private:
        Logger();
    protected:
        mutable QReadWriteLock _sinksLock;
        QSet< std::shared_ptr<ILogSink> > _sinks;
        mutable QMutex _logMutex;
        mutable QAtomicInt _severifyLevelThreshold;
    public:
        virtual ~Logger();

        LogSeverityLevel getSeverityLevelThreshold() const;
        LogSeverityLevel setSeverityLevelThreshold(const LogSeverityLevel newThreshold);

        QSet< std::shared_ptr<ILogSink> > getCurrentLogSinks() const;
        bool addLogSink(const std::shared_ptr<ILogSink>& logSink);
        void removeLogSink(const std::shared_ptr<ILogSink>& logSink);
        void removeAllLogSinks();

#if !defined(SWIG)
        void log(const LogSeverityLevel level, const char* format, va_list args);
        void log(const LogSeverityLevel level, const char* format, ...);
#endif // !defined(SWIG)
        void flush();

        static const std::shared_ptr<Logger>& get();
    };

    inline void LogPrintf(LogSeverityLevel level, const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        Logger::get()->log(level, format, args);
        va_end(args);
    }

    inline void LogFlush()
    {
        Logger::get()->flush();
    }
}

#endif // !defined(_OSMAND_CORE_LOGGING_H_)
