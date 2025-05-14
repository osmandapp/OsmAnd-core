#ifndef _OSMAND_CORE_MAP_PRIMITIVES_METRICS_LAYER_PROVIDER_P_H_
#define _OSMAND_CORE_MAP_PRIMITIVES_METRICS_LAYER_PROVIDER_P_H_

#include "stdlib_common.h"
#include <functional>
#include <array>

#include "QtExtensions.h"
#include <QMutex>
#include <QSet>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "IRasterMapLayerProvider.h"
#include "MapPrimitivesMetricsLayerProvider.h"

namespace OsmAnd
{
    class MapPrimitivesMetricsLayerProvider_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(MapPrimitivesMetricsLayerProvider_P);
    private:
    protected:
        uint32_t tileSize;
        MapPrimitivesMetricsLayerProvider_P(MapPrimitivesMetricsLayerProvider* const owner);

        struct RetainableCacheMetadata : public IMapDataProvider::RetainableCacheMetadata
        {
            RetainableCacheMetadata(
                const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& binaryMapPrimitivesRetainableCacheMetadata);
            virtual ~RetainableCacheMetadata();

            std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata> binaryMapPrimitivesRetainableCacheMetadata;
        };
    public:
        ~MapPrimitivesMetricsLayerProvider_P();

        ImplementationInterface<MapPrimitivesMetricsLayerProvider> owner;

        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric);

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;

    friend class OsmAnd::MapPrimitivesMetricsLayerProvider;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_PRIMITIVES_METRICS_LAYER_PROVIDER_P_H_)
