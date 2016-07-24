#ifndef _OSMAND_CORE_OBF_MAP_OBJECTS_METRICS_LAYER_PROVIDER_P_H_
#define _OSMAND_CORE_OBF_MAP_OBJECTS_METRICS_LAYER_PROVIDER_P_H_

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
#include "ObfMapObjectsMetricsLayerProvider.h"

namespace OsmAnd
{
    class IQueryController;

    class ObfMapObjectsMetricsLayerProvider_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(ObfMapObjectsMetricsLayerProvider_P)
    private:
    protected:
        ObfMapObjectsMetricsLayerProvider_P(ObfMapObjectsMetricsLayerProvider* const owner);

        struct RetainableCacheMetadata : public IMapDataProvider::RetainableCacheMetadata
        {
            RetainableCacheMetadata(
                const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& binaryMapRetainableCacheMetadata);
            virtual ~RetainableCacheMetadata();

            std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata> binaryMapRetainableCacheMetadata;
        };
    public:
        ~ObfMapObjectsMetricsLayerProvider_P();

        ImplementationInterface<ObfMapObjectsMetricsLayerProvider> owner;

        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric);

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;

    friend class OsmAnd::ObfMapObjectsMetricsLayerProvider;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_MAP_OBJECTS_METRICS_LAYER_PROVIDER_P_H_)
