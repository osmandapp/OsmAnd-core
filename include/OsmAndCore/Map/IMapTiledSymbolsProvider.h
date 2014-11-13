#ifndef _OSMAND_CORE_I_MAP_TILED_SYMBOLS_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_TILED_SYMBOLS_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/Map/IMapTiledDataProvider.h>

namespace OsmAnd
{
    class MapSymbolsGroup;

    class OSMAND_CORE_API IMapTiledSymbolsProvider : public IMapTiledDataProvider
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

        class OSMAND_CORE_API Data : public IMapTiledDataProvider::Data
        {
            Q_DISABLE_COPY_AND_MOVE(Data);
        private:
        protected:
        public:
            Data(
                const TileId tileId,
                const ZoomLevel zoom,
                const QList< std::shared_ptr<MapSymbolsGroup> >& symbolsGroups,
                const RetainableCacheMetadata* const pRetainableCacheMetadata = nullptr);
            virtual ~Data();

            QList< std::shared_ptr<MapSymbolsGroup> > symbolsGroups;
        };

    private:
    protected:
        IMapTiledSymbolsProvider();
    public:
        virtual ~IMapTiledSymbolsProvider();

        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
            std::shared_ptr<Metric>* pOutMetric = nullptr,
            const IQueryController* const queryController = nullptr);

# if !defined(SWIG)
        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<Data>& outTiledData,
            std::shared_ptr<Metric>* pOutMetric = nullptr,
            const IQueryController* const queryController = nullptr,
            const FilterCallback filterCallback = nullptr) = 0;
#endif // !defined(SWIG)
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_TILED_SYMBOLS_PROVIDER_H_)
