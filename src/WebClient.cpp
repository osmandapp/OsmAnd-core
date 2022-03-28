#include "WebClient.h"
#include "WebClient_P.h"

OsmAnd::WebClient::WebClient(
    const QString& userAgent /*= globalConstans::APP_VERSION*/,
    const unsigned int concurrentRequestsLimit /*= 1*/,
    const unsigned int retriesLimit /*= 1*/,
    const bool followRedirects /*= true*/)
    : _p(new WebClient_P(this, userAgent, concurrentRequestsLimit, retriesLimit, followRedirects))
{
}

OsmAnd::WebClient::~WebClient()
{
}

QString OsmAnd::WebClient::getUserAgent() const
{
    return _p->getUserAgent();
}

void OsmAnd::WebClient::setUserAgent(const QString& userAgent)
{
    _p->setUserAgent(userAgent);
}

unsigned int OsmAnd::WebClient::getConcurrentRequestsLimit() const
{
    return _p->getConcurrentRequestsLimit();
}

void OsmAnd::WebClient::setConcurrentRequestsLimit(const unsigned int newLimit)
{
    _p->setConcurrentRequestsLimit(newLimit);
}

unsigned int OsmAnd::WebClient::getRetriesLimit() const
{
    return _p->getRetriesLimit();
}

void OsmAnd::WebClient::setRetriesLimit(const unsigned int newLimit)
{
    _p->setRetriesLimit(newLimit);
}

bool OsmAnd::WebClient::getFollowRedirects() const
{
    return _p->getFollowRedirects();
}

void OsmAnd::WebClient::setFollowRedirects(const bool followRedirects)
{
    _p->setFollowRedirects(followRedirects);
}

QByteArray OsmAnd::WebClient::downloadData(
    const QNetworkRequest& networkRequest,
    std::shared_ptr<const IWebClient::IRequestResult>* const requestResult /*= nullptr*/,
    const IWebClient::RequestProgressCallbackSignature progressCallback /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/) const
{
    return _p->downloadData(networkRequest, requestResult, progressCallback);
}

QString OsmAnd::WebClient::downloadString(
    const QNetworkRequest& networkRequest,
    std::shared_ptr<const IWebClient::IRequestResult>* const requestResult /*= nullptr*/,
    const IWebClient::RequestProgressCallbackSignature progressCallback /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/) const
{
    return _p->downloadString(networkRequest, requestResult, progressCallback);
}

bool OsmAnd::WebClient::downloadFile(
    const QNetworkRequest& networkRequest,
    const QString& fileName,
    std::shared_ptr<const IWebClient::IRequestResult>* const requestResult /*= nullptr*/,
    const IWebClient::RequestProgressCallbackSignature progressCallback /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/) const
{
    return _p->downloadFile(networkRequest, fileName, requestResult, progressCallback);
}

QByteArray OsmAnd::WebClient::downloadData(
    const QString& url,
    std::shared_ptr<const IWebClient::IRequestResult>* const requestResult /*= nullptr*/,
    const IWebClient::RequestProgressCallbackSignature progressCallback /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/) const
{
    return downloadData(QNetworkRequest(url), requestResult, progressCallback);
}

QString OsmAnd::WebClient::downloadString(
    const QString& url,
    std::shared_ptr<const IWebClient::IRequestResult>* const requestResult /*= nullptr*/,
    const IWebClient::RequestProgressCallbackSignature progressCallback /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/) const
{
    return downloadString(QNetworkRequest(url), requestResult, progressCallback);
}

bool OsmAnd::WebClient::downloadFile(
    const QString& url,
    const QString& fileName,
    std::shared_ptr<const IWebClient::IRequestResult>* const requestResult /*= nullptr*/,
    const IWebClient::RequestProgressCallbackSignature progressCallback /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/) const
{
    return downloadFile(QNetworkRequest(url), fileName, requestResult, progressCallback);
}

OsmAnd::WebClient::RequestResult::RequestResult(const QNetworkReply* const networkReply)
    : errorCode(networkReply->error())
{
}

OsmAnd::WebClient::RequestResult::~RequestResult()
{
}

OsmAnd::WebClient::HttpRequestResult::HttpRequestResult(const QNetworkReply* const networkReply)
    : RequestResult(networkReply)
    , httpStatusCode(networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toUInt())
{
}

OsmAnd::WebClient::HttpRequestResult::~HttpRequestResult()
{
}

bool OsmAnd::WebClient::HttpRequestResult::isSuccessful() const
{
    return (errorCode == QNetworkReply::NoError);
}

unsigned int OsmAnd::WebClient::HttpRequestResult::getHttpStatusCode() const
{
    return httpStatusCode;
}
