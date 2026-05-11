#include "QFileDeviceInputStream.h"

#include <algorithm>
#include <new>

#include "Logging.h"

namespace OsmAnd
{
    namespace gpb = google::protobuf;
}

namespace
{
    enum
    {
        FallbackBufferSize = 64 * 1024, // 64Kb
    };
}

OsmAnd::QFileDeviceInputStream::QFileDeviceInputStream(
    const std::shared_ptr<QFileDevice>& file_,
    const size_t memoryWindowSize_ /*= DefaultMemoryWindowSize*/)
    : _file(file_)
    , _fileSize(_file->size())
    , _mappedMemory(nullptr)
    , _fallbackBuffer(nullptr)
    , _fallbackBufferCapacity(0)
    , _useMapping(true)
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

    delete[] _fallbackBuffer;
}

bool OsmAnd::QFileDeviceInputStream::readFromFile(
    const void** data,
    int* size,
    const qint64 position,
    const size_t sizeToRead)
{
    *data = nullptr;
    *size = 0;

    try
    {
        const auto fallbackSize = std::min<size_t>(sizeToRead, FallbackBufferSize);
        if (Q_UNLIKELY(fallbackSize == 0))
            return false;

        if (_fallbackBufferCapacity < fallbackSize)
        {
            uint8_t* newBuffer = new(std::nothrow) uint8_t[fallbackSize];
            if (!newBuffer)
                return false;

            delete[] _fallbackBuffer;
            _fallbackBuffer = newBuffer;
            _fallbackBufferCapacity = fallbackSize;
        }

        if (!_file->seek(position))
            return false;

        const auto bytesRead = _file->read(reinterpret_cast<char*>(_fallbackBuffer), fallbackSize);
        if (bytesRead <= 0)
            return false;

        *data = _fallbackBuffer;
        *size = static_cast<int>(bytesRead);
        return true;
    }
    catch (...)
    {
        return false;
    }
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
    if (Q_UNLIKELY(!_file->isOpen()))
    {
        *data = nullptr;
        *size = 0;
        return false;
    }

    // Map new portion of data
    const auto mappedSize = std::min(_memoryWindowSize, static_cast<size_t>(_fileSize - _currentPosition));
    if (_useMapping)
    {
        try
        {
            _mappedMemory = _file->map(_currentPosition, mappedSize);
        }
        catch (...)
        {
            _mappedMemory = nullptr;
        }
    }

    // Check if memory was mapped successfully
    if (Q_LIKELY(_mappedMemory))
    {
        _currentPosition += mappedSize;

        *data = _mappedMemory;
        *size = static_cast<int>(mappedSize);
        return true;
    }

    if (_useMapping)
    {
        _useMapping = false;
        LogPrintf(LogSeverityLevel::Warning,
            "Failed to map %" PRIu64 " bytes starting at %" PRIi64 " offset (handle 0x%08x); using buffered reads",
            static_cast<uint64_t>(mappedSize),
            _currentPosition,
            file->handle());
    }

    const auto readPosition = _currentPosition;
    if (readFromFile(data, size, readPosition, mappedSize))
    {
        _currentPosition += *size;
        return true;
    }

    LogPrintf(LogSeverityLevel::Warning,
        "Failed to read %" PRIu64 " bytes starting at %" PRIi64 " offset after memory mapping failed (handle 0x%08x)",
        static_cast<uint64_t>(mappedSize),
        readPosition,
        file->handle());

    *data = nullptr;
    *size = 0;
    return false;
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
