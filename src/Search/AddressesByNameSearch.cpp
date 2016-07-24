#include "AddressesByNameSearch.h"

#include "Address.h"
#include "Building.h"
#include "ObfAddress.h"
#include "ObfAddressSectionReader.h"
#include "ObfDataInterface.h"
#include "ObfStreetGroup.h"
#include "ObfStreet.h"
#include "StreetGroup.h"
#include "Street.h"
#include "StreetIntersection.h"

OsmAnd::AddressesByNameSearch::AddressesByNameSearch(
        const std::shared_ptr<const IObfsCollection>& obfsCollection_)
    : BaseSearch(obfsCollection_)
{
}

OsmAnd::AddressesByNameSearch::~AddressesByNameSearch()
{
}

void OsmAnd::AddressesByNameSearch::performSearch(
        const OsmAnd::ObfAddressSectionReader::Filter& filter,
        const std::shared_ptr<const OsmAnd::IQueryController>& queryController) const
{
    const auto dataInterface = obfsCollection->obtainDataInterface(
                filter.obfInfoAreaBbox31().getValuePtrOrNullptr(),
                MinZoomLevel,
                MaxZoomLevel,
                ObfDataTypesMask().set(ObfDataType::Address));

    if (filter.parent())
        switch (filter.parent()->type())
        {
        case AddressType::StreetGroup:
            dataInterface->loadStreetsFromGroups(
                        {std::static_pointer_cast<const ObfStreetGroup>(filter.parent())},
                        filter,
                        queryController);
            break;
        case AddressType::Street:
            dataInterface->loadBuildingsFromStreets(
                        {std::static_pointer_cast<const ObfStreet>(filter.parent())},
                        filter,
                        queryController);
            dataInterface->loadIntersectionsFromStreets(
                        {std::static_pointer_cast<const ObfStreet>(filter.parent())},
                        filter,
                        queryController);
            break;
        }
   else
        dataInterface->scanAddressesByName(filter, queryController);
}

void OsmAnd::AddressesByNameSearch::performSearch(
    const ISearch::Criteria& criteria_,
    const NewResultEntryCallback newResultEntryCallback,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/) const
{
//    const auto criteria = *dynamic_cast<const Criteria*>(&criteria_);

//    const auto dataInterface = obfsCollection->obtainDataInterface(
//        criteria.obfInfoAreaFilter.getValuePtrOrNullptr(),
//        MinZoomLevel,
//        MaxZoomLevel,
//        ObfDataTypesMask().set(ObfDataType::Address));

//    if (criteria.addressFilter != nullptr)
//    {
//            switch (criteria.addressFilter->addressType)
//            {
//                case AddressType::StreetGroup:
//                {
//                    const ObfAddressSectionReader::StreetVisitorFunction visitorFunction =
//                    [newResultEntryCallback, criteria_, criteria]
//                    (const std::shared_ptr<const OsmAnd::Street>& street) -> bool
//                    {
//                        bool accept = criteria.name.isEmpty();
//                        accept = accept || street->nativeName.contains(criteria.name, Qt::CaseInsensitive);
//                        for (const auto& localizedName : constOf(street->localizedNames()))
//                        {
//                            accept = accept || localizedName.contains(criteria.name, Qt::CaseInsensitive);
//                            if (accept)
//                                break;
//                        }
                        
//                        if (accept)
//                        {
//                            ResultEntry resultEntry;
//                            resultEntry.address = street;
//                            newResultEntryCallback(criteria_, resultEntry);
//                            return true;
//                        }
//                        else
//                        {
//                            return false;
//                        }
//                    };
                    
//                    QList<std::shared_ptr<const StreetGroup>> streetGroups;
//                    streetGroups << std::static_pointer_cast<const StreetGroup>(criteria.addressFilter);
//                    dataInterface->loadStreetsFromGroups(streetGroups, filter, queryController);
//                    break;
//                }
                    
//                case AddressType::Street:
//                {
//                    const ObfAddressSectionReader::BuildingVisitorFunction visitorFunction =
//                    [newResultEntryCallback, criteria_, criteria]
//                    (const std::shared_ptr<const OsmAnd::Building>& building) -> bool
//                    {
//                        bool accept = true;
//                        if (!criteria.postcode.isEmpty())
//                        {
//                            accept = criteria.postcode.compare(building->postcode, Qt::CaseInsensitive) == 0;
//                        }
//                        else
//                        {
//                            accept = criteria.name.isEmpty() || building->nativeName.contains(criteria.name, Qt::CaseInsensitive);
//                            for (const auto& localizedName : constOf(building->localizedNames))
//                            {
//                                accept = accept || localizedName.contains(criteria.name, Qt::CaseInsensitive);
//                                if (accept)
//                                    break;
//                            }
//                        }
                        
//                        if (accept)
//                        {
//                            ResultEntry resultEntry;
//                            resultEntry.address = building;
//                            newResultEntryCallback(criteria_, resultEntry);
//                            return true;
//                        }
//                        else
//                        {
//                            return false;
//                        }
//                    };
                    
//                    QList<std::shared_ptr<const Street>> streets;
//                    streets << std::static_pointer_cast<const Street>(criteria.addressFilter);
//                    dataInterface->loadBuildingsFromStreets(streets, filter, queryController);
                    
                    
//                    const ObfAddressSectionReader::IntersectionVisitorFunction intersectionVisitorFunction =
//                    [newResultEntryCallback, criteria_, criteria]
//                    (const std::shared_ptr<const OsmAnd::StreetIntersection>& intersection) -> bool
//                    {
//                        bool accept = criteria.name.isEmpty();
//                        accept = accept || intersection->nativeName.contains(criteria.name, Qt::CaseInsensitive);
//                        for (const auto& localizedName : constOf(intersection->localizedNames))
//                        {
//                            accept = accept || localizedName.contains(criteria.name, Qt::CaseInsensitive);
//                            if (accept)
//                                break;
//                        }
                        
//                        if (accept)
//                        {
//                            ResultEntry resultEntry;
//                            resultEntry.address = intersection;
//                            newResultEntryCallback(criteria_, resultEntry);
//                            return true;
//                        }
//                        else
//                        {
//                            return false;
//                        }
//                    };
                    
//                    dataInterface->loadIntersectionsFromStreets(streets, filter, queryController);
                    
//                    break;
//                }
//            }
//    }
//    else
//    {
//        const ObfAddressSectionReader::VisitorFunction visitorFunction =
//        [newResultEntryCallback, criteria_]
//        (const std::shared_ptr<const OsmAnd::Address>& address) -> bool
//        {
//            ResultEntry resultEntry;
//            resultEntry.address = address;
//            newResultEntryCallback(criteria_, resultEntry);
            
//            return true;
//        };
        
//        dataInterface->scanAddressesByName(
//                                           criteria.name,
//                                           nullptr,
//                                           criteria.bbox31.getValuePtrOrNullptr(),
//                                           criteria.streetGroupTypesMask,
//                                           criteria.includeStreets,
//                                           visitorFunction,
//                                           queryController);
//    }
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
