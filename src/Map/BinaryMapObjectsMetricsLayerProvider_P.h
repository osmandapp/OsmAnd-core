#ifndef _OSMAND_CORE_BINARY_MAP_OBJECTS_METRICS_LAYER_PROVIDER_P_H_
#define _OSMAND_CORE_BINARY_MAP_OBJECTS_METRICS_LAYER_PROVIDER_P_H_

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
#include "BinaryMapObjectsMetricsLayerProvider.h"

namespace OsmAnd
{
    class IQueryController;

    class BinaryMapObjectsMetricsLayerProvider_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(BinaryMapObjectsMetricsLayerProvider_P);
    private:
    protected:
        BinaryMapObjectsMetricsLayerProvider_P(BinaryMapObjectsMetricsLayerProvider* const owner);

        struct RetainableCacheMetadata : public IMapDataProvider::RetainableCacheMetadata
        {
            RetainableCacheMetadata(
                const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& binaryMapRetainableCacheMetadata);
            virtual ~RetainableCacheMetadata();

            std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata> binaryMapRetainableCacheMetadata;
        };
    public:
        ~BinaryMapObjectsMetricsLayerProvider_P();

        ImplementationInterface<BinaryMapObjectsMetricsLayerProvider> owner;

        bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<BinaryMapObjectsMetricsLayerProvider::Data>& outTiledData,
            const IQueryController* const queryController);

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;

    friend class OsmAnd::BinaryMapObjectsMetricsLayerProvider;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_OBJECTS_METRICS_LAYER_PROVIDER_P_H_)
