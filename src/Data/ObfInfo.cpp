#include "ObfInfo.h"

OsmAnd::ObfInfo::ObfInfo()
    : _version(-1)
    , _creationTimestamp(0)
    , _isBasemap(false)
    , version(_version)
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
