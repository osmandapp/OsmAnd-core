#ifndef _OSMAND_CORE_I_MAP_OBJECTS_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_OBJECTS_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapTiledDataProvider.h>

namespace OsmAnd
{
    class MapObject;

    class OSMAND_CORE_API IMapObjectsProvider : public IMapTiledDataProvider
    {
        Q_DISABLE_COPY_AND_MOVE(IMapObjectsProvider)
    public:
        class OSMAND_CORE_API Data : public IMapTiledDataProvider::Data
        {
            Q_DISABLE_COPY_AND_MOVE(Data)
        private:
        protected:
        public:
            Data(
                const TileId tileId,
                const ZoomLevel zoom,
                const MapSurfaceType tileSurfaceType,
                const QList< std::shared_ptr<const MapObject> >& mapObjects,
                const RetainableCacheMetadata* const pRetainableCacheMetadata = nullptr);
            virtual ~Data();

            MapSurfaceType tileSurfaceType;
            QList< std::shared_ptr<const MapObject> > mapObjects;
        };

    private:
    protected:
        IMapObjectsProvider();
    public:
        virtual ~IMapObjectsProvider();

        virtual bool obtainTiledMapObjects(
            const Request& request,
            std::shared_ptr<Data>& outMapObjects,
            std::shared_ptr<Metric>* const pOutMetric = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_OBJECTS_PROVIDER_H_)
