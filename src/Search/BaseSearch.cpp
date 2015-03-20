#include "BaseSearch.h"

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
