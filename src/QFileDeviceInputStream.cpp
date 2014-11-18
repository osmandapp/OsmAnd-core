#include "QFileDeviceInputStream.h"

#include "Logging.h"

namespace OsmAnd
{
    namespace gpb = google::protobuf;
}

OsmAnd::QFileDeviceInputStream::QFileDeviceInputStream(
    const std::shared_ptr<QFileDevice>& file_,
    const size_t memoryWindowSize_ /*= DefaultMemoryWindowSize*/)
    : _file(file_)
    , _fileSize(_file->size())
    , _mappedMemory(nullptr)
    , _memoryWindowSize(memoryWindowSize_)
    , _currentPosition(0)
    , _wasInitiallyOpened(_file->isOpen())
    , _originalOpenMode(_file->openMode())
    , _closeOnDestruction(false)
    , file(_file)
{
}

OsmAnd::QFileDeviceInputStream::~QFileDeviceInputStream()
{
    bool ok;

    // Unmap memory if it's still mapped
    if (_mappedMemory)
    {
        ok = _file->unmap(_mappedMemory);
        if (!ok)
        {
            LogPrintf(LogSeverityLevel::Warning,
                "Failed to unmap memory %p of '%s' (handle 0x%08x): (%d) %s",
                _mappedMemory,
                qPrintable(file->fileName()),
                file->handle(),
                static_cast<int>(file->error()),
                qPrintable(file->errorString()));
        }

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
    bool ok;

    // If memory was already mapped, unmap it
    if (Q_LIKELY(_mappedMemory != nullptr))
    {
        ok = _file->unmap(_mappedMemory);
        if (!ok)
        {
            LogPrintf(LogSeverityLevel::Warning,
                "Failed to unmap memory %p of '%s' (handle 0x%08x): (%d) %s",
                _mappedMemory,
                qPrintable(file->fileName()),
                file->handle(),
                static_cast<int>(file->error()),
                qPrintable(file->errorString()));
        }
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
    if (Q_UNLIKELY(!_mappedMemory))
    {
        LogPrintf(LogSeverityLevel::Warning,
            "Failed to map %" PRIu64 " bytes starting at %" PRIi64 " offset from '%s' (handle 0x%08x) into memory: (%d) %s",
            static_cast<uint64_t>(mappedSize),
            _currentPosition,
            qPrintable(file->fileName()),
            file->handle(),
            static_cast<int>(file->error()),
            qPrintable(file->errorString()));

        *data = nullptr;
        *size = 0;
        return false;
    }
    
    _currentPosition += mappedSize;

    *data = _mappedMemory;
    *size = mappedSize;
    return true;
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

    _currentPosition += count;
    return true;
}

OsmAnd::gpb::int64 OsmAnd::QFileDeviceInputStream::ByteCount() const
{
    return static_cast<gpb::int64>(_currentPosition);
}
