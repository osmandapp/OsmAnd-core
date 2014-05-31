#ifndef _OSMAND_CORE_BINARY_MAP_STATIC_SYMBOLS_PROVIDER_H_
#define _OSMAND_CORE_BINARY_MAP_STATIC_SYMBOLS_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/IMapTiledSymbolsProvider.h>

namespace OsmAnd
{
    class BinaryMapDataProvider;
    class BinaryMapDataTile;
    namespace Model
    {
        class MapObject;
    }

    class BinaryMapStaticSymbolsProvider_P;
    class OSMAND_CORE_API BinaryMapStaticSymbolsProvider : public IMapTiledSymbolsProvider
    {
        Q_DISABLE_COPY(BinaryMapStaticSymbolsProvider);
    public:
        OSMAND_CALLABLE(FilterCallback,
            bool,
            const BinaryMapStaticSymbolsProvider* const provider,
            const std::shared_ptr<const Model::MapObject>& object,
            const bool shareable);

    private:
        PrivateImplementation<BinaryMapStaticSymbolsProvider_P> _p;
    protected:
    public:
        BinaryMapStaticSymbolsProvider(const std::shared_ptr<BinaryMapDataProvider>& dataProvider);
        virtual ~BinaryMapStaticSymbolsProvider();

        const std::shared_ptr<BinaryMapDataProvider> dataProvider;

        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<const MapTiledData>& outTiledData,
            const IQueryController* const queryController = nullptr);

        bool obtainData(
            const TileId tileId, const ZoomLevel zoom,
            std::shared_ptr<const MapTiledData>& outTiledData,
            const FilterCallback filterCallback = nullptr,
            const IQueryController* const queryController = nullptr);
    };

    class OSMAND_CORE_API BinaryMapStaticSymbolsTile : public MapTiledSymbols
    {
    private:
    protected:
        BinaryMapStaticSymbolsTile(
            const std::shared_ptr<const BinaryMapDataTile>& dataTile,
            const QList< std::shared_ptr<const MapSymbolsGroup> >& symbolsGroups,
            const TileId tileId, const ZoomLevel zoom);
    public:
        virtual ~BinaryMapStaticSymbolsTile();

        const std::shared_ptr<const BinaryMapDataTile> dataTile;

    friend class OsmAnd::BinaryMapStaticSymbolsProvider;
    friend class OsmAnd::BinaryMapStaticSymbolsProvider_P;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_STATIC_SYMBOLS_PROVIDER_H_)
