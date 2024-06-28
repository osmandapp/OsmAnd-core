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
    auto minZoomDatabase = InvalidZoomLevel;
    if (owner->sourcesCollection)
        minZoomDatabase = owner->sourcesCollection->getMinZoom();
    auto minZoomTiff = InvalidZoomLevel;
    if (owner->filesCollection)
        minZoomTiff = owner->filesCollection->getMinZoom();
    if (minZoomDatabase == InvalidZoomLevel && minZoomTiff != InvalidZoomLevel)
        return minZoomTiff;
    else if (minZoomTiff == InvalidZoomLevel && minZoomDatabase != InvalidZoomLevel)
        return minZoomDatabase;
    else if (minZoomDatabase != InvalidZoomLevel && minZoomTiff != InvalidZoomLevel)
        return std::max(minZoomDatabase, minZoomTiff);
    else
        return MaxZoomLevel;
}

OsmAnd::ZoomLevel OsmAnd::SqliteHeightmapTileProvider_P::getMaxZoom() const
{
    auto maxZoomDatabase = InvalidZoomLevel;
    if (owner->sourcesCollection)
        maxZoomDatabase = owner->sourcesCollection->getMaxZoom();
    auto maxZoomTiff = InvalidZoomLevel;
    if (owner->filesCollection)
        maxZoomTiff = ZoomLevel31;
    if (maxZoomDatabase == InvalidZoomLevel && maxZoomTiff != InvalidZoomLevel)
        return maxZoomTiff;
    else if (maxZoomTiff == InvalidZoomLevel && maxZoomDatabase != InvalidZoomLevel)
        return maxZoomDatabase;
    else if (maxZoomDatabase != InvalidZoomLevel && maxZoomTiff != InvalidZoomLevel)
        return std::min(maxZoomDatabase, maxZoomTiff);
    else
        return MaxZoomLevel;
}

int OsmAnd::SqliteHeightmapTileProvider_P::getMaxMissingDataZoomShift(int defaultMaxMissingDataZoomShift) const
{
    // Set overscale limit for terrain elevation data to make it available on high zoom levels
    const int maxMissingDataZoomShift = std::max(ZoomLevel22 - getMaxZoom(), defaultMaxMissingDataZoomShift);
    return maxMissingDataZoomShift;
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
    if (owner->sourcesCollection)
    {
        for (const auto& database : owner->sourcesCollection->getTileSqliteDatabases(request.tileId, request.zoom))
        {
            if (database->retrieveTileData(request.tileId, request.zoom, data) && !data.isEmpty())
            {
                break;
            }
        }
    }
    if (data.isEmpty() && owner->filesCollection)
    {
        // There was no data in db, so try to get it from GeoTIFF file
        float minValue, maxValue;
        const auto pBuffer = new float[owner->outputTileSize * owner->outputTileSize];
        const auto result = owner->filesCollection->getGeoTiffData(
            request.tileId, request.zoom, owner->outputTileSize, 3, 1, false, minValue, maxValue, pBuffer);
        if (result == GeoTiffCollection::CallResult::Completed)
        {
            outData = std::make_shared<IMapElevationDataProvider::Data>(
                request.tileId,
                request.zoom,
                sizeof(float)*owner->outputTileSize,
                owner->outputTileSize,
                minValue,
                maxValue,
                pBuffer);
            return true;
        }
        else
            delete[] pBuffer;
        if (result == GeoTiffCollection::CallResult::Failed)
            return false;
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
        0.0f,
        0.0f,
        pBuffer);

    return true;
}
