#ifndef _OSMAND_CORE_I_MAP_TILED_DATA_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_TILED_DATA_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapDataProvider.h>
#include <OsmAndCore/IQueryController.h>

namespace OsmAnd
{
    class MapTiledData;

    // IMapTiledDataProvider describes minimal interface that providers with tile-based access must implement
    class OSMAND_CORE_API IMapTiledDataProvider : public IMapDataProvider
    {
        Q_DISABLE_COPY_AND_MOVE(IMapTiledDataProvider);
    private:
    protected:
        IMapTiledDataProvider(const DataType dataType);
    public:
        virtual ~IMapTiledDataProvider();

        virtual ZoomLevel getMinZoom() const = 0;
        virtual ZoomLevel getMaxZoom() const = 0;

        virtual bool obtainData(
            SWIG_OMIT(const) TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<MapTiledData>& outTiledData,
            const IQueryController* const queryController = nullptr) = 0;
    };

    class OSMAND_CORE_API MapTiledData : public MapData
    {
        Q_DISABLE_COPY_AND_MOVE(MapTiledData);
    private:
    protected:
        MapTiledData(const DataType dataType, const TileId tileId, const ZoomLevel zoom);
    public:
        virtual ~MapTiledData();

        const TileId tileId;
        const ZoomLevel zoom;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_TILED_DATA_PROVIDER_H_)
