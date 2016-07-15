#ifndef _OSMAND_CORE_MAP_OBJECTS_PROVIDER_P_H_
#define _OSMAND_CORE_MAP_OBJECTS_PROVIDER_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QMap>
#include <QList>
#include <QMutex>
#include <QReadWriteLock>
#include <QWaitCondition>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "TiledEntriesCollection.h"
#include "MapObjectsProvider.h"
#include "QuadTree.h"

namespace OsmAnd
{
    class MapObject;

    class MapObjectsProvider_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(MapObjectsProvider_P)

    private:
    protected:
        MapObjectsProvider_P(MapObjectsProvider* const owner);

        bool prepareData();

        typedef QuadTree< std::shared_ptr<const MapObject>, int32_t > MapObjectsTree;

        struct PreparedData
        {
            ZoomLevel minZoom;
            ZoomLevel maxZoom;
            AreaI bbox31;

            struct DataByZoomLevel
            {
                AreaI bbox31;

                QList< std::shared_ptr<const MapObject> > mapObjectsList;
                MapObjectsTree mapObjectsTree;
            };
            QMap<ZoomLevel, DataByZoomLevel> dataByZoomLevel;
        };
        Ref<PreparedData> _preparedData;

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
            std::weak_ptr<MapObjectsProvider::Data> dataWeakRef;

            QReadWriteLock loadedConditionLock;
            QWaitCondition loadedCondition;
        };
        mutable TiledEntriesCollection<TileEntry> _tileReferences;

        struct RetainableCacheMetadata : public IMapDataProvider::RetainableCacheMetadata
        {
            RetainableCacheMetadata(
                const std::shared_ptr<TileEntry>& tileEntry);
            virtual ~RetainableCacheMetadata();

            std::weak_ptr<TileEntry> tileEntryWeakRef;
        };
    public:
        virtual ~MapObjectsProvider_P();

        ImplementationInterface<MapObjectsProvider> owner;

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;

        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric);

    friend class OsmAnd::MapObjectsProvider;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_OBJECTS_PROVIDER_P_H_)
