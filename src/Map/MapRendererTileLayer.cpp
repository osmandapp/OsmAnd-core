#include "MapRendererTileLayer.h"

#include <assert.h>

#include "Logging.h"

OsmAnd::MapRendererTileLayer::MapRendererTileLayer(const MapTileLayerId& layerId_)
    : _tilesCollectionMutex(QMutex::Recursive)
    , layerId(layerId_)
{
}

OsmAnd::MapRendererTileLayer::~MapRendererTileLayer()
{
    QMutexLocker scopedLock(&_tilesCollectionMutex);

    // Ensure that no tiles have "Uploaded" state
    QList< std::shared_ptr<TileEntry> > stillUploadedTiles;
    obtainTileEntries(stillUploadedTiles, 1, TileEntry::State::Uploaded);
    if(!stillUploadedTiles.isEmpty())
        LogPrintf(LogSeverityLevel::Error, "One or more tiles still reside in GPU memory. This may cause GPU memory leak");
    assert(stillUploadedTiles.isEmpty());
}

std::shared_ptr<OsmAnd::MapRendererTileLayer::TileEntry> OsmAnd::MapRendererTileLayer::obtainTileEntry( const TileId& tileId, const ZoomLevel& zoom, bool createEmptyIfUnexistent /*= false*/ )
{
    QMutexLocker scopedLock(&_tilesCollectionMutex);

    auto& zoomLevel = _tilesCollection[zoom];
    auto itEntry = zoomLevel.find(tileId);
    if(itEntry != zoomLevel.end())
        return *itEntry;

    if(!createEmptyIfUnexistent)
        return std::shared_ptr<TileEntry>();

    std::shared_ptr<TileEntry> entry(new TileEntry(tileId, zoom, layerId));
    itEntry = zoomLevel.insert(tileId, entry);
    return entry;
}

void OsmAnd::MapRendererTileLayer::obtainTileEntries( QList< std::shared_ptr<TileEntry> >& outList, unsigned int limit /*= 0u*/, const TileEntry::State& withState /*= TileEntry::State::Unknown*/, const ZoomLevel& andZoom /*= ZoomLevel::InvalidZoom*/ )
{
    QMutexLocker scopedLock(&_tilesCollectionMutex);

    unsigned int obtainedTilesCount = 0;

    if(withState == TileEntry::State::Unknown)
    {
        for(int state = TileEntry::State::Unavailable; state != TileEntry::State::Unloaded; state++)
        {
            obtainTileEntries(outList, limit - obtainedTilesCount, static_cast<TileEntry::State>(state), andZoom);
            if(limit > 0 && obtainedTilesCount >= limit)
                break;
        }
        return;
    }

    if(andZoom == ZoomLevel::InvalidZoom)
    {
        for(int zoom = ZoomLevel::MinZoomLevel; zoom != ZoomLevel::MaxZoomLevel; zoom++)
        {
            obtainTileEntries(outList, limit - obtainedTilesCount, withState, static_cast<ZoomLevel>(zoom));
            if(limit > 0 && obtainedTilesCount >= limit)
                break;
        }
        return;
    }

    const auto& zoomLevel = _tilesCollection[andZoom];
    for(auto itEntry = zoomLevel.begin(); itEntry != zoomLevel.end(); ++itEntry)
    {
        const auto& entry = *itEntry;

        if(entry->state != withState)
            continue;

        outList.push_back(entry);
        obtainedTilesCount++;

        if(limit > 0 && obtainedTilesCount >= limit)
            break;
    }

    return;
}

void OsmAnd::MapRendererTileLayer::removeAllEntries()
{
    QMutexLocker scopedLock(&_tilesCollectionMutex);

    // Ensure that no tiles have "Uploaded" state
    QList< std::shared_ptr<TileEntry> > stillUploadedTiles;
    obtainTileEntries(stillUploadedTiles, 1, TileEntry::State::Uploaded);
    if(!stillUploadedTiles.isEmpty())
        LogPrintf(LogSeverityLevel::Error, "One or more tiles still reside in GPU memory. This may cause GPU memory leak!");
    assert(stillUploadedTiles.isEmpty());

    for(int zoom = ZoomLevel::MinZoomLevel; zoom != ZoomLevel::MaxZoomLevel; zoom++)
        _tilesCollection[zoom].clear();
}

OsmAnd::MapRendererTileLayer::TileEntry::TileEntry( const TileId& tileId_, const ZoomLevel& zoom_, const MapTileLayerId& layerId_ )
    : _state(Unknown)
    , tileId(tileId_)
    , zoom(zoom_)
    , layerId(layerId_)
    , state(_state)
    , sourceData(_sourceData)
    , resourceInGPU(_resourceInGPU)
{
}

OsmAnd::MapRendererTileLayer::TileEntry::~TileEntry()
{
    QReadLocker scopedLock(&stateLock);

    if(state == State::Uploaded)
        LogPrintf(LogSeverityLevel::Error, "Tile %dx%d@%d[%d] still resides in GPU memory. This may cause GPU memory leak", tileId.x, tileId.y, zoom, layerId);
    assert(state != State::Uploaded);
}
