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
#include "IMapTiledSymbolsProvider.h"
#include "MapObject.h"
#include "IRetainableResource.h"

namespace OsmAnd
{
    class OfflineMapDataTile;
    class MapSymbolsGroup;
    class MapTiledSymbols;

    class OfflineMapStaticSymbolProvider;
    class OfflineMapStaticSymbolProvider_P
    {
    private:
    protected:
        OfflineMapStaticSymbolProvider_P(OfflineMapStaticSymbolProvider* owner);

        ImplementationInterface<OfflineMapStaticSymbolProvider> owner;

        class Group : public MapSymbolsGroupShareableById
        {
            Q_DISABLE_COPY(Group);
        private:
        protected:
        public:
            Group(const std::shared_ptr<const Model::MapObject>& mapObject);
            virtual ~Group();

            const std::shared_ptr<const Model::MapObject> mapObject;

            virtual QString getDebugTitle() const;
        };

        class Tile
            : public MapTiledSymbols
            , public IRetainableResource
        {
            Q_DISABLE_COPY(Tile);

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
            std::shared_ptr<const MapTiledSymbols>& outTile,
            const IMapSymbolProvider::FilterCallback filterCallback);

    friend class OsmAnd::OfflineMapStaticSymbolProvider;
    };
}

#endif // !defined(_OSMAND_CORE_OFFLINE_MAP_SYMBOL_PROVIDER_P_H_)
