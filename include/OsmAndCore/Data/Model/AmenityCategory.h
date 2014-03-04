#ifndef _OSMAND_CORE_AMENITY_CATEGORY_H_
#define _OSMAND_CORE_AMENITY_CATEGORY_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QStringList>

#include <OsmAndCore.h>
#include <OsmAndCore/MemoryCommon.h>
#include <OsmAndCore/Reference.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    class ObfPoiSectionReader_P;

    namespace Model
    {
        class OSMAND_CORE_API OSMAND_REFERENCEABLE_CLASS(AmenityCategory)
        {
            Q_DISABLE_COPY(AmenityCategory);
            OSMAND_USE_MEMORY_MANAGER(AmenityCategory);
            OSMAND_BE_REFERENCEABLE_CLASS(AmenityCategory);
        private:
        protected:
            AmenityCategory();

            QString _name;
            QStringList _subcategories;
        public:
            virtual ~AmenityCategory();

            const QString& name;
            const QStringList& subcategories;

        friend class OsmAnd::ObfPoiSectionReader_P;
        };
    }
}

#endif // !defined(_OSMAND_CORE_AMENITY_CATEGORY_H_)
