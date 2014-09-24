#ifndef _OSMAND_CORE_I_SEARCH_SESSION_H_
#define _OSMAND_CORE_I_SEARCH_SESSION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QList>
#include <QLinkedList>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/Search/ISearchEngine.h>

namespace OsmAnd
{
    class OSMAND_CORE_API ISearchSession
    {
        Q_DISABLE_COPY_AND_MOVE(ISearchSession);

    private:
    protected:
        ISearchSession();
    public:
        virtual ~ISearchSession();

        virtual QList< std::shared_ptr<const ISearchEngine::IDataSource> > getDataSources() const = 0;

        virtual bool isSearching() const = 0;
        virtual bool startSearch() = 0;
        virtual bool pauseSearch() = 0;
        virtual bool cancelSearch() = 0;

        virtual std::shared_ptr<const ISearchEngine::ISearchResults> getResults() const = 0;

        OSMAND_CALLABLE(ResultsUpdatedCallback,
            void,
            ISearchSession* const session,
            const std::shared_ptr<const ISearchEngine::ISearchResults>& results);
        virtual void setResultsUpdatedCallback(const ResultsUpdatedCallback callback) = 0;
        virtual ResultsUpdatedCallback getResultsUpdatedCallback() const = 0;

        virtual void setQuery(const QString& queryText) = 0;
        virtual QString getQuery() const = 0;

        virtual void setResultsLimit(const unsigned int limit) = 0;
        virtual unsigned int getResultsLimit() const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_SEARCH_SESSION_H_)
