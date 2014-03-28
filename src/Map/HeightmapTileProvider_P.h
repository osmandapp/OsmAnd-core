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
#include "TileDB.h"
#include "IMapElevationDataProvider.h"

namespace OsmAnd
{
    class HeightmapTileProvider;
    class HeightmapTileProvider_P
    {
        Q_DISABLE_COPY(HeightmapTileProvider_P);
    private:
    protected:
        HeightmapTileProvider_P(HeightmapTileProvider* owner, const QDir& dataPath, const QString& indexFilepath);

        HeightmapTileProvider* const owner;
        TileDB _tileDb;

        bool obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const MapTile>& outTile, const IQueryController* const queryController);

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;
    public:
        ~HeightmapTileProvider_P();

    friend class OsmAnd::HeightmapTileProvider;
    };
}

#endif // !defined(_OSMAND_CORE_HEIGHTMAP_TILE_PROVIDER_P_H_)
