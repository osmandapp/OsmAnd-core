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
#include <OsmAndCore/Map/MapMarker.h>
#include <OsmAndCore/QuadTree.h>
#include <OsmAndCore/TextRasterizer.h>
#include <OsmAndCore/Map/MapTiledCollectionPoint.h>

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
                Data(const TileId tileId,
                     const ZoomLevel zoom,
                     const QList< std::shared_ptr<MapSymbolsGroup> >& symbolsGroups,
                     const RetainableCacheMetadata* const pRetainableCacheMetadata = nullptr);
                virtual ~Data();
            };
                
        private:
            typedef OsmAnd::QuadTree<AreaD, AreaD::CoordType> CollectionQuadTree;

            mutable QReadWriteLock _lock;
            uint32_t getTileId(const AreaI& tileBBox31, const PointI& point);
            AreaD calculateRect(double x, double y, double width, double height);
            bool intersects(CollectionQuadTree& boundIntersections, double x, double y, double width, double height);
            QList<std::shared_ptr<OsmAnd::MapSymbolsGroup>> buildMapSymbolsGroups(const TileId tileId, const ZoomLevel zoom, double scale);
        
        protected:
            MapTiledCollectionProvider();
        public:
            virtual ~MapTiledCollectionProvider();

            virtual int getBaseOrder() const = 0;
            virtual QList<PointI> getHiddenPoints() const = 0;
            virtual bool shouldShowCaptions() const = 0;
            virtual TextRasterizer::Style getCaptionStyle() const = 0;
            virtual double getCaptionTopSpace() const = 0;
            virtual float getReferenceTileSizeOnScreenInPixels() const = 0;
            virtual double getScale() const = 0;
            
            virtual PointI getPoint31(const int index) const = 0;
            virtual sk_sp<const SkImage> getImageBitmap(const int index, bool isFullSize = true) const = 0;
            virtual QString getCaption(const int index) const = 0;
            virtual int getPointsCount() const = 0;

            virtual QList<std::shared_ptr<OsmAnd::MapTiledCollectionPoint>> getTilePoints(const TileId tileId, const ZoomLevel zoom) const = 0;

            virtual MapMarker::PinIconVerticalAlignment getPinIconVerticalAlignment() const;
            virtual MapMarker::PinIconHorisontalAlignment getPinIconHorisontalAlignment() const;

            virtual ZoomLevel getMinZoom() const Q_DECL_OVERRIDE = 0;
            virtual ZoomLevel getMaxZoom() const Q_DECL_OVERRIDE = 0;

            virtual bool supportsNaturalObtainData() const Q_DECL_OVERRIDE;

            virtual bool obtainData(const IMapDataProvider::Request& request,
                std::shared_ptr<IMapDataProvider::Data>& outData,
                std::shared_ptr<Metric>* const pOutMetric = nullptr) Q_DECL_OVERRIDE;

            virtual bool supportsNaturalObtainDataAsync() const Q_DECL_OVERRIDE;
            virtual void obtainDataAsync(const IMapDataProvider::Request& request,
                const IMapDataProvider::ObtainDataAsyncCallback callback,
                const bool collectMetric = false) Q_DECL_OVERRIDE;
    };
    
    SWIG_EMIT_DIRECTOR_BEGIN(MapTiledCollectionProvider);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            int,
            getBaseOrder);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            QList<PointI>,
            getHiddenPoints);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            bool,
            shouldShowCaptions);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            TextRasterizer::Style,
            getCaptionStyle);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            double,
            getCaptionTopSpace);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            float,
            getReferenceTileSizeOnScreenInPixels);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            double,
            getScale);
        SWIG_EMIT_DIRECTOR_CONST_METHOD(
            PointI,
            getPoint31,
            const int index);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            int,
            getPointsCount);
        SWIG_EMIT_DIRECTOR_CONST_METHOD(
            sk_sp<const SkImage>,
            getImageBitmap,
            const int index,
            bool isFullSize);
        SWIG_EMIT_DIRECTOR_CONST_METHOD(
            QString,
            getCaption,
            const int index);
        SWIG_EMIT_DIRECTOR_CONST_METHOD(
            QList<std::shared_ptr<OsmAnd::MapTiledCollectionPoint>>,
            getTilePoints,
            const TileId tileId,
            const ZoomLevel zoom);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            ZoomLevel,
            getMinZoom);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            ZoomLevel,
            getMaxZoom);
    SWIG_EMIT_DIRECTOR_END(MapTiledCollectionProvider);
}

#endif // !defined(_OSMAND_CORE_MAP_TILED_COLLECTION_PROVIDER_H_)
