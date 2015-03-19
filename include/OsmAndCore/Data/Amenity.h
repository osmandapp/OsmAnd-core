#ifndef _OSMAND_CORE_AMENITY_H_
#define _OSMAND_CORE_AMENITY_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QSet>
#include <QHash>
#include <QList>
#include <QVariant>

#include <OsmAndCore.h>
#include <OsmAndCore/MemoryCommon.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/DataCommonTypes.h>

namespace OsmAnd
{
    class ObfPoiSectionInfo;

    class OSMAND_CORE_API Amenity Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(Amenity);
        OSMAND_USE_MEMORY_MANAGER(Amenity);
    private:
    protected:
    public:
        Amenity(const std::shared_ptr<const ObfPoiSectionInfo>& obfSection);
        virtual ~Amenity();

        const std::shared_ptr<const ObfPoiSectionInfo> obfSection;

        PointI position31;
        ZoomLevel zoomLevel;
        QList<ObfPoiCategoryId> categories;
        QString nativeName;
        QHash<QString, QString> localizedNames;
        ObfObjectId id;
        QHash<int, QVariant> values;

        QHash<QString, QStringList> getDecodedCategories() const;
        QHash<QString, QString> getDecodedValues() const;
    };
}

#endif // !defined(_OSMAND_CORE_AMENITY_H_)
