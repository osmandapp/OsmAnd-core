#ifndef _OSMAND_CORE_HEIGHT_RASTER_MAP_LAYER_PROVIDER_H_
#define _OSMAND_CORE_HEIGHT_RASTER_MAP_LAYER_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QDir>
#include <QString>
#include <QReadWriteLock>
#include <QThreadPool>
#include <QMutex>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/IRasterMapLayerProvider.h>

namespace OsmAnd
{
    class IGeoTiffCollection;
    class HeightRasterMapLayerProvider_P;
    class OSMAND_CORE_API HeightRasterMapLayerProvider
        : public std::enable_shared_from_this<HeightRasterMapLayerProvider>
        , public IRasterMapLayerProvider
    {
        Q_DISABLE_COPY_AND_MOVE(HeightRasterMapLayerProvider);
    private:
        PrivateImplementation<HeightRasterMapLayerProvider_P> _p;
    protected:
        
        mutable QReadWriteLock _lock;
        int _priority;
        ZoomLevel _lastRequestedZoom;
        ZoomLevel _minVisibleZoom;
        ZoomLevel _maxVisibleZoom;
        mutable QMutex _threadPoolMutex;
        QThreadPool* _threadPool;

        ZoomLevel getLastRequestedZoom() const;
        void setLastRequestedZoom(const ZoomLevel zoomLevel);
        int getAndDecreasePriority();
    public:
        HeightRasterMapLayerProvider(
            const std::shared_ptr<const IGeoTiffCollection>& filesCollection,
            const QString& heightColorsFilename,
            const ZoomLevel minZoom = ZoomLevel6,
            const ZoomLevel maxZoom = ZoomLevel19,
            const uint32_t tileSize = 256,
            const float densityFactor = 1.0f            
        );
        virtual ~HeightRasterMapLayerProvider();

        const std::shared_ptr<const IGeoTiffCollection> filesCollection;

        virtual MapStubStyle getDesiredStubsStyle() const Q_DECL_OVERRIDE;

        virtual ZoomLevel getMinZoom() const Q_DECL_OVERRIDE;
        virtual ZoomLevel getMaxZoom() const Q_DECL_OVERRIDE;

        virtual ZoomLevel getMinVisibleZoom() const Q_DECL_OVERRIDE;
        virtual ZoomLevel getMaxVisibleZoom() const Q_DECL_OVERRIDE;

        void setMinVisibleZoom(const ZoomLevel zoomLevel);
        void setMaxVisibleZoom(const ZoomLevel zoomLevel);

        virtual uint32_t getTileSize() const Q_DECL_OVERRIDE;
        virtual float getTileDensityFactor() const Q_DECL_OVERRIDE;

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
    };
}

#endif // !defined(_OSMAND_CORE_HEIGHT_RASTER_MAP_LAYER_PROVIDER_H_)
