#ifndef _OSMAND_CORE_WEB_CLIENT_H_
#define _OSMAND_CORE_WEB_CLIENT_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QByteArray>
#include <QUrl>
#include <QNetworkReply>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/IWebClient.h>
#include "globalConstants.h"

namespace OsmAnd
{
    class WebClient_P;
    class OSMAND_CORE_API WebClient : public IWebClient
    {
        Q_DISABLE_COPY_AND_MOVE(WebClient);

    public:
        class OSMAND_CORE_API RequestResult
        {
            Q_DISABLE_COPY_AND_MOVE(RequestResult);

        private:
        protected:
            RequestResult(const QNetworkReply* const networkReply);
        public:
            virtual ~RequestResult();

            const QNetworkReply::NetworkError errorCode;
        };
        
        class OSMAND_CORE_API HttpRequestResult
            : public RequestResult
            , public IWebClient::IHttpRequestResult
        {
            Q_DISABLE_COPY_AND_MOVE(HttpRequestResult);

        private:
        protected:
            HttpRequestResult(const QNetworkReply* const networkReply);
        public:
            virtual ~HttpRequestResult();

            const unsigned int httpStatusCode;

            virtual bool isSuccessful() const;
            virtual unsigned int getHttpStatusCode() const;

        friend class OsmAnd::WebClient_P;
        };

    private:
        PrivateImplementation<WebClient_P> _p;
    protected:
    public:
        WebClient(
            const QString& userAgent = globalConstants::APP_VERSION,
            const unsigned int concurrentRequestsLimit = 1,
            const unsigned int retriesLimit = 1,
            const bool followRedirects = true);
        virtual ~WebClient();

        QString getUserAgent() const;
        void setUserAgent(const QString& userAgent);

        unsigned int getConcurrentRequestsLimit() const;
        void setConcurrentRequestsLimit(const unsigned int newLimit);

        unsigned int getRetriesLimit() const;
        void setRetriesLimit(const unsigned int newLimit);

        bool getFollowRedirects() const;
        void setFollowRedirects(const bool followRedirects);

        QByteArray downloadData(
            const QNetworkRequest& networkRequest,
            std::shared_ptr<const IWebClient::IRequestResult>* const requestResult = nullptr,
            const IWebClient::RequestProgressCallbackSignature progressCallback = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr) const;
        QString downloadString(
            const QNetworkRequest& networkRequest,
            std::shared_ptr<const IWebClient::IRequestResult>* const requestResult = nullptr,
            const IWebClient::RequestProgressCallbackSignature progressCallback = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr) const;
        bool downloadFile(
            const QNetworkRequest& networkRequest,
            const QString& fileName,
            std::shared_ptr<const IWebClient::IRequestResult>* const requestResult = nullptr,
            const IWebClient::RequestProgressCallbackSignature progressCallback = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr) const;

        virtual QByteArray downloadData(
            const QString& url,
            std::shared_ptr<const IWebClient::IRequestResult>* const requestResult = nullptr,
            const IWebClient::RequestProgressCallbackSignature progressCallback = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr) const;
        virtual QString downloadString(
            const QString& url,
            std::shared_ptr<const IWebClient::IRequestResult>* const requestResult = nullptr,
            const IWebClient::RequestProgressCallbackSignature progressCallback = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr) const;
        virtual bool downloadFile(
            const QString& url,
            const QString& fileName,
            std::shared_ptr<const IWebClient::IRequestResult>* const requestResult = nullptr,
            const IWebClient::RequestProgressCallbackSignature progressCallback = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr) const;
    };
}

#endif // !defined(_OSMAND_CORE_WEB_CLIENT_H_)

