#include "RasterizedSymbol.h"

OsmAnd::RasterizedSymbol::RasterizedSymbol( const std::shared_ptr<const Model::MapObject>& mapObject_, const std::shared_ptr<const SkBitmap>& iconBitmap_ )
    : mapObject(mapObject_)
    , iconBitmap(iconBitmap_)
{
}

OsmAnd::RasterizedSymbol::~RasterizedSymbol()
{
}
