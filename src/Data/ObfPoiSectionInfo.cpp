#include "ObfPoiSectionInfo.h"

OsmAnd::ObfPoiSectionInfo::ObfPoiSectionInfo(const std::shared_ptr<const ObfInfo>& container)
    : ObfSectionInfo(container)
    , area31(_area31)
{
}

OsmAnd::ObfPoiSectionInfo::~ObfPoiSectionInfo()
{
}
