#ifndef _OSMAND_CORE_TRANSPORT_STOP_SYMBOLS_PROVIDER_P_H_
#define _OSMAND_CORE_TRANSPORT_STOP_SYMBOLS_PROVIDER_P_H_ 

#include "stdlib_common.h"
#include <array>
#include <functional>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QReadWriteLock>
#include <QList>
#include <QVector>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "IMapTiledSymbolsProvider.h"
#include "TransportStop.h"
#include "TransportStopSymbolsProvider.h"

namespace OsmAnd
{
    class TransportStopSymbolsProvider_P Q_DECL_FINAL
    {
    public:
        typedef TransportStopSymbolsProvider::TransportStopSymbolsGroup TransportStopSymbolsGroup;

    private:
    protected:
        TransportStopSymbolsProvider_P(TransportStopSymbolsProvider* owner);

        struct RetainableCacheMetadata : public IMapDataProvider::RetainableCacheMetadata
        {
            RetainableCacheMetadata();
            virtual ~RetainableCacheMetadata();
        };
    public:
        virtual ~TransportStopSymbolsProvider_P();

        ImplementationInterface<TransportStopSymbolsProvider> owner;

        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric);

    friend class OsmAnd::TransportStopSymbolsProvider;
    };
}

#endif // !defined(_OSMAND_CORE_TRANSPORT_STOP_SYMBOLS_PROVIDER_P_H_)
