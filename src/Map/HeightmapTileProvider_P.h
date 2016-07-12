#ifndef _OSMAND_CORE_HEIGHTMAP_TILE_PROVIDER_P_H_
#define _OSMAND_CORE_HEIGHTMAP_TILE_PROVIDER_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QDir>
#include <QMutex>
#include <QQueue>
#include <QSet>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "TileDB.h"
#include "IMapElevationDataProvider.h"
#include "HeightmapTileProvider.h"

namespace OsmAnd
{
    class HeightmapTileProvider_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(HeightmapTileProvider_P);
    private:
    protected:
        HeightmapTileProvider_P(HeightmapTileProvider* const owner,
            const QString& dataPath,//TODO:refactor-remove
            const QString& indexFilename);//TODO:refactor-remove

        TileDB _tileDb;
    public:
        ~HeightmapTileProvider_P();

        ImplementationInterface<HeightmapTileProvider> owner;

        void rebuildTileDbIndex();

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;
        uint32_t getTileSize() const;
        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric);
        bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<HeightmapTileProvider::Data>& outTiledData,
            const std::shared_ptr<const IQueryController>& queryController);

    friend class OsmAnd::HeightmapTileProvider;
    };
}

#endif // !defined(_OSMAND_CORE_HEIGHTMAP_TILE_PROVIDER_P_H_)
