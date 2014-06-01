#ifndef _OSMAND_CORE_I_MAP_ELEVATION_DATA_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_ELEVATION_DATA_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapTiledDataProvider.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IMapElevationDataProvider : public IMapTiledDataProvider
    {
        Q_DISABLE_COPY(IMapElevationDataProvider);
    private:
    protected:
        IMapElevationDataProvider();
    public:
        virtual ~IMapElevationDataProvider();

        virtual uint32_t getTileSize() const = 0;
    };

    class OSMAND_CORE_API ElevationDataTile : public MapTiledData
    {
        Q_DISABLE_COPY(ElevationDataTile);
    public:
        typedef const float* DataPtr;

    private:
    protected:
    public:
        ElevationDataTile(
            const DataPtr data,
            const size_t rowLength,
            const uint32_t size,
            const TileId tileId,
            const ZoomLevel zoom);
        virtual ~ElevationDataTile();

        const DataPtr data;
        const size_t rowLength;
        const uint32_t size;

        virtual std::shared_ptr<MapData> createNoContentInstance() const;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_ELEVATION_DATA_PROVIDER_H_)
