#ifndef _OSMAND_CORE_I_MAP_SYMBOL_COLLECTION_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_SYMBOL_COLLECTION_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapSymbolProvider.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IMapSymbolCollectionProvider : public IMapSymbolProvider
    {
        Q_DISABLE_COPY(IMapSymbolCollectionProvider);
    private:
    protected:
        IMapSymbolCollectionProvider();
    public:
        virtual ~IMapSymbolCollectionProvider();

        virtual bool obtainSymbols(
            QList< std::shared_ptr<const MapSymbolsGroup> >& outSymbolGroups,
            const FilterCallback filterCallback = nullptr) = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_SYMBOL_COLLECTION_PROVIDER_H_)
