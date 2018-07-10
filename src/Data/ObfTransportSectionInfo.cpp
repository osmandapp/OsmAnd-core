#include "ObfTransportSectionInfo.h"

OsmAnd::ObfTransportSectionInfo::ObfTransportSectionInfo(const std::shared_ptr<const ObfInfo>& container)
    : ObfSectionInfo(container)
    , stopsOffset(_stopsOffset)
    , stopsLength(_stopsLength)
    , area31(_area31)
    , stringTable(_stringTable)
{
}

OsmAnd::ObfTransportSectionInfo::~ObfTransportSectionInfo()
{
}
