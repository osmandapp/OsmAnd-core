#include "MapDataCache.h"

#include "OsmAndUtilities.h"
#include "OsmAndLogging.h"

OsmAnd::MapDataCache::MapDataCache(const size_t& memoryLimit_ /*= std::numeric_limits<size_t>::max()*/)
    : _memoryLimit(memoryLimit_)
    , _cachedObjects(0)
    , _approxConsumedMemory(0)
    , sources(_sources)
    , memoryLimit(_memoryLimit)
    , cachedObjects(_cachedObjects)
    , approxConsumedMemory(_approxConsumedMemory)
{
}

OsmAnd::MapDataCache::~MapDataCache()
{
}

void OsmAnd::MapDataCache::addSource( const std::shared_ptr<OsmAnd::ObfReader>& newSource )
{
    QMutexLocker scopeLock(&_sourcesMutex);
    _sources.push_back(newSource);

    purgeAllCache();
}

void OsmAnd::MapDataCache::removeSource( const std::shared_ptr<OsmAnd::ObfReader>& source )
{
    {
        QMutexLocker scopeLock(&_sourcesMutex);
        _sources.removeOne(source);
    }
    
    purgeAllCache();
}

void OsmAnd::MapDataCache::purgeAllCache()
{
    QMutexLocker scopeLock(&_cacheMutex);

    for(auto itZoomLevel = _zoomLevels.begin(); itZoomLevel != _zoomLevels.end(); ++itZoomLevel)
    {
        auto& zoomLevel = *itZoomLevel;

        zoomLevel._cachedTiles.clear();
    }

    _approxConsumedMemory = 0;
    _cachedObjects = 0;
}

void OsmAnd::MapDataCache::purgeCache(const AreaI& retainArea31, uint32_t retainZoom)
{
    //TODO: cleanup by zoom levels first. overall idea is spherical purge
}

bool OsmAnd::MapDataCache::isCached( const AreaI& area31, uint32_t zoom, IQueryController* controller /*= nullptr*/ )
{
    assert(zoom >= 0 && zoom <= 31);

    QMutexLocker scopeLock(&_cacheMutex);
    const auto& cachedLevel = _zoomLevels[zoom];

    const auto& areaZ = Utilities::areaRightShift(area31, 31 - zoom);

    assert(areaZ.top >= 0 && areaZ.top <= (1 << zoom) - 1);
    assert(areaZ.left >= 0 && areaZ.left <= (1 << zoom) - 1);
    assert(areaZ.bottom >= 0 && areaZ.bottom <= (1 << zoom) - 1);
    assert(areaZ.right >= 0 && areaZ.right <= (1 << zoom) - 1);

    for(int32_t x = areaZ.left; x <= areaZ.right; x++)
    {
        if(controller && controller->isAborted())
            return false;

        for(int32_t y = areaZ.top; y <= areaZ.bottom; y++)
        {
            if(controller && controller->isAborted())
                return false;

            const uint64_t tileId = (static_cast<uint64_t>(x) << 32) | y;

            if(!cachedLevel._cachedTiles.contains(tileId))
                return false;
        }
    }

    return true;
}

