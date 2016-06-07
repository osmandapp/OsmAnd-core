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

void OsmAnd::BaseSearch::startSearch(
    const Criteria& criteria,
    const NewResultEntryCallback newResultEntryCallback,
    const SearchCompletedCallback searchCompletedCallback,
    QThreadPool* const threadPool,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/) const
{
    const auto runnable = new QRunnableFunctor(
        [this, criteria, newResultEntryCallback, searchCompletedCallback, queryController]
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
            performSearch(criteria, newResultEntryCallbackWrapper, queryController);
            searchCompletedCallback(criteria, results);
        });
    threadPool->start(runnable);
}
