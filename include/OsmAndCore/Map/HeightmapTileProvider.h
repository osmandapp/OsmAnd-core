#ifndef _OSMAND_CORE_HEIGHTMAP_TILE_PROVIDER_H_
#define _OSMAND_CORE_HEIGHTMAP_TILE_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QDir>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/IMapElevationDataProvider.h>

namespace OsmAnd
{
    class HeightmapTileProvider_P;
    class OSMAND_CORE_API HeightmapTileProvider : public IMapElevationDataProvider
    {
        Q_DISABLE_COPY(HeightmapTileProvider);
    private:
        PrivateImplementation<HeightmapTileProvider_P> _p;
    protected:
    public:
        HeightmapTileProvider(const QDir& dataPath, const QString& indexFilepath = QString());
        virtual ~HeightmapTileProvider();

        void rebuildTileDbIndex();

        static const QString defaultIndexFilename;

        virtual uint32_t getTileSize() const;
        virtual bool obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const MapTile>& outTile, const IQueryController* const queryController);

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;
    };
}

#endif // !defined(_OSMAND_CORE_HEIGHTMAP_TILE_PROVIDER_H_)
