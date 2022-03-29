#ifndef _OSMAND_CORE_GEO_TILE_OBJECTS_PROVIDER_P_H_
#define _OSMAND_CORE_GEO_TILE_OBJECTS_PROVIDER_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QMap>
#include <QList>
#include <QMutex>
#include <QReadWriteLock>
#include <QWaitCondition>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "TiledEntriesCollection.h"
#include "GeoTileObjectsProvider.h"
#include "QuadTree.h"

namespace OsmAnd
{
    class MapObject;

    class GeoTileObjectsProvider_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(GeoTileObjectsProvider_P);

    private:
        mutable QReadWriteLock _dataCacheLock;

        QList<std::shared_ptr<GeoTileObjectsProvider::Data>> _dataCache;
        
    protected:
        GeoTileObjectsProvider_P(GeoTileObjectsProvider* const owner);

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
                , dataIsPresent(false)
            {
            }

            virtual ~TileEntry()
            {
                safeUnlink();
            }

            bool dataIsPresent;
            std::weak_ptr<GeoTileObjectsProvider::Data> dataWeakRef;

            QReadWriteLock loadedConditionLock;
            QWaitCondition loadedCondition;
        };
        mutable TiledEntriesCollection<TileEntry> _tileReferences;
        
        void addDataToCache(const std::shared_ptr<GeoTileObjectsProvider::Data>& data);

    public:
        virtual ~GeoTileObjectsProvider_P();

        ImplementationInterface<GeoTileObjectsProvider> owner;

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;

        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric);

    friend class OsmAnd::GeoTileObjectsProvider;
    };
}

#endif // !defined(_OSMAND_CORE_GEO_TILE_OBJECTS_PROVIDER_P_H_)
