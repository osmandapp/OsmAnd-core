#ifndef _OSMAND_CORE_ARCHIVE_READER_H_
#define _OSMAND_CORE_ARCHIVE_READER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <QString>
#include <QStringList>
#include <QList>
#include <QHash>
#include <QDateTime>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>

namespace OsmAnd
{
    class ArchiveReader_P;
    class OSMAND_CORE_API ArchiveReader
    {
        Q_DISABLE_COPY_AND_MOVE(ArchiveReader);
    public:
        class OSMAND_CORE_API Item
        {
        private:
        protected:
        public:
            Item();
            Item(const Item& other);
            ~Item();

            Item& operator=(const Item& other);

            bool isValid() const;
            
            QString name;
            uint64_t size;
            QDateTime creationTime;
            QDateTime modificationTime;
            QDateTime accessTime;
        };
    private:
        PrivateImplementation<ArchiveReader_P> _p;
    protected:
    public:
        ArchiveReader(const QString& fileName);
        ArchiveReader(const char *data, int size);
        virtual ~ArchiveReader();

        const QString fileName;

        const char *sourceData;
        const int sourceDataSize;

        QList<Item> getItems(bool* const ok = nullptr, const bool isGzip = false) const;

        // Extracts every requested item in a single pass over the archive. Returns false only if the
        // archive could not be read to the end; items that failed on their own are reported through
        // 'failedItemNames' and do not affect the rest.
        bool extractItemsTo(
            const QHash<QString, QString>& destinationByItemName,
            QStringList* const failedItemNames = nullptr,
            uint64_t* const extractedBytes = nullptr) const;

        bool extractItemToDirectory(const QString& itemName, const QString& destinationPath, const bool keepDirectoryStructure = false, uint64_t* const extractedBytes = nullptr) const;
        bool extractItemToFile(const QString& itemName, const QString& fileName, const bool isGzip = false, uint64_t* const extractedBytes = nullptr) const;
        QByteArray extractItemToArray(const QString& itemName, const bool isGzip = false, uint64_t* const extractedBytes = nullptr) const;
        QString extractItemToString(const QString& itemName, const bool isGzip = false, uint64_t* const extractedBytes = nullptr) const;
        bool extractAllItemsTo(const QString& destinationPath, uint64_t* const extractedBytes = nullptr) const;
    };
}

#endif // !defined(_OSMAND_CORE_ARCHIVE_READER_H_)
