#include "FavoriteLocationsGpxCollection.h"
#include "FavoriteLocationsGpxCollection_P.h"

OsmAnd::FavoriteLocationsGpxCollection::FavoriteLocationsGpxCollection()
    : FavoriteLocationsGpxCollection(new FavoriteLocationsGpxCollection_P(this))
{
}

OsmAnd::FavoriteLocationsGpxCollection::FavoriteLocationsGpxCollection(FavoriteLocationsGpxCollection_P* const p)
    : FavoriteLocationsCollection(p)
    , _p(p)
{
}

OsmAnd::FavoriteLocationsGpxCollection::~FavoriteLocationsGpxCollection()
{
}

std::shared_ptr<OsmAnd::GpxDocument> OsmAnd::FavoriteLocationsGpxCollection::loadFrom(const QString& fileName, bool append /*= false*/)
{
    return _p->loadFrom(fileName, append);
}

std::shared_ptr<OsmAnd::GpxDocument> OsmAnd::FavoriteLocationsGpxCollection::loadFrom(QXmlStreamReader& reader, bool append /*= false*/)
{
    return _p->loadFrom(reader, append);
}

std::shared_ptr<OsmAnd::FavoriteLocationsGpxCollection> OsmAnd::FavoriteLocationsGpxCollection::tryLoadFrom(const QString& filename)
{
    std::shared_ptr<FavoriteLocationsGpxCollection> collection(new FavoriteLocationsGpxCollection());
    if (!collection->loadFrom(filename))
        return nullptr;

    return collection;
}

std::shared_ptr<OsmAnd::FavoriteLocationsGpxCollection> OsmAnd::FavoriteLocationsGpxCollection::tryLoadFrom(QXmlStreamReader& reader)
{
    std::shared_ptr<FavoriteLocationsGpxCollection> collection(new FavoriteLocationsGpxCollection());
    if (!collection->loadFrom(reader))
        return nullptr;

    return collection;
}
