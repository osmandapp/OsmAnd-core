#ifndef _OSMAND_CORE_GEO_TILE_EVALUATOR_P_H_
#define _OSMAND_CORE_GEO_TILE_EVALUATOR_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QDir>
#include <QMutex>
#include <QQueue>
#include <QSet>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "LatLon.h"
#include "PrivateImplementation.h"
#include "GeoTileEvaluator.h"

namespace OsmAnd
{
    class GeoTileEvaluator_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(GeoTileEvaluator_P);
    private:
    protected:
        GeoTileEvaluator_P(GeoTileEvaluator* const owner);
        
    public:
        ~GeoTileEvaluator_P();

        ImplementationInterface<GeoTileEvaluator> owner;
        
        bool evaluate(
            const LatLon& latLon,
            QList<double>& outData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

    friend class OsmAnd::GeoTileEvaluator;
    };
}

#endif // !defined(_OSMAND_CORE_GEO_TILE_EVALUATOR_P_H_)
