#include "WeatherRasterLayerProvider_P.h"
#include "WeatherRasterLayerProvider.h"

#include <cassert>

#include "ignore_warnings_on_external_includes.h"
#include <gdal.h>
#include <gdal_priv.h>
#include <cpl_vsi.h>
#include <gdal_utils.h>
#include <SkCanvas.h>
#include "restore_internal_warnings.h"

#include "Logging.h"
#include "MapDataProviderHelpers.h"

OsmAnd::WeatherRasterLayerProvider_P::WeatherRasterLayerProvider_P(
    WeatherRasterLayerProvider* const owner_)
    : owner(owner_)
{
}

OsmAnd::WeatherRasterLayerProvider_P::~WeatherRasterLayerProvider_P()
{
}

OsmAnd::ZoomLevel OsmAnd::WeatherRasterLayerProvider_P::getMinZoom() const
{
    return MinZoomLevel;
}

OsmAnd::ZoomLevel OsmAnd::WeatherRasterLayerProvider_P::getMaxZoom() const
{
    return MaxZoomLevel;
}

bool OsmAnd::WeatherRasterLayerProvider_P::obtainData(
    const IMapDataProvider::Request& request_,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    const auto& request = MapDataProviderHelpers::castRequest<IRasterMapLayerProvider::Request>(request_);
    if (pOutMetric)
        pOutMetric->reset();

    QReadLocker scopedLocker(&_dataLock);
    
    if (geotiffData.length() == 0)
    {
        QMutexLocker scopedLocker(&_dataMutex);

        auto transFilename = QString(owner->geotiffPath) + QStringLiteral(".trans.tiff");
        QFile tileFile(owner->geotiffPath);
        if (tileFile.open(QIODevice::ReadOnly))
        {
            geotiffData = tileFile.readAll();
            tileFile.close();
            
            if (geotiffData.length() == 0)
            {
                // There was no data at all, to avoid further requests, mark this tile as empty
                outData.reset();
                return true;
            }
            
            QString vmemFilename;
            vmemFilename.sprintf("/vsimem/weatherTile@%p", geotiffData.data());
            VSIFileFromMemBuffer(qPrintable(vmemFilename), reinterpret_cast<GByte*>(geotiffData.data()), geotiffData.length(), FALSE);
            auto hSrcDS = GDALOpen(qPrintable(vmemFilename), GA_ReadOnly);
            if (hSrcDS)
            {
                const char* args[] = { "-b", "5", "-srcwin", "600", "500", "150", "125", "-outsize", "2400", "2000", "-r", "lanczos", NULL };
                GDALTranslateOptions* psOptions = GDALTranslateOptionsNew((char**)args, NULL);
                
                int bUsageError = FALSE;
                auto hOutDS = GDALTranslate(qPrintable(transFilename), hSrcDS, psOptions, &bUsageError);
                if (bUsageError)
                {
                    //Usage();
                }
                GDALTranslateOptionsFree(psOptions);
                if (hOutDS)
                {
                    GDALClose(hOutDS);
                }
                GDALClose(hSrcDS);
            }
            VSIUnlink(qPrintable(vmemFilename));
        }
        QFile transFile(transFilename);
        if (transFile.open(QIODevice::ReadOnly))
        {
            auto data = transFile.readAll();
            transFile.close();
            
            if (data.length() == 0)
            {
                // There was no data at all, to avoid further requests, mark this tile as empty
                outData.reset();
                return true;
            }
            
            QString vmemFilename;
            vmemFilename.sprintf("/vsimem/transWeatherTile@%p", data.data());
            VSIFileFromMemBuffer(qPrintable(vmemFilename), reinterpret_cast<GByte*>(data.data()), data.length(), FALSE);
            auto hSrcDS = GDALOpen(qPrintable(vmemFilename), GA_ReadOnly);
            if (hSrcDS)
            {
                auto imgFilename = QString(owner->geotiffPath) + QStringLiteral(".png");
                auto colorFilename = QString(owner->geotiffPath) + QStringLiteral(".clr");

                const char* argv[] = { "-alpha", NULL };

                GDALDEMProcessingOptions *psOptions = GDALDEMProcessingOptionsNew(const_cast<char **>(argv), NULL);
                if (psOptions == NULL)
                {
                    //Usage();
                }
                                                
                int bUsageError = FALSE;
                auto hOutDS = GDALDEMProcessing(qPrintable(imgFilename),
                                                hSrcDS,
                                                "color-relief",
                                                qPrintable(colorFilename),
                                                psOptions,
                                                &bUsageError);
                if (bUsageError)
                {
                    //Usage();
                }
                if (hOutDS)
                {
                    GDALClose(hOutDS);
                }
                if (psOptions)
                {
                    GDALDEMProcessingOptionsFree(psOptions);
                }
                GDALClose(hSrcDS);
            }
        }
    }
    if (geotiffData.length() == 0)
    {
        // There was no data at all, to avoid further requests, mark this tile as empty
        outData.reset();
        return true;
    }

    // We have the data, use GDAL to decode this GeoTIFF
    int bandNumber = 5;
    const auto tileSize = owner->tileSize;
    bool success = false;
    QString vmemFilename;
    vmemFilename.sprintf("/vsimem/weatherTile@%p", geotiffData.data());
    VSIFileFromMemBuffer(qPrintable(vmemFilename), reinterpret_cast<GByte*>(geotiffData.data()), geotiffData.length(), FALSE);
    auto dataset = reinterpret_cast<GDALDataset *>(GDALOpen(qPrintable(vmemFilename), GA_ReadOnly));
    if (dataset != nullptr)
    {
        bool bad = false;
        bad = bad || bandNumber > dataset->GetRasterCount();
        //bad = bad || dataset->GetRasterXSize() != tileSize;
        //bad = bad || dataset->GetRasterYSize() != tileSize;
        if (bad)
        {
            if (bandNumber > dataset->GetRasterCount())
            {
                LogPrintf(LogSeverityLevel::Error,
                    "Weather tile %dx%d@%d requested %d band but dataset has %d bands",
                    request.tileId.x,
                    request.tileId.y,
                    request.zoom,
                    bandNumber,
                    dataset->GetRasterCount());
            }
            /*
            if (dataset->GetRasterXSize() != tileSize || dataset->GetRasterYSize() != tileSize)
            {
                LogPrintf(LogSeverityLevel::Error,
                    "Weather tile %dx%d@%d has %dx%x size instead of %d",
                    request.tileId.x,
                    request.tileId.y,
                    request.zoom,
                    dataset->GetRasterXSize(),
                    dataset->GetRasterYSize(),
                    tileSize);
            }
            */
        }
        else
        {
            auto band = dataset->GetRasterBand(bandNumber);

            bad = bad || band->GetColorTable() != nullptr;
            bad = bad || band->GetRasterDataType() != GDT_Float64;

            if (bad)
            {
                if (band->GetColorTable() != nullptr)
                {
                    LogPrintf(LogSeverityLevel::Error,
                        "Weather tile %dx%d@%d has color table",
                        request.tileId.x,
                        request.tileId.y,
                        request.zoom);
                }
                if (band->GetRasterDataType() != GDT_Float64)
                {
                    LogPrintf(LogSeverityLevel::Error,
                        "Weather tile %dx%d@%d has %s data type in band 1",
                        request.tileId.x,
                        request.tileId.y,
                        request.zoom,
                        GDALGetDataTypeName(band->GetRasterDataType()));
                }
            }
            else
            {
                const auto buffer = new double[tileSize * tileSize];
                //CPLErr res = CE_None;
                const auto res = dataset->RasterIO(
                    GF_Read,
                    0,
                    0,
                    tileSize,
                    tileSize,
                    buffer,
                    tileSize,
                    tileSize,
                    GDT_Float64,
                    1,
                    nullptr,
                    0,
                    0,
                    0);
                if (res != CE_None)
                {
                    LogPrintf(LogSeverityLevel::Error,
                        "Failed to decode weather tile %dx%d@%d: %s",
                        request.tileId.x,
                        request.tileId.y,
                        request.zoom,
                        CPLGetLastErrorMsg());
                }
                else
                {
                    /*
                    outData.reset(new IMapElevationDataProvider::Data(
                        request.tileId,
                        request.zoom,
                        tileSize,
                        sizeof(float)*tileSize,
                        buffer));
                     */
                    
                    // Prepare drawing canvas
                    SkBitmap bitmap;
                    if (!bitmap.tryAllocPixels(SkImageInfo::MakeN32Premul(tileSize, tileSize)))
                    {
                        LogPrintf(LogSeverityLevel::Error,
                            "Failed to allocate buffer for rasterization weather tile %dx%d",
                            tileSize,
                            tileSize);
                    }
                    else
                    {
                        bitmap.eraseColor(SK_ColorLTGRAY);
                        const auto image = bitmap.asImage();
                        if (image)
                        {
                            outData.reset(new IRasterMapLayerProvider::Data(
                                request.tileId,
                                request.zoom,
                                AlphaChannelPresence::Present,
                                owner->densityFactor,
                                bitmap.asImage()));
                            
                            success = true;
                        }
                    }
                }
                delete[] buffer;
            }
        }

        GDALClose(dataset);
    }
    VSIUnlink(qPrintable(vmemFilename));

    return success;
}
