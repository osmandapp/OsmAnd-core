#ifndef _OSMAND_CORE_TRANSPORT_STOP_SYMBOLS_PROVIDER_H_
#define _OSMAND_CORE_TRANSPORT_STOP_SYMBOLS_PROVIDER_H_

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
#include <OsmAndCore/Map/ITransportRouteIconProvider.h>


namespace OsmAnd
{
    class TransportStop;
    class TransportRoute;

    class TransportStopSymbolsProvider_P;
    class OSMAND_CORE_API TransportStopSymbolsProvider : public IMapTiledSymbolsProvider
    {
        Q_DISABLE_COPY_AND_MOVE(TransportStopSymbolsProvider);
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

        class OSMAND_CORE_API TransportStopSymbolsGroup : public MapSymbolsGroup
        {
            Q_DISABLE_COPY_AND_MOVE(TransportStopSymbolsGroup);

        public:
        protected:
        public:
            TransportStopSymbolsGroup(const std::shared_ptr<const TransportStop>& transportStop);
            virtual ~TransportStopSymbolsGroup();

            const std::shared_ptr<const TransportStop> transportStop;

            virtual bool obtainSharingKey(SharingKey& outKey) const;
            virtual bool obtainSortingKey(SortingKey& outKey) const;
            virtual QString toString() const;
        };

    private:
        PrivateImplementation<TransportStopSymbolsProvider_P> _p;
    protected:
    public:
        TransportStopSymbolsProvider(
            const std::shared_ptr<const IObfsCollection>& obfsCollection,
            const int symbolsOrder,
            const std::shared_ptr<const TransportRoute>& transportRoute = nullptr,
            const std::shared_ptr<const ITransportRouteIconProvider>& transportRouteIconProvider = nullptr);
        virtual ~TransportStopSymbolsProvider();

        const std::shared_ptr<const IObfsCollection> obfsCollection;
        
        virtual ZoomLevel getMinZoom() const Q_DECL_OVERRIDE;
        virtual ZoomLevel getMaxZoom() const Q_DECL_OVERRIDE;

        const int symbolsOrder;

        const std::shared_ptr<const TransportRoute> transportRoute;
        const std::shared_ptr<const ITransportRouteIconProvider> transportRouteIconProvider;

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

#endif // !defined(_OSMAND_CORE_TRANSPORT_STOP_SYMBOLS_PROVIDER_H_)
