#include "MapTileDiskCache.h"
#include "MapTileDiskCache_P.h"

#include <cmath>
#include <time.h>

#include "ignore_warnings_on_external_includes.h"
#include <QElapsedTimer>
#include "restore_internal_warnings.h"

// an entry 2x as costly to draw again lives half a half-life longer
const double OsmAnd::MapTileDiskCache::GenerationTimeFactor = 0.5;

OsmAnd::MapTileDiskCache::MapTileDiskCache(const QString& filePath_, const int64_t limitBytes)
    : _p(new MapTileDiskCache_P(this, limitBytes))
    , filePath(filePath_)
{
    _p->open();
}

OsmAnd::MapTileDiskCache::~MapTileDiskCache()
{
    _p->stop();
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

void OsmAnd::MapTileDiskCache::flush()
{
    _p->flush();
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

namespace
{
    const double kHalfLifeSeconds = OsmAnd::MapTileDiskCache::HalfLifeDays * 86400.0;

    // the part of the weight that does not change with use
    double getConstantWeight(const OsmAnd::ZoomLevel zoom, const int generationTimeMs)
    {
        return std::log2(static_cast<double>(OsmAnd::MapTileDiskCache::getZoomWeight(zoom)))
            + OsmAnd::MapTileDiskCache::GenerationTimeFactor * std::log2(1.0 + qMax(0, generationTimeMs));
    }
}

double OsmAnd::MapTileDiskCache::getInitialWeight(const int64_t t, const ZoomLevel zoom, const int generationTimeMs)
{
    // one visit at t
    return t / kHalfLifeSeconds + getConstantWeight(zoom, generationTimeMs);
}

double OsmAnd::MapTileDiskCache::getAccessedWeight(const double weight, const ZoomLevel zoom, const int generationTimeMs,
    const int64_t previousAccess, const int64_t firstAccess, const int64_t lastAccess)
{
    const double constant = getConstantWeight(zoom, generationTimeMs);
    // visits part: log2(decayed count) + t / halfLife, with any t
    double visits = weight - constant;
    if (firstAccess - previousAccess >= VisitGapMinutes * 60)
    {
        // a new visit: count(t) = count(t) + 1, written relative to t
        const double t = firstAccess / kHalfLifeSeconds;
        visits = t + std::log2(std::exp2(visits - t) + 1.0);
    }
    // within a visit the entry is at least as valuable as one visited at the last access
    visits = qMax(visits, lastAccess / kHalfLifeSeconds);
    return visits + constant;
}

double OsmAnd::MapTileDiskCache::getThreadTimeMs()
{
#if defined(CLOCK_THREAD_CPUTIME_ID)
    timespec ts;
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) == 0)
        return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
#endif
    static QElapsedTimer timer;
    if (!timer.isValid())
        timer.start();
    return timer.nsecsElapsed() / 1000000.0;
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

bool OsmAnd::MapTileDiskCache::getEntryInfo(const Key& key, EntryInfo& outInfo)
{
    return _p->getEntryInfo(key, outInfo);
}

bool OsmAnd::MapTileDiskCache::put(const Key& key, const QByteArray& data, const int generationTimeMs)
{
    return _p->put(key, data, generationTimeMs);
}

void OsmAnd::MapTileDiskCache::putLater(const Key& key, const std::function<QByteArray()>& makeData,
    const int generationTimeMs)
{
    _p->putLater(key, makeData, generationTimeMs);
}

void OsmAnd::MapTileDiskCache::setClock(const std::function<int64_t()>& clock)
{
    _p->setClock(clock);
}
