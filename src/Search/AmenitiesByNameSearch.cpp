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
        queryController);
}

void OsmAnd::AmenitiesByNameSearch::performTravelGuidesSearch(
    const QString filename,
    const ISearch::Criteria& criteria_,
    const NewResultEntryCallback newResultEntryCallback,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/) const
{
    const auto criteria = *dynamic_cast<const Criteria*>(&criteria_);

    QList< std::shared_ptr<const OsmAnd::ObfFile> > files = obfsCollection->getObfFiles();
    std::shared_ptr<const OsmAnd::ObfFile> res;
    for (std::shared_ptr<const OsmAnd::ObfFile> file : files)
    {
        if (file->filePath.contains(filename, Qt::CaseInsensitive))
        {
            res = file;
            break;
        }
    }
    std::shared_ptr<OsmAnd::ObfDataInterface> dataInterface = obfsCollection->obtainDataInterface(res);

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
        true);
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
