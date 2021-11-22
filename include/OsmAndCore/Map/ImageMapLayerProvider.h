#ifndef _OSMAND_CORE_IMAGE_MAP_LAYER_PROVIDER_H_
#define _OSMAND_CORE_IMAGE_MAP_LAYER_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QtGlobal>
#include <QThreadPool>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/CommonSWIG.h>
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Map/IRasterMapLayerProvider.h>

class SkBitmap;

namespace OsmAnd
{
    class OSMAND_CORE_API ImageMapLayerProvider
        : public std::enable_shared_from_this<ImageMapLayerProvider>
        , public IRasterMapLayerProvider
    {
        Q_DISABLE_COPY_AND_MOVE(ImageMapLayerProvider);

    public:
        class OSMAND_CORE_API AsyncImage Q_DECL_FINAL
        {
            Q_DISABLE_COPY_AND_MOVE(AsyncImage);

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

            void submit(const bool requestSucceeded, const QByteArray& data) const;

        friend class OsmAnd::ImageMapLayerProvider;
        };

    private:
        const std::shared_ptr<const SkBitmap> getEmptyImage();
        
        mutable QReadWriteLock _lock;
        int _priority;
        ZoomLevel _lastRequestedZoom;
        QThreadPool *_threadPool;
        
        int getAndDecreasePriority();
        ZoomLevel getLastRequestedZoom() const;
        void setLastRequestedZoom(const ZoomLevel zoomLevel);
    protected:
        ImageMapLayerProvider();
        const std::shared_ptr<const SkBitmap> decodeBitmap(const QByteArray& data);
    public:
        virtual ~ImageMapLayerProvider();

        virtual AlphaChannelPresence getAlphaChannelPresence() const = 0;

        virtual bool supportsObtainImageBitmap() const;
        virtual QByteArray obtainImage(
            const SWIG_CLARIFY(ImageMapLayerProvider, Request)& request) = 0;
        virtual const std::shared_ptr<const SkBitmap> obtainImageBitmap(
            const IMapTiledDataProvider::Request& request);

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
        virtual void performAdditionalChecks(std::shared_ptr<const SkBitmap> bitmap);
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
            const SWIG_CLARIFY(ImageMapLayerProvider, Request)& request);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            bool,
            supportsNaturalObtainDataAsync);
        SWIG_EMIT_DIRECTOR_METHOD(
            void,
            obtainImageAsync,
            const SWIG_CLARIFY(ImageMapLayerProvider, Request)& request,
            const SWIG_CLARIFY(ImageMapLayerProvider, AsyncImage)* const asyncImage);
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
