#ifndef _OSMAND_CORE_OFFLINE_MAP_SYMBOL_PROVIDER_P_H_
#define _OSMAND_CORE_OFFLINE_MAP_SYMBOL_PROVIDER_P_H_

#include "stdlib_common.h"
#include <array>
#include <functional>

#include "QtExtensions.h"
#include <QReadWriteLock>
#include <QList>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "IMapSymbolTiledProvider.h"
#include "IRetainableResource.h"

namespace OsmAnd
{
    class OfflineMapDataTile;
    class MapSymbolsGroup;
    class MapSymbolsTile;
    namespace Model
    {
        class ObjectWithId;
    }

    class OfflineMapStaticSymbolProvider;
    class OfflineMapStaticSymbolProvider_P
    {
    private:
    protected:
        OfflineMapStaticSymbolProvider_P(OfflineMapStaticSymbolProvider* owner);

        ImplementationInterface<OfflineMapStaticSymbolProvider> owner;

        class Tile
            : public MapSymbolsTile
            , public IRetainableResource
        {
        private:
        protected:
        public:
            Tile(const QList< std::shared_ptr<const MapSymbolsGroup> >& symbolsGroups, const std::shared_ptr<const OfflineMapDataTile>& dataTile);
            virtual ~Tile();

            const std::shared_ptr<const OfflineMapDataTile> dataTile;

            virtual void releaseNonRetainedData();
        };
    public:
        virtual ~OfflineMapStaticSymbolProvider_P();

        bool obtainSymbols(
            const TileId tileId, const ZoomLevel zoom,
            std::shared_ptr<const MapSymbolsTile>& outTile,
            const IMapSymbolProvider::FilterCallback filterCallback);

    friend class OsmAnd::OfflineMapStaticSymbolProvider;
    };
}

#endif // !defined(_OSMAND_CORE_OFFLINE_MAP_SYMBOL_PROVIDER_P_H_)
