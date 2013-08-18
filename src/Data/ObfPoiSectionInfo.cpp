#include "ObfPoiSectionInfo.h"

OsmAnd::ObfPoiSectionInfo::ObfPoiSectionInfo( const std::weak_ptr<ObfInfo>& owner )
    : ObfSectionInfo(owner)
    , area31(_area31)
{
}

OsmAnd::ObfPoiSectionInfo::~ObfPoiSectionInfo()
{
}
