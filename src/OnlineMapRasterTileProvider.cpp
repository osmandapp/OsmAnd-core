#include "OnlineMapRasterTileProvider.h"

#include "OsmAndCore_private.h"
#include "QMainThreadTaskEvent.h"
#include "OsmAndLogging.h"

#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>

#include <SkImageDecoder.h>

OsmAnd::OnlineMapRasterTileProvider::OnlineMapRasterTileProvider(const QString& id_, const QString& urlPattern_, uint32_t maxZoom_ /*= 31*/, uint32_t minZoom_ /*= 0*/, uint32_t maxConcurrentDownloads_ /*= 1*/)
    : _id(id_)
    , _urlPattern(urlPattern_)
    , _minZoom(minZoom_)
    , _maxZoom(maxZoom_)
    , _maxConcurrentDownloads(maxConcurrentDownloads_)
    , _concurrentDownloadsCounter(0)
    , _networkAccessAllowed(true)
    , localCachePath(_localCachePath)
    , networkAccessAllowed(_networkAccessAllowed)
{
}

OsmAnd::OnlineMapRasterTileProvider::~OnlineMapRasterTileProvider()
{

}

void OsmAnd::OnlineMapRasterTileProvider::setLocalCachePath( const std::shared_ptr<QDir>& localCachePath )
{

}

void OsmAnd::OnlineMapRasterTileProvider::setNetworkAccessPermission( bool allowed )
{

}

bool OsmAnd::OnlineMapRasterTileProvider::obtainTile(
    const uint64_t& tileId, uint32_t zoom,
    std::shared_ptr<SkBitmap>& tile )
{
    if(!_localCachePath)
        return false;

    if(!_localCachePath->exists())
        return false;

    int32_t xZ = static_cast<int32_t>(tileId >> 32);
    int32_t yZ = static_cast<int32_t>(tileId & 0xFFFFFFFF);
    const auto& subPath = _id + QDir::separator() +
        QString::number(zoom) + QDir::separator() +
        QString::number(xZ) + QDir::separator() +
        QString::number(yZ) + QString::fromLatin1(".tile");
    QFile tileFile(_localCachePath->filePath(subPath));
    if(!tileFile.exists())
        return false;

    //TODO: read image from file
    return true;
}

void OsmAnd::OnlineMapRasterTileProvider::obtainTile(
    const uint64_t& tileId, uint32_t zoom,
    std::function<void (const uint64_t& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tile)> receiverCallback )
{
    if(!_networkAccessAllowed)
        return;

    {
        QMutexLocker scopeLock(&_downloadsMutex);
        if(_concurrentDownloadsCounter == _maxConcurrentDownloads)
        {
            //TODO: add to queue
            return;
        }
        _concurrentDownloadsCounter++;
    }

    QCoreApplication::postEvent(gMainThreadTaskHost.get(), new QMainThreadTaskEvent(
        [this, tileId, zoom, receiverCallback] ()
        {
            int32_t xZ = static_cast<int32_t>(tileId >> 32);
            int32_t yZ = static_cast<int32_t>(tileId & 0xFFFFFFFF);
            const auto& tileUrl = _urlPattern
                .replace(QString::fromLatin1("${zoom}"), QString::number(zoom))
                .replace(QString::fromLatin1("${x}"), QString::number(xZ))
                .replace(QString::fromLatin1("${y}"), QString::number(yZ));

            LogPrintf(LogSeverityLevel::Info, "Downloading tile %dx%d@%d from %s\n", xZ, yZ, zoom, tileUrl.toStdString().c_str());

            QNetworkRequest request;
            request.setUrl(QUrl(tileUrl));
            request.setRawHeader("User-Agent", "OsmAnd Core");

            auto reply = gNetworkAccessManager->get(request);
            QObject::connect(reply, &QNetworkReply::finished,
                [this, tileUrl, tileId, zoom, reply, receiverCallback] ()
                {
                    {
                        QMutexLocker scopeLock(&_downloadsMutex);
                        _concurrentDownloadsCounter--;
                    }

                    auto error = reply->error();
                    if(error != QNetworkReply::NetworkError::NoError)
                    {
                        LogPrintf(LogSeverityLevel::Warning, "Failed to download tile from %s\n", tileUrl.toStdString().c_str());

                        //TODO: mark as error
                        reply->deleteLater();
                        return;
                    }

                    LogPrintf(LogSeverityLevel::Warning, "Downloaded tile from %s\n", tileUrl.toStdString().c_str());
                    const auto& data = reply->readAll();

                    if(_localCachePath && _localCachePath->exists())
                    {
                        // Save to a file
                        int32_t xZ = static_cast<int32_t>(tileId >> 32);
                        int32_t yZ = static_cast<int32_t>(tileId & 0xFFFFFFFF);
                        const auto& subPath = _id + QDir::separator() +
                            QString::number(zoom) + QDir::separator() +
                            QString::number(xZ) + QDir::separator() +
                            QString::number(yZ) + QString::fromLatin1(".tile");
                        QFile tileFile(_localCachePath->filePath(subPath));
                        if(tileFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
                        {
                            tileFile.write(data);
                            tileFile.close();
                        }
                    }
                    
                    // Decode in-memory
                    if(receiverCallback)
                    {
                        std::shared_ptr<SkBitmap> tileBitmap(new SkBitmap());
                        if(SkImageDecoder::DecodeMemory(data.data(), data.size(), tileBitmap.get(), SkBitmap::kARGB_8888_Config, SkImageDecoder::kDecodePixels_Mode))
                        {
                            receiverCallback(tileId, zoom, tileBitmap);
                        }
                    }
                    
                    reply->deleteLater();
                }
            );
            /*
            //NOTE: Alias workaround, some compiler just go crazy about this
            void (QNetworkReply::*QNetworkReply_error_alias)(QNetworkReply::NetworkError);
            QNetworkReply_error_alias = &QNetworkReply::error;
            QObject::connect(reply, QNetworkReply_error_alias,
                [reply] (QNetworkReply::NetworkError code)
                {
                    reply->deleteLater();
                }
            );
            */
        }
    ));
}

const std::shared_ptr<OsmAnd::OnlineMapRasterTileProvider> OsmAnd::OnlineMapRasterTileProvider::Mapnik(new OsmAnd::OnlineMapRasterTileProvider(
    "mapnik", "http://mapnik.osmand.net/${zoom}/${x}/${y}.png", 0, 18, 2
));

const std::shared_ptr<OsmAnd::OnlineMapRasterTileProvider> OsmAnd::OnlineMapRasterTileProvider::CycleMap(new OsmAnd::OnlineMapRasterTileProvider(
    "cyclemap", "http://b.tile.opencyclemap.org/cycle/${zoom}/${x}/${y}.png", 0, 18, 2
));
