#include "AddressesByNameSearch.h"

#include "ObfDataInterface.h"
#include "ObfAddressSectionReader.h"
#include "Address.h"

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
        criteria.bbox31.getValuePtrOrNullptr(),
        MinZoomLevel,
        MaxZoomLevel,
        ObfDataTypesMask().set(ObfDataType::Address),
        criteria.sourceFilter);

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
