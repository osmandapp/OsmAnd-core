#include "IWebClient.h"

OsmAnd::IWebClient::IWebClient()
{
}

OsmAnd::IWebClient::~IWebClient()
{
}

OsmAnd::IWebClient::IRequestResult::IRequestResult()
{
}

OsmAnd::IWebClient::IRequestResult::~IRequestResult()
{
}

OsmAnd::IWebClient::IHttpRequestResult::IHttpRequestResult()
{
}

OsmAnd::IWebClient::IHttpRequestResult::~IHttpRequestResult()
{
}

OsmAnd::IWebClient::DataRequest::DataRequest()
    : progressCallback(nullptr)
{
}

OsmAnd::IWebClient::DataRequest::DataRequest(std::nullptr_t)
    : progressCallback(nullptr)
{
}

OsmAnd::IWebClient::DataRequest::~DataRequest()
{
}
