#ifndef _OSMAND_CORE_BINARY_MAP_PRIMITIVES_PROVIDER_H_
#define _OSMAND_CORE_BINARY_MAP_PRIMITIVES_PROVIDER_H_

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
#include <OsmAndCore/Map/BinaryMapDataProvider.h>
#include <OsmAndCore/Map/BinaryMapPrimitivesProvider_Metrics.h>

namespace OsmAnd
{
    class BinaryMapPrimitivesProvider_P;
    class OSMAND_CORE_API BinaryMapPrimitivesProvider : public IMapTiledDataProvider
    {
        Q_DISABLE_COPY_AND_MOVE(BinaryMapPrimitivesProvider);
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
                const std::shared_ptr<const BinaryMapDataProvider::Data>& binaryMapData,
                const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivisedArea,
                const RetainableCacheMetadata* const pRetainableCacheMetadata = nullptr);
            virtual ~Data();

            std::shared_ptr<const BinaryMapDataProvider::Data> binaryMapData;
            std::shared_ptr<const Primitiviser::PrimitivisedArea> primitivisedArea;
        };

    private:
        PrivateImplementation<BinaryMapPrimitivesProvider_P> _p;
    protected:
    public:
        BinaryMapPrimitivesProvider(
            const std::shared_ptr<BinaryMapDataProvider>& dataProvider,
            const std::shared_ptr<Primitiviser>& primitiviser,
            const unsigned int tileSize = 256);
        virtual ~BinaryMapPrimitivesProvider();

        const std::shared_ptr<BinaryMapDataProvider> dataProvider;
        const std::shared_ptr<Primitiviser> primitiviser;
        const unsigned int tileSize;

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;

        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
            const IQueryController* const queryController = nullptr);

        bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<Data>& outTiledData,
            BinaryMapPrimitivesProvider_Metrics::Metric_obtainData* const metric,
            const IQueryController* const queryController);
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_PRIMITIVES_PROVIDER_H_)
