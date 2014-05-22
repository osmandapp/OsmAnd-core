#ifndef _OSMAND_CORE_OBJECT_WITH_ID_H_
#define _OSMAND_CORE_OBJECT_WITH_ID_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    namespace Model
    {
        class OSMAND_CORE_API ObjectWithId
        {
            Q_DISABLE_COPY(ObjectWithId);
        private:
        protected:
            ObjectWithId();

            uint64_t _id;
        public:
            virtual ~ObjectWithId();

            const uint64_t& id;
        };
    }
}

#endif // !defined(_OSMAND_CORE_OBJECT_WITH_ID_H_)
