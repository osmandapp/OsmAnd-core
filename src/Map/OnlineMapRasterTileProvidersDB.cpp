#include "OnlineMapRasterTileProvidersDB.h"
#include "OnlineMapRasterTileProvidersDB_P.h"

OsmAnd::OnlineMapRasterTileProvidersDB::OnlineMapRasterTileProvidersDB()
    : _d(new OnlineMapRasterTileProvidersDB_P(this))
    , entries(_d->_entries)
{
}

OsmAnd::OnlineMapRasterTileProvidersDB::~OnlineMapRasterTileProvidersDB()
{
}

bool OsmAnd::OnlineMapRasterTileProvidersDB::add(const Entry& entry)
{
    return _d->add(entry);
}

bool OsmAnd::OnlineMapRasterTileProvidersDB::remove(const QString& id)
{
    return _d->remove(id);
}

bool OsmAnd::OnlineMapRasterTileProvidersDB::saveTo(const QString& filePath) const
{
    return _d->saveTo(filePath);
}

std::shared_ptr<OsmAnd::OnlineMapRasterTileProvider> OsmAnd::OnlineMapRasterTileProvidersDB::createProvider(const QString& providerId) const
{
    return _d->createProvider(providerId);
}

std::shared_ptr<OsmAnd::OnlineMapRasterTileProvidersDB> OsmAnd::OnlineMapRasterTileProvidersDB::createDefaultDB()
{
    return OnlineMapRasterTileProvidersDB_P::createDefaultDB();
}

std::shared_ptr<OsmAnd::OnlineMapRasterTileProvidersDB> OsmAnd::OnlineMapRasterTileProvidersDB::loadFrom(const QString& filePath)
{
    return OnlineMapRasterTileProvidersDB_P::loadFrom(filePath);
}
