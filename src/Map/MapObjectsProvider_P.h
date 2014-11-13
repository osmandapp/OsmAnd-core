#ifndef _OSMAND_CORE_MAP_OBJECTS_PROVIDER_P_H_
#define _OSMAND_CORE_MAP_OBJECTS_PROVIDER_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QMap>
#include <QList>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "MapObjectsProvider.h"

namespace OsmAnd
{
    class MapObject;

    class MapObjectsProvider_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(MapObjectsProvider_P);

    private:
    protected:
        MapObjectsProvider_P(MapObjectsProvider* const owner);

        bool prepareData();

        struct PreparedData
        {
            ZoomLevel minZoom;
            ZoomLevel maxZoom;

            QMap<ZoomLevel, QList< std::shared_ptr<const MapObject> > > mapObjectsByZoomLevel;
        };
        Ref<PreparedData> _preparedData;
    public:
        virtual ~MapObjectsProvider_P();

        ImplementationInterface<MapObjectsProvider> owner;

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;

        bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<MapObjectsProvider::Data>& outTiledData,
            const IQueryController* const queryController);

    friend class OsmAnd::MapObjectsProvider;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_OBJECTS_PROVIDER_P_H_)
