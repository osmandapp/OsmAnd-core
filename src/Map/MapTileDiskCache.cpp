#include "MapTileDiskCache.h"
#include "MapTileDiskCache_P.h"

OsmAnd::MapTileDiskCache::MapTileDiskCache(const QString& filePath_, const int64_t limitBytes)
    : _p(new MapTileDiskCache_P(this, limitBytes))
    , filePath(filePath_)
{
    _p->open();
}

OsmAnd::MapTileDiskCache::~MapTileDiskCache()
{
}

bool OsmAnd::MapTileDiskCache::isOpen() const
{
    return _p->isOpen();
}

void OsmAnd::MapTileDiskCache::setLimit(const int64_t limitBytes)
{
    _p->setLimit(limitBytes);
}

int64_t OsmAnd::MapTileDiskCache::getLimit() const
{
    return _p->getLimit();
}

void OsmAnd::MapTileDiskCache::setKeptEnvironments(const int count)
{
    _p->setKeptEnvironments(count);
}

int64_t OsmAnd::MapTileDiskCache::getTotalBytes() const
{
    return _p->getTotalBytes();
}

int OsmAnd::MapTileDiskCache::getEntriesCount() const
{
    return _p->getEntriesCount();
}

void OsmAnd::MapTileDiskCache::clear()
{
    _p->clear();
}

bool OsmAnd::MapTileDiskCache::isZoomCached(const ZoomLevel zoom)
{
    return zoom >= MinZoomLevel && zoom <= MaxCachedZoom;
}

int OsmAnd::MapTileDiskCache::getZoomWeight(const ZoomLevel zoom)
{
    // z9-z14 are passed on every zoom in/out of a city and cover a lot of ground, z12 most of all
    if (zoom == ZoomLevel12)
        return 8;
    if (zoom >= ZoomLevel9 && zoom <= ZoomLevel14)
        return 4;
    return 1;
}

void OsmAnd::MapTileDiskCache::useEnvironment(const uint64_t environment)
{
    _p->useEnvironment(environment);
}

int OsmAnd::MapTileDiskCache::syncMaps(const QList<MapFile>& maps)
{
    return _p->syncMaps(maps);
}

bool OsmAnd::MapTileDiskCache::contains(const Key& key) const
{
    return _p->contains(key);
}

QByteArray OsmAnd::MapTileDiskCache::get(const Key& key)
{
    return _p->get(key);
}

bool OsmAnd::MapTileDiskCache::put(const Key& key, const QByteArray& data)
{
    return _p->put(key, data);
}

void OsmAnd::MapTileDiskCache::setClock(const std::function<int64_t()>& clock)
{
    _p->setClock(clock);
}
