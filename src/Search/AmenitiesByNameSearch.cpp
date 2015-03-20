#include "AmenitiesByNameSearch.h"

#include "ObfDataInterface.h"
#include "Amenity.h"

OsmAnd::AmenitiesByNameSearch::AmenitiesByNameSearch(const std::shared_ptr<const IObfsCollection>& obfsCollection_)
    : BaseSearch(obfsCollection_)
{
}

OsmAnd::AmenitiesByNameSearch::~AmenitiesByNameSearch()
{
}

void OsmAnd::AmenitiesByNameSearch::performSearch(
    const ISearch::Criteria& criteria_,
    const NewResultEntryCallback newResultEntryCallback,
    const IQueryController* const controller /*= nullptr*/) const
{
    const auto criteria = *dynamic_cast<const Criteria*>(&criteria_);

    const auto dataInterface = obtainDataInterface(criteria, ObfDataTypesMask().set(ObfDataType::POI));

    const ObfPoiSectionReader::VisitorFunction visitorFunction =
        [newResultEntryCallback, criteria_]
        (const std::shared_ptr<const OsmAnd::Amenity>& amenity) -> bool
        {
            ResultEntry resultEntry;
            resultEntry.amenity = amenity;
            newResultEntryCallback(criteria_, resultEntry);

            return false;
        };

    dataInterface->scanAmenitiesByName(
        criteria.name,
        nullptr,
        criteria.minZoomLevel,
        criteria.maxZoomLevel,
        criteria.bbox31.getValuePtrOrNullptr(),
        criteria.categoriesFilter.isEmpty() ? nullptr : &criteria.categoriesFilter,
        visitorFunction,
        controller);
}

OsmAnd::AmenitiesByNameSearch::Criteria::Criteria()
{
}

OsmAnd::AmenitiesByNameSearch::Criteria::~Criteria()
{
}

OsmAnd::AmenitiesByNameSearch::ResultEntry::ResultEntry()
{
}

OsmAnd::AmenitiesByNameSearch::ResultEntry::~ResultEntry()
{
}
