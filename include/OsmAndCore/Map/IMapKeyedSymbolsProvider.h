#ifndef _OSMAND_CORE_I_MAP_KEYED_SYMBOLS_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_KEYED_SYMBOLS_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapSymbol.h>
#include <OsmAndCore/Map/MapSymbolsGroup.h>
#include <OsmAndCore/Map/IMapKeyedDataProvider.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IMapKeyedSymbolsProvider : public IMapKeyedDataProvider
    {
        Q_DISABLE_COPY_AND_MOVE(IMapKeyedSymbolsProvider)

    public:
        class OSMAND_CORE_API Data : public IMapKeyedDataProvider::Data
        {
            Q_DISABLE_COPY_AND_MOVE(Data)
        private:
        protected:
        public:
            Data(
                const Key key,
                const std::shared_ptr<MapSymbolsGroup>& symbolsGroup,
                const RetainableCacheMetadata* const pRetainableCacheMetadata = nullptr);
            virtual ~Data();

            std::shared_ptr<MapSymbolsGroup> symbolsGroup;
        };
    private:
    protected:
        IMapKeyedSymbolsProvider();
    public:
        virtual ~IMapKeyedSymbolsProvider();

        virtual bool obtainKeyedSymbols(
            const Request& request,
            std::shared_ptr<Data>& outKeyedSymbols,
            std::shared_ptr<Metric>* const pOutMetric = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_KEYED_SYMBOLS_PROVIDER_H_)
