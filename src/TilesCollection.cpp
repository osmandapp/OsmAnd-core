#include "TilesCollection.h"

OsmAnd::TilesCollectionEntry::TilesCollectionEntry( const TileId tileId_, const ZoomLevel zoom_ )
    : tileId(tileId_)
    , zoom(zoom_)
{
}

OsmAnd::TilesCollectionEntry::~TilesCollectionEntry()
{
}