void OsmAnd::MapDataCache::cacheObjects( const AreaI& area31, uint32_t zoom, IQueryController* controller /*= nullptr*/ )
{
    QMutexLocker scopeLock(&_cacheMutex);

    assert(zoom >= 0 && zoom <= 31);

    auto& cachedLevel = _zoomLevels[zoom];

    const auto& areaZ = Utilities::areaRightShift(area31, 31 - zoom);

    assert(areaZ.top >= 0 && areaZ.top <= (1 << zoom) - 1);
    assert(areaZ.left >= 0 && areaZ.left <= (1 << zoom) - 1);
    assert(areaZ.bottom >= 0 && areaZ.bottom <= (1 << zoom) - 1);
    assert(areaZ.right >= 0 && areaZ.right <= (1 << zoom) - 1);

    const auto& area31_ = Utilities::areaLeftShift(areaZ, zoom);

    assert(area31_.top >= 0 && area31_.top <= (1u << 31) - 1);
    assert(area31_.left >= 0 && area31_.left <= (1u << 31) - 1);
    assert(area31_.bottom >= 0 && area31_.bottom <= (1u << 31) - 1);
    assert(area31_.right >= 0 && area31_.right <= (1u << 31) - 1);
    
    _sourcesMutex.lock();
    const auto sources_ = _sources;
    _sourcesMutex.unlock();

    for(int32_t x = areaZ.left; x <= areaZ.right; x++)
    {
        if(controller && controller->isAborted())
            return;

        for(int32_t y = areaZ.top; y <= areaZ.bottom; y++)
        {
            if(controller && controller->isAborted())
                return;

            const uint64_t tileId = (static_cast<uint64_t>(x) << 32) | y;

            if(cachedLevel._cachedTiles.contains(tileId))
                continue;

            for(auto itObf = sources_.begin(); itObf != sources_.end(); ++itObf)
            {
                if(controller && controller->isAborted())
                    return;

                const auto& obf = *itObf;

                for(auto itMapSection = obf->mapSections.begin(); itMapSection != obf->mapSections.end(); ++itMapSection)
                {
                    if(controller && controller->isAborted())
                        return;

                    const auto& mapSection = *itMapSection;

                    std::shared_ptr<CachedTile> cachedArea(new CachedTile());
                    cachedArea->_approxConsumedMemory = 0;

                    OsmAnd::ObfMapSection::loadMapObjects(obf.get(), mapSection.get(), zoom, &area31_, &cachedArea->_cachedObjects, nullptr, controller);

                    _cachedObjects += cachedArea->_cachedObjects.size();
                    for(auto itObject = cachedArea->_cachedObjects.begin(); itObject != cachedArea->_cachedObjects.end(); itObject++)
                    {
                        const auto& object = *itObject;

                        cachedArea->_approxConsumedMemory += object->calculateApproxConsumedMemory();
                        _approxConsumedMemory += object->calculateApproxConsumedMemory();
                    }

                    if(_approxConsumedMemory > _memoryLimit)
                    {
                        LogPrintf(LogSeverityLevel::Warning, "Map data cache approx. consumed memory (%llu) is over limit (%llu)",
                            static_cast<uint64_t>(_approxConsumedMemory),
                            static_cast<uint64_t>(_memoryLimit));
                    }

                    cachedLevel._cachedTiles.insert(tileId, cachedArea);
                }
            }
        }
    }
}

void OsmAnd::MapDataCache::obtainObjects( QList< std::shared_ptr<OsmAnd::Model::MapObject> >& resultOut, const AreaI& area31, uint32_t zoom, IQueryController* controller /*= nullptr*/ )
{
    cacheObjects(area31, zoom, controller);

    if(_approxConsumedMemory > _memoryLimit)
    {
        purgeCache(area31, zoom);
    }

    QMutexLocker scopeLock(&_cacheMutex);
    const auto& cachedLevel = _zoomLevels[zoom];

    cachedLevel.obtainObjects(resultOut, area31, zoom, controller);
}

void OsmAnd::MapDataCache::CachedZoomLevel::obtainObjects( QList< std::shared_ptr<OsmAnd::Model::MapObject> >& resultOut, const AreaI& area31, uint32_t zoom, IQueryController* controller ) const
{
    const auto& areaZ = Utilities::areaRightShift(area31, 31 - zoom);

    assert(areaZ.top >= 0 && areaZ.top <= (1 << zoom) - 1);
    assert(areaZ.left >= 0 && areaZ.left <= (1 << zoom) - 1);
    assert(areaZ.bottom >= 0 && areaZ.bottom <= (1 << zoom) - 1);
    assert(areaZ.right >= 0 && areaZ.right <= (1 << zoom) - 1);

    for(int32_t x = areaZ.left; x <= areaZ.right; x++)
    {
        if(controller && controller->isAborted())
            return;

        for(int32_t y = areaZ.top; y <= areaZ.bottom; y++)
        {
            if(controller && controller->isAborted())
                return;

            const uint64_t tileId = (static_cast<uint64_t>(x) << 32) | y;

            const auto& itTile = _cachedTiles.find(tileId);
            const auto& tile = *itTile;

            for(auto itObject = tile->_cachedObjects.begin(); itObject != tile->_cachedObjects.end(); itObject++)
            {
                const auto& object = *itObject;

                if(area31.intersects(object->bbox31))
                    resultOut.push_back(object);
            }
        }
    }
}
