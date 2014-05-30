#ifndef _OSMAND_CORE_HILLSHADE_TILE_PROVIDER_H_
#define _OSMAND_CORE_HILLSHADE_TILE_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QDir>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/IMapRasterBitmapTileProvider.h>

namespace OsmAnd
{
    class HillshadeTileProvider_P;
    class OSMAND_CORE_API HillshadeTileProvider : public IMapRasterBitmapTileProvider
    {
        Q_DISABLE_COPY(HillshadeTileProvider);
    private:
        PrivateImplementation<HillshadeTileProvider> _p;
    protected:
    public:
        HillshadeTileProvider(const QDir& storagePath);
        virtual ~HillshadeTileProvider();

        const QDir storagePath;
        
        void setIndexFilePath(const QString& indexFilePath);
        const QString& indexFilePath;

        void rebuildIndex();

        virtual float getTileDensity() const;
        virtual uint32_t getTileSize() const;

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;

        virtual bool obtainData(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const MapTile>& outTile);
    };
}

#endif // !defined(_OSMAND_CORE_HILLSHADE_TILE_PROVIDER_H_)
