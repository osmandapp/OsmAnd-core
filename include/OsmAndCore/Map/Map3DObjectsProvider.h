#ifndef _OSMAND_CORE_MAP_3D_OBJECTS_PROVIDER_H_
#define _OSMAND_CORE_MAP_3D_OBJECTS_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QMutex>

#include <OsmAndCore.h>
#include <OsmAndCore/Map/IMapTiledDataProvider.h>
#include <OsmAndCore/Map/MapPrimitivesProvider.h>
#include <OsmAndCore/Map/MapPrimitiviser.h>
#include <memory>

namespace OsmAnd
{
    class Map3DObjectsTiledProvider : public IMapTiledDataProvider, public std::enable_shared_from_this<Map3DObjectsTiledProvider>
    {
    public:
        class Data : public IMapTiledDataProvider::Data
        {
        public:
            Data(const TileId tileId, const ZoomLevel zoom,
                const QList<std::shared_ptr<const MapPrimitiviser::Primitive>>& polygons);
            virtual ~Data();

            QList<std::shared_ptr<const MapPrimitiviser::Primitive>> polygons;
        };

    private:
    protected:
        Map3DObjectsTiledProvider();
    public:
        explicit Map3DObjectsTiledProvider(const std::shared_ptr<MapPrimitivesProvider>& tiledProvider);
        virtual ~Map3DObjectsTiledProvider();

        virtual ZoomLevel getMinZoom() const override;
        virtual ZoomLevel getMaxZoom() const override;

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

    private:
        std::shared_ptr<MapPrimitivesProvider> _tiledProvider;
        
        QSet<const std::shared_ptr<const MapObject>> primitiveFilter;
        QMutex primitiveFilterMutex;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_3D_OBJECTS_PROVIDER_H_)
