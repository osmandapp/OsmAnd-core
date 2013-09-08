#include "OnlineMapRasterTileProvider_P.h"
#include "OnlineMapRasterTileProvider.h"

#include <assert.h>

#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>

#include <SkStream.h>
#include <SkImageDecoder.h>

#include "Network.h"
#include "Logging.h"

OsmAnd::OnlineMapRasterTileProvider_P::OnlineMapRasterTileProvider_P( OnlineMapRasterTileProvider* owner_ )
    : owner(owner_)
    , _currentDownloadsCounter(0)
{
}

OsmAnd::OnlineMapRasterTileProvider_P::~OnlineMapRasterTileProvider_P()
{
}

bool OsmAnd::OnlineMapRasterTileProvider_P::obtainTile( const TileId& tileId, const ZoomLevel& zoom, std::shared_ptr<MapTile>& outTile )
{
    // Check if requested tile is already being processed, and wait until that's done
    // to mark that as being processed.
    lockTile(tileId, zoom);

    //TODO: access to _localCachePath should be atomic, and a check is needed if local cache is available at all
    // Check if requested tile is already in local storage.
    const auto tileLocalRelativePath =
        QString::number(zoom) + QDir::separator() +
        QString::number(tileId.x) + QDir::separator() +
        QString::number(tileId.y) + QString::fromLatin1(".tile");
    QFileInfo localFile(_localCachePath.filePath(tileLocalRelativePath));
    if(localFile.exists())
    {
        // Since tile is in local storage, it's safe to unmark it as being processed
        unlockTile(tileId, zoom);

        // If local file is empty, it means that requested tile does not exist (has no data)
        if(localFile.size() == 0)
        {
            outTile.reset();
            return true;
        }

        //NOTE: Here may be issue that SKIA can not handle opening files on different platforms correctly
        auto bitmap = new SkBitmap();
        SkFILEStream fileStream(qPrintable(localFile.absoluteFilePath()));
        if(!SkImageDecoder::DecodeStream(&fileStream, bitmap, SkBitmap::Config::kNo_Config, SkImageDecoder::kDecodePixels_Mode))
        {
            LogPrintf(LogSeverityLevel::Error, "Failed to decode tile file '%s'", qPrintable(localFile.absoluteFilePath()));

            delete bitmap;

            return false;
        }

        assert(bitmap->width() == bitmap->height());
        assert(bitmap->width() == owner->tileSize);

        // Return tile
        auto tile = new MapBitmapTile(bitmap, owner->alphaChannelData);
        outTile.reset(tile);
        return true;
    }

    // Since tile is not in local cache (or cache is disabled, which is the same),
    // the tile must be downloaded from network:

    // If network access is disallowed, return failure
    if(!_networkAccessAllowed)
    {
        // Before returning, unlock tile
        unlockTile(tileId, zoom);

        return false;
    }

    // Check if there is free download slot. If all download slots are used, wait for one to be freed
    if(owner->maxConcurrentDownloads > 0)
    {
        QMutexLocker scopedLocker(&_currentDownloadsCounterMutex);

        while(_currentDownloadsCounter >= owner->maxConcurrentDownloads)
            _currentDownloadsCounterChanged.wait(&_currentDownloadsCounterMutex);

        _currentDownloadsCounter++;
    }

    // Perform synchronous download
    auto tileUrl = owner->urlPattern;
    tileUrl
        .replace(QString::fromLatin1("${zoom}"), QString::number(zoom))
        .replace(QString::fromLatin1("${x}"), QString::number(tileId.x))
        .replace(QString::fromLatin1("${y}"), QString::number(tileId.y));
    const auto networkReply = Network::Downloader::download(tileUrl).get();

    // Free download slot
    {
        QMutexLocker scopedLocker(&_currentDownloadsCounterMutex);

        _currentDownloadsCounter--;
        _currentDownloadsCounterChanged.wakeAll();
    }

    // Ensure that all directories are created in path to local tile
    localFile.dir().mkpath(localFile.dir().absolutePath());

    // If there was error, check what the error was
    auto networkError = networkReply->error();
    if(networkError != QNetworkReply::NetworkError::NoError)
    {
        const auto httpStatus = networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        LogPrintf(LogSeverityLevel::Warning, "Failed to download tile from %s (HTTP status %d)", qPrintable(tileUrl), httpStatus);

        // 404 means that this tile does not exist, so create a zero file
        if(httpStatus == 404)
        {
            // Save to a file
            QFile tileFile(localFile.absoluteFilePath());
            if(tileFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
            {
                tileFile.close();

                // Unlock the tile
                unlockTile(tileId, zoom);
                networkReply->deleteLater();
                return true;
            }
            else
            {
                LogPrintf(LogSeverityLevel::Error, "Failed to mark tile as non-existent with empty file '%s'", qPrintable(localFile.absoluteFilePath()));

                // Unlock the tile
                unlockTile(tileId, zoom);
                return false;
            }
        }

        // Unlock the tile
        unlockTile(tileId, zoom);
        return false;
    }

    // Save data to a file
#if defined(_DEBUG) || defined(DEBUG)
    LogPrintf(LogSeverityLevel::Info, "Downloaded tile from %s", qPrintable(tileUrl));
#endif
    const auto& data = networkReply->readAll();

    // Save to a file
    QFile tileFile(localFile.absoluteFilePath());
    if(tileFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        tileFile.write(data);
        tileFile.close();

#if defined(_DEBUG) || defined(DEBUG)
        LogPrintf(LogSeverityLevel::Info, "Saved tile from %s to %s", qPrintable(tileUrl), qPrintable(localFile.absoluteFilePath()));
#endif
    }
    else
        LogPrintf(LogSeverityLevel::Error, "Failed to save tile to '%s'", qPrintable(localFile.absoluteFilePath()));

    // Unlock tile, since local storage work is done
    unlockTile(tileId, zoom);

    // Decode in-memory
    auto bitmap = new SkBitmap();
    if(!SkImageDecoder::DecodeMemory(data.data(), data.size(), bitmap, SkBitmap::Config::kNo_Config, SkImageDecoder::kDecodePixels_Mode))
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to decode tile file from '%s'", qPrintable(tileUrl));

        delete bitmap;

        return false;
    }

    assert(bitmap->width() == bitmap->height());
    assert(bitmap->width() == owner->tileSize);

    // Return tile
    auto tile = new MapBitmapTile(bitmap, owner->alphaChannelData);
    outTile.reset(tile);
    return true;
}

void OsmAnd::OnlineMapRasterTileProvider_P::lockTile( const TileId& tileId, const ZoomLevel& zoom )
{
    QMutexLocker scopedLocker(&_tilesInProcessMutex);

    while(_tilesInProcess[zoom].contains(tileId))
        _waitUntilAnyTileIsProcessed.wait(&_tilesInProcessMutex);

    _tilesInProcess[zoom].insert(tileId);
}

void OsmAnd::OnlineMapRasterTileProvider_P::unlockTile( const TileId& tileId, const ZoomLevel& zoom )
{
    QMutexLocker scopedLocker(&_tilesInProcessMutex);

    _tilesInProcess[zoom].remove(tileId);

    _waitUntilAnyTileIsProcessed.wakeAll();
}
