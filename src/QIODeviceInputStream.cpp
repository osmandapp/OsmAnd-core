#include "QIODeviceInputStream.h"

namespace gpb = google::protobuf;

OsmAnd::QIODeviceInputStream::QIODeviceInputStream( const std::shared_ptr<QIODevice>& device, const size_t bufferSize /*= DefaultBufferSize*/ )
    : _device(device)
    , _deviceSize(_device->size())
    , _buffer(new uint8_t[bufferSize])
    , _bufferSize(bufferSize)
    , _closeOnDestruction(!device->isOpen())
{
    // Open the device if it was not yet opened
    if (!_device->isOpen())
        _device->open(QIODevice::ReadOnly);
    assert(_device->isOpen());
}

OsmAnd::QIODeviceInputStream::~QIODeviceInputStream()
{
    // If device was opened by this instance, close it also
    if (_closeOnDestruction && _device->isOpen())
        _device->close();

    // Delete buffer
    delete[] _buffer;
}

bool OsmAnd::QIODeviceInputStream::Next( const void** data, int* size )
{
    const auto bytesRead = _device->read(reinterpret_cast<char*>(_buffer), _bufferSize);
    if (Q_UNLIKELY(bytesRead < 0 || (bytesRead == 0 && _device->atEnd())))
    {
        *data = nullptr;
        *size = 0;
        return false;
    }
    else
    {
        *data = _buffer;
        *size = bytesRead;
        return true;
    }
}

void OsmAnd::QIODeviceInputStream::BackUp( int count )
{
    if (Q_UNLIKELY(!_device->isOpen() && !_closeOnDestruction))
        return;

    if (count > _device->pos())
        _device->seek(0);
    else
        _device->seek(_device->pos() - count);
}

bool OsmAnd::QIODeviceInputStream::Skip( int count )
{
    if (Q_UNLIKELY(!_device->isOpen() && !_closeOnDestruction))
        return false;

    if (Q_UNLIKELY(_device->pos() + count >= _deviceSize))
    {
        _device->seek(_device->size());
        return false;
    }
    else
    {
        _device->seek(_device->pos() + count);
        return true;
    }
}

gpb::int64 OsmAnd::QIODeviceInputStream::ByteCount() const
{
    return _device->pos();
}
