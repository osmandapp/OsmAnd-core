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

        class UniqueBinaryMapObjectId
        {
        private:
        protected:
        public:
            inline UniqueBinaryMapObjectId(const ObfMapSectionReader::DataBlockId& blockId_, const ObfObjectId objectId_)
                : blockId(blockId_)
                , objectId(objectId_)
            {
            }
            
            inline virtual ~UniqueBinaryMapObjectId()
            {
            }

            ObfMapSectionReader::DataBlockId blockId;
            ObfObjectId objectId;
            
            inline operator uint64_t() const
            {
                const int prime = 31;
                int result = 1;
                result = prime * result + qHash(static_cast<uint64_t>(blockId));
                result = prime * result + qHash(static_cast<uint64_t>(objectId));
                return result;
            }
    
            inline UniqueBinaryMapObjectId& operator=(const UniqueBinaryMapObjectId& that)
            {
                blockId.id = that.blockId.id;
                objectId.id = that.objectId.id;
                return *this;
            }

            inline bool operator==(const UniqueBinaryMapObjectId& that) const
            {
                return blockId == that.blockId && objectId == that.objectId;
            }

            inline bool operator!=(const UniqueBinaryMapObjectId& that) const
            {
                return blockId != that.blockId || objectId != that.objectId;
            }
        };

        class UniqueRoadId
        {
        private:
        protected:
        public:
            inline UniqueRoadId(const ObfRoutingSectionReader::DataBlockId& blockId_, const ObfObjectId objectId_)
                : blockId(blockId_)
                , objectId(objectId_)
            {
            }
            
            inline virtual ~UniqueRoadId()
            {
            }

            ObfRoutingSectionReader::DataBlockId blockId;
            ObfObjectId objectId;
            
            inline operator uint64_t() const
            {
                const int prime = 31;
                int result = 1;
                result = prime * result + qHash(static_cast<uint64_t>(blockId));
                result = prime * result + qHash(static_cast<uint64_t>(objectId));
                return result;
            }
    
            inline UniqueRoadId& operator=(const UniqueRoadId& that)
            {
                blockId.id = that.blockId.id;
                objectId.id = that.objectId.id;
                return *this;
            }

            inline bool operator==(const UniqueRoadId& that) const
            {
                return blockId == that.blockId && objectId == that.objectId;
            }

            inline bool operator!=(const UniqueRoadId& that) const
            {
                return blockId != that.blockId || objectId != that.objectId;
            }
        };

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
        mutable SharedByZoomResourcesContainer<UniqueBinaryMapObjectId, const BinaryMapObject> _sharedBinaryMapObjects;

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
        mutable SharedResourcesContainer<UniqueRoadId, const Road> _sharedRoads;

        enum class TileState
        {
            Undefined = -1,

            Loading,
            Loaded,
            Cancelled
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
        const ZoomLevel _coastlineZoom = ZoomLevel::ZoomLevel13;
        mutable TiledEntriesCollection<TileSharedEntry> _coastlineReferences;

        typedef OsmAnd::Link<ObfMapObjectsProvider_P*> Link;
        std::shared_ptr<Link> _link;

        struct RetainableCacheMetadata : public IMapDataProvider::RetainableCacheMetadata
        {
            RetainableCacheMetadata(
                const ZoomLevel zoom,
                const std::shared_ptr<Link>& link,
                const std::shared_ptr<TileSharedEntry>& tileSharedEntry,
                const std::shared_ptr<ObfMapSectionReader::DataBlocksCache>& binaryMapObjectsDataBlocksCache,
                const QList< std::shared_ptr<const ObfMapSectionReader::DataBlock> >& referencedBinaryMapObjectsDataBlocks,
                const QList< std::shared_ptr<const BinaryMapObject> >& referencedBinaryMapObjects,
                const std::shared_ptr<ObfRoutingSectionReader::DataBlocksCache>& roadsDataBlocksCache,
                const QList< std::shared_ptr<const ObfRoutingSectionReader::DataBlock> >& referencedRoadsDataBlocks,
                const QList< std::shared_ptr<const Road> >& referencedRoads);
            virtual ~RetainableCacheMetadata();

            ZoomLevel zoom;
            Link::WeakEnd weakLink;
            std::weak_ptr<TileSharedEntry> tileEntryWeakRef;

            std::weak_ptr<ObfMapSectionReader::DataBlocksCache> binaryMapObjectsDataBlocksCacheWeakRef;
            QList< std::shared_ptr<const ObfMapSectionReader::DataBlock> > referencedBinaryMapObjectsDataBlocks;
            QList< std::shared_ptr<const BinaryMapObject> > referencedBinaryMapObjects;

            std::weak_ptr<ObfRoutingSectionReader::DataBlocksCache> roadsDataBlocksCacheWeakRef;
            QList< std::shared_ptr<const ObfRoutingSectionReader::DataBlock> > referencedRoadsDataBlocks;
            QList< std::shared_ptr<const Road> > referencedRoads;
        };

        static QString getObfSectionDate(const std::shared_ptr<const ObfSectionInfo>& sectionInfo);
        static QString formatObfSectionName(const std::shared_ptr<const ObfSectionInfo>& sectionInfo, const bool withDate);

        void acquireThreadLock();
        void releaseThreadLock();

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
