#include "AddressesByNameSearch.h"

#include "ObfDataInterface.h"
#include "ObfAddressSectionReader.h"
#include "Address.h"
#include "Building.h"
#include "Street.h"
#include "StreetGroup.h"
#include "StreetIntersection.h"

OsmAnd::AddressesByNameSearch::AddressesByNameSearch(const std::shared_ptr<const IObfsCollection>& obfsCollection_)
    : BaseSearch(obfsCollection_)
{
}

OsmAnd::AddressesByNameSearch::~AddressesByNameSearch()
{
}

void OsmAnd::AddressesByNameSearch::performSearch(
    const ISearch::Criteria& criteria_,
    const NewResultEntryCallback newResultEntryCallback,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/) const
{
    const auto criteria = *dynamic_cast<const Criteria*>(&criteria_);

    const auto dataInterface = obfsCollection->obtainDataInterface(
        criteria.obfInfoAreaFilter.getValuePtrOrNullptr(),
        MinZoomLevel,
        MaxZoomLevel,
        ObfDataTypesMask().set(ObfDataType::Address));

    if (criteria.addressFilter != nullptr)
    {
            switch (criteria.addressFilter->addressType)
            {
                case AddressType::StreetGroup:
                {
                    const ObfAddressSectionReader::StreetVisitorFunction visitorFunction =
                    [this, newResultEntryCallback, criteria_, criteria]
                    (const std::shared_ptr<const OsmAnd::Street>& street) -> bool
                    {
                        bool accept = criteria.name.isEmpty();
//                        accept = accept || street->nativeName.contains(criteria.name, Qt::CaseInsensitive);
                        accept = accept || OsmAnd::CollatorStringMatcher::cmatches(stringMatcher.getCollator(), street->nativeName, criteria.name, OsmAnd::CollatorStringMatcher::CHECK_CONTAINS);
                        for (const auto& localizedName : constOf(street->localizedNames))
                        {
//                            accept = accept || localizedName.contains(criteria.name, Qt::CaseInsensitive);
                            accept = accept || OsmAnd::CollatorStringMatcher::cmatches(stringMatcher.getCollator(), localizedName, criteria.name, OsmAnd::CollatorStringMatcher::CHECK_CONTAINS);
                            if (accept)
                                break;
                        }
                        
                        if (accept)
                        {
                            ResultEntry resultEntry;
                            resultEntry.address = street;
                            newResultEntryCallback(criteria_, resultEntry);
                            return true;
                        }
                        else
                        {
                            return false;
                        }
                    };
                    
                    QList<std::shared_ptr<const StreetGroup>> streetGroups;
                    streetGroups << std::static_pointer_cast<const StreetGroup>(criteria.addressFilter);
                    dataInterface->loadStreetsFromGroups(
                                                         streetGroups,
                                                         nullptr,
                                                         criteria.bbox31.getValuePtrOrNullptr(),
                                                         visitorFunction,
                                                         queryController);
                    break;
                }
                    
                case AddressType::Street:
                {
                    const ObfAddressSectionReader::BuildingVisitorFunction visitorFunction =
                    [this, newResultEntryCallback, criteria_, criteria]
                    (const std::shared_ptr<const OsmAnd::Building>& building) -> bool
                    {
                        bool accept = true;
                        if (!criteria.postcode.isEmpty())
                        {
//                            accept = criteria.postcode.compare(building->postcode, Qt::CaseInsensitive) == 0;
                            accept = OsmAnd::CollatorStringMatcher::cmatches(stringMatcher.getCollator(), building->postcode, criteria.postcode, OsmAnd::CollatorStringMatcher::CHECK_EQUALS_FROM_SPACE);
                        }
                        else
                        {
//                            accept = criteria.name.isEmpty() || building->nativeName.contains(criteria.name, Qt::CaseInsensitive);
                            accept = criteria.name.isEmpty() || OsmAnd::CollatorStringMatcher::cmatches(stringMatcher.getCollator(), building->nativeName, criteria.name, OsmAnd::CollatorStringMatcher::CHECK_CONTAINS);
                            for (const auto& localizedName : constOf(building->localizedNames))
                            {
//                                accept = accept || localizedName.contains(criteria.name, Qt::CaseInsensitive);
                                accept = accept || OsmAnd::CollatorStringMatcher::cmatches(stringMatcher.getCollator(), localizedName, criteria.name, OsmAnd::CollatorStringMatcher::CHECK_CONTAINS);
                                if (accept)
                                    break;
                            }
                        }
                        
                        if (accept)
                        {
                            ResultEntry resultEntry;
                            resultEntry.address = building;
                            newResultEntryCallback(criteria_, resultEntry);
                            return true;
                        }
                        else
                        {
                            return false;
                        }
                    };
                    
                    QList<std::shared_ptr<const Street>> streets;
                    streets << std::static_pointer_cast<const Street>(criteria.addressFilter);
                    dataInterface->loadBuildingsFromStreets(
                                                            streets,
                                                            nullptr,
                                                            criteria.bbox31.getValuePtrOrNullptr(),
                                                            visitorFunction,
                                                            queryController);
                    
                    
                    const ObfAddressSectionReader::IntersectionVisitorFunction intersectionVisitorFunction =
                    [this, newResultEntryCallback, criteria_, criteria]
                    (const std::shared_ptr<const OsmAnd::StreetIntersection>& intersection) -> bool
                    {
                        bool accept = criteria.name.isEmpty();
//                        accept = accept || intersection->nativeName.contains(criteria.name, Qt::CaseInsensitive);
                        accept = accept || OsmAnd::CollatorStringMatcher::cmatches(stringMatcher.getCollator(), intersection->nativeName, criteria.name, OsmAnd::CollatorStringMatcher::CHECK_CONTAINS);
                        for (const auto& localizedName : constOf(intersection->localizedNames))
                        {
//                            accept = accept || localizedName.contains(criteria.name, Qt::CaseInsensitive);
                            accept = accept || OsmAnd::CollatorStringMatcher::cmatches(stringMatcher.getCollator(), localizedName, criteria.name, OsmAnd::CollatorStringMatcher::CHECK_CONTAINS);
                            if (accept)
                                break;
                        }
                        
                        if (accept)
                        {
                            ResultEntry resultEntry;
                            resultEntry.address = intersection;
                            newResultEntryCallback(criteria_, resultEntry);
                            return true;
                        }
                        else
                        {
                            return false;
                        }
                    };
                    
                    dataInterface->loadIntersectionsFromStreets(
                                                                streets,
                                                                nullptr,
                                                                criteria.bbox31.getValuePtrOrNullptr(),
                                                                intersectionVisitorFunction,
                                                                queryController);
                    
                    break;
                }
            }
    }
    else
    {
        const ObfAddressSectionReader::VisitorFunction visitorFunction =
        [newResultEntryCallback, criteria_]
        (const std::shared_ptr<const OsmAnd::Address>& address) -> bool
        {
            ResultEntry resultEntry;
            resultEntry.address = address;
            newResultEntryCallback(criteria_, resultEntry);
            
            return true;
        };
        
        dataInterface->scanAddressesByName(
                                           criteria.name,
                                           nullptr,
                                           criteria.bbox31.getValuePtrOrNullptr(),
                                           criteria.streetGroupTypesMask,
                                           criteria.includeStreets,
                                           visitorFunction,
                                           queryController);
    }
}

QVector<OsmAnd::AddressesByNameSearch::ResultEntry> OsmAnd::AddressesByNameSearch::performSearch(
        const OsmAnd::AddressesByNameSearch::Criteria& criteria) const
{
    QVector<ResultEntry> result{};
    performSearch(
                criteria,
                [&result](const OsmAnd::ISearch::Criteria& criteria, const OsmAnd::BaseSearch::IResultEntry& resultEntry) {
        result.append(static_cast<const ResultEntry&>(resultEntry));
    });
    return result;

}

OsmAnd::AddressesByNameSearch::Criteria::Criteria()
    : streetGroupTypesMask(fullObfAddressStreetGroupTypesMask())
    , includeStreets(true)
{
}

OsmAnd::AddressesByNameSearch::Criteria::~Criteria()
{
}

OsmAnd::AddressesByNameSearch::ResultEntry::ResultEntry()
{
}

OsmAnd::AddressesByNameSearch::ResultEntry::~ResultEntry()
{
}
