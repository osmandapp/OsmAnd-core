#ifndef _OSMAND_CORE_ONLINE_RASTER_MAP_LAYER_PROVIDER_H_
#define _OSMAND_CORE_ONLINE_RASTER_MAP_LAYER_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QDir>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Map/IRasterMapLayerProvider.h>

namespace OsmAnd
{
    class OnlineRasterMapLayerProvider_P;
    class OSMAND_CORE_API OnlineRasterMapLayerProvider : public IRasterMapLayerProvider
    {
        Q_DISABLE_COPY_AND_MOVE(OnlineRasterMapLayerProvider);
    private:
        PrivateImplementation<OnlineRasterMapLayerProvider_P> _p;
    protected:
    public:
        OnlineRasterMapLayerProvider(
            const QString& name,
            const QString& urlPattern,
            const ZoomLevel minZoom = MinZoomLevel,
            const ZoomLevel maxZoom = MaxZoomLevel,
            const unsigned int maxConcurrentDownloads = 1,
            const unsigned int tileSize = 256,
            const AlphaChannelPresence alphaChannelPresence = AlphaChannelPresence::Unknown,
            const float tileDensityFactor = 1.0f);
        virtual ~OnlineRasterMapLayerProvider();

        const QString name;
        const QString pathSuffix;
        const QString urlPattern;
#if !defined(SWIG)
        //NOTE: This stuff breaks SWIG due to conflict with get*();
        const ZoomLevel minZoom;
        const ZoomLevel maxZoom;
#endif // !defined(SWIG)
        const unsigned int maxConcurrentDownloads;
#if !defined(SWIG)
        //NOTE: This stuff breaks SWIG due to conflict with get*();
        const unsigned int tileSize;
#endif // !defined(SWIG)
        const AlphaChannelPresence alphaChannelPresence;
#if !defined(SWIG)
        //NOTE: This stuff breaks SWIG due to conflict with get*();
        const float tileDensityFactor;
#endif // !defined(SWIG)

        void setLocalCachePath(const QString& localCachePath, const bool appendPathSuffix = true);
        const QString& localCachePath;

        void setNetworkAccessPermission(bool allowed);
        const bool& networkAccessAllowed;

        virtual MapStubStyle getDesiredStubsStyle() const;

        virtual float getTileDensityFactor() const;
        virtual uint32_t getTileSize() const;

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

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;
    };
}

#endif // !defined(_OSMAND_CORE_ONLINE_RASTER_MAP_LAYER_PROVIDER_H_)
