#ifndef _OSMAND_CORE_GEO_TILE_RASTERIZER_H_
#define _OSMAND_CORE_GEO_TILE_RASTERIZER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QHash>
#include <QList>
#include <QString>
#include <QDir>
#include <QDateTime>

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <SkImage.h>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Metrics.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/IQueryController.h>
#include <OsmAndCore/Map/GeoCommonTypes.h>

namespace OsmAnd
{
    class GeoTileRasterizer_P;
    
    class OSMAND_CORE_API GeoTileRasterizer
        : public std::enable_shared_from_this<GeoTileRasterizer>
    {
        Q_DISABLE_COPY_AND_MOVE(GeoTileRasterizer);

    private:
        PrivateImplementation<GeoTileRasterizer_P> _p;
    protected:
    public:
        GeoTileRasterizer(
            const QByteArray& geoTileData,
            const TileId tileId,
            const ZoomLevel zoom,
            const QList<BandIndex>& bands,
            const QHash<BandIndex, QString>& colorProfiles,
            const uint32_t tileSize = 256,
            const float densityFactor = 1.0f,
            const QString& projSearchPath = QString());
        
        virtual ~GeoTileRasterizer();
        
        const QByteArray geoTileData;

        const TileId tileId;
        const ZoomLevel zoom;
        const QList<BandIndex> bands;
        const QHash<BandIndex, QString>& colorProfiles;

        const uint32_t tileSize;
        const float densityFactor;
        const QString projSearchPath;

        virtual QHash<BandIndex, sk_sp<const SkImage>> rasterize(
            std::shared_ptr<Metric>* const pOutMetric = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

        virtual QHash<BandIndex, sk_sp<const SkImage>> rasterize(
            QHash<BandIndex, QByteArray>& outEncImgData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_GEO_TILE_RASTERIZER_H_)
