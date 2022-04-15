#ifndef _OSMAND_CORE_MAP_TILED_COLLECTION_PROVIDER_H_
#define _OSMAND_CORE_MAP_TILED_COLLECTION_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QHash>
#include <QList>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Nullable.h>
#include <OsmAndCore/Map/IMapTiledSymbolsProvider.h>
#include <OsmAndCore/Map/MapSymbolsGroup.h>
#include <OsmAndCore/Map/IAmenityIconProvider.h>
#include <OsmAndCore/Map/MapMarker.h>
#include <OsmAndCore/QuadTree.h>
#include <OsmAndCore.h>

namespace OsmAnd
{
    class OSMAND_CORE_API MapTiledCollectionProvider : public IMapTiledSymbolsProvider
    {
        Q_DISABLE_COPY_AND_MOVE(MapTiledCollectionProvider);
        public:
            class OSMAND_CORE_API Data : public IMapTiledSymbolsProvider::Data
            {
                Q_DISABLE_COPY_AND_MOVE(Data);
                private:
                
                public:
                    Data(const OsmAnd::TileId tileId,
                        const OsmAnd::ZoomLevel zoom,
                        const QList< std::shared_ptr<OsmAnd::MapSymbolsGroup> >& symbolsGroups,
                        const RetainableCacheMetadata* const pRetainableCacheMetadata = nullptr);
                    virtual ~Data();
            };

        private:
            typedef OsmAnd::QuadTree<OsmAnd::AreaD, OsmAnd::AreaD::CoordType> CollectionQuadTree;

            mutable QReadWriteLock _lock;
            uint32_t getTileId(const OsmAnd::AreaI& tileBBox31, const OsmAnd::PointI& point);
            OsmAnd::AreaD calculateRect(double x, double y, double width, double height);
            bool intersects(CollectionQuadTree& boundIntersections, double x, double y, double width, double height);
            QList<std::shared_ptr<OsmAnd::MapSymbolsGroup>> buildMapSymbolsGroups(const OsmAnd::TileId tileId, const OsmAnd::ZoomLevel zoom, double scale);
        public:
            MapTiledCollectionProvider(const int baseOrder,
                const QList<OsmAnd::PointI>& hiddenPoints,
                const bool showCaptions,
                const OsmAnd::TextRasterizer::Style captionStyle,
                const double captionTopSpace,
                const float referenceTileSizeOnScreenInPixels,
                const double scale);
            virtual ~MapTiledCollectionProvider();

            OsmAnd::MapTiledCollectionProvider * new_MapTiledCollectionProvider(const int baseOrder,
                const QList<OsmAnd::PointI>& hiddenPoints,
                const bool showCaptions,
                const OsmAnd::TextRasterizer::Style captionStyle,
                const double captionTopSpace,
                const float referenceTileSizeOnScreenInPixels,
                const double scale);

            virtual OsmAnd::PointI getPoint31(const int index) const;
            virtual int getPointsCount() const;
            virtual sk_sp<SkImage> getImageBitmap(const int index, bool isFullSize = true);
            virtual QString getCaption(const int index) const;
            virtual OsmAnd::MapMarker::PinIconVerticalAlignment getPinIconVerticalAlignment() const;
            virtual OsmAnd::MapMarker::PinIconHorisontalAlignment getPinIconHorisontalAlignment() const;

            const int baseOrder;
            const QList<OsmAnd::PointI> hiddenPoints;
            const bool showCaptions;
            const OsmAnd::TextRasterizer::Style captionStyle;
            const double captionTopSpace;
            const float referenceTileSizeOnScreenInPixels;
            const double scale;

            virtual OsmAnd::ZoomLevel getMinZoom() const;
            virtual OsmAnd::ZoomLevel getMaxZoom() const;

            virtual bool supportsNaturalObtainData() const Q_DECL_OVERRIDE;

            virtual bool obtainData(const IMapDataProvider::Request& request,
                std::shared_ptr<IMapDataProvider::Data>& outData,
                std::shared_ptr<OsmAnd::Metric>* const pOutMetric = nullptr) Q_DECL_OVERRIDE;

            virtual bool supportsNaturalObtainDataAsync() const Q_DECL_OVERRIDE;
            virtual void obtainDataAsync(const IMapDataProvider::Request& request,
                const IMapDataProvider::ObtainDataAsyncCallback callback,
                const bool collectMetric = false) Q_DECL_OVERRIDE;

    };
}

#endif