#include "BaseSearch.h"

#include "QtExtensions.h"
#include <QThreadPool>

#include "QRunnableFunctor.h"

OsmAnd::BaseSearch::BaseSearch(const std::shared_ptr<const IObfsCollection>& obfsCollection_)
    : obfsCollection(obfsCollection_)
{
}

OsmAnd::BaseSearch::~BaseSearch()
{
}

std::shared_ptr<OsmAnd::ObfDataInterface> OsmAnd::BaseSearch::obtainDataInterface(
    const Criteria& criteria,
    const ObfDataTypesMask desiredDataTypes) const
{
    return obfsCollection->obtainDataInterface(
        criteria.bbox31.getValuePtrOrNullptr(),
        criteria.minZoomLevel,
        criteria.maxZoomLevel,
        desiredDataTypes,
        criteria.sourceFilter);
}

std::shared_ptr<const OsmAnd::IObfsCollection> OsmAnd::BaseSearch::getObfsCollection() const
{
    return obfsCollection;
}

void OsmAnd::BaseSearch::startSearch(
    const Criteria& criteria,
    const NewResultEntryCallback newResultEntryCallback,
    const SearchCompletedCallback searchCompletedCallback,
    QThreadPool* const threadPool,
    const IQueryController* const controller /*= nullptr*/) const
{
    const auto runnable = new QRunnableFunctor(
        [this, criteria, newResultEntryCallback, searchCompletedCallback, controller]
        (const QRunnableFunctor* const runnable)
        {
            QList<IResultEntry> results;
            const NewResultEntryCallback newResultEntryCallbackWrapper =
                [newResultEntryCallback, &results]
                (const Criteria& criteria, const IResultEntry& resultEntry)
                {
                    results.push_back(resultEntry);
                    newResultEntryCallback(criteria, resultEntry);
                };
            performSearch(criteria, newResultEntryCallbackWrapper, controller);
            searchCompletedCallback(criteria, results);
        });
    threadPool->start(runnable);
}
