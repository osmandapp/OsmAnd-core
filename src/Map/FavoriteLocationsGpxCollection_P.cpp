#include "FavoriteLocationsGpxCollection_P.h"
#include "FavoriteLocationsGpxCollection.h"

#include <QIODevice>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

OsmAnd::FavoriteLocationsGpxCollection_P::FavoriteLocationsGpxCollection_P(FavoriteLocationsGpxCollection* const owner_)
	: FavoriteLocationsCollection_P(owner_)
	, owner(owner_)
{
}

OsmAnd::FavoriteLocationsGpxCollection_P::~FavoriteLocationsGpxCollection_P()
{
}

bool OsmAnd::FavoriteLocationsGpxCollection_P::saveTo(const QString& filename) const
{
	return false;
}

bool OsmAnd::FavoriteLocationsGpxCollection_P::loadFrom(const QString& filename)
{
	return false;
}
