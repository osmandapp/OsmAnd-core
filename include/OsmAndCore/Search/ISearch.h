#ifndef _OSMAND_CORE_I_SEARCH_H_
#define _OSMAND_CORE_I_SEARCH_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QList>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/IObfsCollection.h>

class QThreadPool;

namespace OsmAnd
{
    class IQueryController;

    class OSMAND_CORE_API ISearch
    {
        Q_DISABLE_COPY_AND_MOVE(ISearch)
    public:
        struct OSMAND_CORE_API Criteria
        {
        protected:
            Criteria();
        public:
            virtual ~Criteria();
        };

        struct OSMAND_CORE_API IResultEntry
        {
        protected:
            IResultEntry();
        public:
            virtual ~IResultEntry();
        };

        //typedef std::function<void(const Criteria& criteria, const IResultEntry& resultEntry)> NewResultEntryCallback;
        //typedef std::function<void(const Criteria& criteria, const QList<IResultEntry>& resultEntry)> SearchCompletedCallback;
        
        OSMAND_CALLABLE(NewResultEntryCallback,
                        void,
                        const Criteria& criteria,
                        const IResultEntry& resultEntry);

        OSMAND_CALLABLE(SearchCompletedCallback,
                        void,
                        const Criteria& criteria,
                        const QList<IResultEntry>& resultEntry);

    private:
    protected:
        ISearch();
    public:
        virtual ~ISearch();

        virtual void performSearch(
            const Criteria& criteria,
            const NewResultEntryCallback newResultEntryCallback,
            const std::shared_ptr<const IQueryController>& queryController = nullptr) const = 0;

        virtual void startSearch(
            const Criteria& criteria,
            const NewResultEntryCallback newResultEntryCallback,
            const SearchCompletedCallback searchCompletedCallback,
            QThreadPool* const threadPool,
            const std::shared_ptr<const IQueryController>& queryController = nullptr) const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_SEARCH_H_)
