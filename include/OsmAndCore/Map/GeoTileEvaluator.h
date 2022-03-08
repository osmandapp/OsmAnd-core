#ifndef _OSMAND_CORE_GEO_TILE_EVALUATOR_H_
#define _OSMAND_CORE_GEO_TILE_EVALUATOR_H_

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
#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/Metrics.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/IQueryController.h>
#include <OsmAndCore/Map/GeoCommonTypes.h>

namespace OsmAnd
{
    class GeoTileEvaluator_P;
    
    class OSMAND_CORE_API GeoTileEvaluator
        : public std::enable_shared_from_this<GeoTileEvaluator>
    {
        Q_DISABLE_COPY_AND_MOVE(GeoTileEvaluator);

    private:
        PrivateImplementation<GeoTileEvaluator_P> _p;
    protected:
    public:
        GeoTileEvaluator(
            const TileId geoTileId,
            const QByteArray& geoTileData,
            const ZoomLevel zoom,
            const QString& projSearchPath = QString());
                
        virtual ~GeoTileEvaluator();
        
        const TileId geoTileId;
        const QByteArray geoTileData;
        const ZoomLevel zoom;
        const QString projSearchPath;
        
        virtual bool evaluate(
            const LatLon& latLon,
            QHash<BandIndex, double>& outData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_GEO_TILE_EVALUATOR_H_)
