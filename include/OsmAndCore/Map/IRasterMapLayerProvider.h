#ifndef _OSMAND_CORE_I_RASTER_MAP_LAYER_PROVIDER_H_
#define _OSMAND_CORE_I_RASTER_MAP_LAYER_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QtGlobal>

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <SkImage.h>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/CommonSWIG.h>
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Map/IMapLayerProvider.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IRasterMapLayerProvider : public IMapLayerProvider
    {
        Q_DISABLE_COPY_AND_MOVE(IRasterMapLayerProvider);
    public:
        class OSMAND_CORE_API Data : public IMapLayerProvider::Data
        {
            Q_DISABLE_COPY_AND_MOVE(Data);
        private:
        protected:
        public:
            Data(
                TileId tileId,
                ZoomLevel zoom,
                AlphaChannelPresence alphaChannelPresence,
                float densityFactor,
                sk_sp<const SkImage> image,
                const RetainableCacheMetadata* pRetainableCacheMetadata = nullptr);
            Data(
                TileId tileId,
                ZoomLevel zoom,
                AlphaChannelPresence alphaChannelPresence,
                float densityFactor,
                const QHash<int64_t, sk_sp<const SkImage>>& images,
                const RetainableCacheMetadata* pRetainableCacheMetadata = nullptr);
            virtual ~Data();

            AlphaChannelPresence alphaChannelPresence;
            float densityFactor;
            QHash<int64_t, sk_sp<const SkImage>> images;
        };

    private:
    protected:
        IRasterMapLayerProvider();
    public:
        virtual ~IRasterMapLayerProvider();

        virtual uint32_t getTileSize() const = 0;
        virtual float getTileDensityFactor() const = 0;

        virtual bool obtainRasterTile(
            const Request& request,
            std::shared_ptr<Data>& outRasterTile,
            std::shared_ptr<Metric>* const pOutMetric = nullptr);
    };
	
//    SWIG_EMIT_DIRECTOR_BEGIN(IMapRasterBitmapTileProvider);
//        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
//            ZoomLevel,
//            getMinZoom);
//        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
//            ZoomLevel,
//            getMaxZoom);
//NOTE: This won't work due to directors+shared_ptr are not supported. To summarize: it's currently impossible to use any %shared_ptr-marked type in a director declaration
//        SWIG_EMIT_DIRECTOR_METHOD(
//            bool,
//            obtainData,
//            /*SWIG_OMIT(const)*/ TileId tileId,
//            const ZoomLevel zoom,
//            std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
//            const std::shared_ptr<const IQueryController>& queryController);
//        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
//            uint32_t,
//            getTileSize);
//        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
//            float,
//            getTileDensityFactor);
//    SWIG_EMIT_DIRECTOR_END(IMapRasterBitmapTileProvider);
}

#endif // !defined(_OSMAND_CORE_I_RASTER_MAP_LAYER_PROVIDER_H_)
