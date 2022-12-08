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

void OsmAnd::ArchiveWriter_P::createArchive(bool* const ok_, const QString& filePath, const QList<QString>& filesToArcive, const QString& basePath, const bool gzip /*= false*/)
{
    struct archive *a;
    struct archive_entry *entry;
    struct stat st;
    char buff[8192];
    ssize_t len;
    int res;
    
    a = archive_write_new();
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
    auto filePathArray = filePath.toUtf8();
    res = archive_write_open_filename(a, filePathArray.data());
    if (res != ARCHIVE_OK)
    {
        *ok_ = false;
        return;
    }
    for (QString fileName : filesToArcive)
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
        len = archiveFile.read(buff, 8192);
        while ( len > 0 ) {
            archive_write_data(a, buff, len);
            len = archiveFile.read(buff, 8192);
        }
        archiveFile.close();
        archive_entry_free(entry);
    }
    res = archive_write_close(a);
    res = archive_write_free(a);
    *ok_ = res == ARCHIVE_OK;
}
