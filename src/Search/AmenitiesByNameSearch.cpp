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

    dataInterface->scanAmenitiesByName(
        criteria.name,
        nullptr,
        criteria.xy31.getValuePtrOrNullptr(),
        criteria.bbox31.getValuePtrOrNullptr(),
        criteria.tileFilter,
        criteria.categoriesFilter.isEmpty() ? nullptr : &criteria.categoriesFilter,
        (criteria.poiAddtitionalFilter.first.isEmpty() || criteria.poiAddtitionalFilter.second.isEmpty()) ? nullptr : &criteria.poiAddtitionalFilter,
        visitorFunction,
        queryController,
        false,
        criteria.matcherMode);
}

void OsmAnd::AmenitiesByNameSearch::performTravelGuidesSearch(
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

void OsmAnd::AmenitiesByNameSearch::performSearchInFile(
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

    dataInterface->scanAmenitiesByName(
        criteria.name,
        nullptr,
        criteria.xy31.getValuePtrOrNullptr(),
        criteria.bbox31.getValuePtrOrNullptr(),
        criteria.tileFilter,
        criteria.categoriesFilter.isEmpty() ? nullptr : &criteria.categoriesFilter,
        (criteria.poiAddtitionalFilter.first.isEmpty() || criteria.poiAddtitionalFilter.second.isEmpty()) ? nullptr : &criteria.poiAddtitionalFilter,
        visitorFunction,
        queryController,
        true,
        criteria.matcherMode);
}

OsmAnd::AmenitiesByNameSearch::Criteria::Criteria()
    :matcherMode(StringMatcherMode::CHECK_STARTS_FROM_SPACE)
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
