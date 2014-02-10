#ifndef _OSMAND_CORE_I_MAP_ELEVATION_DATA_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_ELEVATION_DATA_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapTileProvider.h>

namespace OsmAnd
{
    class OSMAND_CORE_API MapElevationDataTile : public MapTile
    {
        Q_DISABLE_COPY(MapElevationDataTile);
    private:
    protected:
    public:
        MapElevationDataTile(const float* data, size_t rowLength, uint32_t size);
        virtual ~MapElevationDataTile();
    };

    class OSMAND_CORE_API IMapElevationDataProvider : public IMapTileProvider
    {
        Q_DISABLE_COPY(IMapElevationDataProvider);
    private:
    protected:
        IMapElevationDataProvider();
    public:
        virtual ~IMapElevationDataProvider();      
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_ELEVATION_DATA_PROVIDER_H_)
