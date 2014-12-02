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
            const float symbolsScaleFactor = 1.0f);
        virtual ~MapObjectsSymbolsProvider();

        const std::shared_ptr<MapPrimitivesProvider> primitivesProvider;
        const float referenceTileSizeOnScreenInPixels;
        const float symbolsScaleFactor;

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;

#if !defined(SWIG)
        //NOTE: For some reason, produces 'SWIGTYPE_p_std__shared_ptrT_OsmAnd__MapSymbolsGroup_const_t'
        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<IMapTiledSymbolsProvider::Data>& outTiledData,
            std::shared_ptr<Metric>* pOutMetric = nullptr,
            const IQueryController* const queryController = nullptr,
            const FilterCallback filterCallback = nullptr);
#endif // !defined(SWIG)
    };
}

#endif // !defined(_OSMAND_CORE_MAP_OBJECTS_SYMBOLS_PROVIDER_H_)
