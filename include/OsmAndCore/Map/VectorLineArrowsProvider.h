#ifndef _OSMAND_CORE_VECTOR_LINE_ARROWS_PROVIDER_H_
#define _OSMAND_CORE_VECTOR_LINE_ARROWS_PROVIDER_H_

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
#include <OsmAndCore/Map/IMapTiledSymbolsProvider.h>
#include <OsmAndCore/Map/MapSymbolsGroup.h>
#include <OsmAndCore/Map/IUpdatableMapSymbolsGroup.h>

namespace OsmAnd
{
    class VectorLine;
    class VectorLinesCollection;

    class VectorLineArrowsProvider_P;
    class OSMAND_CORE_API VectorLineArrowsProvider
        : public std::enable_shared_from_this<VectorLineArrowsProvider>
        , public IMapTiledSymbolsProvider
    {
        Q_DISABLE_COPY_AND_MOVE(VectorLineArrowsProvider);
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

        class SymbolsGroup
            : public MapSymbolsGroup
            , public IUpdatableMapSymbolsGroup
        {
        private:
            const std::weak_ptr<VectorLineArrowsProvider_P> _providerP;
            const std::weak_ptr<VectorLine> _vectorLine;
        protected:
        public:
            SymbolsGroup(
                const std::shared_ptr<VectorLineArrowsProvider_P>& providerP,
                const std::shared_ptr<VectorLine>& vectorLine);
            virtual ~SymbolsGroup();

            virtual bool obtainSharingKey(SharingKey& outKey) const Q_DECL_OVERRIDE;
            virtual bool obtainSortingKey(SortingKey& outKey) const Q_DECL_OVERRIDE;
            virtual QString toString() const Q_DECL_OVERRIDE;
            
            virtual bool updatesPresent() Q_DECL_OVERRIDE;
            virtual UpdateResult update(const MapState& mapState) Q_DECL_OVERRIDE;
            
            virtual bool supportsResourcesRenew() Q_DECL_OVERRIDE;
        };

    private:
        PrivateImplementation<VectorLineArrowsProvider_P> _p;
                
    protected:
    public:
        VectorLineArrowsProvider(
            const std::shared_ptr<const VectorLinesCollection>& vectorLinesCollection);
        virtual ~VectorLineArrowsProvider();

        const std::shared_ptr<const VectorLinesCollection> vectorLinesCollection;
        
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
        
    friend class OsmAnd::VectorLineArrowsProvider_P;
    };
}

#endif // !defined(_OSMAND_CORE_VECTOR_LINE_ARROWS_PROVIDER_H_)
