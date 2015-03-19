#ifndef _OSMAND_CORE_OBF_POI_SECTION_INFO_H_
#define _OSMAND_CORE_OBF_POI_SECTION_INFO_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QStringList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/ObfSectionInfo.h>

namespace OsmAnd
{
    class ObfPoiSectionReader_P;

    class OSMAND_CORE_API ObfPoiSectionCategories Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(ObfPoiSectionCategories);

    private:
    protected:
    public:
        ObfPoiSectionCategories();
        virtual ~ObfPoiSectionCategories();

        QStringList mainCategories;
        QList<QStringList> subCategories;

    friend class OsmAnd::ObfPoiSectionReader_P;
    };

    class ObfPoiSectionInfo_P;
    class OSMAND_CORE_API ObfPoiSectionInfo : public ObfSectionInfo
    {
        Q_DISABLE_COPY_AND_MOVE(ObfPoiSectionInfo);
    private:
        PrivateImplementation<ObfPoiSectionInfo_P> _p;
    protected:
    public:
        ObfPoiSectionInfo(const std::shared_ptr<const ObfInfo>& container);
        virtual ~ObfPoiSectionInfo();

        AreaI area31;

        bool hasCategories;
        uint32_t firstCategoryInnerOffset;

        bool hasNameIndex;
        uint32_t nameIndexInnerOffset;

        std::shared_ptr<const ObfPoiSectionCategories> getCategories() const;

    friend class OsmAnd::ObfPoiSectionReader_P;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_POI_SECTION_INFO_H_)
