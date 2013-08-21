#include "OnlineMapRasterTileProvider_P.h"
#include "OnlineMapRasterTileProvider.h"

#include <assert.h>

#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>

#include <SkStream.h>
#include <SkImageDecoder.h>

#include "Logging.h"

OsmAnd::OnlineMapRasterTileProvider_P::OnlineMapRasterTileProvider_P( OnlineMapRasterTileProvider* owner_ )
    : owner(owner_)
    , _currentDownloadsCounterMutex(QMutex::Recursive)
    , _currentDownloadsCount(0)
    , _networkAccessAllowed(true)
    , _taskHostBridge(this)
{
}

OsmAnd::OnlineMapRasterTileProvider_P::~OnlineMapRasterTileProvider_P()
{
}

void OsmAnd::OnlineMapRasterTileProvider_P::obtainTileDeffered( const TileId& tileId, const ZoomLevel& zoom, IMapTileProvider::TileReadyCallback readyCallback )
{
    assert(readyCallback != nullptr);

    std::shared_ptr<TileEntry> tileEntry;
    _tiles.obtainTileEntry(tileEntry, tileId, zoom, true);
    {
        QWriteLocker scopeLock(&tileEntry->stateLock);
        if(tileEntry->state != TileState::Unknown)
            return;

        auto tileUrl = owner->urlPattern;
        tileUrl
            .replace(QString::fromLatin1("${zoom}"), QString::number(zoom))
            .replace(QString::fromLatin1("${x}"), QString::number(tileId.x))
            .replace(QString::fromLatin1("${y}"), QString::number(tileId.y));
        tileEntry->sourceUrl = QUrl(tileUrl);

        const auto& subPath = owner->id + QDir::separator() +
            QString::number(zoom) + QDir::separator() +
            QString::number(tileId.x) + QDir::separator() +
            QString::number(tileId.y) + QString::fromLatin1(".tile");
        tileEntry->localPath = QFileInfo(_localCachePath.filePath(subPath));

        tileEntry->callback = readyCallback;

        tileEntry->state = TileState::Requested;
    }

    Concurrent::pools->localStorage->start(new Concurrent::HostedTask(_taskHostBridge,
        [tileId, zoom, readyCallback](const Concurrent::Task* task, QEventLoop& eventLoop)
    {
        const auto pThis = reinterpret_cast<OnlineMapRasterTileProvider_P*>(static_cast<const Concurrent::HostedTask*>(task)->lockedOwner);

        // Check if this tile is only in requested state
        std::shared_ptr<TileEntry> tileEntry;
        pThis->_tiles.obtainTileEntry(tileEntry, tileId, zoom, true);
        {
            QWriteLocker scopeLock(&tileEntry->stateLock);
            if(tileEntry->state != TileState::Requested)
                return;

            tileEntry->state = TileState::LocalLookup;
        }

        // Check if file is already in local storage
        if(tileEntry->localPath.exists())
        {
            // 0-sized tile means there is no data at all
            if(tileEntry->localPath.size() == 0)
            {
                std::shared_ptr<IMapTileProvider::Tile> emptyTile;
                readyCallback(tileId, zoom, emptyTile, true);

                pThis->_tiles.removeEntry(tileEntry);

                return;
            }

            //NOTE: Here may be issue that SKIA can not handle opening files on different platforms correctly
            auto skBitmap = new SkBitmap();
            SkFILEStream fileStream(qPrintable(tileEntry->localPath.absoluteFilePath()));
            if(!SkImageDecoder::DecodeStream(&fileStream, skBitmap,  SkBitmap::Config::kNo_Config, SkImageDecoder::kDecodePixels_Mode))
            {
                LogPrintf(LogSeverityLevel::Error, "Failed to decode tile file '%s'", qPrintable(tileEntry->localPath.absoluteFilePath()));

                delete skBitmap;
                std::shared_ptr<IMapTileProvider::Tile> emptyTile;
                readyCallback(tileId, zoom, emptyTile, false);

                pThis->_tiles.removeEntry(tileEntry);

                return;
            }

            assert(skBitmap->width() == skBitmap->height());
            assert(skBitmap->width() == pThis->owner->tileDimension);

            // Construct tile response
            std::shared_ptr<OnlineMapRasterTileProvider::Tile> tile(new OnlineMapRasterTileProvider::Tile(skBitmap, pThis->owner->alphaChannelData));
            readyCallback(tileId, zoom, tile, true);

            pThis->_tiles.removeEntry(tileEntry);

            return;
        }

        // Well, tile is not in local cache, we need to download it
        pThis->obtainTileDeffered(tileEntry);
    }));
}

void OsmAnd::OnlineMapRasterTileProvider_P::obtainTileDeffered( const std::shared_ptr<TileEntry>& tileEntry )
{
    if(!_networkAccessAllowed)
        return;

    {
        QMutexLocker scopedLock(&_currentDownloadsCounterMutex);
        QWriteLocker scopeLock(&tileEntry->stateLock);
        if(tileEntry->state != TileState::LocalLookup && tileEntry->state != TileState::EnqueuedForDownload)
            return;

        // If all download slots are taken, enqueue
        if(_currentDownloadsCount == owner->maxConcurrentDownloads)
        {
            tileEntry->state = TileState::EnqueuedForDownload;
            return;
        }

        // Else, simply increment counter
        _currentDownloadsCount++;
        tileEntry->state = TileState::Downloading;
    }

    Concurrent::pools->network->start(new Concurrent::HostedTask(_taskHostBridge,
        [tileEntry](const Concurrent::Task* task, QEventLoop& eventLoop)
    {
        const auto pThis = reinterpret_cast<OnlineMapRasterTileProvider_P*>(static_cast<const Concurrent::HostedTask*>(task)->lockedOwner);

        QNetworkAccessManager networkAccessManager;
        QNetworkRequest request;
        request.setUrl(tileEntry->sourceUrl);
        request.setRawHeader("User-Agent", "OsmAnd Core");

        auto reply = networkAccessManager.get(request);
        QObject::connect(reply, &QNetworkReply::finished,
            [pThis, reply, tileEntry, &eventLoop, &networkAccessManager]()
        {
            pThis->replyFinishedHandler(reply, tileEntry, eventLoop, networkAccessManager);
        });
        eventLoop.exec();
        return;
    }));
}

