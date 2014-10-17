#ifndef _OSMAND_CORE_BINARY_MAP_PRIMITIVES_METRICS_LAYER_PROVIDER_P_H_
#define _OSMAND_CORE_BINARY_MAP_PRIMITIVES_METRICS_LAYER_PROVIDER_P_H_

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
#include "BinaryMapPrimitivesMetricsLayerProvider.h"

namespace OsmAnd
{
    class BinaryMapPrimitivesMetricsLayerProvider_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(BinaryMapPrimitivesMetricsLayerProvider_P);
    private:
    protected:
        BinaryMapPrimitivesMetricsLayerProvider_P(BinaryMapPrimitivesMetricsLayerProvider* const owner);

        struct RetainableCacheMetadata : public IMapDataProvider::RetainableCacheMetadata
        {
            RetainableCacheMetadata(
                const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& binaryMapPrimitivesRetainableCacheMetadata);
            virtual ~RetainableCacheMetadata();

            std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata> binaryMapPrimitivesRetainableCacheMetadata;
        };
    public:
        ~BinaryMapPrimitivesMetricsLayerProvider_P();

        ImplementationInterface<BinaryMapPrimitivesMetricsLayerProvider> owner;

        bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<BinaryMapPrimitivesMetricsLayerProvider::Data>& outTiledData,
            const IQueryController* const queryController);

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;

    friend class OsmAnd::BinaryMapPrimitivesMetricsLayerProvider;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_PRIMITIVES_METRICS_LAYER_PROVIDER_P_H_)
