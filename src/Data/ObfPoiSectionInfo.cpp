#include "ObfPoiSectionInfo.h"
#include "ObfPoiSectionInfo_P.h"

OsmAnd::ObfPoiSectionInfo::ObfPoiSectionInfo(const std::shared_ptr<const ObfInfo>& container)
    : ObfSectionInfo(container)
    , _p(new ObfPoiSectionInfo_P(this))
    , firstCategoryInnerOffset(0)
    , nameIndexInnerOffset(0)
    , subtypesInnerOffset(0)
    , firstBoxInnerOffset(0)
    , bboxIndexCache(AreaI(0, 0, INT_MAX, INT_MAX), 8)
{
}

OsmAnd::ObfPoiSectionInfo::~ObfPoiSectionInfo()
{
}

OsmAnd::TagValue::TagValue(QString tag, QString value)
    : tag(tag)
    , value(value)
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

QList<OsmAnd::TagValue> OsmAnd::ObfPoiSectionInfo::getTagValues(int id) const
{
    auto it = tagGroups.find(id);
    if (it != tagGroups.end())
    {
        return *it;
    }
    else
    {
        return {};
    }
}

OsmAnd::ObfPoiSectionCategories::ObfPoiSectionCategories()
{
}

OsmAnd::ObfPoiSectionCategories::~ObfPoiSectionCategories()
{
}

OsmAnd::ObfPoiSectionSubtypes::ObfPoiSectionSubtypes()
    : openingHoursSubtypeIndex(-1)
    , websiteSubtypeIndex(-1)
    , phoneSubtypeIndex(-1)
    , descriptionSubtypeIndex(-1)
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
