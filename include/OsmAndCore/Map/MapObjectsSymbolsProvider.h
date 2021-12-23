#ifndef _OSMAND_CORE_MAP_OBJECTS_SYMBOLS_PROVIDER_H_
#define _OSMAND_CORE_MAP_OBJECTS_SYMBOLS_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/IMapTiledSymbolsProvider.h>
#include <OsmAndCore/Map/MapPrimitivesProvider.h>
#include <OsmAndCore/Map/MapSymbolsGroup.h>
#include <OsmAndCore/Map/SymbolRasterizer.h>

namespace OsmAnd
{
    class MapObject;
    
    class MapObjectsSymbolsProvider_P;
    class OSMAND_CORE_API MapObjectsSymbolsProvider : public IMapTiledSymbolsProvider
    {
        Q_DISABLE_COPY_AND_MOVE(MapObjectsSymbolsProvider);
    public:
        class OSMAND_CORE_API Data : public IMapTiledSymbolsProvider::Data
        {
            Q_DISABLE_COPY_AND_MOVE(Data);
        private:
        protected:
        public:
            Data(
                const TileId tileId,
                const ZoomLevel zoom,
                const QList< std::shared_ptr<MapSymbolsGroup> >& symbolsGroups,
                const std::shared_ptr<const MapPrimitivesProvider::Data>& binaryMapPrimitivisedData,
                const RetainableCacheMetadata* const pRetainableCacheMetadata = nullptr);
            virtual ~Data();

            std::shared_ptr<const MapPrimitivesProvider::Data> mapPrimitivesData;
        };

        class OSMAND_CORE_API MapObjectSymbolsGroup : public MapSymbolsGroup
        {
            Q_DISABLE_COPY_AND_MOVE(MapObjectSymbolsGroup);

        public:
        protected:
        public:
            MapObjectSymbolsGroup(const std::shared_ptr<const MapObject>& sourceObject);
            virtual ~MapObjectSymbolsGroup();

            const std::shared_ptr<const MapObject> mapObject;

            virtual bool obtainSharingKey(SharingKey& outKey) const;
            virtual bool obtainSortingKey(SortingKey& outKey) const;
            virtual QString toString() const;
        };

    private:
        PrivateImplementation<MapObjectsSymbolsProvider_P> _p;
    protected:
    public:
        MapObjectsSymbolsProvider(
            const std::shared_ptr<MapPrimitivesProvider>& primitivesProvider,
            const float referenceTileSizeOnScreenInPixels,
            const std::shared_ptr<const SymbolRasterizer>& symbolRasterizer = std::shared_ptr<const SymbolRasterizer>(new SymbolRasterizer()));
        virtual ~MapObjectsSymbolsProvider();

        const std::shared_ptr<MapPrimitivesProvider> primitivesProvider;
        const float referenceTileSizeOnScreenInPixels;
        const std::shared_ptr<const SymbolRasterizer> symbolRasterizer;

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

#endif // !defined(_OSMAND_CORE_MAP_OBJECTS_SYMBOLS_PROVIDER_H_)
