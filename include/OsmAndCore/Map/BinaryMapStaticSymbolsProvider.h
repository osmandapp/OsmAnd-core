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
#include <OsmAndCore/Map/BinaryMapPrimitivesProvider.h>

namespace OsmAnd
{
    namespace Model
    {
        class BinaryMapObject;
    }

    class BinaryMapStaticSymbolsProvider_P;
    class OSMAND_CORE_API BinaryMapStaticSymbolsProvider : public IMapTiledSymbolsProvider
    {
        Q_DISABLE_COPY_AND_MOVE(BinaryMapStaticSymbolsProvider);
    public:
        class OSMAND_CORE_API Data : public IMapTiledSymbolsProvider::Data
        {
            Q_DISABLE_COPY_AND_MOVE(Data);
        private:
        protected:
        public:
            Data(
                const TileId tileId,
                const ZoomLevel zoom,
                const QList< std::shared_ptr<MapSymbolsGroup> >& symbolsGroups,
                const std::shared_ptr<const BinaryMapPrimitivesProvider::Data>& binaryMapPrimitivisedData,
                const RetainableCacheMetadata* const pRetainableCacheMetadata = nullptr);
            virtual ~Data();

            std::shared_ptr<const BinaryMapPrimitivesProvider::Data> binaryMapPrimitivisedData;
        };

    private:
        PrivateImplementation<BinaryMapStaticSymbolsProvider_P> _p;
    protected:
    public:
        BinaryMapStaticSymbolsProvider(
            const std::shared_ptr<BinaryMapPrimitivesProvider>& primitivesProvider,
            const unsigned int referenceTileSizeInPixels);
        virtual ~BinaryMapStaticSymbolsProvider();

        const std::shared_ptr<BinaryMapPrimitivesProvider> primitivesProvider;
        unsigned int referenceTileSizeInPixels;

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;

#if !defined(SWIG)
        //NOTE: For some reason, produces 'SWIGTYPE_p_std__shared_ptrT_OsmAnd__MapSymbolsGroup_const_t'
        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<IMapTiledSymbolsProvider::Data>& outTiledData,
            const FilterCallback filterCallback = nullptr,
            const IQueryController* const queryController = nullptr);
#endif // !defined(SWIG)
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_STATIC_SYMBOLS_PROVIDER_H_)
