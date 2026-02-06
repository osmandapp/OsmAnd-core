#ifndef _OSMAND_CORE_VECTOR_LINE_ARROWS_PROVIDER_H_
#define _OSMAND_CORE_VECTOR_LINE_ARROWS_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapKeyedSymbolsProvider.h>

namespace OsmAnd
{
    class VectorLinesCollection;
    class VectorLineArrowsProvider_P;
    class OSMAND_CORE_API VectorLineArrowsProvider
        : public std::enable_shared_from_this<VectorLineArrowsProvider>
        , public IMapKeyedSymbolsProvider
    {
        Q_DISABLE_COPY_AND_MOVE(VectorLineArrowsProvider);

    private:
        PrivateImplementation<VectorLineArrowsProvider_P> _p;
    protected:
    public:
        VectorLineArrowsProvider(const std::shared_ptr<VectorLinesCollection>& collection);
        virtual ~VectorLineArrowsProvider();

        void removeLineMarkers(int lineId);
        void removeAllMarkers();

        virtual QList<IMapKeyedSymbolsProvider::Key> getProvidedDataKeys() const Q_DECL_OVERRIDE;

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
        
        virtual ZoomLevel getMinZoom() const Q_DECL_OVERRIDE;
        virtual ZoomLevel getMaxZoom() const Q_DECL_OVERRIDE;
    };
}

#endif // !defined(_OSMAND_CORE_VECTOR_LINE_ARROWS_PROVIDER_H_)
