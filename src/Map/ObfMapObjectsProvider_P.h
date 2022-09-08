#ifndef _OSMAND_CORE_OBF_MAP_DATA_PROVIDER_P_H_
#define _OSMAND_CORE_OBF_MAP_DATA_PROVIDER_P_H_

#include "stdlib_common.h"
#include <utility>

#include "QtExtensions.h"
#include <QHash>
#include <QAtomicInt>
#include <QMutex>
#include <QReadWriteLock>
#include <QWaitCondition>

#include "OsmAndCore.h"
#include "Link.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "IMapTiledDataProvider.h"
#include "TiledEntriesCollection.h"
#include "SharedByZoomResourcesContainer.h"
#include "ObfMapSectionReader.h"
#include "ObfRoutingSectionReader.h"
#include "ObfMapObjectsProvider.h"
#include "ObfMapObjectsProvider_Metrics.h"

namespace OsmAnd
{
    class BinaryMapObject;
    class Road;

    class ObfMapObjectsProvider_P /*Q_DECL_FINAL*/
    {
    private:
    protected:
        ObfMapObjectsProvider_P(ObfMapObjectsProvider* owner);

        class BinaryMapObjectsDataBlocksCache : public ObfMapSectionReader::DataBlocksCache
        {
            Q_DISABLE_COPY_AND_MOVE(BinaryMapObjectsDataBlocksCache);
        private:
        protected:
        public:
            BinaryMapObjectsDataBlocksCache(const bool cacheTileInnerDataBlocks);
            virtual ~BinaryMapObjectsDataBlocksCache();

            const bool cacheTileInnerDataBlocks;

            virtual bool shouldCacheBlock(
                const DataBlockId id,
                const AreaI blockBBox31,
                const AreaI* const queryArea31 = nullptr) const;
        };
        const std::shared_ptr<ObfMapSectionReader::DataBlocksCache> _binaryMapObjectsDataBlocksCache;
        mutable SharedByZoomResourcesContainer<ObfObjectId, const BinaryMapObject> _sharedBinaryMapObjects;

        class RoadsDataBlocksCache : public ObfRoutingSectionReader::DataBlocksCache
        {
            Q_DISABLE_COPY_AND_MOVE(RoadsDataBlocksCache);
        private:
        protected:
        public:
            RoadsDataBlocksCache(const bool cacheTileInnerDataBlocks);
            virtual ~RoadsDataBlocksCache();

            const bool cacheTileInnerDataBlocks;

            virtual bool shouldCacheBlock(
                const DataBlockId id,
                const RoutingDataLevel dataLevel,
                const AreaI blockBBox31,
                const AreaI* const queryArea31 = nullptr) const;
        };
        const std::shared_ptr<ObfRoutingSectionReader::DataBlocksCache> _roadsDataBlocksCache;
        mutable SharedResourcesContainer<ObfObjectId, const Road> _sharedRoads;

        enum class TileState
        {
            Undefined = -1,

            Loading,
            Loaded
        };
        struct TileEntry : TiledEntriesCollectionEntryWithState < TileEntry, TileState, TileState::Undefined >
        {
            TileEntry(const TiledEntriesCollection<TileEntry>& collection, const TileId tileId, const ZoomLevel zoom)
                : TiledEntriesCollectionEntryWithState(collection, tileId, zoom)
            {
            }

            virtual ~TileEntry()
            {
                safeUnlink();
            }

            std::weak_ptr<ObfMapObjectsProvider::Data> dataWeakRef;

            QReadWriteLock loadedConditionLock;
            QWaitCondition loadedCondition;
        };
        struct TileSharedEntry : TiledEntriesCollectionEntryWithState < TileSharedEntry, TileState, TileState::Undefined >
        {
            TileSharedEntry(const TiledEntriesCollection<TileSharedEntry>& collection, const TileId tileId, const ZoomLevel zoom)
                : TiledEntriesCollectionEntryWithState(collection, tileId, zoom)
            {
            }

            virtual ~TileSharedEntry()
            {
                safeUnlink();
            }

            std::shared_ptr<ObfMapObjectsProvider::Data> dataSharedRef;

            QReadWriteLock loadedConditionLock;
            QWaitCondition loadedCondition;
        };
        mutable TiledEntriesCollection<TileEntry> _tileReferences;
        const ZoomLevel _coastlineZoom = ZoomLevel::ZoomLevel13;
        mutable TiledEntriesCollection<TileSharedEntry> _coastlineReferences;

        typedef OsmAnd::Link<ObfMapObjectsProvider_P*> Link;
        std::shared_ptr<Link> _link;

        struct RetainableCacheMetadata : public IMapDataProvider::RetainableCacheMetadata
        {
            RetainableCacheMetadata(
                const ZoomLevel zoom,
                const std::shared_ptr<Link>& link,
                const std::shared_ptr<TileEntry>& tileEntry,
                const std::shared_ptr<ObfMapSectionReader::DataBlocksCache>& binaryMapObjectsDataBlocksCache,
                const QList< std::shared_ptr<const ObfMapSectionReader::DataBlock> >& referencedBinaryMapObjectsDataBlocks,
                const QList< std::shared_ptr<const BinaryMapObject> >& referencedBinaryMapObjects,
                const std::shared_ptr<ObfRoutingSectionReader::DataBlocksCache>& roadsDataBlocksCache,
                const QList< std::shared_ptr<const ObfRoutingSectionReader::DataBlock> >& referencedRoadsDataBlocks,
                const QList< std::shared_ptr<const Road> >& referencedRoads);
            virtual ~RetainableCacheMetadata();

            ZoomLevel zoom;
            Link::WeakEnd weakLink;
            std::weak_ptr<TileEntry> tileEntryWeakRef;

            std::weak_ptr<ObfMapSectionReader::DataBlocksCache> binaryMapObjectsDataBlocksCacheWeakRef;
            QList< std::shared_ptr<const ObfMapSectionReader::DataBlock> > referencedBinaryMapObjectsDataBlocks;
            QList< std::shared_ptr<const BinaryMapObject> > referencedBinaryMapObjects;

            std::weak_ptr<ObfRoutingSectionReader::DataBlocksCache> roadsDataBlocksCacheWeakRef;
            QList< std::shared_ptr<const ObfRoutingSectionReader::DataBlock> > referencedRoadsDataBlocks;
            QList< std::shared_ptr<const Road> > referencedRoads;
        };
    public:
        ~ObfMapObjectsProvider_P();

        ImplementationInterface<ObfMapObjectsProvider> owner;

        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric);

        bool obtainTiledObfMapObjects(
            const ObfMapObjectsProvider::Request& request,
            std::shared_ptr<ObfMapObjectsProvider::Data>& outMapObjects,
            ObfMapObjectsProvider_Metrics::Metric_obtainData* const metric);

    friend class OsmAnd::ObfMapObjectsProvider;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_MAP_DATA_PROVIDER_P_H_)
