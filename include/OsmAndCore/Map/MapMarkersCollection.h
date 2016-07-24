#ifndef _OSMAND_CORE_MAP_MARKERS_COLLECTION_H_
#define _OSMAND_CORE_MAP_MARKERS_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapKeyedSymbolsProvider.h>
#include <OsmAndCore/Map/MapMarker.h>

class SkBitmap;

namespace OsmAnd
{
    class MapMarkerBuilder;
    class MapMarkerBuilder_P;

    class MapMarkersCollection_P;
    class OSMAND_CORE_API MapMarkersCollection : public IMapKeyedSymbolsProvider
    {
        Q_DISABLE_COPY_AND_MOVE(MapMarkersCollection)

    private:
        PrivateImplementation<MapMarkersCollection_P> _p;
    protected:
    public:
        MapMarkersCollection(const ZoomLevel minZoom = MinZoomLevel, const ZoomLevel maxZoom = MaxZoomLevel);
        virtual ~MapMarkersCollection();

        QList< std::shared_ptr<MapMarker> > getMarkers() const;
        bool removeMarker(const std::shared_ptr<MapMarker>& marker);
        void removeAllMarkers();

        const ZoomLevel minZoom;
        const ZoomLevel maxZoom;

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;

        virtual QList<IMapKeyedSymbolsProvider::Key> getProvidedDataKeys() const;

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

    friend class OsmAnd::MapMarkerBuilder;
    friend class OsmAnd::MapMarkerBuilder_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_MARKERS_COLLECTION_H_)
