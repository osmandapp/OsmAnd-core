#ifndef _OSMAND_CORE_OBJECT_WITH_ID_H_
#define _OSMAND_CORE_OBJECT_WITH_ID_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/DataCommonTypes.h>

namespace OsmAnd
{
    namespace Model
    {
        class OSMAND_CORE_API ObjectWithId
        {
            Q_DISABLE_COPY_AND_MOVE(ObjectWithId);
        private:
        protected:
            ObjectWithId();

            ObfObjectId _id;
        public:
            virtual ~ObjectWithId();

            const ObfObjectId& id;
        };
    }
}

#endif // !defined(_OSMAND_CORE_OBJECT_WITH_ID_H_)
