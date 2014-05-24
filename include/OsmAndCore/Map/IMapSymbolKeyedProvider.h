#ifndef _OSMAND_CORE_I_MAP_SYMBOL_KEYED_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_SYMBOL_KEYED_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapSymbolProvider.h>
#include <OsmAndCore/Map/IMapKeyedProvider.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IMapSymbolKeyedProvider
        : public IMapSymbolProvider
        , public virtual IMapKeyedProvider
    {
        Q_DISABLE_COPY(IMapSymbolKeyedProvider);
    private:
    protected:
        IMapSymbolKeyedProvider();
    public:
        virtual ~IMapSymbolKeyedProvider();

        virtual bool obtainSymbolsGroup(const Key key, std::shared_ptr<const MapSymbolsGroup>& outSymbolGroups) = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_SYMBOL_KEYED_PROVIDER_H_)
