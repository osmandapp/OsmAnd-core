#include "QIODeviceLogSink.h"

#include "stdlib_common.h"
#include <cassert>

#include "QtExtensions.h"
#include <QFileDevice>
#include <QFile>

OsmAnd::QIODeviceLogSink::QIODeviceLogSink(const std::shared_ptr<QIODevice>& outputDevice, const bool autoClose /*= true*/)
    : _device(outputDevice)
    , _autoClose(autoClose)
{
    assert(_device->isOpen() && _device->isWritable());
}

OsmAnd::QIODeviceLogSink::~QIODeviceLogSink()
{
    if (_autoClose)
        _device->close();
}

void OsmAnd::QIODeviceLogSink::log(const LogSeverityLevel level, const char* format, va_list args)
{
    auto line = QString::vasprintf(format, args);
    switch (level)
    {
        case LogSeverityLevel::Verbose:
            line.prepend(QLatin1String("[V] "));
            break;
        case LogSeverityLevel::Debug:
            line.prepend(QLatin1String("[D] "));
            break;
        case LogSeverityLevel::Info:
            line.prepend(QLatin1String("[I] "));
            break;
        case LogSeverityLevel::Warning:
            line.prepend(QLatin1String("[W] "));
            break;
        case LogSeverityLevel::Error:
            line.prepend(QLatin1String("[E] "));
            break;
        default:
            line.prepend(QLatin1String("[?] "));
            break;
    }
    line.append(QLatin1String("\n"));

    _device->write(line.toUtf8());
}

void OsmAnd::QIODeviceLogSink::flush()
{
    if (const auto device = std::dynamic_pointer_cast<QFileDevice>(_device))
        device->flush();
}

std::shared_ptr<OsmAnd::QIODeviceLogSink> OsmAnd::QIODeviceLogSink::createFileLogSink(const QString& fileName)
{
    std::shared_ptr<QFile> outputFile(new QFile(fileName));
    if (!outputFile->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return nullptr;

    return std::shared_ptr<QIODeviceLogSink>(new QIODeviceLogSink(outputFile));
}
