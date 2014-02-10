#ifndef _OSMAND_CORE_I_RETAINABLE_RESOURCE_H_
#define _OSMAND_CORE_I_RETAINABLE_RESOURCE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IRetainableResource
    {
        Q_DISABLE_COPY(IRetainableResource);
    protected:
        IRetainableResource();
    public:
        virtual ~IRetainableResource();

        virtual void releaseNonRetainedData() = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_RETAINABLE_RESOURCE_H_)
