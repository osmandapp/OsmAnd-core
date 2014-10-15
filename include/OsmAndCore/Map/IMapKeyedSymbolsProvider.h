#ifndef _OSMAND_CORE_I_MAP_KEYED_SYMBOLS_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_KEYED_SYMBOLS_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapSymbol.h>
#include <OsmAndCore/Map/IMapKeyedDataProvider.h>
#include <OsmAndCore/Map/IMapSymbolsProvider.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IMapKeyedSymbolsProvider
        : public IMapKeyedDataProvider
        , public IMapSymbolsProvider
    {
        Q_DISABLE_COPY_AND_MOVE(IMapKeyedSymbolsProvider);
    private:
    protected:
        IMapKeyedSymbolsProvider();
    public:
        virtual ~IMapKeyedSymbolsProvider();
    };

    class OSMAND_CORE_API KeyedMapSymbolsData : public MapKeyedData
    {
        Q_DISABLE_COPY_AND_MOVE(KeyedMapSymbolsData);

    private:
    protected:
    public:
        KeyedMapSymbolsData(const std::shared_ptr<MapSymbolsGroup>& symbolsGroup, const Key key);
        virtual ~KeyedMapSymbolsData();

        std::shared_ptr<MapSymbolsGroup> symbolsGroup;

        virtual void releaseConsumableContent();
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_KEYED_SYMBOLS_PROVIDER_H_)
