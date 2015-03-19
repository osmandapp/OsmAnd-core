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
