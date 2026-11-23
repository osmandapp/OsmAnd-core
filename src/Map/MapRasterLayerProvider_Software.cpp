#include "MapRasterLayerProvider_Software.h"
#include "MapRasterLayerProvider_Software_P.h"
#include "MapPrimitivesProvider_P.h"
#include "MapRasterLayerCache.h"

OsmAnd::MapRasterLayerProvider_Software::MapRasterLayerProvider_Software(
    const std::shared_ptr<MapPrimitivesProvider>& primitivesProvider_,
    const bool fillBackground_ /* = true */,
    const bool forceObtainDataAsync_ /* = false */,
    const bool adjustToDetailedZoom_ /* = false */)
    : MapRasterLayerProvider(new MapRasterLayerProvider_Software_P(this),
        primitivesProvider_, fillBackground_, forceObtainDataAsync_, adjustToDetailedZoom_)
{
}

OsmAnd::MapRasterLayerProvider_Software::~MapRasterLayerProvider_Software()
{
    setLocalCache(QString(), 0);
}

std::shared_ptr<OsmAnd::MapRasterLayerCache> OsmAnd::MapRasterLayerProvider_Software::getLocalCache() const
{
    QMutexLocker scopedLocker(&_localCacheMutex);
    return _localCache;
}

void OsmAnd::MapRasterLayerProvider_Software::attachToRenderer()
{
    QMutexLocker lock(&_localCacheMutex);
    ++_rendererBindings;
    if (_cacheSuspended)
    {
        _cacheSuspended = false;
        if (_localCache)
            _localCache->setSuspended(false);
    }
}

void OsmAnd::MapRasterLayerProvider_Software::detachFromRenderer()
{
    QMutexLocker lock(&_localCacheMutex);
    if (--_rendererBindings == 0)
    {
        _cacheSuspended = true;
        if (_localCache)
            _localCache->setSuspended(true);
    }
}

bool OsmAnd::MapRasterLayerProvider_Software::setLocalCache(
    const QString& filename, uint64_t maxBytes, ZoomLevel minZoom, ZoomLevel maxZoom)
{
    std::shared_ptr<MapRasterLayerCache> cache;
    if (!filename.isEmpty() && maxBytes != 0)
    {
        cache = std::make_shared<MapRasterLayerCache>(
            primitivesProvider, filename, fillBackground, maxBytes, minZoom, maxZoom);
        if (!cache->isOpened())
            return false;
    }
    std::shared_ptr<MapRasterLayerCache> previous;
    {
        QMutexLocker scopedLocker(&_localCacheMutex);
        previous = _localCache;
        if (previous)
            previous->setSuspended(true);
        if (cache && _cacheSuspended)
            cache->setSuspended(true);
        _localCache = cache;
        if (cache)
            primitivesProvider->_p->setRasterCache(cache);
        else
            primitivesProvider->_p->clearRasterCache(previous);
    }
    return true;
}

bool OsmAnd::MapRasterLayerProvider_Software::clearLocalCache()
{
    const auto cache = getLocalCache();
    return !cache || cache->clear();
}

bool OsmAnd::MapRasterLayerProvider_Software::removeOlderTilesFromCache(int64_t time)
{
    const auto cache = getLocalCache();
    return !cache || cache->removeOlderTiles(time);
}
