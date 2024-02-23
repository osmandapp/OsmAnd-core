#ifndef _OSMAND_CORE_MAP_DATA_PROVIDER_HELPERS_H_
#define _OSMAND_CORE_MAP_DATA_PROVIDER_HELPERS_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Map/IMapDataProvider.h>

namespace OsmAnd
{
    class OSMAND_CORE_API MapDataProviderHelpers Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(MapDataProviderHelpers);

        MapDataProviderHelpers();
        ~MapDataProviderHelpers();

    public:
        template<class T>
        static bool obtainData(
            IMapDataProvider* const provider,
            const IMapDataProvider::Request& request,
            std::shared_ptr<T>& outData,
            std::shared_ptr<Metric>* const pOutMetric)
        {
            std::shared_ptr<IMapDataProvider::Data> data;
            const auto result = provider->obtainData(request, data, pOutMetric);
            outData = std::static_pointer_cast<T>(data);
            return result;
        }

        template<class T>
        static const T& castRequest(const IMapDataProvider::Request& request)
        {
            return *dynamic_cast<const T*>(&request);
        }

        template<class T>
        static T& castRequest(IMapDataProvider::Request& request)
        {
            return *dynamic_cast<T*>(&request);
        }

        static bool nonNaturalObtainData(
            IMapDataProvider* const provider,
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr);

        static void nonNaturalObtainDataAsync(
            const std::shared_ptr<IMapDataProvider>& provider,
            const IMapDataProvider::Request& request,
            const IMapDataProvider::ObtainDataAsyncCallback callback,
            const bool collectMetric = false);
    };
}

#endif // !defined(_OSMAND_CORE_MAP_DATA_PROVIDER_HELPERS_H_)
