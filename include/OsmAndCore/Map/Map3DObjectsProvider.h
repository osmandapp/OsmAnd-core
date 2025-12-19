#ifndef _OSMAND_CORE_MAP_3D_OBJECTS_PROVIDER_H_
#define _OSMAND_CORE_MAP_3D_OBJECTS_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QMutex>

#include <OsmAndCore.h>
#include <OsmAndCore/Map/IMapTiledDataProvider.h>
#include <OsmAndCore/Map/MapPrimitivesProvider.h>
#include <OsmAndCore/Map/MapPrimitiviser.h>
#include <OsmAndCore/Map/MapPresentationEnvironment.h>
#include <OsmAndCore/Map/MapRenderer3DObjects.h>
#include <OsmAndCore/Map/IMapElevationDataProvider.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <memory>

namespace OsmAnd
{
    class Map3DObjectsTiledProvider_P;
    class OSMAND_CORE_API Map3DObjectsTiledProvider : public IMapTiledDataProvider, public std::enable_shared_from_this<Map3DObjectsTiledProvider>
    {
    public:
        struct Data : public IMapTiledDataProvider::Data
        {
            Data(const TileId tileId, const ZoomLevel zoom, Buildings3D&& buildings);
            virtual ~Data();

            Buildings3D buildings3D;
        };

        explicit Map3DObjectsTiledProvider(
            const std::shared_ptr<MapPrimitivesProvider>& tiledProvider,
            const std::shared_ptr<MapPresentationEnvironment>& environment);
        virtual ~Map3DObjectsTiledProvider();

        virtual ZoomLevel getMinZoom() const override;
        virtual ZoomLevel getMaxZoom() const override;

        void setElevationDataProvider(const std::shared_ptr<IMapElevationDataProvider>& elevationProvider);

        virtual bool obtainTiledData(
            const IMapTiledDataProvider::Request& request,
            std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr) override;

        virtual bool supportsNaturalObtainData() const override;
        virtual bool supportsNaturalObtainDataAsync() const override;

        virtual bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr) override;

        virtual void obtainDataAsync(
            const IMapDataProvider::Request& request,
            const ObtainDataAsyncCallback callback,
            const bool collectMetric = false) override;

    protected:
        Map3DObjectsTiledProvider();

    private:
        PrivateImplementation<Map3DObjectsTiledProvider_P> _p;

        friend class OsmAnd::Map3DObjectsTiledProvider_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_3D_OBJECTS_PROVIDER_H_)
