#ifndef _OSMAND_CORE_AMENITY_SYMBOLS_PROVIDER_H_
#define _OSMAND_CORE_AMENITY_SYMBOLS_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <array>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QHash>
#include <QList>
#include <QReadWriteLock>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Nullable.h>
#include <OsmAndCore/IObfsCollection.h>
#include <OsmAndCore/Data/ObfPoiSectionReader.h>
#include <OsmAndCore/Map/IMapTiledSymbolsProvider.h>
#include <OsmAndCore/Map/MapSymbolsGroup.h>
#include <OsmAndCore/Map/IAmenityIconProvider.h>
#include <OsmAndCore/Map/CoreResourcesAmenityIconProvider.h>

namespace OsmAnd
{
    class Amenity;

    class AmenitySymbolsProvider_P;
    class OSMAND_CORE_API AmenitySymbolsProvider
        : public std::enable_shared_from_this<AmenitySymbolsProvider>
        , public IMapTiledSymbolsProvider
    {
        Q_DISABLE_COPY_AND_MOVE(AmenitySymbolsProvider);
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
                const RetainableCacheMetadata* const pRetainableCacheMetadata = nullptr);
            virtual ~Data();
        };

        class OSMAND_CORE_API Cache Q_DECL_FINAL
        {
            Q_DISABLE_COPY_AND_MOVE(Cache);

        public:
            struct OSMAND_CORE_API Entry
            {
                Entry();
                Entry(const QList<std::shared_ptr<const Amenity>>& amenities);

                QList<std::shared_ptr<const Amenity>> amenities;
            };

        private:
            struct LruEntry
            {
                TileId tileId;
                ZoomLevel zoom;
                uint64_t generation;
            };
            struct CacheEntry
            {
                QList<std::shared_ptr<const Amenity>> amenities;
                uint64_t generation;
            };

            mutable QReadWriteLock _lock;
            std::array<QHash<TileId, CacheEntry>, ZoomLevelsCount> _storage;
            QList<LruEntry> _lru;
            uint64_t _nextGeneration;

            void removeEntryUnlocked(const TileId tileId, const ZoomLevel zoom);
            void shrinkToCapacityUnlocked();

        public:
            explicit Cache(const uint32_t capacity);
            virtual ~Cache();

            const uint32_t capacity;

            uint32_t getSize() const;
            bool contains(const TileId tileId, const ZoomLevel zoom) const;
            bool obtainEntry(const TileId tileId, const ZoomLevel zoom, Entry& outEntry);
            bool obtainAmenities(
                const TileId tileId,
                const ZoomLevel zoom,
                QList<std::shared_ptr<const Amenity>>& outAmenities);
            void put(
                const TileId tileId,
                const ZoomLevel zoom,
                const QList<std::shared_ptr<const Amenity>>& amenities);
            void remove(const TileId tileId, const ZoomLevel zoom);
            void clear();
        };

        class OSMAND_CORE_API AmenitySymbolsGroup : public MapSymbolsGroup
        {
            Q_DISABLE_COPY_AND_MOVE(AmenitySymbolsGroup);

        public:
        protected:
        public:
            AmenitySymbolsGroup(const std::shared_ptr<const Amenity>& amenity);
            virtual ~AmenitySymbolsGroup();

            const std::shared_ptr<const Amenity> amenity;

            virtual bool obtainSharingKey(SharingKey& outKey) const;
            virtual bool obtainSortingKey(SortingKey& outKey) const;
            virtual QString toString() const;
        };

    private:
        PrivateImplementation<AmenitySymbolsProvider_P> _p;
    protected:
    public:
        AmenitySymbolsProvider(
            const std::shared_ptr<const IObfsCollection>& obfsCollection,
            const float displayDensityFactor,
            const float referenceTileSizeOnScreenInPixels,
            const QHash<QString, QStringList>* const categoriesFilter = nullptr,
            const ObfPoiSectionReader::VisitorFunction amentitiesFilter = nullptr,
            const std::shared_ptr<IAmenityIconProvider>& amenityIconProvider = std::make_shared<CoreResourcesAmenityIconProvider>(),
            const int baseOrder = 10000,
            const uint32_t cacheSize = 0);
        virtual ~AmenitySymbolsProvider();

        int subsection;

        const std::shared_ptr<const IObfsCollection> obfsCollection;
        const float displayDensityFactor;
        const float referenceTileSizeOnScreenInPixels;
        const Nullable< QHash<QString, QStringList> > categoriesFilter;
        const Nullable< QPair<QString, QString> > poiAdditionalFilter;
        const ObfPoiSectionReader::VisitorFunction amentitiesFilter;
        const std::shared_ptr<IAmenityIconProvider> amenityIconProvider;
        const int baseOrder;
        const uint32_t cacheSize;
        const std::shared_ptr<Cache> cache;

        virtual ZoomLevel getMinZoom() const Q_DECL_OVERRIDE;
        virtual ZoomLevel getMaxZoom() const Q_DECL_OVERRIDE;

        //virtual int getMaxMissingDataUnderZoomShift() const Q_DECL_OVERRIDE;

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

#endif // !defined(_OSMAND_CORE_AMENITY_SYMBOLS_PROVIDER_H_)
