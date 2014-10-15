#ifndef _OSMAND_CORE_I_MAP_TILED_SYMBOLS_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_TILED_SYMBOLS_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapSymbolsGroup.h>
#include <OsmAndCore/Map/IMapTiledDataProvider.h>
#include <OsmAndCore/Map/IMapSymbolsProvider.h>

namespace OsmAnd
{
    class TiledMapSymbolsData;

    class OSMAND_CORE_API IMapTiledSymbolsProvider
        : public IMapTiledDataProvider
        , public IMapSymbolsProvider
    {
        Q_DISABLE_COPY_AND_MOVE(IMapTiledSymbolsProvider);

    public:
#if !defined(SWIG)
        //NOTE: For some reason, produces 'SWIGTYPE_p_std__shared_ptrT_OsmAnd__MapSymbolsGroup_const_t'
        OSMAND_CALLABLE(FilterCallback,
            bool,
            const IMapTiledSymbolsProvider* const provider,
            const std::shared_ptr<const MapSymbolsGroup>& symbolsGroup);
#endif // !defined(SWIG)

    private:
    protected:
        IMapTiledSymbolsProvider();
    public:
        virtual ~IMapTiledSymbolsProvider();

        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<MapTiledData>& outTiledData,
            const IQueryController* const queryController = nullptr);

# if !defined(SWIG)
        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<TiledMapSymbolsData>& outTiledData,
            const FilterCallback filterCallback = nullptr,
            const IQueryController* const queryController = nullptr) = 0;
#endif // !defined(SWIG)
    };

    class OSMAND_CORE_API TiledMapSymbolsData : public MapTiledData
    {
    private:
    protected:
    public:
        TiledMapSymbolsData(const QList< std::shared_ptr<MapSymbolsGroup> >& symbolsGroups, const TileId tileId, const ZoomLevel zoom);
        virtual ~TiledMapSymbolsData();

        QList< std::shared_ptr<MapSymbolsGroup> > symbolsGroups;
        virtual void releaseConsumableContent();
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_TILED_SYMBOLS_PROVIDER_H_)
