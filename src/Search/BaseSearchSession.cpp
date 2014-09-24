#include "BaseSearchSession.h"
#include "BaseSearchSession_P.h"

OsmAnd::BaseSearchSession::BaseSearchSession(
    BaseSearchSession_P* const p_,
    const QList< std::shared_ptr<const ISearchEngine::IDataSource> >& dataSources_)
    : _p(p_)
    , dataSources(dataSources_)
{
}

OsmAnd::BaseSearchSession::~BaseSearchSession()
{
}

QList< std::shared_ptr<const OsmAnd::ISearchEngine::IDataSource> > OsmAnd::BaseSearchSession::getDataSources() const
{
    return dataSources;
}

bool OsmAnd::BaseSearchSession::isSearching() const
{
    return _p->isSearching();
}

bool OsmAnd::BaseSearchSession::startSearch()
{
    return _p->startSearch();
}

bool OsmAnd::BaseSearchSession::pauseSearch()
{
    return _p->pauseSearch();
}

bool OsmAnd::BaseSearchSession::cancelSearch()
{
    return _p->cancelSearch();
}

std::shared_ptr<const OsmAnd::ISearchEngine::ISearchResults> OsmAnd::BaseSearchSession::getResults() const
{
    return _p->getResults();
}

void OsmAnd::BaseSearchSession::setResultsUpdatedCallback(const ResultsUpdatedCallback callback)
{
    _p->setResultsUpdatedCallback(callback);
}

OsmAnd::BaseSearchSession::ResultsUpdatedCallback OsmAnd::BaseSearchSession::getResultsUpdatedCallback() const
{
    return _p->getResultsUpdatedCallback();
}

void OsmAnd::BaseSearchSession::setQuery(const QString& queryText)
{
    _p->setQuery(queryText);
}

QString OsmAnd::BaseSearchSession::getQuery() const
{
    return _p->getQuery();
}

void OsmAnd::BaseSearchSession::setResultsLimit(const unsigned int limit)
{
    _p->setResultsLimit(limit);
}

unsigned int OsmAnd::BaseSearchSession::getResultsLimit() const
{
    return _p->getResultsLimit();
}
