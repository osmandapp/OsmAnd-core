#include "AmenitiesInAreaSearch.h"

#include "ObfDataInterface.h"
#include "Amenity.h"

OsmAnd::AmenitiesInAreaSearch::AmenitiesInAreaSearch(const std::shared_ptr<const IObfsCollection>& obfsCollection_)
    : BaseSearch(obfsCollection_)
{
}

OsmAnd::AmenitiesInAreaSearch::~AmenitiesInAreaSearch()
{
}

void OsmAnd::AmenitiesInAreaSearch::performSearch(
    const ISearch::Criteria& criteria_,
    const NewResultEntryCallback newResultEntryCallback,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/) const
{
    const auto criteria = *dynamic_cast<const Criteria*>(&criteria_);

    const auto dataInterface = criteria.localResources.isEmpty()
        ? obfsCollection->obtainDataInterface(criteria.obfInfoAreaFilter.getValuePtrOrNullptr(), MinZoomLevel, MaxZoomLevel, ObfDataTypesMask().set(ObfDataType::POI))
        : obfsCollection->obtainDataInterface(criteria.localResources);

    const ObfPoiSectionReader::VisitorFunction visitorFunction =
        [newResultEntryCallback, criteria_]
        (const std::shared_ptr<const OsmAnd::Amenity>& amenity) -> bool
        {
            ResultEntry resultEntry;
            resultEntry.amenity = amenity;
            newResultEntryCallback(criteria_, resultEntry);

            return true;
        };

    dataInterface->loadAmenities(
        nullptr,
        criteria.bbox31.getValuePtrOrNullptr(),
        criteria.tileFilter,
        criteria.zoomFilter,
        criteria.categoriesFilter.isEmpty() ? nullptr : &criteria.categoriesFilter,
        visitorFunction,
        queryController);
}

void OsmAnd::AmenitiesInAreaSearch::performTravelGuidesSearch(
    const ISearch::Criteria& criteria_,
    const NewResultEntryCallback newResultEntryCallback,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/) const
{
    const auto criteria = *dynamic_cast<const Criteria*>(&criteria_);

    const auto dataInterface = obfsCollection->obtainTravelGuidesDataInterface(criteria.obfInfoAreaFilter.getValuePtrOrNullptr(), MinZoomLevel, MaxZoomLevel, ObfDataTypesMask().set(ObfDataType::POI));

    const ObfPoiSectionReader::VisitorFunction visitorFunction =
        [newResultEntryCallback, criteria_]
        (const std::shared_ptr<const OsmAnd::Amenity>& amenity) -> bool
        {
            ResultEntry resultEntry;
            resultEntry.amenity = amenity;
            newResultEntryCallback(criteria_, resultEntry);

            return true;
        };

    dataInterface->loadAmenities(
        nullptr,
        criteria.bbox31.getValuePtrOrNullptr(),
        criteria.tileFilter,
        criteria.zoomFilter,
        criteria.categoriesFilter.isEmpty() ? nullptr : &criteria.categoriesFilter,
        visitorFunction,
        queryController);
}

OsmAnd::AmenitiesInAreaSearch::Criteria::Criteria()
    : zoomFilter(InvalidZoomLevel)
{
}

OsmAnd::AmenitiesInAreaSearch::Criteria::~Criteria()
{
}

OsmAnd::AmenitiesInAreaSearch::ResultEntry::ResultEntry()
{
}

OsmAnd::AmenitiesInAreaSearch::ResultEntry::~ResultEntry()
{
}
