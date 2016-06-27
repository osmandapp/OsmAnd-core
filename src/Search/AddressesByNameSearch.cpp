#include "AddressesByNameSearch.h"

#include "ObfDataInterface.h"
#include "ObfAddressSectionReader.h"
#include "Address.h"
#include "Building.h"
#include "Street.h"
#include "StreetGroup.h"

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

    if (!criteria.addressFilter.isEmpty())
    {
        QList<std::shared_ptr<const StreetGroup>> streetGroupsList;
        QList<std::shared_ptr<const Street>> streetsList;
        for (const auto& address : constOf(criteria.addressFilter))
        {
            switch (address->addressType)
            {
                case AddressType::StreetGroup:
                    streetGroupsList << std::static_pointer_cast<const StreetGroup>(address);
                    break;
                    
                case AddressType::Street:
                    streetsList << std::static_pointer_cast<const Street>(address);
                    break;
            }
        }
        
        if (!streetGroupsList.isEmpty())
        {
            const ObfAddressSectionReader::StreetVisitorFunction visitorFunction =
            [newResultEntryCallback, criteria_, criteria]
            (const std::shared_ptr<const OsmAnd::Street>& street) -> bool
            {
                bool accept = criteria.name.isEmpty();
                accept = accept || street->nativeName.contains(criteria.name, Qt::CaseInsensitive);
                for (const auto& localizedName : constOf(street->localizedNames))
                {
                    accept = accept || localizedName.contains(criteria.name, Qt::CaseInsensitive);
                    if (accept)
                        break;
                }
                
                if (accept)
                {
                    ResultEntry resultEntry;
                    resultEntry.address = std::static_pointer_cast<const Address>(street);
                    newResultEntryCallback(criteria_, resultEntry);
                    return true;
                }
                else
                {
                    return false;
                }
            };
            
            dataInterface->loadStreetsFromGroups(
                                                 streetGroupsList,
                                                 nullptr,
                                                 criteria.bbox31.getValuePtrOrNullptr(),
                                                 visitorFunction,
                                                 queryController);
        }
        else if (!streetsList.isEmpty())
        {
            const ObfAddressSectionReader::BuildingVisitorFunction visitorFunction =
            [newResultEntryCallback, criteria_, criteria]
            (const std::shared_ptr<const OsmAnd::Building>& building) -> bool
            {
                bool accept = true;
                if (!criteria.postcode.isEmpty())
                {
                    accept = criteria.postcode.compare(building->postcode, Qt::CaseInsensitive) == 0;
                }
                else
                {
                    accept = criteria.name.isEmpty() || building->nativeName.contains(criteria.name, Qt::CaseInsensitive);
                    for (const auto& localizedName : constOf(building->localizedNames))
                    {
                        accept = accept || localizedName.contains(criteria.name, Qt::CaseInsensitive);
                        if (accept)
                            break;
                    }
                }
                
                if (accept)
                {
                    ResultEntry resultEntry;
                    resultEntry.building = building;
                    newResultEntryCallback(criteria_, resultEntry);
                    return true;
                }
                else
                {
                    return false;
                }
            };
            
            dataInterface->loadBuildingsFromStreets(
                                                    streetsList,
                                                    nullptr,
                                                    criteria.bbox31.getValuePtrOrNullptr(),
                                                    visitorFunction,
                                                    queryController);
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
