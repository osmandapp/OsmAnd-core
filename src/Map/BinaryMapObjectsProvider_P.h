#ifndef _OSMAND_CORE_BINARY_MAP_DATA_PROVIDER_P_H_
#define _OSMAND_CORE_BINARY_MAP_DATA_PROVIDER_P_H_

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
#include "BinaryMapObjectsProvider.h"
#include "BinaryMapObjectsProvider_Metrics.h"

namespace OsmAnd
{
    class BinaryMapObject;

    class BinaryMapObjectsProvider_P Q_DECL_FINAL
    {
    private:
    protected:
        BinaryMapObjectsProvider_P(BinaryMapObjectsProvider* owner);

        class DataBlocksCache : public ObfMapSectionReader::DataBlocksCache
        {
        private:
        protected:
        public:
            DataBlocksCache(const bool cacheTileInnerDataBlocks);
            virtual ~DataBlocksCache();

            const bool cacheTileInnerDataBlocks;

            virtual bool shouldCacheBlock(const DataBlockId id, const AreaI blockBBox31, const AreaI* const queryArea31 = nullptr) const;
        };
        const std::shared_ptr<ObfMapSectionReader::DataBlocksCache> _dataBlocksCache;

        mutable SharedByZoomResourcesContainer<ObfObjectId, const BinaryMapObject> _sharedMapObjects;

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

            std::weak_ptr<BinaryMapObjectsProvider::Data> dataWeakRef;

            QReadWriteLock loadedConditionLock;
            QWaitCondition loadedCondition;
        };
        mutable TiledEntriesCollection<TileEntry> _tileReferences;

        typedef OsmAnd::Link<BinaryMapObjectsProvider_P*> Link;
        std::shared_ptr<Link> _link;

        struct RetainableCacheMetadata : public IMapDataProvider::RetainableCacheMetadata
        {
            RetainableCacheMetadata(
                const ZoomLevel zoom,
                const std::shared_ptr<Link>& link,
                const std::shared_ptr<TileEntry>& tileEntry,
                const std::shared_ptr<ObfMapSectionReader::DataBlocksCache>& dataBlocksCache,
                const QList< std::shared_ptr<const ObfMapSectionReader::DataBlock> >& referencedDataBlocks,
                const QList< std::shared_ptr<const BinaryMapObject> >& referencedMapObjects);
            virtual ~RetainableCacheMetadata();

            ZoomLevel zoom;
            Link::WeakEnd weakLink;
            std::weak_ptr<TileEntry> tileEntryWeakRef;
            std::weak_ptr<ObfMapSectionReader::DataBlocksCache> dataBlocksCacheWeakRef;
            QList< std::shared_ptr<const ObfMapSectionReader::DataBlock> > referencedDataBlocks;
            QList< std::shared_ptr<const BinaryMapObject> > referencedMapObjects;
        };
    public:
        ~BinaryMapObjectsProvider_P();

        ImplementationInterface<BinaryMapObjectsProvider> owner;

        bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<BinaryMapObjectsProvider::Data>& outTiledData,
            BinaryMapObjectsProvider_Metrics::Metric_obtainData* const metric,
            const IQueryController* const queryController);

    friend class OsmAnd::BinaryMapObjectsProvider;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_DATA_PROVIDER_P_H_)
