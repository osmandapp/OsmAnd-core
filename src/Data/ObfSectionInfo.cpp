#include "ObfSectionInfo.h"

OsmAnd::ObfSectionInfo::ObfSectionInfo( const std::weak_ptr<ObfInfo>& owner_ )
    : name(_name)
    , length(_length)
    , offset(_offset)
    , owner(owner_)
{
}

OsmAnd::ObfSectionInfo::~ObfSectionInfo()
{
}
