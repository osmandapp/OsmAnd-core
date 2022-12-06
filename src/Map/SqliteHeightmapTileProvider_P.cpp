#include "SqliteHeightmapTileProvider_P.h"
#include "SqliteHeightmapTileProvider.h"

#include <cassert>
#include <memory>

#include "ignore_warnings_on_external_includes.h"
#include <gdal.h>
#include <cpl_vsi.h>
#include "restore_internal_warnings.h"

#include "Logging.h"
#include "MapDataProviderHelpers.h"
#include "TileSqliteDatabasesCollection.h"
#include "GeoTiffCollection.h"
#include "TileSqliteDatabase.h"

OsmAnd::SqliteHeightmapTileProvider_P::SqliteHeightmapTileProvider_P(
    SqliteHeightmapTileProvider* const owner_)
    : owner(owner_)
{
}

OsmAnd::SqliteHeightmapTileProvider_P::~SqliteHeightmapTileProvider_P()
{
}

OsmAnd::ZoomLevel OsmAnd::SqliteHeightmapTileProvider_P::getMinZoom() const
{
    if (owner->filesCollection)
        return ZoomLevel10;
    else
        return owner->sourcesCollection->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::SqliteHeightmapTileProvider_P::getMaxZoom() const
{
    if (owner->filesCollection)
        return owner->filesCollection->getMaxZoom(owner->outputTileSize - 3);
    else
        return owner->sourcesCollection->getMaxZoom();
}

bool OsmAnd::SqliteHeightmapTileProvider_P::obtainData(
    const IMapDataProvider::Request& request_,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    const auto& request = MapDataProviderHelpers::castRequest<IMapElevationDataProvider::Request>(request_);

    if (pOutMetric)
        pOutMetric->reset();

    QByteArray data;
    for (const auto& database : owner->sourcesCollection->getTileSqliteDatabases(request.tileId, request.zoom))
    {
        if (database->obtainTileData(request.tileId, request.zoom, data) && !data.isEmpty())
        {
            break;
        }
    }
    if (data.isEmpty() && owner->filesCollection)
    {
        // There was no data in db, so try to get it from GeoTIFF file
        QString filename = owner->filesCollection->getGeoTiffFilename(
            request.tileId, request.zoom, owner->outputTileSize, 3, getMinZoom());
        if (!filename.isEmpty())
        {
            const auto pBuffer = new float[owner->outputTileSize*owner->outputTileSize];
            if (!owner->filesCollection->getGeoTiffData(
                filename, request.tileId, request.zoom, owner->outputTileSize, 3, pBuffer))
                delete[] pBuffer;
            outData = std::make_shared<IMapElevationDataProvider::Data>(
                request.tileId,
                request.zoom,
                sizeof(float)*owner->outputTileSize,
                owner->outputTileSize,
                pBuffer);
            return true;
        }
    }
    if (data.isEmpty())
    {
        // There was no data at all, to avoid further requests, mark this tile as empty
        outData.reset();
        return true;
    }

    // Map data to VSI memory
    const auto filename = QString::asprintf("/vsimem/heightmapTile@%p", data.data());
    const auto file = VSIFileFromMemBuffer(
        qPrintable(filename),
        reinterpret_cast<GByte*>(data.data()),
        data.length(),
        FALSE
    );
    if (!file)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to map tile %dx%d@%d",
            request.tileId.x,
            request.tileId.y,
            request.zoom);
        return false;
    }
    VSIFCloseL(file);

    // Decode data as GeoTIFF
    std::shared_ptr<void> hDataset(
        GDALOpen(qPrintable(filename), GA_ReadOnly),
        [filename]
        (auto hDataset)
        {
            GDALClose(hDataset);
            VSIUnlink(qPrintable(filename));
        }
    );
    if (!hDataset)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to open map tile %dx%d@%d as GeoTIFF",
            request.tileId.x,
            request.tileId.y,
            request.zoom);
        return false;
    }

    // Load heightmap tile from dataset
    const auto bandsCount = GDALGetRasterCount(hDataset.get());
    if (bandsCount != 1)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Height tile %dx%d@%d has %d bands instead of 1",
            request.tileId.x,
            request.tileId.y,
            request.zoom,
            bandsCount);
        return false;
    }
    const auto hBand = GDALGetRasterBand(hDataset.get(), 1);
    if (GDALGetRasterColorTable(hBand) != nullptr)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Height tile %dx%d@%d has color table",
            request.tileId.x,
            request.tileId.y,
            request.zoom);
        return false;
    }

    const auto pBuffer = new float[owner->outputTileSize*owner->outputTileSize];
    const auto res = GDALDatasetRasterIO(
        hDataset.get(),
        GF_Read,
        0, 0, GDALGetRasterXSize(hDataset.get()), GDALGetRasterYSize(hDataset.get()),
        pBuffer, owner->outputTileSize, owner->outputTileSize,
        GDT_Float32,
        1,
        nullptr,
        0,
        0,
        0);
    if (res != CE_None)
    {
        delete[] pBuffer;
        LogPrintf(LogSeverityLevel::Error,
            "Failed to decode height tile %dx%d@%d: %s",
            request.tileId.x,
            request.tileId.y,
            request.zoom,
            CPLGetLastErrorMsg());
        return false;
    }

    outData = std::make_shared<IMapElevationDataProvider::Data>(
        request.tileId,
        request.zoom,
        sizeof(float)*owner->outputTileSize,
        owner->outputTileSize,
        pBuffer);

    return true;
}
