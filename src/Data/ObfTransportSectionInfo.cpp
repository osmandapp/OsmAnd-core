#include "ObfTransportSectionInfo.h"

OsmAnd::ObfTransportSectionInfo::ObfTransportSectionInfo( const std::weak_ptr<ObfInfo>& owner )
    : ObfSectionInfo(owner)
    , area24(_area24)
{
}

OsmAnd::ObfTransportSectionInfo::~ObfTransportSectionInfo()
{
}
