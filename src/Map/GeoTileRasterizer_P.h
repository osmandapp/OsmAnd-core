#ifndef _OSMAND_CORE_GEO_TILE_RASTERIZER_P_H_
#define _OSMAND_CORE_GEO_TILE_RASTERIZER_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QDir>
#include <QMutex>
#include <QQueue>
#include <QSet>
#include <QRandomGenerator>
#include <gdal.h>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "GeoTileRasterizer.h"

namespace OsmAnd
{
    class GeoTileRasterizer_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(GeoTileRasterizer_P);
    private:
        mutable QMutex _dataMutex;

        static QString doubleToStr(double value);

        QHash<BandIndex, sk_sp<const SkImage>> rasterize(
            QHash<BandIndex, QByteArray>& outEncImgData,
            bool fillEncImgData = false,
            std::shared_ptr<Metric>* const pOutMetric = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

        sk_sp<SkImage> rasterizeContours(
            GDALDatasetH hDataset,
            const OsmAnd::BandIndex band,
            double levelInterval,
            sk_sp<const SkImage> image,
            const AreaI tileBBox31,
            const ZoomLevel zoom);

        QHash<BandIndex, sk_sp<const SkImage>> rasterizeContours(
            QHash<BandIndex, QByteArray>& outEncImgData,
            bool fillEncImgData = false,
            std::shared_ptr<Metric>* const pOutMetric = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);
        
        QList<std::shared_ptr<GeoContour>> evaluateBandContours(
            GDALDatasetH hDataset,
            const OsmAnd::BandIndex band,
            QList<double>& levels,
            const AreaI tileBBox31,
            const ZoomLevel zoom);

        QVector<QVector<PointI>> calculateVisibleSegments(const QVector<PointI>& points, const AreaI bbox31) const;

    protected:
        GeoTileRasterizer_P(GeoTileRasterizer* const owner);
        
    public:
        ~GeoTileRasterizer_P();

        ImplementationInterface<GeoTileRasterizer> owner;

        QHash<BandIndex, sk_sp<const SkImage>> rasterize(
            std::shared_ptr<Metric>* const pOutMetric = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

        QHash<BandIndex, sk_sp<const SkImage>> rasterize(
            QHash<BandIndex, QByteArray>& outEncImgData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

        QHash<BandIndex, sk_sp<const SkImage>> rasterizeContours(
            std::shared_ptr<Metric>* const pOutMetric = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

        QHash<BandIndex, sk_sp<const SkImage>> rasterizeContours(
            QHash<BandIndex, QByteArray>& outEncImgData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

        QHash<BandIndex, QList<std::shared_ptr<GeoContour>>> evaluateContours(
            std::shared_ptr<Metric>* const pOutMetric = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);
        
        static sk_sp<SkImage> rasterizeBandContours(
            const QList<std::shared_ptr<GeoContour>>& contours,
            const TileId tileId,
            const ZoomLevel zoom,
            const int width,
            const int height);

    friend class OsmAnd::GeoTileRasterizer;
    };
}

#endif // !defined(_OSMAND_CORE_GEO_TILE_RASTERIZER_P_H_)
