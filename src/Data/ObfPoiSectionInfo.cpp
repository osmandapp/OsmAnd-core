#include "ObfPoiSectionInfo.h"
#include "ObfPoiSectionInfo_P.h"

OsmAnd::ObfPoiSectionInfo::ObfPoiSectionInfo(const std::shared_ptr<const ObfInfo>& container)
    : ObfSectionInfo(container)
    , _p(new ObfPoiSectionInfo_P(this))
    , hasCategories(false)
    , firstCategoryInnerOffset(0)
    , hasNameIndex(false)
    , nameIndexInnerOffset(0)
{
}

OsmAnd::ObfPoiSectionInfo::~ObfPoiSectionInfo()
{
}

std::shared_ptr<const OsmAnd::ObfPoiSectionCategories> OsmAnd::ObfPoiSectionInfo::getCategories() const
{
    return _p->getCategories();
}

OsmAnd::ObfPoiSectionCategories::ObfPoiSectionCategories()
{
}

OsmAnd::ObfPoiSectionCategories::~ObfPoiSectionCategories()
{
}
