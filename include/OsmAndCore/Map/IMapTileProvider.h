#ifndef _OSMAND_CORE_I_MAP_TILE_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_TILE_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapProvider.h>

namespace OsmAnd
{
    STRONG_ENUM(MapTileDataType)
    {
        Bitmap,
        ElevationData
    } STRONG_ENUM_TERMINATOR;

    class OSMAND_CORE_API MapTile
    {
        Q_DISABLE_COPY(MapTile);
    public:
        typedef const void* DataPtr;
    private:
    protected:
        MapTile(const MapTileDataType& dataType, const DataPtr data, size_t rowLength, uint32_t size);

        DataPtr _data;
    public:
        virtual ~MapTile();

        const MapTileDataType dataType;

        const DataPtr& data;
        const size_t rowLength;
        const uint32_t size;
    };

    class OSMAND_CORE_API IMapTileProvider : public IMapProvider
    {
        Q_DISABLE_COPY(IMapTileProvider);
    private:
    protected:
        IMapTileProvider(const MapTileDataType& dataType);
    public:
        virtual ~IMapTileProvider();

        const MapTileDataType dataType;
        virtual uint32_t getTileSize() const = 0;

        virtual bool obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const MapTile>& outTile) = 0;
    };

}

#endif // !defined(_OSMAND_CORE_I_MAP_TILE_PROVIDER_H_)
