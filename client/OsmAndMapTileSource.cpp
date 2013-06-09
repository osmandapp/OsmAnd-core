#include "OsmAndMapTileSource.h"


const std::shared_ptr<OsmAnd::MapTileSource> OsmAnd::MapTileSource::kMapnik =
        std::shared_ptr<OsmAnd::MapTileSource>(new OsmAnd::TileSourceTemplate("Mapnik", "http://mapnik.osmand.net/${x}/${y}/${z}.png", ".png", 18, 1, 256, 8, 18000));

const std::shared_ptr<OsmAnd::MapTileSource> OsmAnd::MapTileSource::kCyclemap =
        std::shared_ptr<OsmAnd::MapTileSource>(new OsmAnd::TileSourceTemplate("CycleMap", "http://b.tile.opencyclemap.org/cycle/${x}/${y}/${z}.png", ".png", 16, 1, 256, 32, 18000));
