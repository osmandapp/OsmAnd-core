#ifndef _OSMAND_CORE_ARCHIVE_READER_H_
#define _OSMAND_CORE_ARCHIVE_READER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <QString>
#include <QList>
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
            
            QString name = {};
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
        virtual ~ArchiveReader();

        const QString fileName;

        QList<Item> getItems(bool* const ok = nullptr, const bool isGzip = false) const;

        bool extractItemToDirectory(const QString& itemName, const QString& destinationPath, const bool keepDirectoryStructure = false, uint64_t* const extractedBytes = nullptr) const;
        bool extractItemToFile(const QString& itemName, const QString& fileName, const bool isGzip = false, uint64_t* const extractedBytes = nullptr) const;
        bool extractAllItemsTo(const QString& destinationPath, uint64_t* const extractedBytes = nullptr) const;
    };
}

#endif // !defined(_OSMAND_CORE_ARCHIVE_READER_H_)
