#ifndef _OSMAND_CORE_ONLINE_MAP_RASTER_TILE_PROVIDERS_DB_P_H_
#define _OSMAND_CORE_ONLINE_MAP_RASTER_TILE_PROVIDERS_DB_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QHash>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "MapTypes.h"
#include "OnlineMapRasterTileProvidersDB.h"

namespace OsmAnd
{
    class OnlineMapRasterTileProvidersDB_P
    {
    private:
    protected:
        OnlineMapRasterTileProvidersDB_P(OnlineMapRasterTileProvidersDB* owner);

        QHash< QString, OnlineMapRasterTileProvidersDB::Entry > _entries;

        bool add(const OnlineMapRasterTileProvidersDB::Entry& entry);
        bool remove(const QString& id);

        bool saveTo(const QString& filePath) const;

        std::shared_ptr<OnlineMapRasterTileProvider> createProvider(const QString& providerId) const;

        static std::shared_ptr<OnlineMapRasterTileProvidersDB> createDefaultDB();
        static std::shared_ptr<OnlineMapRasterTileProvidersDB> loadFrom(const QString& filePath);
    public:
        virtual ~OnlineMapRasterTileProvidersDB_P();

        ImplementationInterface<OnlineMapRasterTileProvidersDB> owner;

    friend class OsmAnd::OnlineMapRasterTileProvidersDB;
    };
}

#endif // !defined(_OSMAND_CORE_ONLINE_MAP_RASTER_TILE_PROVIDERS_DB_P_H_)
