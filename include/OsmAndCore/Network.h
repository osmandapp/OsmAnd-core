#ifndef _OSMAND_CORE_NETWORK_H_
#define _OSMAND_CORE_NETWORK_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QUrl>
#include <QNetworkReply>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    namespace Network
    {
        struct OSMAND_CORE_API DownloadSettings
        {
            DownloadSettings();
            ~DownloadSettings();

            bool autoFollowRedirects;
            QString userAgent;
        };

        class OSMAND_CORE_API Downloader
        {
        private:
            Downloader();
            ~Downloader();
        protected:
        public:
            static std::shared_ptr<QNetworkReply> download(const QUrl& url, const DownloadSettings& settings = DownloadSettings());
        };
    } // namespace Network
} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_NETWORK_H_)
