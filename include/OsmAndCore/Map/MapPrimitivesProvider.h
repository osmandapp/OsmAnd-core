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
#include <OsmAndCore/Map/Primitiviser.h>
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
                const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivisedArea,
                const RetainableCacheMetadata* const pRetainableCacheMetadata = nullptr);
            virtual ~Data();

            std::shared_ptr<const IMapObjectsProvider::Data> mapObjectsData;
            std::shared_ptr<const Primitiviser::PrimitivisedArea> primitivisedArea;
        };

    private:
        PrivateImplementation<MapPrimitivesProvider_P> _p;
    protected:
    public:
        MapPrimitivesProvider(
            const std::shared_ptr<IMapObjectsProvider>& mapObjectsProvider,
            const std::shared_ptr<Primitiviser>& primitiviser,
            const unsigned int tileSize = 256);
        virtual ~MapPrimitivesProvider();

        const std::shared_ptr<IMapObjectsProvider> mapObjectsProvider;
        const std::shared_ptr<Primitiviser> primitiviser;
        const unsigned int tileSize;

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;

        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
            std::shared_ptr<Metric>* pOutMetric = nullptr,
            const IQueryController* const queryController = nullptr);

        bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<Data>& outTiledData,
            MapPrimitivesProvider_Metrics::Metric_obtainData* const metric,
            const IQueryController* const queryController);
    };
}

#endif // !defined(_OSMAND_CORE_MAP_PRIMITIVES_PROVIDER_H_)
