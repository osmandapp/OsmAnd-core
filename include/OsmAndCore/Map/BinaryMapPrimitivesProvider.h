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
            std::shared_ptr<MapTiledData>& outTiledData,
            const IQueryController* const queryController = nullptr);

        bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<MapTiledData>& outTiledData,
            BinaryMapPrimitivesProvider_Metrics::Metric_obtainData* const metric,
            const IQueryController* const queryController = nullptr);
    };

    class BinaryMapPrimitivesTile_P;
    class OSMAND_CORE_API BinaryMapPrimitivesTile : public MapTiledData
    {
        Q_DISABLE_COPY_AND_MOVE(BinaryMapPrimitivesTile);
    private:
        PrivateImplementation<BinaryMapPrimitivesTile_P> _p;
    protected:
        BinaryMapPrimitivesTile(
            const std::shared_ptr<const BinaryMapDataTile> dataTile,
            const std::shared_ptr<const Primitiviser::PrimitivisedArea> primitivisedArea,
            const TileId tileId,
            const ZoomLevel zoom);
    public:
        virtual ~BinaryMapPrimitivesTile();

        const std::shared_ptr<const BinaryMapDataTile> dataTile;
        const std::shared_ptr<const Primitiviser::PrimitivisedArea> primitivisedArea;

        virtual void releaseConsumableContent();

    friend class OsmAnd::BinaryMapPrimitivesProvider;
    friend class OsmAnd::BinaryMapPrimitivesProvider_P;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_PRIMITIVES_PROVIDER_H_)
