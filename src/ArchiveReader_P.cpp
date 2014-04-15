#include "ArchiveReader_P.h"
#include "ArchiveReader.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <libarchive/archive_entry.h>

OsmAnd::ArchiveReader_P::ArchiveReader_P(ArchiveReader* const owner_)
    : owner(owner_)
{
}

OsmAnd::ArchiveReader_P::~ArchiveReader_P()
{
}

QList<OsmAnd::ArchiveReader_P::Item> OsmAnd::ArchiveReader_P::getItems(bool* const ok_) const
{
    QList<Item> result;

    const auto ok = processArchive(owner->fileName, 
        [&result]
        (archive* archive, archive_entry* entry, bool& doStop) -> bool
        {
            Item item;
            item.name = QString(reinterpret_cast<const QChar*>(archive_entry_pathname_w(entry)));
            result.push_back(item);

            return true;
        });

    if(ok_)
        *ok_ = ok;
    return result;
}

bool OsmAnd::ArchiveReader_P::extractItemToDirectory(const QString& itemName, const QString& destinationPath, const bool keepDirectoryStructure) const
{
    const auto fileName = QDir(destinationPath).absoluteFilePath(keepDirectoryStructure ? itemName : QFileInfo(itemName).fileName());
    return extractItemToFile(itemName, fileName);
}

bool OsmAnd::ArchiveReader_P::extractItemToFile(const QString& itemName, const QString& fileName) const
{
    return processArchive(owner->fileName, 
        [itemName, fileName]
        (archive* archive, archive_entry* entry, bool& doStop) -> bool
        {
            const QString currentItemName(reinterpret_cast<const QChar*>(archive_entry_pathname_w(entry)));
            if(currentItemName != itemName)
                return true;
            
            bool ok = extractArchiveEntryAsFile(archive, entry, fileName);
            if(!ok)
                return false;

            doStop = true;
            return true;
        });
}

bool OsmAnd::ArchiveReader_P::extractAllItemsTo(const QString& destinationPath) const
{
    return processArchive(owner->fileName, 
        [destinationPath]
        (archive* archive, archive_entry* entry, bool& doStop) -> bool
        {
            const QString currentItemName(reinterpret_cast<const QChar*>(archive_entry_pathname_w(entry)));
            return extractArchiveEntryAsFile(archive, entry, QDir(destinationPath).absoluteFilePath(currentItemName));
        });
}

bool OsmAnd::ArchiveReader_P::processArchive(const QString& fileName, const ArchiveEntryHander handler)
{
    QFile archiveFile(fileName);
    if(!archiveFile.exists())
        return false;

    bool ok = processArchive(&archiveFile, handler);

    archiveFile.close();

    return ok;
}

bool OsmAnd::ArchiveReader_P::processArchive(QIODevice* const ioDevice, const ArchiveEntryHander handler)
{
    auto archive = archive_read_new();
    if(!archive)
        return false;

    ArchiveData archiveData;
    archiveData.ioDevice = ioDevice;
    archiveData.buffer = new uint8_t[BufferSize];
    int res;

    // Open archive
    for(;;)
    {
        res = archive_read_support_compression_all(archive);
        if(res != ARCHIVE_OK)
            break;

        res = archive_read_support_format_all(archive);
        if(res != ARCHIVE_OK)
            break;

        res = archive_read_set_callback_data(archive, &archiveData);
        if(res != ARCHIVE_OK)
            break;

        res = archive_read_set_open_callback(archive, &ArchiveReader_P::archiveOpen);
        if(res != ARCHIVE_OK)
            break;

        res = archive_read_set_read_callback(archive, &ArchiveReader_P::archiveRead);
        if(res != ARCHIVE_OK)
            break;

        res = archive_read_set_skip_callback(archive, &ArchiveReader_P::archiveSkip);
        if(res != ARCHIVE_OK)
            break;

        res = archive_read_set_seek_callback(archive, &ArchiveReader_P::archiveSeek);
        if(res != ARCHIVE_OK)
            break;

        res = archive_read_set_close_callback(archive, &ArchiveReader_P::archiveClose);
        if(res != ARCHIVE_OK)
            break;

        res = archive_read_open1(archive);

        break;
    }
    if(res != ARCHIVE_OK)
    {
        archive_read_finish(archive);
        delete[] archiveData.buffer;

        return false;
    }

    // Process items
    bool ok;
    archive_entry* archiveEntry = nullptr;
    while(archive_read_next_header(archive, &archiveEntry) == ARCHIVE_OK)
    {
        bool doStop = false;
        ok = handler(archive, archiveEntry, doStop);
        if(!ok || doStop)
            break;
    }

    // Close archive
    res = archive_read_finish(archive);
    if(res != ARCHIVE_OK)
        ok = false;
    delete[] archiveData.buffer;

    return ok;
}

