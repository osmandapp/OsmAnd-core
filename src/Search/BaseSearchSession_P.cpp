#include "BaseSearchSession_P.h"

OsmAnd::BaseSearchSession_P::BaseSearchSession_P(BaseSearchSession* const owner_)
    : owner(owner_)
{
}

OsmAnd::BaseSearchSession_P::~BaseSearchSession_P()
{
}

bool OsmAnd::BaseSearchSession_P::isSearching() const
{
    return false;
}

bool OsmAnd::BaseSearchSession_P::startSearch()
{
    return false;
}

bool OsmAnd::BaseSearchSession_P::pauseSearch()
{
    return false;
}

bool OsmAnd::BaseSearchSession_P::cancelSearch()
{
    return false;
}

std::shared_ptr<const OsmAnd::ISearchEngine::ISearchResults> OsmAnd::BaseSearchSession_P::getResults() const
{
    return nullptr;
}

void OsmAnd::BaseSearchSession_P::setResultsUpdatedCallback(const ResultsUpdatedCallback callback)
{
    int i = 5;
}

OsmAnd::BaseSearchSession_P::ResultsUpdatedCallback OsmAnd::BaseSearchSession_P::getResultsUpdatedCallback() const
{
    return nullptr;
}

void OsmAnd::BaseSearchSession_P::setQuery(const QString& queryText)
{
    int i = 5;
}

QString OsmAnd::BaseSearchSession_P::getQuery() const
{
    return QString();
}

void OsmAnd::BaseSearchSession_P::setResultsLimit(const unsigned int limit)
{
    int i = 5;
}

unsigned int OsmAnd::BaseSearchSession_P::getResultsLimit() const
{
    return 0;
}
