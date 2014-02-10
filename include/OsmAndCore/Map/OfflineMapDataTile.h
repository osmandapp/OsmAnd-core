#ifndef _OSMAND_CORE_OFFLINE_MAP_DATA_TILE_H_
#define _OSMAND_CORE_OFFLINE_MAP_DATA_TILE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapTypes.h>
#include <OsmAndCore/Map/RasterizerContext.h>

namespace OsmAnd {

    class OfflineMapDataProvider;
    class OfflineMapDataProvider_P;
    namespace Model {
        class MapObject;
    }
    class RasterizerContext;

    class OfflineMapDataTile_P;
    class OSMAND_CORE_API OfflineMapDataTile
    {
        Q_DISABLE_COPY(OfflineMapDataTile);
    private:
        const std::unique_ptr<OfflineMapDataTile_P> _d;
    protected:
        OfflineMapDataTile(
            const TileId tileId, const ZoomLevel zoom,
            const MapFoundationType tileFoundation, const QList< std::shared_ptr<const Model::MapObject> >& mapObjects,
            const std::shared_ptr< const RasterizerContext >& rasterizerContext, const bool nothingToRasterize);
    public:
        virtual ~OfflineMapDataTile();

        const TileId tileId;
        const ZoomLevel zoom;

        const MapFoundationType tileFoundation;
        const QList< std::shared_ptr<const Model::MapObject> >& mapObjects;

        const std::shared_ptr< const RasterizerContext >& rasterizerContext;

        const bool nothingToRasterize;

    friend class OsmAnd::OfflineMapDataProvider;
    friend class OsmAnd::OfflineMapDataProvider_P;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OFFLINE_MAP_DATA_TILE_H_)
