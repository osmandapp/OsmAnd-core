#include "SlopeRasterMapLayerProvider_P.h"
#include "SlopeRasterMapLayerProvider.h"

#include <cassert>

#include "QtExtensions.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkData.h>
#include <SkImage.h>
#include <cpl_vsi.h>
#include "restore_internal_warnings.h"

#include "MapDataProviderHelpers.h"
#include "MapRenderer.h"
#include "GeoTiffCollection.h"
#include "SkiaUtilities.h"
#include "Logging.h"
#include "Utilities.h"

OsmAnd::SlopeRasterMapLayerProvider_P::SlopeRasterMapLayerProvider_P(
    SlopeRasterMapLayerProvider* owner_,
    const QString& slopeColorsFilename_,
    const ZoomLevel minZoom_,
    const ZoomLevel maxZoom_,
    const uint32_t tileSize_,
    const float densityFactor_)
    : owner(owner_)
    , minZoom(minZoom_)
    , maxZoom(maxZoom_)
    , tileSize(tileSize_)
    , densityFactor(densityFactor_)
{
    const auto fbSize = 1024;
    auto fileBuffer = new char[fbSize];
    memset(fileBuffer, 0, fbSize);
    procParameters.rasterType = GeoTiffCollection::RasterType::Slope;
    procParameters.resultColorsFilename = QString("/vsimem/slopeColorProfile");
    if (const auto file = VSIFileFromMemBuffer(qPrintable(procParameters.resultColorsFilename),
        reinterpret_cast<GByte*>(fileBuffer), fbSize, TRUE))
    {
        bool ok = VSIOverwriteFile(file, qPrintable(slopeColorsFilename_));
        VSIFCloseL(file);
        if (!ok)
        {
            VSIUnlink(qPrintable(procParameters.resultColorsFilename));
            procParameters.resultColorsFilename = QString();
        }
    }
}

OsmAnd::SlopeRasterMapLayerProvider_P::~SlopeRasterMapLayerProvider_P()
{
}

OsmAnd::ZoomLevel OsmAnd::SlopeRasterMapLayerProvider_P::getMinZoom() const
{
    return minZoom;
}

OsmAnd::ZoomLevel OsmAnd::SlopeRasterMapLayerProvider_P::getMaxZoom() const
{
    return maxZoom;
}

bool OsmAnd::SlopeRasterMapLayerProvider_P::obtainData(
    const IMapDataProvider::Request& request_,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    const auto& request = MapDataProviderHelpers::castRequest<SlopeRasterMapLayerProvider::Request>(request_);

    if (pOutMetric)
        pOutMetric->reset();

    // Check provider can supply this zoom level
    if (request.zoom > owner->getMaxZoom() || request.zoom < owner->getMinZoom())
    {
        outData.reset();
        return true;
    }

    // Produce slope RGBA values from height values
    const int bandCount = 4;
    // Extra pixels on both edges give more accurate slope calculations
    const int overlap = 4;
    const auto bufferSize = tileSize * tileSize * bandCount;
    float minValue, maxValue;
    const auto pBuffer = new char[bufferSize];
    const auto result = owner->filesCollection->getGeoTiffData(
        request.tileId,
        request.zoom,
        tileSize + overlap,
        overlap,
        bandCount,
        true,
        minValue,
        maxValue,
        pBuffer,
        &procParameters);
    if (result == GeoTiffCollection::CallResult::Completed)
    {
        auto image = OsmAnd::SkiaUtilities::createSkImageARGB888With(
            QByteArray::fromRawData(pBuffer, bufferSize), tileSize, tileSize, SkAlphaType::kUnpremul_SkAlphaType);
        if (image)
        {
            outData.reset(new IRasterMapLayerProvider::Data(
                request.tileId,
                request.zoom,
                AlphaChannelPresence::Present,
                owner->getTileDensityFactor(),
                image));
        }
        else
        {
            delete[] pBuffer;
            return false;
        }
    }
    else
        outData.reset();
    delete[] pBuffer;
    return result != GeoTiffCollection::CallResult::Failed;
}
