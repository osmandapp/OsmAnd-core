#ifndef _OSMAND_CORE_MAP_MARKERS_COLLECTION_P_H_
#define _OSMAND_CORE_MAP_MARKERS_COLLECTION_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include <QSet>
#include <QReadWriteLock>

#include <SkBitmap.h>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "CommonTypes.h"
#include "IMapSymbolKeyedProvider.h"
#include "MapMarkersCollection.h"
#include "MapMarker.h"
#include "MapMarkerBuilder.h"

namespace OsmAnd
{
    class MapMarkersCollection;
    class MapMarkersCollection_P
    {
        Q_DISABLE_COPY(MapMarkersCollection_P);

    public:
        typedef MapMarkersCollection::Key Key;
        typedef MapMarkersCollection::FilterCallback FilterCallback;
    private:
    protected:
        MapMarkersCollection_P(MapMarkersCollection* const owner);

        mutable QReadWriteLock _markersLock;
        QSet< std::shared_ptr<MapMarker> > _markers;
    public:
        virtual ~MapMarkersCollection_P();

        ImplementationInterface<MapMarkersCollection> owner;

        QSet< std::shared_ptr<MapMarker> > getMarkers() const;
        bool removeMarker(const std::shared_ptr<MapMarker>& marker);
        void removeAllMarkers();

        QSet<Key> getKeys() const;
        bool obtainSymbolsGroup(const Key key, std::shared_ptr<const MapSymbolsGroup>& outSymbolGroups);

    friend class OsmAnd::MapMarkersCollection;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_MARKERS_COLLECTION_P_H_)
