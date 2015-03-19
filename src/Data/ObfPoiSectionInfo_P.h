#ifndef _OSMAND_CORE_OBF_POI_SECTION_INFO_P_H_
#define _OSMAND_CORE_OBF_POI_SECTION_INFO_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QMutex>
#include <QSet>
#include <QHash>
#include <QMap>
#include <QString>
#include <QAtomicInt>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "DataCommonTypes.h"

namespace OsmAnd
{
    class ObfPoiSectionCategories;
    class ObfPoiSectionReader_P;

    class ObfPoiSectionInfo;
    class ObfPoiSectionInfo_P Q_DECL_FINAL
    {
    private:
    protected:
        ObfPoiSectionInfo_P(ObfPoiSectionInfo* owner);

        mutable std::shared_ptr<ObfPoiSectionCategories> _categories;
        mutable QAtomicInt _categoriesLoaded;
        mutable QMutex _categoriesLoadMutex;
    public:
        virtual ~ObfPoiSectionInfo_P();

        ImplementationInterface<ObfPoiSectionInfo> owner;

        std::shared_ptr<const ObfPoiSectionCategories> getCategories() const;

    friend class OsmAnd::ObfPoiSectionInfo;
    friend class OsmAnd::ObfPoiSectionReader_P;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_POI_SECTION_INFO_P_H_)
