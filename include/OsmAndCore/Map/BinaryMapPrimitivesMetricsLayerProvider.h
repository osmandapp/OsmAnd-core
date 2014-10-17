#ifndef _OSMAND_CORE_BINARY_MAP_PRIMITIVES_METRICS_LAYER_PROVIDER_H_
#define _OSMAND_CORE_BINARY_MAP_PRIMITIVES_METRICS_LAYER_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>
#include <array>

#include <OsmAndCore/QtExtensions.h>
#include <QMutex>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/IRasterMapLayerProvider.h>
#include <OsmAndCore/Map/BinaryMapPrimitivesProvider.h>

namespace OsmAnd
{
    class BinaryMapPrimitivesMetricsLayerProvider_P;
    class OSMAND_CORE_API BinaryMapPrimitivesMetricsLayerProvider : public IRasterMapLayerProvider
    {
        Q_DISABLE_COPY_AND_MOVE(BinaryMapPrimitivesMetricsLayerProvider);
    public:
        class OSMAND_CORE_API Data : public IRasterMapLayerProvider::Data
        {
            Q_DISABLE_COPY_AND_MOVE(Data);
        private:
        protected:
        public:
            Data(
                const TileId tileId,
                const ZoomLevel zoom,
                const AlphaChannelData alphaChannelData,
                const float densityFactor,
                const std::shared_ptr<const SkBitmap>& bitmap,
                const std::shared_ptr<const BinaryMapPrimitivesProvider::Data>& binaryMapPrimitives,
                const RetainableCacheMetadata* const pRetainableCacheMetadata = nullptr);
            virtual ~Data();

            std::shared_ptr<const BinaryMapPrimitivesProvider::Data> binaryMapPrimitives;
        };

    private:
        PrivateImplementation<BinaryMapPrimitivesMetricsLayerProvider_P> _p;
    protected:
    public:
        BinaryMapPrimitivesMetricsLayerProvider(
            const std::shared_ptr<BinaryMapPrimitivesProvider>& privitivesProvider,
            const uint32_t tileSize = 256,
            const float densityFactor = 1.0f);
        virtual ~BinaryMapPrimitivesMetricsLayerProvider();

        const std::shared_ptr<BinaryMapPrimitivesProvider> primitivesProvider;
        const uint32_t tileSize;
        const float densityFactor;

        virtual float getTileDensityFactor() const;
        virtual uint32_t getTileSize() const;

        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
            const IQueryController* const queryController = nullptr);

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_PRIMITIVES_METRICS_LAYER_PROVIDER_H_)
