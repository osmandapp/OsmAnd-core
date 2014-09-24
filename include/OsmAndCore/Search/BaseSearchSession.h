#ifndef _OSMAND_CORE_BASE_SEARCH_SESSION_H_
#define _OSMAND_CORE_BASE_SEARCH_SESSION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QList>
#include <QLinkedList>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Search/ISearchSession.h>

namespace OsmAnd
{
    class BaseSearchSession_P;
    class OSMAND_CORE_API BaseSearchSession : public ISearchSession
    {
        Q_DISABLE_COPY_AND_MOVE(BaseSearchSession);

    private:
    protected:
        BaseSearchSession(
            BaseSearchSession_P* const p,
            const QList< std::shared_ptr<const ISearchEngine::IDataSource> >& dataSources);

        PrivateImplementation<BaseSearchSession_P> _p;
    public:
        virtual ~BaseSearchSession();

        const QList< std::shared_ptr<const ISearchEngine::IDataSource> > dataSources;
        virtual QList< std::shared_ptr<const ISearchEngine::IDataSource> > getDataSources() const;

        virtual bool isSearching() const;
        virtual bool startSearch();
        virtual bool pauseSearch();
        virtual bool cancelSearch();

        virtual std::shared_ptr<const ISearchEngine::ISearchResults> getResults() const;

        virtual void setResultsUpdatedCallback(const ResultsUpdatedCallback callback);
        virtual ResultsUpdatedCallback getResultsUpdatedCallback() const;

        virtual void setQuery(const QString& queryText);
        virtual QString getQuery() const;

        virtual void setResultsLimit(const unsigned int limit);
        virtual unsigned int getResultsLimit() const;
    };
}

#endif // !defined(_OSMAND_CORE_BASE_SEARCH_SESSION_H_)
