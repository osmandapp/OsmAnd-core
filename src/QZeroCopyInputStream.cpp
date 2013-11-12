#include "QZeroCopyInputStream.h"

namespace gpb = google::protobuf;

OsmAnd::QZeroCopyInputStream::QZeroCopyInputStream( const std::shared_ptr<QIODevice>& device )
    : _device(device)
    , _buffer(new uint8_t[BufferSize])
    , _closeOnDestruction(!device->isOpen())
{
    if(!_device->isOpen())
        _device->open(QIODevice::ReadOnly);
    assert(_device->isOpen());
}

OsmAnd::QZeroCopyInputStream::~QZeroCopyInputStream()
{
    if(_closeOnDestruction)
        _device->close();

    delete[] _buffer;
}

bool OsmAnd::QZeroCopyInputStream::Next( const void** data, int* size )
{
    qint64 bytesRead = _device->read(reinterpret_cast<char*>(_buffer), BufferSize);
    if (bytesRead < 0 || (bytesRead == 0 && _device->atEnd()))
    {
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

void OsmAnd::QZeroCopyInputStream::BackUp( int count )
{
    if(!_device->isOpen() && !_closeOnDestruction)
        return;

    if (count > _device->pos())
        _device->seek(0);
    else
        _device->seek(_device->pos() - count);
}

bool OsmAnd::QZeroCopyInputStream::Skip( int count )
{
    if(!_device->isOpen() && !_closeOnDestruction)
        return false;

    if (_device->pos() + count > _device->size() || _device->atEnd())
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

gpb::int64 OsmAnd::QZeroCopyInputStream::ByteCount() const
{
    return _device->pos();
}
