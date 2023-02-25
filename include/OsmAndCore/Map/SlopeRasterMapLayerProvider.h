#ifndef _OSMAND_CORE_SLOPE_RASTER_MAP_LAYER_PROVIDER_H_
#define _OSMAND_CORE_SLOPE_RASTER_MAP_LAYER_PROVIDER_H_

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
    class SlopeRasterMapLayerProvider_P;
    class OSMAND_CORE_API SlopeRasterMapLayerProvider
        : public std::enable_shared_from_this<SlopeRasterMapLayerProvider>
        , public IRasterMapLayerProvider
    {
        Q_DISABLE_COPY_AND_MOVE(SlopeRasterMapLayerProvider);
    private:
        PrivateImplementation<SlopeRasterMapLayerProvider_P> _p;
    protected:
        
        mutable QReadWriteLock _lock;
        int _priority;
        ZoomLevel _lastRequestedZoom;
        mutable QMutex _threadPoolMutex;
        QThreadPool* _threadPool;

        ZoomLevel getLastRequestedZoom() const;
        void setLastRequestedZoom(const ZoomLevel zoomLevel);
        int getAndDecreasePriority();
    public:
        SlopeRasterMapLayerProvider(
            const std::shared_ptr<const IGeoTiffCollection>& filesCollection,
            const QString& slopeColorsFilename,
            const uint32_t tileSize = 256,
            const float densityFactor = 1.0f            
        );
        virtual ~SlopeRasterMapLayerProvider();

        const std::shared_ptr<const IGeoTiffCollection> filesCollection;

        virtual MapStubStyle getDesiredStubsStyle() const Q_DECL_OVERRIDE;

        virtual ZoomLevel getMinZoom() const Q_DECL_OVERRIDE;
        virtual ZoomLevel getMaxZoom() const Q_DECL_OVERRIDE;
        virtual int getMaxMissingDataUnderZoomShift() const Q_DECL_OVERRIDE;

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

#endif // !defined(_OSMAND_CORE_SLOPE_RASTER_MAP_LAYER_PROVIDER_H_)
