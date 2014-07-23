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

namespace OsmAnd
{
    namespace Model
    {
        class BinaryMapObject;
    }
    class BinaryMapDataTile;
    class BinaryMapDataTile_P;
    class RasterizerContext;

    class BinaryMapDataProvider;
    class BinaryMapDataProvider_P Q_DECL_FINAL
    {
    private:
    protected:
        BinaryMapDataProvider_P(BinaryMapDataProvider* owner);

        const std::shared_ptr<ObfMapSectionReader::DataBlocksCache> _dataBlocksCache;

        mutable SharedByZoomResourcesContainer<uint64_t, const Model::BinaryMapObject> _sharedMapObjects;

        enum class TileState
        {
            Undefined = -1,

            Loading,
            Loaded,
            Released
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

            std::weak_ptr<BinaryMapDataTile> _tile;

            QReadWriteLock _loadedConditionLock;
            QWaitCondition _loadedCondition;
        };
        mutable TiledEntriesCollection<TileEntry> _tileReferences;

        typedef Link<BinaryMapDataProvider_P*> Link;
        std::shared_ptr<Link> _link;
    public:
        ~BinaryMapDataProvider_P();

        ImplementationInterface<BinaryMapDataProvider> owner;

        bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<MapTiledData>& outTiledData,
            const IQueryController* const queryController);

        friend class OsmAnd::BinaryMapDataProvider;
        friend class OsmAnd::BinaryMapDataTile_P;
    };

    class BinaryMapDataTile;
    class BinaryMapDataTile_P Q_DECL_FINAL
    {
    private:
    protected:
        BinaryMapDataTile_P(BinaryMapDataTile* owner);

        BinaryMapDataProvider_P::Link::WeakEnd _weakLink;
        std::weak_ptr<BinaryMapDataProvider_P::TileEntry> _refEntry;

        QList< std::shared_ptr<const ObfMapSectionReader::DataBlock> > _referencedDataBlocks;

        QList< std::shared_ptr<const Model::BinaryMapObject> > _mapObjects;
        std::shared_ptr< const RasterizerContext > _rasterizerContext;

        void cleanup();
    public:
        virtual ~BinaryMapDataTile_P();

        ImplementationInterface<BinaryMapDataTile> owner;

    friend class OsmAnd::BinaryMapDataTile;
    friend class OsmAnd::BinaryMapDataProvider_P;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_DATA_PROVIDER_P_H_)
