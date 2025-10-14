#ifndef _OSMAND_CORE_MAP_3D_OBJECTS_KEYED_PROVIDER_H_
#define _OSMAND_CORE_MAP_3D_OBJECTS_KEYED_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Map/IMapKeyedDataProvider.h>
#include <OsmAndCore/Map/IMapTiledDataProvider.h>
#include <OsmAndCore/Map/MapPrimitivesProvider.h>
#include <OsmAndCore/Map/MapPrimitiviser.h>
#include <memory>

namespace OsmAnd
{
    class Map3DObjectsKeyedProvider : public IMapKeyedDataProvider, public std::enable_shared_from_this<Map3DObjectsKeyedProvider>
    {
    public:
        class Data : public IMapKeyedDataProvider::Data
        {
        public:
            Data(const Key key, const QList<std::shared_ptr<const MapPrimitiviser::Primitive>>& polygons);
            virtual ~Data();

            QList<std::shared_ptr<const MapPrimitiviser::Primitive>> polygons;
        };

    private:
    protected:
        Map3DObjectsKeyedProvider();
    public:
        explicit Map3DObjectsKeyedProvider(const std::shared_ptr<MapPrimitivesProvider>& tiledProvider);
        virtual ~Map3DObjectsKeyedProvider();

        virtual ZoomLevel getMinZoom() const override;
        virtual ZoomLevel getMaxZoom() const override;
        virtual int64_t getPriority() const override;
        virtual void setPriority(int64_t priority) override;

        virtual QList<Key> getProvidedDataKeys() const override;

        virtual bool obtainKeyedData(
            const Request& request,
            std::shared_ptr<Data>& outKeyedData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr);

        virtual bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr) override;

        virtual bool supportsNaturalObtainData() const override;
        virtual bool supportsNaturalObtainDataAsync() const override;

        virtual void obtainDataAsync(
            const IMapDataProvider::Request& request,
            const ObtainDataAsyncCallback callback,
            const bool collectMetric = false) override;

    private:
        std::shared_ptr<MapPrimitivesProvider> _tiledProvider;
        static const Key SCREEN_KEY;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_3D_OBJECTS_KEYED_PROVIDER_H_)
