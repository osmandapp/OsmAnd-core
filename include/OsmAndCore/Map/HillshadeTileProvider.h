#ifndef _OSMAND_CORE_HILLSHADE_TILE_PROVIDER_H_
#define _OSMAND_CORE_HILLSHADE_TILE_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QDir>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

#include <OsmAndCore/Map/IMapBitmapTileProvider.h>

namespace OsmAnd
{
    class HillshadeTileProvider_P;
    class OSMAND_CORE_API HillshadeTileProvider : public IMapBitmapTileProvider
    {
        Q_DISABLE_COPY(HillshadeTileProvider);
    private:
        const std::unique_ptr<HillshadeTileProvider> _d;
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

        virtual bool obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const MapTile>& outTile);
    };
}

#endif // !defined(_OSMAND_CORE_HILLSHADE_TILE_PROVIDER_H_)
