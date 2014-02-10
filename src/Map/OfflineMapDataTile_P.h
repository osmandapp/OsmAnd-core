#ifndef _OSMAND_CORE_OFFLINE_MAP_DATA_TILE_P_H_
#define _OSMAND_CORE_OFFLINE_MAP_DATA_TILE_P_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <CommonTypes.h>
#include <OfflineMapDataProvider_P.h>

namespace OsmAnd {

    class RasterizerContext;
    namespace Model {
        class MapObject;
    }

    class OfflineMapDataTile;
    class OfflineMapDataTile_P
    {
    private:
    protected:
        OfflineMapDataTile_P(OfflineMapDataTile* owner);

        OfflineMapDataTile* const owner;

        std::weak_ptr<OfflineMapDataProvider_P::Link> _link;
        std::weak_ptr<OfflineMapDataProvider_P::TileEntry> _refEntry;

        QList< std::shared_ptr<const Model::MapObject> > _mapObjects;
        std::shared_ptr< const RasterizerContext > _rasterizerContext;

        void cleanup();
    public:
        virtual ~OfflineMapDataTile_P();

    friend class OsmAnd::OfflineMapDataTile;
    friend class OsmAnd::OfflineMapDataProvider_P;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OFFLINE_MAP_DATA_TILE_P_H_)
