#ifndef _OSMAND_CORE_AMENITY_SYMBOLS_PROVIDER_P_H_
#define _OSMAND_CORE_AMENITY_SYMBOLS_PROVIDER_P_H_

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
#include "Amenity.h"
#include "AmenitySymbolsProvider.h"

namespace OsmAnd
{
    class AmenitySymbolsProvider_P Q_DECL_FINAL
    {
    public:
        typedef AmenitySymbolsProvider::AmenitySymbolsGroup AmenitySymbolsGroup;

    private:
    protected:
        AmenitySymbolsProvider_P(AmenitySymbolsProvider* owner);

        struct RetainableCacheMetadata : public IMapDataProvider::RetainableCacheMetadata
        {
            RetainableCacheMetadata();
            virtual ~RetainableCacheMetadata();
        };
    public:
        virtual ~AmenitySymbolsProvider_P();

        ImplementationInterface<AmenitySymbolsProvider> owner;

        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric);

    friend class OsmAnd::AmenitySymbolsProvider;
    };
}

#endif // !defined(_OSMAND_CORE_AMENITY_SYMBOLS_PROVIDER_P_H_)
