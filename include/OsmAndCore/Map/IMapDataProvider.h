#ifndef _OSMAND_CORE_I_MAP_DATA_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_DATA_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/Metrics.h>

namespace OsmAnd
{
    class IQueryController;

    class OSMAND_CORE_API IMapDataProvider
    {
        Q_DISABLE_COPY_AND_MOVE(IMapDataProvider);

    public:
        struct OSMAND_CORE_API RetainableCacheMetadata
        {
            virtual ~RetainableCacheMetadata() = 0;
        };

        class OSMAND_CORE_API Data
        {
            Q_DISABLE_COPY_AND_MOVE(Data);

        private:
        protected:
            void release();
        public:
            Data(const RetainableCacheMetadata* const pRetainableCacheMetadata = nullptr);
            virtual ~Data();

            // If data provider supports caching, it may need to store some metadata to maintain cache
            std::shared_ptr<const RetainableCacheMetadata> retainableCacheMetadata;
        };

        struct OSMAND_CORE_API Request
        {
            Request();
            virtual ~Request();

            std::shared_ptr<const IQueryController> queryController;

            virtual std::shared_ptr<Request> clone() const;
            static void copy(Request& dst, const Request& src);

        protected:
            Request(const Request& that);
        };

        OSMAND_CALLABLE(ObtainDataAsyncCallback,
            void,
            const IMapDataProvider* provider,
            bool requestSucceeded,
            const std::shared_ptr<Data>& data,
            const std::shared_ptr<Metric>& metric);

    private:
    protected:
        IMapDataProvider();
    public:
        virtual ~IMapDataProvider();

        virtual bool supportsNaturalObtainData() const = 0;
        virtual bool obtainData(
            const Request& request,
            std::shared_ptr<Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr) = 0;

        virtual bool supportsNaturalObtainDataAsync() const = 0;
        virtual void obtainDataAsync(
            const Request& request,
            const ObtainDataAsyncCallback callback,
            const bool collectMetric = false) = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_DATA_PROVIDER_H_)
