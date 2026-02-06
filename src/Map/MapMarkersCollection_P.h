#ifndef _OSMAND_CORE_MAP_MARKERS_COLLECTION_P_H_
#define _OSMAND_CORE_MAP_MARKERS_COLLECTION_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include <QHash>
#include <QList>
#include <QReadWriteLock>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "CommonTypes.h"
#include "IMapKeyedSymbolsProvider.h"
#include "MapMarker.h"
#include "MapMarkerBuilder.h"

namespace OsmAnd
{
    class MapMarkerBuilder;
    class MapMarkerBuilder_P;

    class MapMarkersCollection;
    class MapMarkersCollection_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(MapMarkersCollection_P);

    private:
    protected:
        MapMarkersCollection_P(MapMarkersCollection* const owner);

        mutable QReadWriteLock _markersLock;
        QHash< int, QHash< IMapKeyedSymbolsProvider::Key, std::shared_ptr<MapMarker> > > _markers;
        QHash< int, QHash< int, std::shared_ptr<MapMarker> > > _markersById;

        bool addMarker(const std::shared_ptr<MapMarker>& marker);
    public:
        virtual ~MapMarkersCollection_P();

        ImplementationInterface<MapMarkersCollection> owner;

        std::shared_ptr<MapMarker> getMarkerById(int markerId, int groupId) const;
        QList< std::shared_ptr<MapMarker> > getMarkers() const;
        int getMarkersCountByGroupId(int groupId) const;
        void removeMarkersByGroupId(int groupId);
        bool removeMarkerById(int markerId, int groupId);
        bool removeMarker(const std::shared_ptr<MapMarker>& marker);
        void removeAllMarkers();

        QList<IMapKeyedSymbolsProvider::Key> getProvidedDataKeys() const;
        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData);

    friend class OsmAnd::MapMarkersCollection;
    friend class OsmAnd::MapMarkerBuilder;
    friend class OsmAnd::MapMarkerBuilder_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_MARKERS_COLLECTION_P_H_)
