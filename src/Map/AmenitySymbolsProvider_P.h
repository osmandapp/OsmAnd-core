#ifndef _OSMAND_CORE_AMENITY_SYMBOLS_PROVIDER_P_H_
#define _OSMAND_CORE_AMENITY_SYMBOLS_PROVIDER_P_H_

#include "stdlib_common.h"
#include <array>
#include <functional>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QHash>
#include <QReadWriteLock>
#include <QList>
#include <QWaitCondition>
#include <QVector>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "IMapTiledSymbolsProvider.h"
#include "TiledEntriesCollection.h"
#include "Amenity.h"
#include "AmenitySymbolsProvider.h"
#include "TextRasterizer.h"
#include "QuadTree.h"

namespace OsmAnd
{
    class AmenitySymbolsProvider_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(AmenitySymbolsProvider_P);

    public:
        typedef AmenitySymbolsProvider::AmenitySymbolsGroup AmenitySymbolsGroup;
        typedef QuadTree<AreaD, AreaD::CoordType> SymbolsQuadTree;

    private:
        std::shared_ptr<const TextRasterizer> textRasterizer;

        enum class TileState
        {
            Undefined = -1,

            Loading,
            Loaded,
            Cancelled
        };
        struct TileEntry : TiledEntriesCollectionEntryWithState < TileEntry, TileState, TileState::Undefined >
        {
            TileEntry(const TiledEntriesCollection<TileEntry>& collection, const TileId tileId, const ZoomLevel zoom)
                : TiledEntriesCollectionEntryWithState(collection, tileId, zoom)
                , dataIsPresent(false)
            {
            }

            virtual ~TileEntry()
            {
                safeUnlink();
            }

            bool dataIsPresent;
            std::weak_ptr<AmenitySymbolsProvider::Data> dataWeakRef;

            QReadWriteLock loadedConditionLock;
            QWaitCondition loadedCondition;
        };
        mutable TiledEntriesCollection<TileEntry> _tileReferences;

        uint32_t getTileId(const AreaI& tileBBox31, const PointI& point);
        AreaD calculateRect(double x, double y, double width, double height);
        bool intersects(SymbolsQuadTree& boundIntersections, double x, double y, double width, double height);
    protected:
        AmenitySymbolsProvider_P(AmenitySymbolsProvider* owner);

        struct RetainableCacheMetadata : public IMapDataProvider::RetainableCacheMetadata
        {
            RetainableCacheMetadata(const std::shared_ptr<TileEntry>& tileEntry);
            RetainableCacheMetadata();
            virtual ~RetainableCacheMetadata();

            std::weak_ptr<TileEntry> tileEntryWeakRef;
        };
    public:
        virtual ~AmenitySymbolsProvider_P();

        ImplementationInterface<AmenitySymbolsProvider> owner;

        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric);

    friend class OsmAnd::AmenitySymbolsProvider;
    };
}

#endif // !defined(_OSMAND_CORE_AMENITY_SYMBOLS_PROVIDER_P_H_)
