#ifndef _OSMAND_CORE_I_MAP_TILED_SYMBOLS_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_TILED_SYMBOLS_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/CommonSWIG.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/Map/MapSymbol.h>
#include <OsmAndCore/Map/MapSymbolsGroup.h>
#include <OsmAndCore/Map/IMapTiledDataProvider.h>
#include <OsmAndCore/Map/MapRendererState.h>

namespace OsmAnd
{
    class MapSymbolsGroup;

    class OSMAND_CORE_API IMapTiledSymbolsProvider : public IMapTiledDataProvider
    {
        Q_DISABLE_COPY_AND_MOVE(IMapTiledSymbolsProvider);

    public:
        //NOTE: 'SWIGTYPE_p_std__shared_ptrT_OsmAnd__MapSymbolsGroup_const_t' is produced
        // due to director+shared_ptr is not supported in SWIG
        //OSMAND_CALLABLE(FilterCallback,
        //    bool,
        //    const IMapTiledSymbolsProvider* const provider,
        //    const std::shared_ptr<const MapSymbolsGroup>& symbolsGroup);
        typedef std::function< bool(
            const IMapTiledSymbolsProvider* const provider,
            const std::shared_ptr<const MapSymbolsGroup>& symbolsGroup)> FilterCallback;

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

        struct OSMAND_CORE_API Request : public IMapTiledDataProvider::Request
        {
            MapState mapState;
            
            Request();
            Request(const IMapDataProvider::Request& that);
            virtual ~Request();

            FilterCallback filterCallback;
            bool combineTilesData = false;

            static void copy(Request& dst, const IMapDataProvider::Request& src);
            virtual std::shared_ptr<IMapDataProvider::Request> clone() const Q_DECL_OVERRIDE;

        private:
            typedef IMapTiledDataProvider::Request super;
        protected:
            Request(const Request& that);
        };

    private:
    protected:
        IMapTiledSymbolsProvider();
    public:
        virtual ~IMapTiledSymbolsProvider();

        virtual bool obtainTiledSymbols(
            const Request& request,
            std::shared_ptr<Data>& outTiledSymbols,
            std::shared_ptr<Metric>* const pOutMetric = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_TILED_SYMBOLS_PROVIDER_H_)
