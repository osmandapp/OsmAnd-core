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

bool OsmAnd::MapStylesCollection::obtainStyle(const QString& name, std::shared_ptr<const MapStyle>& outStyle) const
{
    return _p->obtainStyle(name, outStyle);
}
