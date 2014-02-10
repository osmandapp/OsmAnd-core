#ifndef _OSMAND_CORE_I_MAP_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>

namespace OsmAnd {

    class OSMAND_CORE_API IMapProvider
    {
        Q_DISABLE_COPY(IMapProvider);
    private:
    protected:
        IMapProvider();
    public:
        virtual ~IMapProvider();
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_PROVIDER_H_)
