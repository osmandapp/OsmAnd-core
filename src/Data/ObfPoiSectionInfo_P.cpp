#include "ObfPoiSectionInfo_P.h"
#include "ObfPoiSectionInfo.h"

OsmAnd::ObfPoiSectionInfo_P::ObfPoiSectionInfo_P(ObfPoiSectionInfo* owner_)
    : owner(owner_)
{
}

OsmAnd::ObfPoiSectionInfo_P::~ObfPoiSectionInfo_P()
{
}

std::shared_ptr<const OsmAnd::ObfPoiSectionCategories> OsmAnd::ObfPoiSectionInfo_P::getCategories() const
{
    if (_categoriesLoaded.loadAcquire() != 0)
    {
        if (!_categories)
        {
            QMutexLocker scopedLocker(&_categoriesLoadMutex);
            return _categories;
        }
        return _categories;
    }
    return nullptr;
}

std::shared_ptr<const OsmAnd::ObfPoiSectionSubtypes> OsmAnd::ObfPoiSectionInfo_P::getSubtypes() const
{
    if (_subtypesLoaded.loadAcquire() != 0)
    {
        if (!_subtypes)
        {
            QMutexLocker scopedLocker(&_subtypesLoadMutex);
            return _subtypes;
        }
        return _subtypes;
    }
    return nullptr;
}

QList<QPair<QString, QString>> OsmAnd::ObfPoiSectionInfo_P::getTagValues(uint32_t id) const
{
    QList<QPair<QString, QString>> res;
    {
        QReadLocker tagGroupsLocker(&_tagGroupsLock);
        auto it = _tagGroups.find(id);
        if (it != _tagGroups.end())
        {
            res = *it;
        }
    }
    return res;
}

void OsmAnd::ObfPoiSectionInfo_P::addTagGroups(QHash<uint32_t, QList<QPair<QString, QString>>> & tagGroups) const
{
    QWriteLocker tagGroupsLocker(&_tagGroupsLock);
    _tagGroups.insert(tagGroups);
}
