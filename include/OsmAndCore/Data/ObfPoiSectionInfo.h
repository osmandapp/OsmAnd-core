#ifndef _OSMAND_CORE_OBF_POI_SECTION_INFO_H_
#define _OSMAND_CORE_OBF_POI_SECTION_INFO_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QStringList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/DataCommonTypes.h>
#include <OsmAndCore/Data/ObfSectionInfo.h>
#include <OsmAndCore/QuadTree.h>

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

    struct OSMAND_CORE_API ObfPoiSectionSubtype Q_DECL_FINAL
    {
        ObfPoiSectionSubtype();
        virtual ~ObfPoiSectionSubtype();

        QString name;
        QString tagName;
        bool isText;
        int frequency;
        QStringList possibleValues;
    };

    class OSMAND_CORE_API ObfPoiSectionSubtypes Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(ObfPoiSectionSubtypes);

    private:
    protected:
    public:
        ObfPoiSectionSubtypes();
        virtual ~ObfPoiSectionSubtypes();

        QList< std::shared_ptr<const ObfPoiSectionSubtype> > subtypes;
        QList< std::shared_ptr<const ObfPoiSectionSubtype> > topIndexSubtypes;

        // "opening_hours" OSM tag
        int openingHoursSubtypeIndex;
        std::shared_ptr<const ObfPoiSectionSubtype> openingHoursSubtype;

        // "website" OSM tag
        int websiteSubtypeIndex;
        std::shared_ptr<const ObfPoiSectionSubtype> websiteSubtype;

        // "phone" OSM tag
        int phoneSubtypeIndex;
        std::shared_ptr<const ObfPoiSectionSubtype> phoneSubtype;

        // "description" OSM tag
        int descriptionSubtypeIndex;
        std::shared_ptr<const ObfPoiSectionSubtype> descriptionSubtype;

    friend class OsmAnd::ObfPoiSectionReader_P;
    };

    typedef OsmAnd::QuadTree<int32_t, AreaI::CoordType> BBoxIndexTree;

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

        uint32_t firstCategoryInnerOffset;
        uint32_t nameIndexInnerOffset;
        uint32_t subtypesInnerOffset;
        uint32_t firstBoxInnerOffset;
        mutable QHash<uint32_t, QList<QPair<QString, QString>>> tagGroups;

        std::shared_ptr<const ObfPoiSectionCategories> getCategories() const;
        std::shared_ptr<const ObfPoiSectionSubtypes> getSubtypes() const;
        QList<QPair<QString, QString>> getTagValues(uint32_t id) const;

    friend class OsmAnd::ObfPoiSectionReader_P;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_POI_SECTION_INFO_H_)
