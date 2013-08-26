#include "ObfMapSectionInfo.h"
#include "ObfMapSectionInfo_P.h"

OsmAnd::ObfMapSectionInfo::ObfMapSectionInfo( const std::weak_ptr<ObfInfo>& owner )
    : ObfSectionInfo(owner)
    , _d(new ObfMapSectionInfo_P(this))
    , isBasemap(_isBasemap)
    , levels(_levels)
{
}

OsmAnd::ObfMapSectionInfo::~ObfMapSectionInfo()
{
}

OsmAnd::ObfMapSectionLevel::ObfMapSectionLevel()
    : _d(new ObfMapSectionLevel_P(this))
    , offset(_offset)
    , length(_length)
    , minZoom(_minZoom)
    , maxZoom(_maxZoom)
    , area31(_area31)
{
}

OsmAnd::ObfMapSectionLevel::~ObfMapSectionLevel()
{
}
