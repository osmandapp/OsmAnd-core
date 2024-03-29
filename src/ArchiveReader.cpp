#include "ArchiveReader.h"
#include "ArchiveReader_P.h"

OsmAnd::ArchiveReader::ArchiveReader(const QString& fileName_)
    : _p(new ArchiveReader_P(this))
    , fileName(fileName_)
	, sourceData(nullptr)
	, sourceDataSize(-1)
{
}

OsmAnd::ArchiveReader::ArchiveReader(const char *data, int size)
    : _p(new ArchiveReader_P(this))
	, fileName(QString())
    , sourceData(data)
	, sourceDataSize(size)
{
}

OsmAnd::ArchiveReader::~ArchiveReader()
{
}

QList<OsmAnd::ArchiveReader::Item> OsmAnd::ArchiveReader::getItems(bool* const ok /*= nullptr*/, const bool isGzip) const
{
    return _p->getItems(ok, isGzip);
}

bool OsmAnd::ArchiveReader::extractItemToDirectory(const QString& itemName, const QString& destinationPath, const bool keepDirectoryStructure /*= false*/, uint64_t* const extractedBytes /*= nullptr*/) const
{
    return _p->extractItemToDirectory(itemName, destinationPath, keepDirectoryStructure, extractedBytes);
}

bool OsmAnd::ArchiveReader::extractItemToFile(const QString& itemName, const QString& fileName, const bool isGzip, uint64_t* const extractedBytes /*= nullptr*/) const
{
    return _p->extractItemToFile(itemName, fileName, extractedBytes, isGzip);
}

QByteArray OsmAnd::ArchiveReader::extractItemToArray(const QString& itemName, const bool isGzip, uint64_t* const extractedBytes) const
{
    return _p->extractItemToArray(itemName, extractedBytes, isGzip);
}

QString OsmAnd::ArchiveReader::extractItemToString(const QString& itemName, const bool isGzip, uint64_t* const extractedBytes) const
{
    return _p->extractItemToString(itemName, extractedBytes, isGzip);
}

bool OsmAnd::ArchiveReader::extractAllItemsTo(const QString& destinationPath, uint64_t* const extractedBytes /*= nullptr*/) const
{
    return _p->extractAllItemsTo(destinationPath, extractedBytes);
}

OsmAnd::ArchiveReader::Item::Item()
    : name(QString::null)
{
}

OsmAnd::ArchiveReader::Item::Item(const Item& other)
    : name(other.name)
    , size(other.size)
    , creationTime(other.creationTime)
    , modificationTime(other.modificationTime)
    , accessTime(other.accessTime)
{
}

OsmAnd::ArchiveReader::Item::~Item()
{
}

OsmAnd::ArchiveReader::Item& OsmAnd::ArchiveReader::Item::operator=(const Item& other)
{
    name = other.name;
    size = other.size;
    creationTime = other.creationTime;
    modificationTime = other.modificationTime;
    accessTime = other.accessTime;

    return *this;
}

bool OsmAnd::ArchiveReader::Item::isValid() const
{
    return !name.isNull();
}
