#ifndef _OSMAND_CORE_MAP_PRIMITIVES_CACHE_H_
#define _OSMAND_CORE_MAP_PRIMITIVES_CACHE_H_

#include "MapPrimitivesProvider.h"
#include <QByteArray>

namespace OsmAnd
{
    // Stores the primitives referenced by symbols and the source objects needed for polygon queries.
    // Restoring a tile performs no style evaluation or OBF geometry reads.
    class MapPrimitivesCache
    {
    public:
        static QByteArray serialize(const MapPrimitivesProvider::Data& data);
        static bool deserialize(
            const QByteArray& bytes,
            const MapPrimitivesProvider& provider,
            const IMapTiledDataProvider::Request& request,
            std::shared_ptr<IMapObjectsProvider::Data>& objects,
            std::shared_ptr<MapPrimitiviser::PrimitivisedObjects>& primitives);
    };
}

#endif
