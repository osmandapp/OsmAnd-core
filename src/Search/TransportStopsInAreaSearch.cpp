#include "TransportStopsInAreaSearch.h"

#include "ObfDataInterface.h"
#include "TransportStop.h"

OsmAnd::TransportStopsInAreaSearch::TransportStopsInAreaSearch(const std::shared_ptr<const IObfsCollection>& obfsCollection_)
    : BaseSearch(obfsCollection_)
{
}

OsmAnd::TransportStopsInAreaSearch::~TransportStopsInAreaSearch()
{
}

void OsmAnd::TransportStopsInAreaSearch::performSearch(
    const ISearch::Criteria& criteria_,
    const NewResultEntryCallback newResultEntryCallback,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/) const
{
    const auto criteria = *dynamic_cast<const Criteria*>(&criteria_);

    const auto dataInterface = obfsCollection->obtainDataInterface(criteria.obfInfoAreaFilter.getValuePtrOrNullptr(), MinZoomLevel, MaxZoomLevel, ObfDataTypesMask().set(ObfDataType::Transport));

    const ObfTransportSectionReader::TransportStopVisitorFunction visitorFunction =
        [newResultEntryCallback, criteria_]
        (const std::shared_ptr<const OsmAnd::TransportStop>& transportStop) -> bool
        {
            ResultEntry resultEntry;
            resultEntry.transportStop = transportStop;
            newResultEntryCallback(criteria_, resultEntry);

            return true;
        };

    auto stringTable = std::make_shared<ObfSectionInfo::StringTable>();

    Nullable<AreaI> cbbox31;
    if (criteria.bbox31.isSet())
    {
        cbbox31 = AreaI(criteria.bbox31->top() >> (31 - TRANSPORT_STOP_ZOOM),
                        criteria.bbox31->left() >> (31 - TRANSPORT_STOP_ZOOM),
                        criteria.bbox31->bottom() >> (31 - TRANSPORT_STOP_ZOOM),
                        criteria.bbox31->right() >> (31 - TRANSPORT_STOP_ZOOM));
    }
    
    dataInterface->searchTransportIndex(
        nullptr,
        cbbox31.getValuePtrOrNullptr(),
        stringTable.get(),
        visitorFunction,
        queryController);
}

OsmAnd::TransportStopsInAreaSearch::Criteria::Criteria()
{
}

OsmAnd::TransportStopsInAreaSearch::Criteria::~Criteria()
{
}

OsmAnd::TransportStopsInAreaSearch::ResultEntry::ResultEntry()
{
}

OsmAnd::TransportStopsInAreaSearch::ResultEntry::~ResultEntry()
{
}
