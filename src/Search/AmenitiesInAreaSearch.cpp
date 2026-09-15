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
        criteria.poiAdditionalFilter.first.isEmpty() ? nullptr : &criteria.poiAdditionalFilter,
        visitorFunction,
        queryController);
}

void OsmAnd::AmenitiesInAreaSearch::performTravelGuidesSearch(
    const QString filename,
    const ISearch::Criteria& criteria_,
    const NewResultEntryCallback newResultEntryCallback,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/) const
{
    const QList< std::shared_ptr<const ObfFile> > files = obfsCollection->getObfFiles();
    std::shared_ptr<const ObfFile> res;
    for (const auto& file : files)
    {
        if (file->filePath.contains(filename, Qt::CaseInsensitive))
        {
            res = file;
            break;
        }
    }

    performSearchInFile(res, criteria_, newResultEntryCallback, queryController);
}

void OsmAnd::AmenitiesInAreaSearch::performSearchInFile(
    const std::shared_ptr<const ObfFile>& obfFile,
    const ISearch::Criteria& criteria_,
    const NewResultEntryCallback newResultEntryCallback,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/) const
{
    if (!obfFile)
        return;

    const auto criteria = *dynamic_cast<const Criteria*>(&criteria_);
    const auto dataInterface = obfsCollection->obtainDataInterface(obfFile);

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
        criteria.poiAdditionalFilter.first.isEmpty() ? nullptr : &criteria.poiAdditionalFilter,
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
