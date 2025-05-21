#ifndef _OSMAND_CORE_MAP_RENDERER_TILED_RESOURCES_COLLECTION_H_
#define _OSMAND_CORE_MAP_RENDERER_TILED_RESOURCES_COLLECTION_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QReadWriteLock>

#include "OsmAndCore.h"
#include "MapRendererResourceType.h"
#include "MapRendererResourceState.h"
#include "TiledEntriesCollection.h"
#include "MapRendererBaseTiledResource.h"
#include "MapRendererBaseResourcesCollection.h"
#include "IMapRendererTiledResourcesCollection.h"

namespace OsmAnd
{
    class MapRendererResourcesManager;

    // Tiled resources collection
    class MapRendererTiledResourcesCollection
        : public MapRendererBaseResourcesCollection
        , public IMapRendererTiledResourcesCollection
        , public TiledEntriesCollection<MapRendererBaseTiledResource>
    {
    public:
        class Snapshot
            : public IMapRendererResourcesCollection
            , public IMapRendererTiledResourcesCollection
        {
        private:
        protected:
            Snapshot(const MapRendererResourceType type);

            mutable QReadWriteLock _lock;
            MapRendererTiledResourcesCollection::Storage _storage;
        public:
            virtual ~Snapshot();

            const MapRendererResourceType type;
            virtual MapRendererResourceType getType() const;

            virtual int getResourcesCount() const;
            virtual void forEachResourceExecute(const ResourceActionCallback action);
            virtual void obtainResources(
                QList< std::shared_ptr<MapRendererBaseResource> >* outList,
                const ResourceFilterCallback filter);

            virtual bool obtainResource(
                const TileId tileId,
                const ZoomLevel zoomLevel,
                std::shared_ptr<MapRendererBaseTiledResource>& outResource) const;
            virtual bool containsResource(
                const TileId tileId,
                const ZoomLevel zoomLevel,
                const TiledResourceAcceptorCallback filter = nullptr) const;
            virtual bool containsResources(const ZoomLevel zoomLevel) const;
            virtual void setLoadingState(const bool isLoading);
            virtual bool isLoading() const;

        friend class OsmAnd::MapRendererTiledResourcesCollection;
        };

    private:
    protected:
        MapRendererTiledResourcesCollection(const MapRendererResourceType type);

        void verifyNoUploadedResourcesPresent() const;
        virtual void removeAllEntries();

        volatile bool _someResourceIsLoading;
        const std::shared_ptr<Snapshot> _snapshot;
        mutable QAtomicInt _collectionSnapshotInvalidatesCount;
        virtual void onCollectionModified() const;
    public:
        virtual ~MapRendererTiledResourcesCollection();

        virtual bool obtainResource(
            const TileId tileId,
            const ZoomLevel zoomLevel,
            std::shared_ptr<MapRendererBaseTiledResource>& outResource) const;
        virtual bool containsResource(
            const TileId tileId,
            const ZoomLevel zoomLevel,
            const TiledResourceAcceptorCallback filter = nullptr) const;
        virtual bool containsResources(const ZoomLevel zoomLevel) const;
        virtual void setLoadingState(const bool isLoading);
        virtual bool isLoading() const;

        virtual int getResourcesCount() const;
        virtual void forEachResourceExecute(const ResourceActionCallback action);
        virtual void obtainResources(QList< std::shared_ptr<MapRendererBaseResource> >* outList, const ResourceFilterCallback filter);
        virtual void removeResources(const ResourceFilterCallback filter);

        virtual bool updateCollectionSnapshot() const;
        virtual bool collectionSnapshotInvalidated() const;
        virtual std::shared_ptr<const IMapRendererResourcesCollection> getCollectionSnapshot() const;
        virtual std::shared_ptr<IMapRendererResourcesCollection> getCollectionSnapshot();

        void requestNeededTiledResources(const QSet<TileId>& activeTiles, const ZoomLevel activeZoom);

    friend class OsmAnd::MapRendererResourcesManager;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_TILED_RESOURCES_COLLECTION_H_)
