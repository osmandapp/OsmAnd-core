#ifndef _OSMAND_CORE_MODEL_STREET_GROUP_H_
#define _OSMAND_CORE_MODEL_STREET_GROUP_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>

namespace OsmAnd {

    namespace Model {

        class OSMAND_CORE_API StreetGroup
        {
        private:
        protected:
        public:
            StreetGroup();
            virtual ~StreetGroup();

            int64_t _id;
            QString _name;
            QString _latinName;
            double _longitude;
            double _latitude;
            unsigned int _offset;
        };

    } // namespace Model

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_MODEL_STREET_GROUP_H_)
