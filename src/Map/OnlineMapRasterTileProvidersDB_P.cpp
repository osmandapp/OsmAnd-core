#include "OnlineMapRasterTileProvidersDB_P.h"
#include "OnlineMapRasterTileProvidersDB.h"

#include "OnlineMapRasterTileProvider.h"

OsmAnd::OnlineMapRasterTileProvidersDB_P::OnlineMapRasterTileProvidersDB_P(OnlineMapRasterTileProvidersDB* owner_)
    : owner(owner_)
{
}

OsmAnd::OnlineMapRasterTileProvidersDB_P::~OnlineMapRasterTileProvidersDB_P()
{
}

bool OsmAnd::OnlineMapRasterTileProvidersDB_P::add(const OnlineMapRasterTileProvidersDB::Entry& entry)
{
    if(_entries.contains(entry.id))
        return false;

    _entries.insert(entry.id, entry);
    return true;
}

bool OsmAnd::OnlineMapRasterTileProvidersDB_P::remove(const QString& id)
{
    return (_entries.remove(id) > 0);
}

bool OsmAnd::OnlineMapRasterTileProvidersDB_P::saveTo(const QString& filePath) const
{
    //TODO: save
    return false;
}

std::shared_ptr<OsmAnd::OnlineMapRasterTileProvider> OsmAnd::OnlineMapRasterTileProvidersDB_P::createProvider(const QString& providerId) const
{
    std::shared_ptr<OsmAnd::OnlineMapRasterTileProvider> result;

    const auto& citEntry = _entries.constFind(providerId);
    if(citEntry == _entries.cend())
        return result;

    const auto& entry = *citEntry;
    result.reset(new OnlineMapRasterTileProvider(
        entry.id,
        entry.urlPattern,
        entry.minZoom,
        entry.maxZoom,
        entry.maxConcurrentDownloads,
        entry.tileSize,
        entry.alphaChannelData));

    return result;
}

std::shared_ptr<OsmAnd::OnlineMapRasterTileProvidersDB> OsmAnd::OnlineMapRasterTileProvidersDB_P::createDefaultDB()
{
    std::shared_ptr<OsmAnd::OnlineMapRasterTileProvidersDB> db(new OnlineMapRasterTileProvidersDB());

    // Mapnik (hosted at OsmAnd)
    OnlineMapRasterTileProvidersDB::Entry entryMapnikAtOsmAnd = {
        QLatin1String("mapnik_osmand"),
        QLatin1String("Mapnik (OsmAnd)"),
        QLatin1String("http://mapnik.osmand.net/${zoom}/${x}/${y}.png"),
        ZoomLevel0, ZoomLevel19,
        0,
        256,
        AlphaChannelData::NotPresent
    };
    db->add(entryMapnikAtOsmAnd);

    return db;
}

std::shared_ptr<OsmAnd::OnlineMapRasterTileProvidersDB> OsmAnd::OnlineMapRasterTileProvidersDB_P::loadFrom(const QString& filePath)
{
    std::shared_ptr<OsmAnd::OnlineMapRasterTileProvidersDB> db(new OnlineMapRasterTileProvidersDB());

    //TODO: load
    return db;
}
