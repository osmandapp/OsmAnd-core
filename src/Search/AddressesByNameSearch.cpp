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

    const auto dataInterface = criteria.localResources.isEmpty() ? obfsCollection->obtainDataInterface(
        criteria.obfInfoAreaFilter.getValuePtrOrNullptr(), MinZoomLevel, MaxZoomLevel, ObfDataTypesMask().set(ObfDataType::Address)) : obfsCollection->obtainDataInterface(criteria.localResources);

    if (criteria.addressFilter != nullptr)
    {
        const OsmAnd::CollatorStringMatcher stringMatcher(criteria.name, criteria.matcherMode);
        switch (criteria.addressFilter->addressType)
        {
            case AddressType::StreetGroup:
            {
                const ObfAddressSectionReader::StreetVisitorFunction visitorFunction =
                [newResultEntryCallback, criteria_, criteria, &stringMatcher]
                (const std::shared_ptr<const OsmAnd::Street>& street) -> bool
                {
                    bool accept = criteria.name.isEmpty();
                    accept = accept || stringMatcher.matches(street->nativeName);
                    for (const auto& localizedName : constOf(street->localizedNames))
                    {
                        accept = accept || stringMatcher.matches(localizedName);
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
                [newResultEntryCallback, criteria_, criteria, &stringMatcher]
                (const std::shared_ptr<const OsmAnd::Building>& building) -> bool
                {
                    bool accept = true;
                    if (!criteria.postcode.isEmpty())
                    {
                        accept = OsmAnd::CollatorStringMatcher::cmatches(building->postcode, criteria.postcode, criteria.matcherMode);
                    }
                    else
                    {
                        accept = criteria.name.isEmpty() || stringMatcher.matches(building->nativeName);
                        for (const auto& localizedName : constOf(building->localizedNames))
                        {
                            accept = accept || stringMatcher.matches(localizedName);
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
                [newResultEntryCallback, criteria_, criteria, &stringMatcher]
                (const std::shared_ptr<const OsmAnd::StreetIntersection>& intersection) -> bool
                {
                    bool accept = criteria.name.isEmpty();
                    accept = accept || stringMatcher.matches(intersection->nativeName);
                    for (const auto& localizedName : constOf(intersection->localizedNames))
                    {
                        accept = accept || stringMatcher.matches(localizedName);
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
            case AddressType::Building:
            case AddressType::StreetIntersection:
            default:
                break;
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
                                           criteria.matcherMode,
                                           nullptr,
                                           criteria.bbox31.getValuePtrOrNullptr(),
                                           criteria.streetGroupTypesMask,
                                           criteria.includeStreets,
                                           criteria.strictMatch,
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
    , matcherMode(StringMatcherMode::CHECK_STARTS_FROM_SPACE)
    , strictMatch(false)
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
