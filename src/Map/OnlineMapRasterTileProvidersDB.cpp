#include "OnlineMapRasterTileProvidersDB.h"
#include "OnlineMapRasterTileProvidersDB_P.h"

OsmAnd::OnlineMapRasterTileProvidersDB::OnlineMapRasterTileProvidersDB()
    : _p(new OnlineMapRasterTileProvidersDB_P(this))
    , entries(_p->_entries)
{
}

OsmAnd::OnlineMapRasterTileProvidersDB::~OnlineMapRasterTileProvidersDB()
{
}

bool OsmAnd::OnlineMapRasterTileProvidersDB::add(const Entry& entry)
{
    return _p->add(entry);
}

bool OsmAnd::OnlineMapRasterTileProvidersDB::remove(const QString& id)
{
    return _p->remove(id);
}

bool OsmAnd::OnlineMapRasterTileProvidersDB::saveTo(const QString& filePath) const
{
    return _p->saveTo(filePath);
}

std::shared_ptr<OsmAnd::OnlineMapRasterTileProvider> OsmAnd::OnlineMapRasterTileProvidersDB::createProvider(const QString& providerId) const
{
    return _p->createProvider(providerId);
}

std::shared_ptr<OsmAnd::OnlineMapRasterTileProvidersDB> OsmAnd::OnlineMapRasterTileProvidersDB::createDefaultDB()
{
    return OnlineMapRasterTileProvidersDB_P::createDefaultDB();
}

std::shared_ptr<OsmAnd::OnlineMapRasterTileProvidersDB> OsmAnd::OnlineMapRasterTileProvidersDB::loadFrom(const QString& filePath)
{
    return OnlineMapRasterTileProvidersDB_P::loadFrom(filePath);
}
