#include "TileZoomCache.h"

#include <assert.h>

OsmAnd::TileZoomCache::TileZoomCache()
    : _tilesCount(0)
    , _usedMemory(0)
    , tilesCount(_tilesCount)
    , usedMemory(_usedMemory)
{
    // Allocate zoom levels
    uint32_t zoom = 0;
    for(auto itZoomLevel = _zoomLevels.begin(); itZoomLevel != _zoomLevels.end(); ++itZoomLevel, zoom++)
    {
        auto& zoomLevel = *itZoomLevel;

        zoomLevel = allocateCachedZoomLevel(zoom);
    }
}

OsmAnd::TileZoomCache::~TileZoomCache()
{
}

std::shared_ptr<OsmAnd::TileZoomCache::ZoomLevel> OsmAnd::TileZoomCache::allocateCachedZoomLevel( uint32_t /*zoom*/ )
{
    return std::shared_ptr<ZoomLevel>(new ZoomLevel());
}

void OsmAnd::TileZoomCache::putTile( const std::shared_ptr<Tile>& tile )
{
    const auto& level = _zoomLevels[tile->zoom];

    assert(!level->_tiles.contains(tile->id));

    level->_tiles.insert(tile->id, tile);
    _tilesCount += 1;
    _usedMemory += tile->usedMemory;
}

bool OsmAnd::TileZoomCache::getTile( const uint32_t& zoom, const TileId& id, std::shared_ptr<Tile>& tileOut ) const
{
    const auto& level = _zoomLevels[zoom];

    const auto& itTile = level->_tiles.find(id);

    if(itTile == level->_tiles.end())
    {
        tileOut.reset();
        return false;
    }

    tileOut = *itTile;
    return true;
}

bool OsmAnd::TileZoomCache::contains( const uint32_t& zoom, const TileId& id ) const
{
    const auto& level = _zoomLevels[zoom];
    return level->_tiles.contains(id);
}

void OsmAnd::TileZoomCache::clearAll()
{
    for(auto itZoomLevel = _zoomLevels.begin(); itZoomLevel != _zoomLevels.end(); ++itZoomLevel)
    {
        auto& level = *itZoomLevel;

        level->_tiles.clear();
        level->_usedMemory = 0;
    }

    _usedMemory = 0;
    _tilesCount = 0;
}

void OsmAnd::TileZoomCache::clearExceptCube( const AreaI& bbox31, uint32_t fromZoom, uint32_t toZoom, bool byDistanceOrder /*= false*/, size_t untilMemoryThreshold /*= 0*/ )
{

}

void OsmAnd::TileZoomCache::clearExceptInterestBox( const AreaI& bbox31, uint32_t baseZoom, uint32_t fromZoom, uint32_t toZoom, float interestFactor /*= 2.0f*/, bool byDistanceOrder /*= false*/, size_t untilMemoryThreshold /*= 0*/ )
{

}

void OsmAnd::TileZoomCache::clearExceptInterestSet( const QSet<TileId>& tiles, uint32_t baseZoom, uint32_t fromZoom, uint32_t toZoom, float interestFactor /*= 2.0f*/, bool byDistanceOrder /*= false*/, size_t untilMemoryThreshold /*= 0*/ )
{

}

OsmAnd::TileZoomCache::ZoomLevel::ZoomLevel()
    : _usedMemory(0)
{
}

OsmAnd::TileZoomCache::ZoomLevel::~ZoomLevel()
{
}

OsmAnd::TileZoomCache::Tile::Tile( const uint32_t& zoom_, const TileId& id_, const size_t& usedMemory_ )
    : zoom(zoom_)
    , id(id_)
    , usedMemory(usedMemory_)
{
}

OsmAnd::TileZoomCache::Tile::~Tile()
{
}
