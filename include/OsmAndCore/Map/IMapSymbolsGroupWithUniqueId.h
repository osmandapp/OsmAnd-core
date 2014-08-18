#ifndef _OSMAND_CORE_I_MAP_SYMBOLS_GROUP_WITH_UNIQUE_ID_H_
#define _OSMAND_CORE_I_MAP_SYMBOLS_GROUP_WITH_UNIQUE_ID_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IMapSymbolsGroupWithUniqueId
    {
    private:
    protected:
        IMapSymbolsGroupWithUniqueId();
    public:
        virtual ~IMapSymbolsGroupWithUniqueId();

        virtual uint64_t getId() const = 0;
        virtual bool isSharableById() const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_SYMBOLS_GROUP_WITH_UNIQUE_ID_H_)
