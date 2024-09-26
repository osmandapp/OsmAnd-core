#ifndef _OSMAND_CORE_AMENITY_H_
#define _OSMAND_CORE_AMENITY_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QSet>
#include <QHash>
#include <QList>
#include <QVariant>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/MemoryCommon.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/DataCommonTypes.h>

namespace OsmAnd
{
    class ObfPoiSectionInfo;
    struct ObfPoiSectionSubtype;
    struct TagValue;

    class OSMAND_CORE_API Amenity Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(Amenity);
        OSMAND_USE_MEMORY_MANAGER(Amenity);
    public:
        struct OSMAND_CORE_API DecodedCategory Q_DECL_FINAL
        {
            DecodedCategory();
            ~DecodedCategory();

            QString category;
            QString subcategory;
        };

        struct OSMAND_CORE_API DecodedValue Q_DECL_FINAL
        {
            DecodedValue();
            ~DecodedValue();

            std::shared_ptr<const ObfPoiSectionSubtype> declaration;
            QString value;
        };

    private:
        QHash<int, QList<TagValue>> tagGroups;
    protected:
    public:
        Amenity(const std::shared_ptr<const ObfPoiSectionInfo>& obfSection);
        virtual ~Amenity();

        const std::shared_ptr<const ObfPoiSectionInfo> obfSection;

        PointI position31;
        QList<ObfPoiCategoryId> categories;
        QString nativeName;
        QHash<QString, QString> localizedNames;
        ObfObjectId id;
        QHash<int, QVariant> values;

        QString type;
        QString subType;

        void evaluateTypes();
        QList<DecodedCategory> getDecodedCategories() const;
        QList<DecodedValue> getDecodedValues() const;
        QHash<QString, QString> getDecodedValuesHash() const;

        static QHash< QString, QHash<QString, QList< std::shared_ptr<const OsmAnd::Amenity> > > > groupByCategories(
            const QList< std::shared_ptr<const OsmAnd::Amenity> >& input);

        QString getName(const QString lang, bool transliterate) const;
        void addTagGroup(int id, QList<TagValue> tagValues);
        QString getCityFromTagGroups(QString lang) const;
    };
}

#endif // !defined(_OSMAND_CORE_AMENITY_H_)
