#ifndef _OSMAND_CORE_AMENITY_SYMBOLS_PROVIDER_H_
#define _OSMAND_CORE_AMENITY_SYMBOLS_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QList>
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
    class OSMAND_CORE_API AmenitySymbolsProvider : public IMapTiledSymbolsProvider
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
            const std::shared_ptr<const IAmenityIconProvider>& amenityIconProvider = std::make_shared<CoreResourcesAmenityIconProvider>(),
            const int baseOrder = 10000);
        virtual ~AmenitySymbolsProvider();

        const std::shared_ptr<const IObfsCollection> obfsCollection;
        const float displayDensityFactor;
        const float referenceTileSizeOnScreenInPixels;
        const Nullable< QHash<QString, QStringList> > categoriesFilter;
        const ObfPoiSectionReader::VisitorFunction amentitiesFilter;
        const std::shared_ptr<const IAmenityIconProvider> amenityIconProvider;
        const int baseOrder;

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

#endif // !defined(_OSMAND_CORE_AMENITY_SYMBOLS_PROVIDER_H_)