void OsmAnd::OnlineMapRasterTileProvider_P::replyFinishedHandler( QNetworkReply* reply, const std::shared_ptr<TileEntry>& tileEntry, QEventLoop& eventLoop, QNetworkAccessManager& networkAccessManager )
{
    auto redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if(!redirectUrl.isEmpty())
    {
        reply->deleteLater();

        QNetworkRequest newRequest;
        newRequest.setUrl(redirectUrl);
        newRequest.setRawHeader("User-Agent", "OsmAnd Core");
        auto newReply = networkAccessManager.get(newRequest);
        QObject::connect(newReply, &QNetworkReply::finished,
            [this, newReply, tileEntry, &eventLoop, &networkAccessManager]()
        {
            replyFinishedHandler(newReply, tileEntry, eventLoop, networkAccessManager);
        });
        return;
    }

    // Remove current download and process pending queue
    {
        QMutexLocker scopedLock(&_currentDownloadsCounterMutex);
        _currentDownloadsCount--;
    }

    // Enqueue other downloads
    _tiles.obtainTileEntries(nullptr, [this](const std::shared_ptr<TileEntry>& entry, bool& cancel) -> bool
    {
        if(_currentDownloadsCount == owner->maxConcurrentDownloads)
        {
            cancel = true;
            return false;
        }

        if(entry->state == TileState::EnqueuedForDownload)
            obtainTileDeffered(entry);

        return false;
    });

    handleNetworkReply(reply, tileEntry);

    reply->deleteLater();
    eventLoop.exit();
}

void OsmAnd::OnlineMapRasterTileProvider_P::handleNetworkReply( QNetworkReply* reply, const std::shared_ptr<TileEntry>& tileEntry )
{
    tileEntry->localPath.dir().mkpath(tileEntry->localPath.dir().absolutePath());

    auto error = reply->error();
    if(error != QNetworkReply::NetworkError::NoError)
    {
        const auto httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        LogPrintf(LogSeverityLevel::Warning, "Failed to download tile from %s (HTTP status %d)", qPrintable(reply->request().url().toString()), httpStatus);

        // 404 means that this tile does not exist, so create a zero file
        if(httpStatus == 404)
        {
            // Save to a file
            QFile tileFile(tileEntry->localPath.absoluteFilePath());
            if(tileFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
                tileFile.close();
            else
                LogPrintf(LogSeverityLevel::Error, "Failed to mark tile as non-existent with empty file '%s'", qPrintable(tileEntry->localPath.absoluteFilePath()));
        }

        _tiles.removeEntry(tileEntry);

        return;
    }

#if defined(_DEBUG) || defined(DEBUG)
    LogPrintf(LogSeverityLevel::Info, "Downloaded tile from %s", reply->request().url().toString().toStdString().c_str());
#endif
    const auto& data = reply->readAll();

    // Save to a file
    QFile tileFile(tileEntry->localPath.absoluteFilePath());
    if(tileFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        tileFile.write(data);
        tileFile.close();

#if defined(_DEBUG) || defined(DEBUG)
        LogPrintf(LogSeverityLevel::Info, "Saved tile from %s to %s", qPrintable(reply->request().url().toString()), qPrintable(tileEntry->localPath.absoluteFilePath()));
#endif
    }
    else
        LogPrintf(LogSeverityLevel::Error, "Failed to save tile to '%s'", qPrintable(tileEntry->localPath.absoluteFilePath()));

    // Decode in-memory if we have receiver
    if(tileEntry->callback)
    {
        auto skBitmap = new SkBitmap();
        if(!SkImageDecoder::DecodeMemory(data.data(), data.size(), skBitmap, SkBitmap::Config::kNo_Config, SkImageDecoder::kDecodePixels_Mode))
        {
            LogPrintf(LogSeverityLevel::Error, "Failed to decode tile file from '%s'", reply->request().url().toString().toStdString().c_str());

            delete skBitmap;
            std::shared_ptr<IMapTileProvider::Tile> emptyTile;
            tileEntry->callback(tileEntry->tileId, tileEntry->zoom, emptyTile, false);

            _tiles.removeEntry(tileEntry);

            return;
        }

        assert(skBitmap->width() == skBitmap->height());
        assert(skBitmap->width() == owner->tileDimension);

        std::shared_ptr<OnlineMapRasterTileProvider::Tile> tile(new OnlineMapRasterTileProvider::Tile(skBitmap, owner->alphaChannelData));
        tileEntry->callback(tileEntry->tileId, tileEntry->zoom, tile, true);

        _tiles.removeEntry(tileEntry);
    }
}

OsmAnd::OnlineMapRasterTileProvider_P::TileEntry::TileEntry( const TileId& tileId, const ZoomLevel& zoom )
    : TilesCollectionEntryWithState(tileId, zoom)
{
}

OsmAnd::OnlineMapRasterTileProvider_P::TileEntry::~TileEntry()
{
}