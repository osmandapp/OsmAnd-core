#include "RasterizedSymbol.h"

OsmAnd::RasterizedSymbol::RasterizedSymbol( const std::shared_ptr<const Model::MapObject>& mapObject_, const std::shared_ptr<const SkBitmap>& icon_, const QList< std::shared_ptr<const SkBitmap> >& texts_ )
    : mapObject(mapObject_)
    , icon(icon_)
    , texts(texts_)
{
}

OsmAnd::RasterizedSymbol::~RasterizedSymbol()
{
}
