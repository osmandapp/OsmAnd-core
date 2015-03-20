#ifndef _OSMAND_CORE_I_SEARCH_H_
#define _OSMAND_CORE_I_SEARCH_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Nullable.h>
#include <OsmAndCore/IObfsCollection.h>

namespace OsmAnd
{
    class IQueryController;

    class OSMAND_CORE_API ISearch
    {
        Q_DISABLE_COPY_AND_MOVE(ISearch);
    public:
        typedef const void* SearchKey;

        struct OSMAND_CORE_API Criteria
        {
        protected:
            Criteria();
        public:
            virtual ~Criteria();

            IObfsCollection::AcceptorFunction sourceFilter;
            ZoomLevel minZoomLevel;
            ZoomLevel maxZoomLevel;
            Nullable<AreaI> bbox31;
        };

        struct OSMAND_CORE_API IResultEntry
        {
        protected:
            IResultEntry();
        public:
            virtual ~IResultEntry();
        };

        typedef std::function<void(const Criteria& criteria, const IResultEntry& resultEntry)> NewResultEntryCallback;

    private:
    protected:
        ISearch();
    public:
        virtual ~ISearch();

        virtual std::shared_ptr<const IObfsCollection> getObfsCollection() const = 0;

        virtual void performSearch(
            const Criteria& criteria,
            const NewResultEntryCallback newResultEntryCallback,
            const IQueryController* const controller = nullptr) const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_SEARCH_H_)
