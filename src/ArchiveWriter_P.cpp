#include "ArchiveWriter_P.h"
#include "ArchiveWriter.h"

#include <sys/stat.h>
#include <fcntl.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <libarchive/archive_entry.h>

OsmAnd::ArchiveWriter_P::ArchiveWriter_P(ArchiveWriter* const owner_)
    : owner(owner_)
{
}

OsmAnd::ArchiveWriter_P::~ArchiveWriter_P()
{
}

void OsmAnd::ArchiveWriter_P::createArchive(bool* const ok_, const QString& filePath, const QList<QString>& filesToArchive, const QString& basePath, const bool gzip /*= false*/)
{
    struct archive *a = archive_write_new();
    setupArchive(a, gzip);

    auto filePathArray = filePath.toUtf8();
    int res = archive_write_open_filename(a, filePathArray.data());
    if (res != ARCHIVE_OK)
    {
        *ok_ = false;
        return;
    }

    writeFiles(a, filesToArchive, basePath);

    res = archive_write_close(a);
    res = archive_write_free(a);
    *ok_ = res == ARCHIVE_OK;
}

QByteArray OsmAnd::ArchiveWriter_P::createArchive(bool* const ok_, const QList<QString>& filesToArchive, const QString& basePath, const bool gzip /*= false*/)
{
    struct archive *a = archive_write_new();
    setupArchive(a, gzip);

    int res = archive_write_open_memory(a);
    if (res != ARCHIVE_OK)
    {
        *ok_ = false;
        return nullptr;
    }

    writeFiles(a, filesToArchive, basePath);

    res = archive_write_close(a);
    res = archive_write_free(a);
    *ok_ = res == ARCHIVE_OK;

    return result;
}

void OsmAnd::ArchiveWriter_P::setupArchive(struct archive *a, const bool gzip)
{
    if (gzip)
    {
        archive_write_add_filter_gzip(a);
        archive_write_set_format_raw(a);
    }
    else
    {
        archive_write_set_format_zip(a);
        archive_write_add_filter_none(a);
    }
    archive_write_zip_set_compression_deflate(a);
}

void OsmAnd::ArchiveWriter_P::writeFiles(struct archive *a, const QList<QString>& filesToArchive, const QString& basePath)
{
    struct archive_entry *entry;
    struct stat st;

    int buffSize = 16384;
    char buff[buffSize];
    size_t len;

    for (auto fileName : filesToArchive)
    {
        QFile archiveFile(fileName);
        if (!archiveFile.exists())
            continue;

        QFileInfo fi(fileName);
        auto fileNameArray = fileName.toUtf8();
        const char* filename = fileNameArray.data();
        stat(filename, &st);
        entry = archive_entry_new();
        auto fileNameReplacedArray = fileName.replace(basePath + QStringLiteral("/"), QStringLiteral("")).toUtf8();
        archive_entry_set_pathname_utf8(entry, fileNameReplacedArray.data());
        archive_entry_set_size(entry, st.st_size);
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0664);
        archive_write_header(a, entry);
        archiveFile.open(QIODevice::ReadOnly);
        len = archiveFile.read(buff, buffSize);
        while (len > 0)
        {
            archive_write_data(a, buff, len);
            len = archiveFile.read(buff, buffSize);
        }
        archiveFile.close();
        archive_entry_free(entry);
    }
}

int OsmAnd::ArchiveWriter_P::archive_write_open_memory(struct archive *a)
{
    return (archive_write_open(a, &result, memory_write_open, memory_write, NULL));
}

int OsmAnd::ArchiveWriter_P::memory_write_open(struct archive *a, void *result)
{
    /* Disable padding if it hasn't been set explicitly. */
    if (-1 == archive_write_get_bytes_in_last_block(a))
        archive_write_set_bytes_in_last_block(a, 1);
    return (ARCHIVE_OK);
}

ssize_t OsmAnd::ArchiveWriter_P::memory_write(struct archive *a, void *result, const void *buff, size_t length)
{
    QByteArray *array = reinterpret_cast<QByteArray *>(result);
    array->append(reinterpret_cast<const char *>(buff), static_cast<int>(length));

    return (length);
}

