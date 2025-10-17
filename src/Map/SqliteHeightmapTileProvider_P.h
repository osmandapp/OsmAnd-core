#ifndef _OSMAND_CORE_SQLITE_HEIGHTMAP_TILE_PROVIDER_P_H_
#define _OSMAND_CORE_SQLITE_HEIGHTMAP_TILE_PROVIDER_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "IMapElevationDataProvider.h"
#include "SqliteHeightmapTileProvider.h"

namespace OsmAnd
{
    class SqliteHeightmapTileProvider_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(SqliteHeightmapTileProvider_P);
    private:
    protected:
        SqliteHeightmapTileProvider_P(SqliteHeightmapTileProvider* const owner);
    public:
        ~SqliteHeightmapTileProvider_P();

        ImplementationInterface<SqliteHeightmapTileProvider> owner;

        bool hasDataResources() const;
        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;

        int getMaxMissingDataZoomShift(int defaultMaxMissingDataZoomShift) const;

        virtual bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr);

    friend class OsmAnd::SqliteHeightmapTileProvider;
    };
}

#endif // !defined(_OSMAND_CORE_SQLITE_HEIGHTMAP_TILE_PROVIDER_P_H_)
