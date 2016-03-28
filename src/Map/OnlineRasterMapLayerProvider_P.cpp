#include "OnlineRasterMapLayerProvider_P.h"
#include "OnlineRasterMapLayerProvider.h"

#include <cassert>

#include "QtExtensions.h"
#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>

#include "ignore_warnings_on_external_includes.h"
#include <SkStream.h>
#include <SkImageDecoder.h>
#include "restore_internal_warnings.h"

#include "MapDataProviderHelpers.h"
#include "Logging.h"
#include "Utilities.h"

OsmAnd::OnlineRasterMapLayerProvider_P::OnlineRasterMapLayerProvider_P(
    OnlineRasterMapLayerProvider* owner_,
    const std::shared_ptr<const IWebClient>& downloadManager_)
    : owner(owner_)
    , _downloadManager(downloadManager_)
    , _networkAccessAllowed(true)
{
}

OsmAnd::OnlineRasterMapLayerProvider_P::~OnlineRasterMapLayerProvider_P()
{
}

bool OsmAnd::OnlineRasterMapLayerProvider_P::obtainData(
    const IMapDataProvider::Request& request_,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    const auto& request = MapDataProviderHelpers::castRequest<OnlineRasterMapLayerProvider::Request>(request_);

    if (pOutMetric)
        pOutMetric->reset();

    // Check provider can supply this zoom level
    if (request.zoom > owner->maxZoom || request.zoom < owner->minZoom)
    {
        outData.reset();
        return true;
    }

    // Check if requested tile is already being processed, and wait until that's done
    // to mark that as being processed.
    lockTile(request.tileId, request.zoom);

    // Check if requested tile is already in local storage.
    const auto tileLocalRelativePath =
        QString::number(request.zoom) + QDir::separator() +
        QString::number(request.tileId.x) + QDir::separator() +
        QString::number(request.tileId.y) + QLatin1String(".tile");
    QFileInfo localFile;
    {
        QMutexLocker scopedLocker(&_localCachePathMutex);
        localFile.setFile(QDir(_localCachePath).absoluteFilePath(tileLocalRelativePath));
    }
    if (localFile.exists())
    {
        // Since tile is in local storage, it's safe to unmark it as being processed
        unlockTile(request.tileId, request.zoom);

        // If local file is empty, it means that requested tile does not exist (has no data)
        if (localFile.size() == 0)
        {
            outData.reset();
            return true;
        }

        const std::shared_ptr<SkBitmap> bitmap(new SkBitmap());
        SkFILEStream fileStream(qPrintable(localFile.absoluteFilePath()));
        if (!SkImageDecoder::DecodeStream(
                &fileStream,
                bitmap.get(),
                SkColorType::kUnknown_SkColorType,
                SkImageDecoder::kDecodePixels_Mode))
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to decode tile file '%s'",
                qPrintable(localFile.absoluteFilePath()));

            return false;
        }

        assert(bitmap->width() == bitmap->height());
        assert(bitmap->width() == owner->tileSize);

        // Return tile
        outData.reset(new OnlineRasterMapLayerProvider::Data(
            request.tileId,
            request.zoom,
            owner->alphaChannelPresence,
            owner->getTileDensityFactor(),
            bitmap));
        return true;
    }

    // Since tile is not in local cache (or cache is disabled, which is the same),
    // the tile must be downloaded from network:

    // If network access is disallowed, return failure
    if (!_networkAccessAllowed)
    {
        // Before returning, unlock tile
        unlockTile(request.tileId, request.zoom);

        return false;
    }

    // Perform synchronous download
    const auto tileSize = (1u << request.zoom);
    const auto tileUrl = QString(owner->urlPattern)
        .replace(QLatin1String("${osm_zoom}"), QString::number(request.zoom))
        .replace(QLatin1String("${osm_x}"), QString::number(request.tileId.x))
        .replace(QLatin1String("${osm_x_inv}"), QString::number(tileSize - request.tileId.x - 1))
        .replace(QLatin1String("${osm_y}"), QString::number(request.tileId.y))
        .replace(QLatin1String("${osm_y_inv}"), QString::number(tileSize - request.tileId.y - 1))
        .replace(QLatin1String("${quadkey}"), Utilities::getQuadKey(request.tileId.x, request.tileId.y, request.zoom));
    std::shared_ptr<const IWebClient::IRequestResult> requestResult;
    const auto& downloadResult = _downloadManager->downloadData(tileUrl, &requestResult);

    // Ensure that all directories are created in path to local tile
    localFile.dir().mkpath(QLatin1String("."));

    // If there was error, check what the error was
    if (!requestResult->isSuccessful())
    {
        const auto httpStatus = std::dynamic_pointer_cast<const IWebClient::IHttpRequestResult>(requestResult)->getHttpStatusCode();

        LogPrintf(LogSeverityLevel::Warning,
            "Failed to download tile from %s (HTTP status %d)",
            qPrintable(tileUrl),
            httpStatus);

        // 404 means that this tile does not exist, so create a zero file
        if (httpStatus == 404)
        {
            // Save to a file
            QFile tileFile(localFile.absoluteFilePath());
            if (tileFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
            {
                tileFile.close();

                // Unlock the tile
                unlockTile(request.tileId, request.zoom);
                return true;
            }
            else
            {
                LogPrintf(LogSeverityLevel::Error,
                    "Failed to mark tile as non-existent with empty file '%s'",
                    qPrintable(localFile.absoluteFilePath()));

                // Unlock the tile
                unlockTile(request.tileId, request.zoom);
                return false;
            }
        }

        // Unlock the tile
        unlockTile(request.tileId, request.zoom);
        return false;
    }

    // Obtain all data
    LogPrintf(LogSeverityLevel::Verbose,
        "Downloaded tile from %s",
        qPrintable(tileUrl));

    // Save to a file
    QFile tileFile(localFile.absoluteFilePath());
    if (tileFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        tileFile.write(downloadResult);
        tileFile.close();

        LogPrintf(LogSeverityLevel::Verbose,
            "Saved tile from %s to %s",
            qPrintable(tileUrl),
            qPrintable(localFile.absoluteFilePath()));
    }
    else
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to save tile to '%s'",
            qPrintable(localFile.absoluteFilePath()));
    }

    // Unlock tile, since local storage work is done
    unlockTile(request.tileId, request.zoom);

    // Decode in-memory
    const std::shared_ptr<SkBitmap> bitmap(new SkBitmap());
    if (!SkImageDecoder::DecodeMemory(
            downloadResult.constData(), downloadResult.size(),
            bitmap.get(),
            SkColorType::kUnknown_SkColorType,
            SkImageDecoder::kDecodePixels_Mode))
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to decode tile file from '%s'",
            qPrintable(tileUrl));

        return false;
    }

    assert(bitmap->width() == bitmap->height());
    assert(bitmap->width() == owner->tileSize);

    // Return tile
    outData.reset(new OnlineRasterMapLayerProvider::Data(
        request.tileId,
        request.zoom,
        owner->alphaChannelPresence,
        owner->getTileDensityFactor(),
        bitmap));
    return true;
}

void OsmAnd::OnlineRasterMapLayerProvider_P::lockTile(const TileId tileId, const ZoomLevel zoom)
{
    QMutexLocker scopedLocker(&_tilesInProcessMutex);

    while(_tilesInProcess[zoom].contains(tileId))
        _waitUntilAnyTileIsProcessed.wait(&_tilesInProcessMutex);

    _tilesInProcess[zoom].insert(tileId);
}

void OsmAnd::OnlineRasterMapLayerProvider_P::unlockTile(const TileId tileId, const ZoomLevel zoom)
{
    QMutexLocker scopedLocker(&_tilesInProcessMutex);

    _tilesInProcess[zoom].remove(tileId);

    _waitUntilAnyTileIsProcessed.wakeAll();
}
