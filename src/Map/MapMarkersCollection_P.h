#ifndef _OSMAND_CORE_MAP_MARKERS_COLLECTION_P_H_
#define _OSMAND_CORE_MAP_MARKERS_COLLECTION_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include <QHash>
#include <QList>
#include <QReadWriteLock>

#include <SkBitmap.h>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "CommonTypes.h"
#include "IMapKeyedSymbolsProvider.h"
#include "MapMarkersCollection.h"
#include "MapMarker.h"
#include "MapMarkerBuilder.h"

namespace OsmAnd
{
    class MapMarkerBuilder;
    class MapMarkerBuilder_P;

    class MapMarkersCollection;
    class MapMarkersCollection_P
    {
        Q_DISABLE_COPY(MapMarkersCollection_P);

    public:
        typedef MapMarkersCollection::Key Key;
    private:
    protected:
        MapMarkersCollection_P(MapMarkersCollection* const owner);

        mutable QReadWriteLock _markersLock;
        QHash< Key, std::shared_ptr<MapMarker> > _markers;

        bool addMarker(const std::shared_ptr<MapMarker>& marker);
    public:
        virtual ~MapMarkersCollection_P();

        ImplementationInterface<MapMarkersCollection> owner;

        QList< std::shared_ptr<MapMarker> > getMarkers() const;
        bool removeMarker(const std::shared_ptr<MapMarker>& marker);
        void removeAllMarkers();

        QList<Key> getProvidedDataKeys() const;
        bool obtainData(
            const Key key,
            std::shared_ptr<const MapKeyedData>& outKeyedData,
            const IQueryController* const queryController);

    friend class OsmAnd::MapMarkersCollection;
    friend class OsmAnd::MapMarkerBuilder;
    friend class OsmAnd::MapMarkerBuilder_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_MARKERS_COLLECTION_P_H_)
