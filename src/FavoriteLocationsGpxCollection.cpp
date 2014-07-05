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

bool OsmAnd::FavoriteLocationsGpxCollection::saveTo(const QString& filename) const
{
	return _p->saveTo(filename);
}

bool OsmAnd::FavoriteLocationsGpxCollection::saveTo(QXmlStreamWriter& writer) const
{
	return _p->saveTo(writer);
}

bool OsmAnd::FavoriteLocationsGpxCollection::loadFrom(const QString& filename)
{
	return _p->loadFrom(filename);
}

bool OsmAnd::FavoriteLocationsGpxCollection::loadFrom(QXmlStreamReader& reader)
{
	return _p->loadFrom(reader);
}
