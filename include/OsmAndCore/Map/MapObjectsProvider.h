#ifndef _OSMAND_CORE_MAP_OBJECTS_PROVIDER_H_
#define _OSMAND_CORE_MAP_OBJECTS_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/IMapObjectsProvider.h>

namespace OsmAnd
{
    class MapObject;

    class MapObjectsProvider_P;
    class OSMAND_CORE_API MapObjectsProvider Q_DECL_FINAL : public IMapObjectsProvider
    {
        Q_DISABLE_COPY_AND_MOVE(MapObjectsProvider);

    private:
        PrivateImplementation<MapObjectsProvider_P> _p;
    protected:
    public:
        MapObjectsProvider(const QList< std::shared_ptr<const MapObject> >& mapObjects);
        virtual ~MapObjectsProvider();

        const QList< std::shared_ptr<const MapObject> > mapObjects;

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;

        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<Data>& outTiledData,
            std::shared_ptr<Metric>* pOutMetric = nullptr,
            const IQueryController* const queryController = nullptr);

        virtual SourceType getSourceType() const;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_OBJECTS_PROVIDER_H_)
