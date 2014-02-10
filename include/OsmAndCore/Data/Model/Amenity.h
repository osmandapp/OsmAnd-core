#ifndef _OSMAND_CORE_AMENITY_H_
#define _OSMAND_CORE_AMENITY_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QStringList>

#include <OsmAndCore.h>
#include <OsmAndCore/Memory.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    class ObfPoiSectionReader_P;

    namespace Model
    {
        class OSMAND_CORE_API Amenity
        {
            //Q_DISABLE_COPY(Amenity);
        private:
        protected:
            Amenity();

            uint64_t _id;
            QString _name;
            QString _latinName;
            PointI _point31;
            QString _openingHours;
            QString _site;
            QString _phone;
            QString _description;
            uint32_t _categoryId;
            uint32_t _subcategoryId;
        public:
            virtual ~Amenity();

            const uint64_t& id;
            const QString& name;
            const QString& latinName;
            const PointI& point31;
            const QString& openingHours;
            const QString& site;
            const QString& phone;
            const QString& description;
            const uint32_t& categoryId;
            const uint32_t& subcategoryId;

        friend class OsmAnd::ObfPoiSectionReader_P;
        };
    } // namespace Model
} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_AMENITY_H_)
