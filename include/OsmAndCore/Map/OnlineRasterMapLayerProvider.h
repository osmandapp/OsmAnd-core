#ifndef _OSMAND_CORE_ONLINE_RASTER_MAP_LAYER_PROVIDER_H_
#define _OSMAND_CORE_ONLINE_RASTER_MAP_LAYER_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QDir>
#include <QReadWriteLock>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/IWebClient.h>
#include <OsmAndCore/WebClient.h>
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Map/IRasterMapLayerProvider.h>
#include <OsmAndCore/Map/IOnlineTileSources.h>

namespace OsmAnd
{
    class OnlineRasterMapLayerProvider_P;
    class OSMAND_CORE_API OnlineRasterMapLayerProvider
        : public std::enable_shared_from_this<OnlineRasterMapLayerProvider>
        , public IRasterMapLayerProvider
    {
        Q_DISABLE_COPY_AND_MOVE(OnlineRasterMapLayerProvider);
    private:
        PrivateImplementation<OnlineRasterMapLayerProvider_P> _p;
    protected:
        
        QThreadPool *_threadPool;
        
        mutable QReadWriteLock _lock;
        ZoomLevel _lastRequestedZoom;
        int _priority;

        ZoomLevel getLastRequestedZoom() const;
        void setLastRequestedZoom(const ZoomLevel zoomLevel);
        int getAndDecreasePriority();
    public:
        OnlineRasterMapLayerProvider(
            const std::shared_ptr<const IOnlineTileSources::Source> tileSource,
            const std::shared_ptr<const IWebClient>& webClient = std::shared_ptr<const IWebClient>(new WebClient()));
        virtual ~OnlineRasterMapLayerProvider();

        const QString name;
        const QString pathSuffix;
        const std::shared_ptr<const IOnlineTileSources::Source> _tileSource;
        const unsigned int maxConcurrentDownloads;
        const AlphaChannelPresence alphaChannelPresence;
#if !defined(SWIG)
        //NOTE: This stuff breaks SWIG due to conflict with get*();
        const float tileDensityFactor;
#endif // !defined(SWIG)

        void setLocalCachePath(const QString& localCachePath, const bool appendPathSuffix = true);
        const QString& localCachePath;

        void setNetworkAccessPermission(bool allowed);
        const bool& networkAccessAllowed;

        virtual MapStubStyle getDesiredStubsStyle() const Q_DECL_OVERRIDE;

        virtual float getTileDensityFactor() const Q_DECL_OVERRIDE;
        virtual uint32_t getTileSize() const Q_DECL_OVERRIDE;

        virtual bool supportsNaturalObtainData() const Q_DECL_OVERRIDE;
        virtual bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr) Q_DECL_OVERRIDE;

        virtual bool supportsNaturalObtainDataAsync() const Q_DECL_OVERRIDE;
        virtual void obtainDataAsync(
            const IMapDataProvider::Request& request,
            const IMapDataProvider::ObtainDataAsyncCallback callback,
            const bool collectMetric = false) Q_DECL_OVERRIDE;

        virtual ZoomLevel getMinZoom() const Q_DECL_OVERRIDE;
        virtual ZoomLevel getMaxZoom() const Q_DECL_OVERRIDE;
        
        static const QString buildUrlToLoad(const QString& urlToLoad, const QList<QString> randomsArray, int32_t x, int32_t y, const ZoomLevel zoom);
    };
}

#endif // !defined(_OSMAND_CORE_ONLINE_RASTER_MAP_LAYER_PROVIDER_H_)
