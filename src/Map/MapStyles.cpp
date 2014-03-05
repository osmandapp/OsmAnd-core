#include "MapStyles.h"
#include "MapStyles_P.h"

OsmAnd::MapStyles::MapStyles()
    : _d(new MapStyles_P(this))
{
}

OsmAnd::MapStyles::~MapStyles()
{
}

bool OsmAnd::MapStyles::registerStyle( const QString& filePath )
{
    return _d->registerStyle(filePath);
}

bool OsmAnd::MapStyles::obtainStyle(const QString& name, std::shared_ptr<const MapStyle>& outStyle) const
{
    return _d->obtainStyle(name, outStyle);
}
