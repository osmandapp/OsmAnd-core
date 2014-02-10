#ifndef _OSMAND_CORE_MODEL_STREET_H_
#define _OSMAND_CORE_MODEL_STREET_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd {

    class ObfAddressSectionReader_P;

    namespace Model {

        class OSMAND_CORE_API Street
        {
        private:
        protected:
            uint64_t _id;
            QString _name;
            QString _latinName;
            PointI _tile24;
            uint32_t _offset;

            Street();
        public:
            virtual ~Street();

            const uint64_t& id;
            const QString& name;
            const QString& latinName;
            const PointI& tile24;

        friend class OsmAnd::ObfAddressSectionReader_P;
        };

    } // namespace Model

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_MODEL_STREET_H_)
