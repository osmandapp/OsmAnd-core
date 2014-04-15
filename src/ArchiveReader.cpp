#include "ArchiveReader.h"
#include "ArchiveReader_P.h"

OsmAnd::ArchiveReader::ArchiveReader(const QString& fileName_)
    : _p(new ArchiveReader_P(this))
    , fileName(fileName_)
{
}

OsmAnd::ArchiveReader::~ArchiveReader()
{
}

QList<OsmAnd::ArchiveReader::Item> OsmAnd::ArchiveReader::getItems(bool* const ok /*= nullptr*/) const
{
    return _p->getItems(ok);
}

bool OsmAnd::ArchiveReader::extractItemTo(const QString& itemName, const QString& destinationPath, const bool keepDirectoryStructure /*= false*/) const
{
    return _p->extractItemTo(itemName, destinationPath, keepDirectoryStructure);
}

bool OsmAnd::ArchiveReader::extractAllItemsTo(const QString& destinationPath) const
{
    return _p->extractAllItemsTo(destinationPath);
}

OsmAnd::ArchiveReader::Item::Item()
    : name(QString::null)
{
}

OsmAnd::ArchiveReader::Item::Item(const Item& other)
    : name(other.name)
{
}

OsmAnd::ArchiveReader::Item::~Item()
{
}

OsmAnd::ArchiveReader::Item& OsmAnd::ArchiveReader::Item::operator=(const Item& other)
{
    name = other.name;

    return *this;
}

bool OsmAnd::ArchiveReader::Item::isValid() const
{
    return !name.isNull();
}
