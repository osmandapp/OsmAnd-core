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

namespace OsmAnd
{
    class MapMarkerBuilder;
    class MapMarkerBuilder_P;

    class MapMarkersCollection_P;
    class OSMAND_CORE_API MapMarkersCollection
        : public std::enable_shared_from_this<MapMarkersCollection>
        , public IMapKeyedSymbolsProvider
    {
        Q_DISABLE_COPY_AND_MOVE(MapMarkersCollection);

    private:
        PrivateImplementation<MapMarkersCollection_P> _p;
        int64_t priority;
    protected:
    public:
        MapMarkersCollection();
        virtual ~MapMarkersCollection();

        int subsection;

        std::shared_ptr<MapMarker> getMarkerById(int markerId, int groupId = 0) const;
        QList< std::shared_ptr<MapMarker> > getMarkers() const;
        int getMarkersCountByGroupId(int groupId) const;
        void removeMarkersByGroupId(int groupId);
        bool removeMarkerById(int markerId, int groupId);
        bool removeMarker(const std::shared_ptr<MapMarker>& marker);
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

        virtual int64_t getPriority() const Q_DECL_OVERRIDE;
        virtual void setPriority(int64_t priority) Q_DECL_OVERRIDE;

    friend class OsmAnd::MapMarkerBuilder;
    friend class OsmAnd::MapMarkerBuilder_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_MARKERS_COLLECTION_H_)
