#include "ObfInfo.h"

OsmAnd::ObfInfo::ObfInfo()
    : version(_version)
    , creationTimestamp(_creationTimestamp)
    , isBasemap(_isBasemap)
    , mapSections(_mapSections)
    , addressSections(_addressSections)
    , routingSections(_routingSections)
    , poiSections(_poiSections)
    , transportSections(_transportSections)
{
}

OsmAnd::ObfInfo::~ObfInfo()
{
}
