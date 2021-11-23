#ifndef _OSMAND_CORE_ARCHIVE_READER_P_H_
#define _OSMAND_CORE_ARCHIVE_READER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include <QString>
#include <QList>
#include <QIODevice>

#include <libarchive/archive.h>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "ArchiveReader.h"

namespace OsmAnd
{
    class ArchiveReader;
    class ArchiveReader_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(ArchiveReader_P);
    public:
        typedef ArchiveReader::Item Item;

    private:
        struct ArchiveData
        {
            QIODevice* ioDevice;
            uint8_t* buffer;
        };
        typedef std::function<bool(archive* archive, archive_entry* entry, bool& doStop, bool& match)> ArchiveEntryHander;
        static bool processArchive(const QString& fileName, const ArchiveEntryHander handler, const bool isGzip = false);
        static bool processArchive(QIODevice* const ioDevice, const ArchiveEntryHander handler, const bool isGzip = false);

        static bool extractArchiveEntryAsFile(archive* archive, archive_entry* entry, const QString& fileName, uint64_t& bytesExtracted);

        static int archiveOpen(archive *, void *_client_data);
        static __LA_SSIZE_T archiveRead(archive *, void *_client_data, const void **_buffer);
        static __LA_INT64_T archiveSkip(archive *, void *_client_data, __LA_INT64_T request);
        static __LA_INT64_T archiveSeek(archive *, void *_client_data, __LA_INT64_T offset, int whence);
        static int archiveClose(archive *, void *_client_data);

        enum {
            BufferSize = 16 * 1024
        };
    protected:
        ArchiveReader_P(ArchiveReader* const owner);
    public:
        virtual ~ArchiveReader_P();

        ImplementationInterface<ArchiveReader> owner;

        QList<Item> getItems(bool* const ok, const bool isGzip) const;

        bool extractItemToDirectory(const QString& itemName, const QString& destinationPath, const bool keepDirectoryStructure, uint64_t* const extractedBytes) const;
        bool extractItemToFile(const QString& itemName, const QString& fileName, uint64_t* const extractedBytes, const bool isGzip = false) const;
        bool extractAllItemsTo(const QString& destinationPath, uint64_t* const extractedBytes) const;

    friend class OsmAnd::ArchiveReader;
    };
}

#endif // !defined(_OSMAND_CORE_ARCHIVE_READER_P_H_)
