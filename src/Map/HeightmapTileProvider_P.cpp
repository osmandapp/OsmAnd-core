#include "HeightmapTileProvider_P.h"
#include "HeightmapTileProvider.h"

#include <cassert>

#include <gdal.h>
#include <gdal_priv.h>
#include <cpl_vsi.h>

#include "Logging.h"

OsmAnd::HeightmapTileProvider_P::HeightmapTileProvider_P( HeightmapTileProvider* const owner_, const QDir& dataPath, const QString& indexFilepath )
    : owner(owner_)
    , _tileDb(dataPath, indexFilepath)
{
}

OsmAnd::HeightmapTileProvider_P::~HeightmapTileProvider_P()
{
}

bool OsmAnd::HeightmapTileProvider_P::obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const MapTile>& outTile, const IQueryController* const queryController)
{
    // Obtain raw data from DB
    QByteArray data;
    bool ok = _tileDb.obtainTileData(tileId, zoom, data);
    if(!ok || data.length() == 0)
    {
        // There was no data at all, to avoid further requests, mark this tile as empty
        outTile.reset();
        return true;
    }

    const auto tileSize = owner->getTileSize();

    // We have the data, use GDAL to decode this GeoTIFF
    bool success = false;
    QString vmemFilename;
    vmemFilename.sprintf("/vsimem/heightmapTile@%p", data.data());
    VSIFileFromMemBuffer(qPrintable(vmemFilename), reinterpret_cast<GByte*>(data.data()), data.length(), FALSE);
    auto dataset = reinterpret_cast<GDALDataset*>(GDALOpen(qPrintable(vmemFilename), GA_ReadOnly));
    if(dataset != nullptr)
    {
        bool bad = false;
        bad = bad || dataset->GetRasterCount() != 1;
        bad = bad || dataset->GetRasterXSize() != tileSize;
        bad = bad || dataset->GetRasterYSize() != tileSize;
        if(bad)
        {
            if(dataset->GetRasterCount() != 1)
                LogPrintf(LogSeverityLevel::Error, "Height tile %dx%d@%d has %d bands instead of 1", tileId.x, tileId.y, zoom, dataset->GetRasterCount());
            if(dataset->GetRasterXSize() != tileSize || dataset->GetRasterYSize() != tileSize)
            {
                LogPrintf(LogSeverityLevel::Error, "Height tile %dx%d@%d has %dx%x size instead of %d", tileId.x, tileId.y, zoom,
                    dataset->GetRasterXSize(), dataset->GetRasterYSize(), tileSize);
            }
        }
        else
        {
            auto band = dataset->GetRasterBand(1);

            bad = bad || band->GetColorTable() != nullptr;
            bad = bad || band->GetRasterDataType() != GDT_Int16;

            if(bad)
            {
                if(band->GetColorTable() != nullptr)
                    LogPrintf(LogSeverityLevel::Error, "Height tile %dx%d@%d has color table", tileId.x, tileId.y, zoom);
                if(band->GetRasterDataType() != GDT_Int16)
                    LogPrintf(LogSeverityLevel::Error, "Height tile %dx%d@%d has %s data type in band 1", tileId.x, tileId.y, zoom, GDALGetDataTypeName(band->GetRasterDataType()));
            }
            else
            {
                auto buffer = new float[tileSize*tileSize];

                auto res = dataset->RasterIO(GF_Read, 0, 0, tileSize, tileSize, buffer, tileSize, tileSize, GDT_Float32, 1, nullptr, 0, 0, 0);
                if(res != CE_None)
                {
                    delete[] buffer;
                    LogPrintf(LogSeverityLevel::Error, "Failed to decode height tile %dx%d@%d: %s", tileId.x, tileId.y, zoom, CPLGetLastErrorMsg());
                }
                else
                {
                    outTile.reset(new MapElevationDataTile(buffer, sizeof(float)*tileSize, tileSize));
                    success = true;
                }
            }
        }

        GDALClose(dataset);
    }
    VSIUnlink(qPrintable(vmemFilename));

    return success;
}

OsmAnd::ZoomLevel OsmAnd::HeightmapTileProvider_P::getMinZoom() const
{
    return MinZoomLevel;
}

OsmAnd::ZoomLevel OsmAnd::HeightmapTileProvider_P::getMaxZoom() const
{
    return MaxZoomLevel;
}
