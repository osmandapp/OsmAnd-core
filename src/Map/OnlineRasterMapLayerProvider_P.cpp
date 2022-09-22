#include "OnlineRasterMapLayerProvider_P.h"
#include "OnlineRasterMapLayerProvider.h"

#include <cassert>

#include "QtExtensions.h"
#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QDateTime>

#include "ignore_warnings_on_external_includes.h"
#include <SkData.h>
#include <SkImage.h>
#include "restore_internal_warnings.h"

#include "MapDataProviderHelpers.h"
#include "SkiaUtilities.h"
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
    if (request.zoom > owner->getMaxZoom() || request.zoom < owner->getMinZoom())
    {
        outData.reset();
        return true;
    }

    auto tileId = request.tileId;
    auto zoom = request.zoom;
    double offsetY = 0;
    const auto& source = owner->_tileSource;
    if (source->ellipticYTile)
    {
        double latitude = OsmAnd::Utilities::getLatitudeFromTile(zoom, tileId.y);
        auto numberOffset = OsmAnd::Utilities::getTileEllipsoidNumberAndOffsetY(zoom, latitude, source->tileSize);
        tileId.y = numberOffset.x + (outData ? 1 : 0);
        offsetY = numberOffset.y;
    }
    bool shiftedTile = offsetY > 0;

    // Check if requested tile is already being processed, and wait until that's done
    // to mark that as being processed.
    lockTile(tileId, zoom);
    
    const auto tileLocalRelativePath =
        QString::number(zoom) + QDir::separator() +
        QString::number(tileId.x) + QDir::separator() +
        QString::number(tileId.y) + QLatin1String(".tile");
    
    QFileInfo localFile;
    {
        QMutexLocker scopedLocker(&_localCachePathMutex);
        localFile.setFile(QDir(_localCachePath).absoluteFilePath(tileLocalRelativePath));
    }
    
    bool isExpired = false;
    bool isExists = localFile.exists();
    if (isExists && source->expirationTimeMillis != -1)
    {
        long currentTime = QDateTime::currentMSecsSinceEpoch();
        long lastModifiedTime = localFile.lastModified().toUTC().toMSecsSinceEpoch();
        long expirationTime = source->expirationTimeMillis;
        if (currentTime - lastModifiedTime > expirationTime)
            isExpired = true;
    }
    
    if (isExists && !isExpired)
    {
        // Since tile is in local storage, it's safe to unmark it as being processed
        unlockTile(tileId, zoom);

        // If local file is empty, it means that requested tile does not exist (has no data)
        if (localFile.size() == 0)
        {
            outData.reset();
            return true;
        }

        auto image = OsmAnd::SkiaUtilities::createImageFromFile(localFile);
        if (!image)
        {
            QFile(localFile.absoluteFilePath()).remove();
            return false;
        }

        assert(image->width() == image->height());

        bool shouldRequestNextTile = shiftedTile && !outData;
        if (shiftedTile && outData)
        {
            if (auto data = std::dynamic_pointer_cast<OnlineRasterMapLayerProvider::Data>(outData))
            {
                image = OsmAnd::SkiaUtilities::createTileImage(data->image, image, offsetY);
            }
        }
        
        // Return tile
        outData.reset(new OnlineRasterMapLayerProvider::Data(
            request.tileId,
            request.zoom,
            owner->alphaChannelPresence,
            owner->getTileDensityFactor(),
            image));

        if (shouldRequestNextTile)
            return obtainData(request, outData, pOutMetric);

        return true;
    }

    // Since tile is not in local cache (or cache is disabled, which is the same),
    // the tile must be downloaded from network:

    // If network access is disallowed, return failure
    if (!_networkAccessAllowed)
    {
        // Before returning, unlock tile
        unlockTile(tileId, zoom);
        return false;
    }

    // Perform synchronous download
    const auto tileUrl = getUrlToLoad(tileId.x, tileId.y, zoom);
    std::shared_ptr<const IWebClient::IRequestResult> requestResult;
    const auto& downloadResult = _downloadManager->downloadData(tileUrl, &requestResult, nullptr, request.queryController);

    // Ensure that all directories are created in path to local tile
    localFile.dir().mkpath(QLatin1String("."));

    // If there was error, check what the error was
    if (requestResult && !requestResult->isSuccessful())
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
                unlockTile(tileId, zoom);
                return true;
            }
            else
            {
                LogPrintf(LogSeverityLevel::Error,
                    "Failed to mark tile as non-existent with empty file '%s'",
                    qPrintable(localFile.absoluteFilePath()));

                // Unlock the tile
                unlockTile(tileId, zoom);
                return false;
            }
        }

        // Unlock the tile
        unlockTile(tileId, zoom);
        return false;
    }

    if (downloadResult.isEmpty())
    {
        // Unlock the tile
        unlockTile(tileId, zoom);
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
    unlockTile(tileId, zoom);

    // Decode in-memory
    auto image = SkiaUtilities::createImageFromData(downloadResult);
    if (!image)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to decode tile file from '%s'",
            qPrintable(tileUrl));

        return false;
    }

    assert(image->width() == image->height());

    bool shouldRequestNextTile = shiftedTile && !outData;
    if (shiftedTile && outData)
    {
        if (auto data = std::dynamic_pointer_cast<OnlineRasterMapLayerProvider::Data>(outData))
        {
            image = OsmAnd::SkiaUtilities::createTileImage(data->image, image, offsetY);
        }
    }

    // Return tile
    outData.reset(new OnlineRasterMapLayerProvider::Data(
        request.tileId,
        request.zoom,
        owner->alphaChannelPresence,
        owner->getTileDensityFactor(),
        image));

    if (shouldRequestNextTile)
        return obtainData(request, outData, pOutMetric);

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

