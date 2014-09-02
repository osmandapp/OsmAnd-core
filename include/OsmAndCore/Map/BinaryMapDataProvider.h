#ifndef _OSMAND_CORE_BINARY_MAP_DATA_PROVIDER_H_
#define _OSMAND_CORE_BINARY_MAP_DATA_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QList>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapTiledDataProvider.h>
#include <OsmAndCore/Map/BinaryMapDataProvider_Metrics.h>
#include <OsmAndCore/Data/ObfMapSectionReader.h>

namespace OsmAnd
{
    class IObfsCollection;
    class BinaryMapDataTile;
    namespace Model
    {
        class BinaryMapObject;
    }

    class BinaryMapDataProvider_P;
    class OSMAND_CORE_API BinaryMapDataProvider : public IMapTiledDataProvider
    {
        Q_DISABLE_COPY_AND_MOVE(BinaryMapDataProvider);
    private:
        PrivateImplementation<BinaryMapDataProvider_P> _p;
    protected:
    public:
        BinaryMapDataProvider(const std::shared_ptr<const IObfsCollection>& obfsCollection);
        virtual ~BinaryMapDataProvider();

        const std::shared_ptr<const IObfsCollection> obfsCollection;

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
            BinaryMapDataProvider_Metrics::Metric_obtainData* const metric,
            const IQueryController* const queryController);
    };

    class BinaryMapDataTile_P;
    class OSMAND_CORE_API BinaryMapDataTile : public MapTiledData
    {
        Q_DISABLE_COPY_AND_MOVE(BinaryMapDataTile);
    private:
        PrivateImplementation<BinaryMapDataTile_P> _p;
    protected:
        BinaryMapDataTile(
            const std::shared_ptr<ObfMapSectionReader::DataBlocksCache>& dataBlocksCache,
            const QList< std::shared_ptr<const ObfMapSectionReader::DataBlock> >& referencedDataBlocks,
            const MapFoundationType tileFoundation,
            const QList< std::shared_ptr<const Model::BinaryMapObject> >& mapObjects,
            const TileId tileId,
            const ZoomLevel zoom);
    public:
        virtual ~BinaryMapDataTile();

        const std::weak_ptr<ObfMapSectionReader::DataBlocksCache> dataBlocksCache;
        const QList< std::shared_ptr<const ObfMapSectionReader::DataBlock> >& referencedDataBlocks;

        const MapFoundationType tileFoundation;
        const QList< std::shared_ptr<const Model::BinaryMapObject> >& mapObjects;

        virtual void releaseConsumableContent();

    friend class OsmAnd::BinaryMapDataProvider;
    friend class OsmAnd::BinaryMapDataProvider_P;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_DATA_PROVIDER_H_)
