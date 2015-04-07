#ifndef _OSMAND_CORE_IMAGE_MAP_LAYER_PROVIDER_H_
#define _OSMAND_CORE_IMAGE_MAP_LAYER_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QtGlobal>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/CommonSWIG.h>
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Map/IRasterMapLayerProvider.h>

class SkBitmap;

namespace OsmAnd
{
    class OSMAND_CORE_API ImageMapLayerProvider : public IRasterMapLayerProvider
    {
        Q_DISABLE_COPY_AND_MOVE(ImageMapLayerProvider);
    private:
    protected:
        ImageMapLayerProvider();
    public:
        virtual ~ImageMapLayerProvider();

        virtual AlphaChannelPresence getAlphaChannelPresence() const = 0;
        virtual QByteArray obtainImage(
            const TileId tileId,
            const ZoomLevel zoom) = 0;

        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
            std::shared_ptr<Metric>* pOutMetric = nullptr,
            const IQueryController* const queryController = nullptr) Q_DECL_FINAL;
    };

    SWIG_EMIT_DIRECTOR_BEGIN(ImageMapLayerProvider);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            ZoomLevel,
            getMinZoom);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            ZoomLevel,
            getMaxZoom);
        SWIG_EMIT_DIRECTOR_METHOD(
            QByteArray,
            obtainImage,
            SWIG_OMIT(const) TileId tileId,
            const ZoomLevel zoom);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            uint32_t,
            getTileSize);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            float,
            getTileDensityFactor);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            AlphaChannelPresence,
            getAlphaChannelPresence);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            MapStubStyle,
            getDesiredStubsStyle);
    SWIG_EMIT_DIRECTOR_END(ImageMapLayerProvider);
}

#endif // !defined(_OSMAND_CORE_IMAGE_MAP_LAYER_PROVIDER_H_)
