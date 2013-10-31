#include "RasterizedSymbol.h"

#include <cassert>

OsmAnd::RasterizedSymbol::RasterizedSymbol( const std::shared_ptr<const Model::MapObject>& mapObject_, const PointI& location31_, const std::shared_ptr<const SkBitmap>& icon_, const QList< std::shared_ptr<const SkBitmap> >& texts_ )
    : mapObject(mapObject_)
    , location31(location31_)
    , icon(icon_)
    , texts(texts_)
{
    assert(mapObject_);
}

OsmAnd::RasterizedSymbol::~RasterizedSymbol()
{
}
