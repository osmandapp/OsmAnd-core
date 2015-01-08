#include "ObfTransportSectionInfo.h"

OsmAnd::ObfTransportSectionInfo::ObfTransportSectionInfo(const std::shared_ptr<const ObfInfo>& container)
    : ObfSectionInfo(container)
    , area24(_area24)
{
}

OsmAnd::ObfTransportSectionInfo::~ObfTransportSectionInfo()
{
}
