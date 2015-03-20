#include "ObfPoiSectionInfo.h"
#include "ObfPoiSectionInfo_P.h"

OsmAnd::ObfPoiSectionInfo::ObfPoiSectionInfo(const std::shared_ptr<const ObfInfo>& container)
    : ObfSectionInfo(container)
    , _p(new ObfPoiSectionInfo_P(this))
    , firstCategoryInnerOffset(0)
    , nameIndexInnerOffset(0)
    , subtypesInnerOffset(0)
    , firstBoxInnerOffset(0)
{
}

OsmAnd::ObfPoiSectionInfo::~ObfPoiSectionInfo()
{
}

std::shared_ptr<const OsmAnd::ObfPoiSectionCategories> OsmAnd::ObfPoiSectionInfo::getCategories() const
{
    return _p->getCategories();
}

std::shared_ptr<const OsmAnd::ObfPoiSectionSubtypes> OsmAnd::ObfPoiSectionInfo::getSubtypes() const
{
    return _p->getSubtypes();
}

OsmAnd::ObfPoiSectionCategories::ObfPoiSectionCategories()
{
}

OsmAnd::ObfPoiSectionCategories::~ObfPoiSectionCategories()
{
}

OsmAnd::ObfPoiSectionSubtypes::ObfPoiSectionSubtypes()
{
}

OsmAnd::ObfPoiSectionSubtypes::~ObfPoiSectionSubtypes()
{
}

OsmAnd::ObfPoiSectionSubtype::ObfPoiSectionSubtype()
    : isText(false)
    , frequency(-1)
{
}

OsmAnd::ObfPoiSectionSubtype::~ObfPoiSectionSubtype()
{
}
