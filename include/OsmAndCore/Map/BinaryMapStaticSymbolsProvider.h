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
    class BinaryMapPrimitivesProvider;
    class BinaryMapPrimitivesTile;
    namespace Model
    {
        class BinaryMapObject;
    }

    class BinaryMapStaticSymbolsProvider_P;
    class OSMAND_CORE_API BinaryMapStaticSymbolsProvider : public IMapTiledSymbolsProvider
    {
        Q_DISABLE_COPY(BinaryMapStaticSymbolsProvider);
    private:
        PrivateImplementation<BinaryMapStaticSymbolsProvider_P> _p;
    protected:
    public:
        BinaryMapStaticSymbolsProvider(const std::shared_ptr<BinaryMapPrimitivesProvider>& primitivesProvider);
        virtual ~BinaryMapStaticSymbolsProvider();

        const std::shared_ptr<BinaryMapPrimitivesProvider> primitivesProvider;

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;

        virtual bool obtainData(
            const TileId tileId, const ZoomLevel zoom,
            std::shared_ptr<MapTiledData>& outTiledData,
            const FilterCallback filterCallback = nullptr,
            const IQueryController* const queryController = nullptr);
    };

    class OSMAND_CORE_API BinaryMapStaticSymbolsTile : public TiledMapSymbolsData
    {
    private:
    protected:
        BinaryMapStaticSymbolsTile(
            const std::shared_ptr<const BinaryMapPrimitivesTile>& dataTile,
            const QList< std::shared_ptr<MapSymbolsGroup> >& symbolsGroups,
            const TileId tileId, const ZoomLevel zoom);
    public:
        virtual ~BinaryMapStaticSymbolsTile();

        const std::shared_ptr<const BinaryMapPrimitivesTile> dataTile;

    friend class OsmAnd::BinaryMapStaticSymbolsProvider;
    friend class OsmAnd::BinaryMapStaticSymbolsProvider_P;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_STATIC_SYMBOLS_PROVIDER_H_)
