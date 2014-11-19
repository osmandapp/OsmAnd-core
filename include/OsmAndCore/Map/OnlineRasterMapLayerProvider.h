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
        OnlineRasterMapLayerProvider(const QString& name, const QString& urlPattern,
            const ZoomLevel minZoom = MinZoomLevel, const ZoomLevel maxZoom = MaxZoomLevel,
            const uint32_t maxConcurrentDownloads = 1, const uint32_t providerTileSize = 256,
            const AlphaChannelPresence alphaChannelPresence = AlphaChannelPresence::Unknown);
        virtual ~OnlineRasterMapLayerProvider();

        const QString name;
        const QString pathSuffix;
        const QString urlPattern;
#if !defined(SWIG)
        //NOTE: This stuff breaks SWIG due to conflict with get*();
        const ZoomLevel minZoom;
        const ZoomLevel maxZoom;
#endif // !defined(SWIG)
        const uint32_t maxConcurrentDownloads;
        const uint32_t providerTileSize;
        const AlphaChannelPresence alphaChannelPresence;

        void setLocalCachePath(const QDir& localCachePath, const bool appendPathSuffix = true);
        const QDir& localCachePath;

        void setNetworkAccessPermission(bool allowed);
        const bool& networkAccessAllowed;

        virtual float getTileDensityFactor() const;
        virtual uint32_t getTileSize() const;

        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
            std::shared_ptr<Metric>* pOutMetric = nullptr,
            const IQueryController* const queryController = nullptr);

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;
    };
}

#endif // !defined(_OSMAND_CORE_ONLINE_RASTER_MAP_LAYER_PROVIDER_H_)
