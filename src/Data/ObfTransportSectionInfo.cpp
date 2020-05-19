#include "ObfTransportSectionInfo.h"

OsmAnd::ObfTransportSectionInfo::ObfTransportSectionInfo(const std::shared_ptr<const ObfInfo>& container)
    : ObfSectionInfo(container)
    , area31(_area31)
    , stopsOffset(_stopsOffset)
    , stopsLength(_stopsLength)
    , incompleteRoutesOffset(_incompleteRoutesOffset)
    , incompleteRoutesLength(_incompleteRoutesLength)
    , stringTable(_stringTable)
{
}

OsmAnd::ObfTransportSectionInfo::~ObfTransportSectionInfo()
{
}
