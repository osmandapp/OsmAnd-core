#include "MapStylesCollection.h"
#include "MapStylesCollection_P.h"

OsmAnd::MapStylesCollection::MapStylesCollection()
    : _p(new MapStylesCollection_P(this))
{
}

OsmAnd::MapStylesCollection::~MapStylesCollection()
{
}

bool OsmAnd::MapStylesCollection::registerStyle( const QString& filePath )
{
    return _p->registerStyle(filePath);
}

QList< std::shared_ptr<const OsmAnd::MapStyle> > OsmAnd::MapStylesCollection::getCollection() const
{
    return _p->getCollection();
}

std::shared_ptr<const OsmAnd::MapStyle> OsmAnd::MapStylesCollection::getAsIsStyle(const QString& name) const
{
    return _p->getAsIsStyle(name);
}

bool OsmAnd::MapStylesCollection::obtainBakedStyle(const QString& name, std::shared_ptr<const MapStyle>& outStyle) const
{
    return _p->obtainBakedStyle(name, outStyle);
}
