#ifndef _OSMAND_CORE_MAP_PRIMITIVES_PROVIDER_H_
#define _OSMAND_CORE_MAP_PRIMITIVES_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QList>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapTiledDataProvider.h>
#include <OsmAndCore/Map/MapPrimitiviser.h>
#include <OsmAndCore/Map/IMapObjectsProvider.h>
#include <OsmAndCore/Map/MapPrimitivesProvider_Metrics.h>

namespace OsmAnd
{
    class MapPrimitivesProvider_P;
    class OSMAND_CORE_API MapPrimitivesProvider : public IMapTiledDataProvider
    {
        Q_DISABLE_COPY_AND_MOVE(MapPrimitivesProvider);
    public:
        class OSMAND_CORE_API Data : public IMapTiledDataProvider::Data
        {
            Q_DISABLE_COPY_AND_MOVE(Data);
        private:
        protected:
        public:
            Data(
                const TileId tileId,
                const ZoomLevel zoom,
                const std::shared_ptr<const IMapObjectsProvider::Data>& mapObjectsData,
                const std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects>& primitivisedObjects,
                const RetainableCacheMetadata* const pRetainableCacheMetadata = nullptr);
            virtual ~Data();

            std::shared_ptr<const IMapObjectsProvider::Data> mapObjectsData;
            std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects> primitivisedObjects;
        };

        enum class Mode
        {
            AllObjectsWithoutPolygonFiltering,
            AllObjectsWithPolygonFiltering,
            WithoutSurface,
            WithSurface
        };

    private:
        PrivateImplementation<MapPrimitivesProvider_P> _p;
    protected:
    public:
        MapPrimitivesProvider(
            const std::shared_ptr<IMapObjectsProvider>& mapObjectsProvider,
            const std::shared_ptr<MapPrimitiviser>& primitiviser,
            const unsigned int tileSize = 256,
            const Mode mode = Mode::WithSurface);
        virtual ~MapPrimitivesProvider();

        const std::shared_ptr<IMapObjectsProvider> mapObjectsProvider;
        const std::shared_ptr<MapPrimitiviser> primitiviser;
        const unsigned int tileSize;
        const Mode mode;

        virtual ZoomLevel getMinZoom() const Q_DECL_OVERRIDE;
        virtual ZoomLevel getMaxZoom() const Q_DECL_OVERRIDE;

        virtual bool obtainTiledPrimitives(
            const Request& request,
            std::shared_ptr<Data>& outTiledPrimitives,
            MapPrimitivesProvider_Metrics::Metric_obtainData* metric = nullptr);

        virtual bool supportsNaturalObtainData() const Q_DECL_OVERRIDE;
        virtual bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr) Q_DECL_OVERRIDE;

        virtual bool supportsNaturalObtainDataAsync() const Q_DECL_OVERRIDE;
        virtual void obtainDataAsync(
            const IMapDataProvider::Request& request,
            const IMapDataProvider::ObtainDataAsyncCallback callback,
            const bool collectMetric = false) Q_DECL_OVERRIDE;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_PRIMITIVES_PROVIDER_H_)
