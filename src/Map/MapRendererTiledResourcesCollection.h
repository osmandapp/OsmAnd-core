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
        , public TiledEntriesCollection < MapRendererBaseTiledResource >
    {
    public:
        class Shadow
            : public IMapRendererResourcesCollection
            , public IMapRendererTiledResourcesCollection
        {
        private:
        protected:
            Shadow(const MapRendererResourceType type);

            mutable QReadWriteLock _lock;
            MapRendererTiledResourcesCollection::Storage _storage;
        public:
            virtual ~Shadow();

            const MapRendererResourceType type;
            virtual MapRendererResourceType getType() const;

            virtual int getResourcesCount() const;
            virtual void forEachResourceExecute(const ResourceActionCallback action);
            virtual void obtainResources(QList< std::shared_ptr<MapRendererBaseResource> >* outList, const ResourceFilterCallback filter);

            virtual bool obtainResource(
                const TileId tileId,
                const ZoomLevel zoomLevel,
                std::shared_ptr<MapRendererBaseTiledResource>& outResource) const;

        friend class OsmAnd::MapRendererTiledResourcesCollection;
        };

    private:
    protected:
        MapRendererTiledResourcesCollection(const MapRendererResourceType type);

        void verifyNoUploadedResourcesPresent() const;
        virtual void removeAllEntries();

        const std::shared_ptr<Shadow> _shadow;
        mutable QAtomicInt _shadowCollectionInvalidatesCount;
        virtual void onCollectionModified() const;
    public:
        virtual ~MapRendererTiledResourcesCollection();

        virtual bool obtainResource(
            const TileId tileId,
            const ZoomLevel zoomLevel,
            std::shared_ptr<MapRendererBaseTiledResource>& outResource) const;

        virtual int getResourcesCount() const;
        virtual void forEachResourceExecute(const ResourceActionCallback action);
        virtual void obtainResources(QList< std::shared_ptr<MapRendererBaseResource> >* outList, const ResourceFilterCallback filter);
        virtual void removeResources(const ResourceFilterCallback filter);

        virtual bool updateShadowCollection() const;
        virtual std::shared_ptr<const IMapRendererResourcesCollection> getShadowCollection() const;
        virtual std::shared_ptr<IMapRendererResourcesCollection> getShadowCollection();

    friend class OsmAnd::MapRendererResourcesManager;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_TILED_RESOURCES_COLLECTION_H_)
