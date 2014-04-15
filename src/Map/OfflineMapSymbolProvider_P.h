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
#include "IMapSymbolProvider.h"
#include "IRetainableResource.h"

namespace OsmAnd
{
    class OfflineMapDataTile;
    class MapSymbolsGroup;
    class MapSymbolsTile;
    namespace Model
    {
        class MapObject;
    }

    class OfflineMapSymbolProvider;
    class OfflineMapSymbolProvider_P
    {
    private:
    protected:
        OfflineMapSymbolProvider_P(OfflineMapSymbolProvider* owner);

        ImplementationInterface<OfflineMapSymbolProvider> owner;

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
        virtual ~OfflineMapSymbolProvider_P();

        bool obtainSymbols(
            const TileId tileId, const ZoomLevel zoom,
            std::shared_ptr<const MapSymbolsTile>& outTile,
            std::function<bool (const std::shared_ptr<const Model::MapObject>& mapObject)> filter);

        bool canSymbolsBeSharedFrom(const std::shared_ptr<const Model::MapObject>& mapObject);

    friend class OsmAnd::OfflineMapSymbolProvider;
    };
}

#endif // !defined(_OSMAND_CORE_OFFLINE_MAP_SYMBOL_PROVIDER_P_H_)
