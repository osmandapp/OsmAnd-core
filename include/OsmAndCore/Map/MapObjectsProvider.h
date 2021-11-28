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

        virtual ZoomLevel getMinZoom() const Q_DECL_OVERRIDE;
        virtual ZoomLevel getMaxZoom() const Q_DECL_OVERRIDE;

        virtual bool supportsNaturalObtainData() const Q_DECL_OVERRIDE;
        virtual bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr) Q_DECL_OVERRIDE;

        virtual bool supportsNaturalObtainDataAsync() const Q_DECL_OVERRIDE;
        virtual void obtainDataAsync(
            const IMapDataProvider::Request& request,
            const IMapDataProvider::ObtainDataAsyncCallback callback,
            const bool collectMetric = false) Q_DECL_OVERRIDE;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_OBJECTS_PROVIDER_H_)
