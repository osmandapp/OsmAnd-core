#include "GeoTileEvaluator_P.h"
#include "GeoTileEvaluator.h"

#include <cassert>

#include "ignore_warnings_on_external_includes.h"
#include <gdal.h>
#include <gdal_priv.h>
#include <cpl_vsi.h>
#include <gdal_utils.h>
#include <SkCanvas.h>
#include "restore_internal_warnings.h"

#include "Logging.h"
#include "Utilities.h"
#include "SkiaUtilities.h"
#include "GlobalMercator.h"
#include "GeoTileRasterizer.h"
#include "FunctorQueryController.h"
#include "WeatherTileResourceProvider.h"

OsmAnd::GeoTileEvaluator_P::GeoTileEvaluator_P(
    GeoTileEvaluator* const owner_)
    : owner(owner_)
{
}

OsmAnd::GeoTileEvaluator_P::~GeoTileEvaluator_P()
{
}

bool OsmAnd::GeoTileEvaluator_P::evaluate(
    const LatLon& latLon,
    QList<double>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    if (pOutMetric)
        pOutMetric->reset();
    
    if (owner->geoTileData.length() == 0)
        return false;
        
    const auto geoTileId = owner->geoTileId;
    const auto geoTileZoom = WeatherTileResourceProvider::getGeoTileZoom();
    const auto zoom = owner->zoom;

    // Map data to VSI memory
    auto geoTileData(owner->geoTileData);
    const auto filename = QString::asprintf("/vsimem/geoTile@%p_%dx%dx%d_%s_eval_source",
        this, (int) zoom, geoTileId.x, geoTileId.y, qPrintable(latLon.toQString()));
    const auto file = VSIFileFromMemBuffer(
        qPrintable(filename),
        reinterpret_cast<GByte*>(geoTileData.data()),
        geoTileData.length(),
        FALSE
    );
    if (!file)
    {
        LogPrintf(LogSeverityLevel::Error,
                  "Failed to map geotiff %dx%d@%d for evaluation",
                  geoTileId.x,
                  geoTileId.y,
                  geoTileZoom);
        return false;
    }
    VSIFCloseL(file);
    
    if (queryController && queryController->isAborted())
        return false;

    auto projSearchPath = owner->projSearchPath.toUtf8();
    const char* projPaths[] = { projSearchPath.constData(), NULL };
    OSRSetPROJSearchPaths(projPaths);
    
    // Decode data as GeoTIFF
    std::shared_ptr<void> hSourceDS(
        GDALOpen(qPrintable(filename), GA_ReadOnly),
            [filename]
            (auto hSourceDS)
            {
                GDALClose(hSourceDS);
                VSIUnlink(qPrintable(filename));
            }
        );
    if (!hSourceDS)
    {
        LogPrintf(LogSeverityLevel::Error,
                  "Failed to open %dx%d@%d as GeoTIFF for evaluation",
                  geoTileId.x,
                  geoTileId.y,
                  geoTileZoom);
        return false;
    }

    int bandsCount = GDALGetRasterCount(hSourceDS.get());
    if (bandsCount == 0)
    {
        LogPrintf(LogSeverityLevel::Error,
                  "No bands %dx%d@%d for evaluation",
                  geoTileId.x,
                  geoTileId.y,
                  geoTileZoom);
        return false;
    }
        
    double geoTransform[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    
    double minLat;
    double minLon;
    double maxLat;
    double maxLon;

    double pxSizeLat;
    double pxSizeLon;
    
    if (GDALGetGeoTransform(hSourceDS.get(), geoTransform) == CE_None)
    {
        const auto width = GDALGetRasterXSize(hSourceDS.get());
        const auto height = GDALGetRasterYSize(hSourceDS.get());

        minLon = geoTransform[0];
        maxLon = geoTransform[0] + width * geoTransform[1];
        minLat = geoTransform[3];
        maxLat = geoTransform[3] - height * geoTransform[1];
        pxSizeLon = geoTransform[1];
        pxSizeLat = geoTransform[1];
    }
    else
    {
        LogPrintf(LogSeverityLevel::Error,
                  "Failed get geo transform %dx%d@%d for evaluation",
                  geoTileId.x,
                  geoTileId.y,
                  geoTileZoom);
        return false;
    }
        
    auto zoomCoef = Utilities::getPowZoom(zoom - geoTileZoom);
    zoomCoef = zoomCoef > 64.0 ? 64.0 : zoomCoef;
    
    int originMatrixSize = 4;
    int matrixSize = (int)(originMatrixSize * zoomCoef);
    int bufferSize = matrixSize * matrixSize * bandsCount;

    double iy = (latLon.latitude - minLat) / -pxSizeLat;
    double ix = (latLon.longitude - minLon) / pxSizeLon;
    int px = (int) floor(ix) - originMatrixSize / 2;
    int py = (int) floor(iy) - originMatrixSize / 2;
    px = px < 0 ? 0 : px;
    py = py < 0 ? 0 : py;
    double dx = (ix - px) * zoomCoef;
    double dy = (iy - py) * zoomCoef;

    float *pBuffer = static_cast<float *>(CPLMalloc(sizeof(float) * bufferSize));
    GDALRasterIOExtraArg extraArg;
    INIT_RASTERIO_EXTRA_ARG(extraArg);
    extraArg.eResampleAlg = GRIORA_Cubic;
    
    const auto res = GDALDatasetRasterIOEx(
        hSourceDS.get(),
        GF_Read,
        px, py, originMatrixSize, originMatrixSize,
        pBuffer, matrixSize, matrixSize,
        GDT_Float32,
        bandsCount,
        nullptr,
        0,
        0,
        0,
        &extraArg);
    if (res != CE_None)
    {
        CPLFree(pBuffer);
        LogPrintf(LogSeverityLevel::Error,
            "Failed to decode interpolated tile %dx%d for evaluation: %s", px, py, CPLGetLastErrorMsg());
        return false;
    }
        
    outData.append(0);
    for (int band = 1; band <= bandsCount; band++)
    {
        int bandShift = matrixSize * matrixSize * (band - 1);
        int index = bandShift + (int)dy * matrixSize + (int)dx;
        if (index >= bufferSize)
        {
            CPLFree(pBuffer);
            LogPrintf(LogSeverityLevel::Error,
                      "Failed to acquire interpolated value at %fx%f@%d for evaluation: index(%d) >= bufferSize(%d)", dx, dy, band, index, bufferSize);
            return false;
        }
        else
        {
            double value = pBuffer[index];
            const auto hBand = GDALGetRasterBand(hSourceDS.get(), band);
            
            int ok = false;
            double noDataValue = GDALGetRasterNoDataValue(hBand, &ok);
            if (ok && value == noDataValue)
                continue;

            double minValue = GDALGetRasterMinimum(hBand, &ok);
            if (ok && value < minValue)
                value = minValue;

            double maxValue = GDALGetRasterMaximum(hBand, &ok);
            if (ok && value > maxValue)
                value = maxValue;

            outData.append(value);

//            LogPrintf(LogSeverityLevel::Debug,
//                      "Evaluated geotiff at %s@%d = %f",
//                      qPrintable(latLon.toQString()), band, value);
        }
    }
    
    CPLFree(pBuffer);
    return true;
}
