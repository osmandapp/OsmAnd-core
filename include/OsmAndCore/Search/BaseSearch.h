#ifndef _OSMAND_CORE_BASE_SEARCH_H_
#define _OSMAND_CORE_BASE_SEARCH_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Data/DataCommonTypes.h>
#include <OsmAndCore/Search/ISearch.h>

namespace OsmAnd
{
    class ObfDataInterface;

    class OSMAND_CORE_API BaseSearch : public ISearch
    {
        Q_DISABLE_COPY_AND_MOVE(BaseSearch);
    private:
    protected:
        BaseSearch(const std::shared_ptr<const IObfsCollection>& obfsCollection);

        std::shared_ptr<ObfDataInterface> obtainDataInterface(
            const Criteria& criteria,
            const ObfDataTypesMask desiredDataTypes) const;
    public:
        virtual ~BaseSearch();

        const std::shared_ptr<const IObfsCollection> obfsCollection;
        virtual std::shared_ptr<const IObfsCollection> getObfsCollection() const;

        virtual void startSearch(
            const Criteria& criteria,
            const NewResultEntryCallback newResultEntryCallback,
            const SearchCompletedCallback searchCompletedCallback,
            QThreadPool* const threadPool,
            const IQueryController* const controller = nullptr) const;
    };
}

#endif // !defined(_OSMAND_CORE_BASE_SEARCH_H_)
