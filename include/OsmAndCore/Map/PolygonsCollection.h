#ifndef _OSMAND_CORE_POLYGONS_COLLECTION_H_
#define _OSMAND_CORE_POLYGONS_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QVector>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapKeyedSymbolsProvider.h>
#include <OsmAndCore/Map/Polygon.h>

namespace OsmAnd
{
    class PolygonBuilder;
    class PolygonBuilder_P;

    class PolygonsCollection_P;
    class OSMAND_CORE_API PolygonsCollection : public IMapKeyedSymbolsProvider
    {
        Q_DISABLE_COPY_AND_MOVE(PolygonsCollection);

    private:
        PrivateImplementation<PolygonsCollection_P> _p;
    protected:
    public:
        PolygonsCollection();
        virtual ~PolygonsCollection();

        QList< std::shared_ptr<Polygon> > getPolygons() const;
        bool removePolygon(const std::shared_ptr<Polygon>& polygon);
        void removeAllPolygons();

        QList<IMapKeyedSymbolsProvider::Key> getProvidedDataKeys() const Q_DECL_OVERRIDE;
        
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

    friend class OsmAnd::PolygonBuilder;
    friend class OsmAnd::PolygonBuilder_P;
    };
}

#endif // !defined(_OSMAND_CORE_POLYGONS_COLLECTION_H_)