bool OsmAnd::ArchiveReader_P::extractArchiveEntryAsFile(archive* archive, archive_entry* entry, const QString& fileName)
{
    const auto itemSize = archive_entry_size(entry);
    if(itemSize <= 0)
        return false;

    QFile targetFile(fileName);
    if(!QDir(QFileInfo(fileName).absolutePath()).mkpath("."))
        return false;

    bool ok;
    for(;;)
    {
        ok = targetFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
        if(!ok)
            break;

        ok = targetFile.resize(itemSize);
        if(!ok)
            break;

        const auto buffer = new uint8_t[BufferSize];
        for(; ok;)
        {
            const auto bytesRead = archive_read_data(archive, buffer, BufferSize);
            if(bytesRead <= 0)
            {
                ok = (bytesRead == 0);
                break;
            }

            uint64_t bytesWritten = 0;
            do
            {
                const auto writtenChunkSize = targetFile.write(
                    reinterpret_cast<char*>(buffer + bytesWritten),
                    bytesRead - bytesWritten);
                if(writtenChunkSize <= 0)
                {
                    ok = false;
                    break;
                }
                bytesWritten += writtenChunkSize;
            } while(bytesWritten != bytesRead);
        }
        delete[] buffer;

        break;
    }
    if(!ok)
    {
        targetFile.close();
        targetFile.remove();
        return false;
    }

    targetFile.flush();
    targetFile.close();

    return true;
}

int OsmAnd::ArchiveReader_P::archiveOpen(archive *, void *_client_data)
{
    const auto archiveData = reinterpret_cast<ArchiveData*>(_client_data);

    const auto ok = archiveData->ioDevice->open(QIODevice::ReadOnly);
    return ok ? ARCHIVE_OK : ARCHIVE_FATAL;
}

__LA_SSIZE_T OsmAnd::ArchiveReader_P::archiveRead(archive *, void *_client_data, const void **_buffer)
{
    const auto archiveData = reinterpret_cast<ArchiveData*>(_client_data);

    *_buffer = archiveData->buffer;
    const auto bytesRead = archiveData->ioDevice->read(reinterpret_cast<char*>(archiveData->buffer), BufferSize);
    return bytesRead;
}

__LA_INT64_T OsmAnd::ArchiveReader_P::archiveSkip(archive *, void *_client_data, __LA_INT64_T request)
{
    const auto archiveData = reinterpret_cast<ArchiveData*>(_client_data);

    const auto currentPos = archiveData->ioDevice->pos();
    archiveData->ioDevice->seek(currentPos + request);
    const auto actuallySkipped = (archiveData->ioDevice->pos() - currentPos);
    return actuallySkipped;
}

__LA_INT64_T OsmAnd::ArchiveReader_P::archiveSeek(archive *, void *_client_data, __LA_INT64_T offset, int whence)
{
    const auto archiveData = reinterpret_cast<ArchiveData*>(_client_data);

    if(archiveData->ioDevice->isSequential())
        return ARCHIVE_FATAL;

    switch (whence)
    {
    case SEEK_SET:
        archiveData->ioDevice->seek(offset);
        break;
    case SEEK_CUR:
        archiveData->ioDevice->seek(archiveData->ioDevice->pos() + offset);
        break;
    case SEEK_END:
        archiveData->ioDevice->seek(archiveData->ioDevice->size() - offset);
        break;
    }
    
    const auto newPosition = archiveData->ioDevice->pos();
    return newPosition;
}

int OsmAnd::ArchiveReader_P::archiveClose(archive *, void *_client_data)
{
    const auto archiveData = reinterpret_cast<ArchiveData*>(_client_data);

    if(archiveData->ioDevice->isOpen())
        archiveData->ioDevice->close();

    return ARCHIVE_OK;
}
