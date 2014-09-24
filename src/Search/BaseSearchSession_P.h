#ifndef _OSMAND_CORE_BASE_SEARCH_SESSION_P_H_
#define _OSMAND_CORE_BASE_SEARCH_SESSION_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QList>
#include <QLinkedList>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "Callable.h"
#include "PrivateImplementation.h"
#include "ISearchSession.h"

namespace OsmAnd
{
    class BaseSearchSession;
    class BaseSearchSession_P
    {
        Q_DISABLE_COPY_AND_MOVE(BaseSearchSession_P);

    public:
        typedef ISearchSession::ResultsUpdatedCallback ResultsUpdatedCallback;

    private:
    protected:
        BaseSearchSession_P(BaseSearchSession* const owner);
    public:
        virtual ~BaseSearchSession_P();

        ImplementationInterface<BaseSearchSession> owner;

        bool isSearching() const;
        bool startSearch();
        bool pauseSearch();
        bool cancelSearch();

        std::shared_ptr<const ISearchEngine::ISearchResults> getResults() const;

        void setResultsUpdatedCallback(const ResultsUpdatedCallback callback);
        ResultsUpdatedCallback getResultsUpdatedCallback() const;

        void setQuery(const QString& queryText);
        QString getQuery() const;

        void setResultsLimit(const unsigned int limit);
        unsigned int getResultsLimit() const;

    friend class OsmAnd::BaseSearchSession;
    };
}

#endif // !defined(_OSMAND_CORE_BASE_SEARCH_SESSION_P_H_)
