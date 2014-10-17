#ifndef _OSMAND_CORE_BINARY_MAP_DATA_METRICS_LAYER_PROVIDER_P_H_
#define _OSMAND_CORE_BINARY_MAP_DATA_METRICS_LAYER_PROVIDER_P_H_

#include "stdlib_common.h"
#include <functional>
#include <array>

#include "QtExtensions.h"
#include <QMutex>
#include <QSet>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "IMapDataProvider.h"
#include "BinaryMapDataMetricsLayerProvider.h"

namespace OsmAnd
{
    class IQueryController;

    class BinaryMapDataMetricsLayerProvider_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(BinaryMapDataMetricsLayerProvider_P);
    private:
    protected:
        BinaryMapDataMetricsLayerProvider_P(BinaryMapDataMetricsLayerProvider* const owner);

        struct RetainableCacheMetadata : public IMapDataProvider::RetainableCacheMetadata
        {
            RetainableCacheMetadata(
                const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& binaryMapRetainableCacheMetadata);
            virtual ~RetainableCacheMetadata();

            std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata> binaryMapRetainableCacheMetadata;
        };
    public:
        ~BinaryMapDataMetricsLayerProvider_P();

        ImplementationInterface<BinaryMapDataMetricsLayerProvider> owner;

        bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<BinaryMapDataMetricsLayerProvider::Data>& outTiledData,
            const IQueryController* const queryController);

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;

    friend class OsmAnd::BinaryMapDataMetricsLayerProvider;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_DATA_METRICS_LAYER_PROVIDER_P_H_)
