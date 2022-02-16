#ifndef _OSMAND_CORE_GEO_TILE_RASTERIZER_P_H_
#define _OSMAND_CORE_GEO_TILE_RASTERIZER_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QDir>
#include <QMutex>
#include <QQueue>
#include <QSet>

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

    friend class OsmAnd::GeoTileRasterizer;
    };
}

#endif // !defined(_OSMAND_CORE_GEO_TILE_RASTERIZER_P_H_)
