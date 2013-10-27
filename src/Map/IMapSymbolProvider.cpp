#include "IMapSymbolProvider.h"

#include <SkBitmap.h>

OsmAnd::IMapSymbolProvider::IMapSymbolProvider()
{
}

OsmAnd::IMapSymbolProvider::~IMapSymbolProvider()
{
}

OsmAnd::MapSymbol::MapSymbol(const uint64_t id_, const PointI& location_, const ZoomLevel minZoom_, const ZoomLevel maxZoom_, SkBitmap* bitmap_)
    : _bitmap(bitmap_)
    , id(id_)
    , location(location_)
    , minZoom(minZoom_)
    , maxZoom(maxZoom_)
    , bitmap(_bitmap)
{
}

OsmAnd::MapSymbol::~MapSymbol()
{
}
