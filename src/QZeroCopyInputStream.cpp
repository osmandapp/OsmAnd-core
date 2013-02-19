#include "QZeroCopyInputStream.h"

namespace gpb = google::protobuf;

OsmAnd::QZeroCopyInputStream::QZeroCopyInputStream( QIODevice* device )
    : _device(device)
    , _closeOnDestruction(!device->isOpen())
{
    if(!_device->isOpen())
        _device->open(QIODevice::ReadOnly);
}

OsmAnd::QZeroCopyInputStream::~QZeroCopyInputStream()
{
    if(_closeOnDestruction)
        _device->close();
}

bool OsmAnd::QZeroCopyInputStream::Next( const void** data, int* size )
{
    char* buffer = new char[BufferSize];

    qint64 bytesRead = _device->read(buffer, BufferSize);
    if (bytesRead < 0)
    {
        delete[] buffer;
        *size = 0;
        return false;
    }
    else
    {
        *data = buffer;
        *size = bytesRead;
        return true;
    }
}

void OsmAnd::QZeroCopyInputStream::BackUp( int count )
{
    if (count > _device->pos())
        _device->seek(0);
    else
        _device->seek(_device->pos() - count);
}

bool OsmAnd::QZeroCopyInputStream::Skip( int count )
{
    if (_device->pos() + count > _device->size())
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
