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
        Q_DISABLE_COPY_AND_MOVE(HeightmapTileProvider);
    private:
        PrivateImplementation<HeightmapTileProvider_P> _p;
    protected:
    public:
        HeightmapTileProvider(const QString& dataPath, const QString& indexFilename = QString::null);
        virtual ~HeightmapTileProvider();

        const QString dataPath;
        const QString indexFilename;

        void rebuildTileDbIndex();

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;
        virtual uint32_t getTileSize() const;
        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<MapTiledData>& outTiledData,
            const IQueryController* const queryController = nullptr);

        static const QString defaultIndexFilename;
    };
}

#endif // !defined(_OSMAND_CORE_HEIGHTMAP_TILE_PROVIDER_H_)
