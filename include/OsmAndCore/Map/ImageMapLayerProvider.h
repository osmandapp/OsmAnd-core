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
        Q_DISABLE_COPY_AND_MOVE(ImageMapLayerProvider)

    public:
        class OSMAND_CORE_API AsyncImage Q_DECL_FINAL
        {
            Q_DISABLE_COPY_AND_MOVE(AsyncImage)

        private:
        protected:
            AsyncImage(
                const ImageMapLayerProvider* const provider,
                const ImageMapLayerProvider::Request& request,
                const IMapDataProvider::ObtainDataAsyncCallback callback);
        public:
            virtual ~AsyncImage();

            const ImageMapLayerProvider* const provider;
            const std::shared_ptr<const ImageMapLayerProvider::Request> request;
            const IMapDataProvider::ObtainDataAsyncCallback callback;

            void submit(const bool requestSucceeded, const QByteArray& image) const;

        friend class OsmAnd::ImageMapLayerProvider;
        };

    private:
    protected:
        ImageMapLayerProvider();
    public:
        virtual ~ImageMapLayerProvider();

        virtual AlphaChannelPresence getAlphaChannelPresence() const = 0;

        virtual QByteArray obtainImage(
            const SWIG_CLARIFY(ImageMapLayerProvider, Request)& request) = 0;
        virtual bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr) Q_DECL_OVERRIDE Q_DECL_FINAL;

        virtual void obtainImageAsync(
            const SWIG_CLARIFY(ImageMapLayerProvider, Request)& request,
            const SWIG_CLARIFY(ImageMapLayerProvider, AsyncImage)* const asyncImage) = 0;
        virtual void obtainDataAsync(
            const IMapDataProvider::Request& request,
            const IMapDataProvider::ObtainDataAsyncCallback callback,
            const bool collectMetric = false) Q_DECL_OVERRIDE Q_DECL_FINAL;
    };

    SWIG_EMIT_DIRECTOR_BEGIN(ImageMapLayerProvider);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            ZoomLevel,
            getMinZoom);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            ZoomLevel,
            getMaxZoom);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            bool,
            supportsNaturalObtainData);
        SWIG_EMIT_DIRECTOR_METHOD(
            QByteArray,
            obtainImage,
            SWIG_OMIT(const) SWIG_CLARIFY(ImageMapLayerProvider, Request)& request);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            bool,
            supportsNaturalObtainDataAsync);
        SWIG_EMIT_DIRECTOR_METHOD(
            void,
            obtainImageAsync,
            SWIG_OMIT(const) SWIG_CLARIFY(ImageMapLayerProvider, Request)& request,
            const SWIG_CLARIFY(ImageMapLayerProvider, AsyncImage)* SWIG_OMIT(const) asyncImage);
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
