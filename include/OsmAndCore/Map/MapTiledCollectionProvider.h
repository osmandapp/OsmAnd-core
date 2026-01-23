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
#include <OsmAndCore/SingleSkImage.h>

namespace OsmAnd
{
    class OSMAND_CORE_API MapTiledCollectionProvider
        : public std::enable_shared_from_this<MapTiledCollectionProvider>
        , public IMapTiledSymbolsProvider
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
            virtual QList<OsmAnd::PointI> getPoints31() const = 0;
            virtual QList<OsmAnd::PointI> getHiddenPoints() const = 0;
            virtual bool shouldShowCaptions() const = 0;
            virtual OsmAnd::TextRasterizer::Style getCaptionStyle() const = 0;
            virtual double getCaptionTopSpace() const = 0;
            virtual float getReferenceTileSizeOnScreenInPixels() const = 0;
            virtual double getScale() const = 0;
            
            virtual SingleSkImage getImageBitmap(const int index, bool isFullSize = true) const = 0;
            virtual QString getCaption(const int index) const = 0;

            virtual QList<std::shared_ptr<OsmAnd::MapTiledCollectionPoint>> getTilePoints(const OsmAnd::TileId& tileId, const OsmAnd::ZoomLevel zoom) const = 0;

            virtual OsmAnd::MapMarker::PinIconVerticalAlignment getPinIconVerticalAlignment() const;
            virtual OsmAnd::MapMarker::PinIconHorisontalAlignment getPinIconHorisontalAlignment() const;

            virtual OsmAnd::ZoomLevel getMinZoom() const = 0;
            virtual OsmAnd::ZoomLevel getMaxZoom() const = 0;

            virtual OsmAnd::PointI getPinIconOffset() const;

            virtual bool supportsNaturalObtainData() const Q_DECL_OVERRIDE;

            virtual bool obtainData(const IMapDataProvider::Request& request,
                std::shared_ptr<IMapDataProvider::Data>& outData,
                std::shared_ptr<OsmAnd::Metric>* const pOutMetric = nullptr) Q_DECL_OVERRIDE;

            virtual bool supportsNaturalObtainDataAsync() const Q_DECL_OVERRIDE;
            virtual void obtainDataAsync(const IMapDataProvider::Request& request,
                const IMapDataProvider::ObtainDataAsyncCallback callback,
                const bool collectMetric = false) Q_DECL_OVERRIDE;

            virtual bool waitForLoading() const Q_DECL_OVERRIDE;
    };
    
    SWIG_EMIT_DIRECTOR_BEGIN(MapTiledCollectionProvider);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            int,
            getBaseOrder);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            QList<OsmAnd::PointI>,
            getPoints31);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            QList<OsmAnd::PointI>,
            getHiddenPoints);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            bool,
            shouldShowCaptions);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            OsmAnd::TextRasterizer::Style,
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
            SingleSkImage,
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
            const OsmAnd::TileId& tileId,
            const OsmAnd::ZoomLevel zoom);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            OsmAnd::MapMarker::PinIconVerticalAlignment,
            getPinIconVerticalAlignment);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            OsmAnd::MapMarker::PinIconHorisontalAlignment,
            getPinIconHorisontalAlignment);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            OsmAnd::ZoomLevel,
            getMinZoom);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            OsmAnd::ZoomLevel,
            getMaxZoom);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            bool,
            supportsNaturalObtainDataAsync);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            OsmAnd::PointI,
            getPinIconOffset);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            bool,
            waitForLoading);
    SWIG_EMIT_DIRECTOR_END(MapTiledCollectionProvider);
}

#endif // !defined(_OSMAND_CORE_MAP_TILED_COLLECTION_PROVIDER_H_)
