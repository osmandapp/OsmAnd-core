#include "HeightmapTileProvider_P.h"
#include "HeightmapTileProvider.h"

#include <cassert>

#include "ignore_warnings_on_external_includes.h"
#include <gdal.h>
#include <gdal_priv.h>
#include <cpl_vsi.h>
#include "restore_internal_warnings.h"

#include "Logging.h"
#include "MapDataProviderHelpers.h"

OsmAnd::HeightmapTileProvider_P::HeightmapTileProvider_P(
    HeightmapTileProvider* const owner_,
    const QString& dataPath,
    const QString& indexFilename)
    : _tileDb(dataPath, indexFilename)
    , owner(owner_)
{
}

OsmAnd::HeightmapTileProvider_P::~HeightmapTileProvider_P()
{
}

void OsmAnd::HeightmapTileProvider_P::rebuildTileDbIndex()
{
    _tileDb.rebuildIndex();
}

OsmAnd::ZoomLevel OsmAnd::HeightmapTileProvider_P::getMinZoom() const
{
    return MinZoomLevel;
}

OsmAnd::ZoomLevel OsmAnd::HeightmapTileProvider_P::getMaxZoom() const
{
    return MaxZoomLevel;
}

uint32_t OsmAnd::HeightmapTileProvider_P::getTileSize() const
{
    return 32;
}

bool OsmAnd::HeightmapTileProvider_P::obtainData(
    const IMapDataProvider::Request& request_,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    const auto& request = MapDataProviderHelpers::castRequest<HeightmapTileProvider::Request>(request_);

    if (pOutMetric)
        pOutMetric->reset();

    // Obtain raw data from DB
    QByteArray data;
    bool ok = _tileDb.obtainTileData(request.tileId, request.zoom, data);
    if (!ok || data.length() == 0)
    {
        // There was no data at all, to avoid further requests, mark this tile as empty
        outData.reset();
        return true;
    }

    // We have the data, use GDAL to decode this GeoTIFF
    const auto tileSize = getTileSize();
    bool success = false;
    auto vmemFilename = QString::asprintf("/vsimem/heightmapTile@%p", data.data());
    VSIFileFromMemBuffer(qPrintable(vmemFilename), reinterpret_cast<GByte*>(data.data()), data.length(), FALSE);
    auto dataset = reinterpret_cast<GDALDataset*>(GDALOpen(qPrintable(vmemFilename), GA_ReadOnly));
    if (dataset != nullptr)
    {
        bool bad = false;
        bad = bad || dataset->GetRasterCount() != 1;
        bad = bad || dataset->GetRasterXSize() != tileSize;
        bad = bad || dataset->GetRasterYSize() != tileSize;
        if (bad)
        {
            if (dataset->GetRasterCount() != 1)
            {
                LogPrintf(LogSeverityLevel::Error,
                    "Height tile %dx%d@%d has %d bands instead of 1",
                    request.tileId.x,
                    request.tileId.y,
                    request.zoom,
                    dataset->GetRasterCount());
            }
            if (dataset->GetRasterXSize() != tileSize || dataset->GetRasterYSize() != tileSize)
            {
                LogPrintf(LogSeverityLevel::Error,
                    "Height tile %dx%d@%d has %dx%x size instead of %d",
                    request.tileId.x,
                    request.tileId.y,
                    request.zoom,
                    dataset->GetRasterXSize(),
                    dataset->GetRasterYSize(),
                    tileSize);
            }
        }
        else
        {
            auto band = dataset->GetRasterBand(1);

            bad = bad || band->GetColorTable() != nullptr;
            bad = bad || band->GetRasterDataType() != GDT_Int16;

            if (bad)
            {
                if (band->GetColorTable() != nullptr)
                {
                    LogPrintf(LogSeverityLevel::Error,
                        "Height tile %dx%d@%d has color table",
                        request.tileId.x,
                        request.tileId.y,
                        request.zoom);
                }
                if (band->GetRasterDataType() != GDT_Int16)
                {
                    LogPrintf(LogSeverityLevel::Error,
                        "Height tile %dx%d@%d has %s data type in band 1",
                        request.tileId.x,
                        request.tileId.y,
                        request.zoom,
                        GDALGetDataTypeName(band->GetRasterDataType()));
                }
            }
            else
            {
                const auto buffer = new float[tileSize*tileSize];
                const auto res = dataset->RasterIO(
                    GF_Read,
                    0,
                    0,
                    tileSize,
                    tileSize,
                    buffer,
                    tileSize,
                    tileSize,
                    GDT_Float32,
                    1,
                    nullptr,
                    0,
                    0,
                    0);
                if (res != CE_None)
                {
                    delete[] buffer;
                    LogPrintf(LogSeverityLevel::Error,
                        "Failed to decode height tile %dx%d@%d: %s",
                        request.tileId.x,
                        request.tileId.y,
                        request.zoom,
                        CPLGetLastErrorMsg());
                }
                else
                {
                    outData.reset(new IMapElevationDataProvider::Data(
                        request.tileId,
                        request.zoom,
                        tileSize,
                        sizeof(float)*tileSize,
                        buffer));
                    success = true;
                }
            }
        }

        GDALClose(dataset);
    }
    VSIUnlink(qPrintable(vmemFilename));

    return success;
}