const QString OsmAnd::OnlineRasterMapLayerProvider_P::getUrlToLoad(int32_t x, int32_t y, const ZoomLevel zoom) const
{
    const auto& source = owner->_tileSource;
    if (source->urlToLoad.isNull() || source->urlToLoad.isEmpty()) {
        return QString();
    }
    if (source->invertedYTile)
        y = (1 << zoom) - 1 - y;
    
    return buildUrlToLoad(source->urlToLoad, source->randomsArray, x, y, zoom);
}

const QString OsmAnd::OnlineRasterMapLayerProvider_P::buildUrlToLoad(const QString& urlToLoad, const QList<QString> randomsArray, int32_t x, int32_t y, const ZoomLevel zoom)
{
    QString finalUrl = QString(urlToLoad);
    if (randomsArray.count() > 0)
    {
        QString rand;
        if (randomsArray[0] == QStringLiteral("wikimapia"))
            rand = QString::number(x % 4 + (y % 4) * 4);
        else
            rand = randomsArray[(x + y) % randomsArray.count()];
        
        finalUrl = finalUrl.replace(QLatin1String("{rnd}"), rand);
    }
    else if (urlToLoad.contains(QLatin1String("{rnd}")))
    {
        LogPrintf(LogSeverityLevel::Error,
                  "Failed to resolve randoms for url '%s'",
                  qPrintable(urlToLoad));
        return QString();
    }
    
    int bingQuadKeyParamIndex = urlToLoad.indexOf(QStringLiteral("{q}"));
    if (bingQuadKeyParamIndex != -1)
        return finalUrl.replace(QStringLiteral("{q}"), eqtBingQuadKey(zoom, x, y));
    
    int bbKeyParamIndex = urlToLoad.indexOf(QStringLiteral("{bbox}"));
    if (bbKeyParamIndex != -1)
        return finalUrl.replace(QStringLiteral("{bbox}"), calcBoundingBoxForTile(zoom, x, y));
    
    return finalUrl
        .replace(QLatin1String("{0}"), QString::number(zoom))
        .replace(QLatin1String("{1}"), QString::number(x))
        .replace(QLatin1String("{2}"), QString::number(y))
        .replace(QLatin1String("{z}"), QString::number(zoom))
        .replace(QLatin1String("{x}"), QString::number(x))
        .replace(QLatin1String("{y}"), QString::number(y));
}

const QString OsmAnd::OnlineRasterMapLayerProvider_P::eqtBingQuadKey(ZoomLevel z, int32_t x, int32_t y)
{
    char NUM_CHAR[] = {'0', '1', '2', '3'};
    char tn[z];
    for (int i = z - 1; i >= 0; i--)
    {
        int num = (x % 2) | ((y % 2) << 1);
        tn[i] = NUM_CHAR[num];
        x >>= 1;
        y >>= 1;
    }
    QString res;
    for (char &ch : tn)
        res = res.append(ch);
    return res;
}

const QString OsmAnd::OnlineRasterMapLayerProvider_P::calcBoundingBoxForTile(ZoomLevel zoom, int32_t x, int32_t y)
{
    double xmin = Utilities::getLongitudeFromTile(zoom, x);
    double xmax = Utilities::getLongitudeFromTile(zoom, x+1);
    double ymin = Utilities::getLatitudeFromTile(zoom, y+1);
    double ymax = Utilities::getLatitudeFromTile(zoom, y);
    return QString("%1,%2,%3,%4").arg(xmin, 0, 'f', 8)
                                 .arg(ymin, 0, 'f', 8)
                                 .arg(xmax, 0, 'f', 8)
                                 .arg(ymax, 0, 'f', 8);
}
