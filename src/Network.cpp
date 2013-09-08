#include "Network.h"

#include <cassert>

#include "Concurrent.h"

OsmAnd::Network::DownloadSettings::DownloadSettings()
    : autoFollowRedirects(true)
    , userAgent(QLatin1String("OsmAnd Core"))
{
}

OsmAnd::Network::DownloadSettings::~DownloadSettings()
{
}

OsmAnd::Network::Downloader::Downloader()
{
}

OsmAnd::Network::Downloader::~Downloader()
{
}

std::future< std::shared_ptr<QNetworkReply> > OsmAnd::Network::Downloader::download( const QUrl& url_, const DownloadSettings& settings_ /*= DownloadSettings()*/ )
{
    auto promise = new std::promise< std::shared_ptr<QNetworkReply> >();

    const auto url = url_;
    const auto settings = settings_;

    Concurrent::pools->network->start(new Concurrent::Task([url, settings, promise](const Concurrent::Task* task, QEventLoop& eventLoop)
        {
            QNetworkAccessManager networkAccessManager;

            // Create request
            QNetworkRequest request;
            request.setUrl(url);
            request.setRawHeader("User-Agent", settings.userAgent.toLocal8Bit());

            // Process until final reply is ready
            auto reply = networkAccessManager.get(request);
            for(;;)
            {
                // Wait for reply to be ready
                QObject::connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
                eventLoop.exec();

                // If settings specify that redirects must be followed, do that
                if(settings.autoFollowRedirects)
                {
                    auto redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
                    if(!redirectUrl.isEmpty())
                    {
                        // Delete current reply
                        reply->deleteLater();

                        // Create new request, and wait for reply to arrive
                        QNetworkRequest newRequest;
                        newRequest.setUrl(redirectUrl);
                        request.setRawHeader("User-Agent", settings.userAgent.toLocal8Bit());
                        reply = networkAccessManager.get(newRequest);
                        continue;
                    }
                }

                // Since reply is ready, exit loop
                break;
            }

            // Propagate final reply
            reply->setParent(nullptr);
            promise->set_value(std::shared_ptr<QNetworkReply>(reply));
            delete promise;

            return;
        }));

    return promise->get_future();
}
