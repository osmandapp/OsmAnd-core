#ifndef _OSMAND_CORE_I_MAP_SYMBOLS_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_SYMBOLS_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IMapSymbolsProvider
    {
        Q_DISABLE_COPY_AND_MOVE(IMapSymbolsProvider);
    private:
    protected:
        IMapSymbolsProvider();
    public:
        virtual ~IMapSymbolsProvider();
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_SYMBOLS_PROVIDER_H_)
