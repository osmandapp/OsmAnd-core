#ifndef _OSMAND_CORE_QIODEVICE_LOG_SINK_H_
#define _OSMAND_CORE_QIODEVICE_LOG_SINK_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QIODevice>

#include <OsmAndCore.h>
#include <OsmAndCore/ILogSink.h>

namespace OsmAnd
{
    class OSMAND_CORE_API QIODeviceLogSink : public ILogSink
    {
        Q_DISABLE_COPY_AND_MOVE(QIODeviceLogSink);
    private:
        const std::shared_ptr<QIODevice> _device;
        const bool _autoClose;
    protected:
    public:
        QIODeviceLogSink(const std::shared_ptr<QIODevice>& outputDevice, const bool autoClose = true);
        virtual ~QIODeviceLogSink();

        virtual void log(const LogSeverityLevel level, const char* format, va_list args);
        virtual void flush();
    };
}

#endif // !defined(_OSMAND_CORE_QIODEVICE_LOG_SINK_H_)
