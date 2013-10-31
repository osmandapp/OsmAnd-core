#include "IMapSymbolProvider.h"

#include <SkBitmap.h>

OsmAnd::IMapSymbolProvider::IMapSymbolProvider()
{
}

OsmAnd::IMapSymbolProvider::~IMapSymbolProvider()
{
}

OsmAnd::MapSymbol::MapSymbol(const uint64_t id_, const PointI& location_, const ZoomLevel zoom_, const std::shared_ptr<const SkBitmap>& icon_, const QList< std::shared_ptr<const SkBitmap> >& texts_)
    : id(id_)
    , location(location_)
    , zoom(zoom_)
    , icon(icon_)
    , texts(texts_)
{
}

OsmAnd::MapSymbol::~MapSymbol()
{
}
