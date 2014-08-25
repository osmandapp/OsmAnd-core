#ifndef _OSMAND_CORE_MAP_RENDERER_KEYED_RESOURCES_COLLECTION_H_
#define _OSMAND_CORE_MAP_RENDERER_KEYED_RESOURCES_COLLECTION_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "MapRendererResourceType.h"
#include "MapRendererResourceState.h"
#include "KeyedEntriesCollection.h"
#include "MapRendererBaseKeyedResource.h"
#include "MapRendererBaseResourcesCollection.h"
#include "IMapKeyedDataProvider.h"
#include "IMapRendererKeyedResourcesCollection.h"

namespace OsmAnd
{
    class MapRendererResourcesManager;

    // Keyed resources collection:
    class MapRendererKeyedResourcesCollection
        : public MapRendererBaseResourcesCollection
        , public IMapRendererKeyedResourcesCollection
        , public KeyedEntriesCollection<IMapRendererKeyedResourcesCollection::Key, MapRendererBaseKeyedResource >
    {
    public:
        typedef IMapRendererKeyedResourcesCollection::Key Key;

        class Snapshot
            : public IMapRendererResourcesCollection
            , public IMapRendererKeyedResourcesCollection
        {
        private:
        protected:
            Snapshot(const MapRendererResourceType type);

            mutable QReadWriteLock _lock;
            MapRendererKeyedResourcesCollection::Storage _storage;
        public:
            virtual ~Snapshot();

            const MapRendererResourceType type;
            virtual MapRendererResourceType getType() const;

            virtual int getResourcesCount() const;
            virtual void forEachResourceExecute(const ResourceActionCallback action);
            virtual void obtainResources(QList< std::shared_ptr<MapRendererBaseResource> >* outList, const ResourceFilterCallback filter);

            virtual QList<Key> getKeys() const;

            virtual bool obtainResource(
                const Key key,
                std::shared_ptr<MapRendererBaseKeyedResource>& outResource) const;

        friend class OsmAnd::MapRendererKeyedResourcesCollection;
        };

    private:
    protected:
        MapRendererKeyedResourcesCollection(const MapRendererResourceType type);

        void verifyNoUploadedResourcesPresent() const;
        virtual void removeAllEntries();

        const std::shared_ptr<Snapshot> _snapshot;
        mutable QAtomicInt _collectionSnapshotInvalidatesCount;
        virtual void onCollectionModified() const;
    public:
        virtual ~MapRendererKeyedResourcesCollection();

        virtual QList<Key> getKeys() const;

        virtual bool obtainResource(
            const Key key,
            std::shared_ptr<MapRendererBaseKeyedResource>& outResource) const;

        virtual int getResourcesCount() const;
        virtual void forEachResourceExecute(const ResourceActionCallback action);
        virtual void obtainResources(QList< std::shared_ptr<MapRendererBaseResource> >* outList, const ResourceFilterCallback filter);
        virtual void removeResources(const ResourceFilterCallback filter);

        virtual bool updateCollectionSnapshot() const;
        virtual bool collectionSnapshotInvalidated() const;
        virtual std::shared_ptr<const IMapRendererResourcesCollection> getCollectionSnapshot() const;
        virtual std::shared_ptr<IMapRendererResourcesCollection> getCollectionSnapshot();

    friend class OsmAnd::MapRendererResourcesManager;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_KEYED_RESOURCES_COLLECTION_H_)
