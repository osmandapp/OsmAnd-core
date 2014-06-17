#ifndef _OSMAND_CORE_BINARY_MAP_STATIC_SYMBOLS_PROVIDER_P_H_
#define _OSMAND_CORE_BINARY_MAP_STATIC_SYMBOLS_PROVIDER_P_H_

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
#include "MapSymbolsGroupShareableById.h"
#include "BinaryMapObject.h"
#include "BinaryMapStaticSymbolsProvider.h"

namespace OsmAnd
{
    class BinaryMapDataTile;
    class MapSymbolsGroup;
    class TiledMapSymbolsData;

    class BinaryMapStaticSymbolsProvider;
    class BinaryMapStaticSymbolsProvider_P
    {
    public:
        typedef BinaryMapStaticSymbolsProvider::FilterCallback FilterCallback;

    private:
    protected:
        BinaryMapStaticSymbolsProvider_P(BinaryMapStaticSymbolsProvider* owner);

        ImplementationInterface<BinaryMapStaticSymbolsProvider> owner;

        class MapSymbolsGroupShareableByMapObjectId : public MapSymbolsGroupShareableById
        {
            Q_DISABLE_COPY(MapSymbolsGroupShareableByMapObjectId);
        private:
        protected:
        public:
            MapSymbolsGroupShareableByMapObjectId(const std::shared_ptr<const Model::BinaryMapObject>& mapObject);
            virtual ~MapSymbolsGroupShareableByMapObjectId();

            const std::shared_ptr<const Model::BinaryMapObject> mapObject;

            virtual QString getDebugTitle() const;
        };
    public:
        virtual ~BinaryMapStaticSymbolsProvider_P();

        bool obtainData(
            const TileId tileId, const ZoomLevel zoom,
            std::shared_ptr<MapTiledData>& outTiledData,
            const FilterCallback filterCallback,
            const IQueryController* const queryController);

    friend class OsmAnd::BinaryMapStaticSymbolsProvider;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_STATIC_SYMBOLS_PROVIDER_P_H_)
