#include "QFileDeviceInputStream.h"

namespace gpb = google::protobuf;

OsmAnd::QFileDeviceInputStream::QFileDeviceInputStream(const std::shared_ptr<QFileDevice>& file, const size_t memoryWindowSize /*= DefaultMemoryWindowSize*/)
    : _file(file)
    , _fileSize(_file->size())
    , _mappedMemory(nullptr)
    , _memoryWindowSize(memoryWindowSize)
    , _currentPosition(0)
    , _wasInitiallyOpened(_file->isOpen())
    , _originalOpenMode(_file->openMode())
    , _closeOnDestruction(false)
{
}

OsmAnd::QFileDeviceInputStream::~QFileDeviceInputStream()
{
    // Unmap memory if it's still mapped
    if (_mappedMemory)
    {
        bool ok;
        ok = _file->unmap(_mappedMemory);
        assert(ok);

        _mappedMemory = nullptr;
    }

    // If file device was initially opened, but is closed right now, reopen it
    if (_wasInitiallyOpened && !_file->isOpen())
        _file->open(_originalOpenMode);

    // If file was opened during work, close it
    if (_closeOnDestruction && _file->isOpen())
        _file->close();
}

bool OsmAnd::QFileDeviceInputStream::Next(const void** data, int* size)
{
    // If memory was already mapped, unmap it
    if (Q_LIKELY(_mappedMemory != nullptr))
    {
        _file->unmap(_mappedMemory);
        _mappedMemory = nullptr;
    }

    // Check if current position is in valid range
    if (Q_UNLIKELY(_currentPosition < 0 || _currentPosition >= _fileSize))
    {
        *data = nullptr;
        *size = 0;
        return false;
    }

    // If file is not opened, open it
    if (!_file->isOpen())
    {
        if (_wasInitiallyOpened)
            _file->open(_originalOpenMode);
        else
        {
            _file->open(QIODevice::ReadOnly);
            _closeOnDestruction = true;
        }
    }

    // Map new portion of data
    auto mappedSize = _memoryWindowSize;
    if (_currentPosition + mappedSize >= _fileSize)
        mappedSize = _fileSize - _currentPosition;
    _mappedMemory = _file->map(_currentPosition, mappedSize);

    // Check if memory was mapped successfully
    if (!_mappedMemory)
    {
        *data = nullptr;
        *size = 0;
        return false;
    }
    else
    {
        _currentPosition += mappedSize;

        *data = _mappedMemory;
        *size = mappedSize;
        return true;
    }
}

void OsmAnd::QFileDeviceInputStream::BackUp(int count)
{
    if (count > _currentPosition)
        _currentPosition = 0;
    else
        _currentPosition -= count;
}

bool OsmAnd::QFileDeviceInputStream::Skip(int count)
{
    if (Q_UNLIKELY(_currentPosition + count >= _fileSize))
    {
        _currentPosition = _fileSize;
        return false;
    }
    else
    {
        _currentPosition += count;
        return true;
    }
}

gpb::int64 OsmAnd::QFileDeviceInputStream::ByteCount() const
{
    return static_cast<gpb::int64>(_currentPosition);
}
