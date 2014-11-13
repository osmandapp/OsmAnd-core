#ifndef _OSMAND_CORE_I_MAP_KEYED_DATA_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_KEYED_DATA_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Metrics.h>
#include <OsmAndCore/Map/IMapDataProvider.h>

namespace OsmAnd
{
    class IQueryController;

    class OSMAND_CORE_API IMapKeyedDataProvider : public IMapDataProvider
    {
        Q_DISABLE_COPY_AND_MOVE(IMapKeyedDataProvider);

    public:
        typedef const void* Key;

        class OSMAND_CORE_API Data : public IMapDataProvider::Data
        {
            Q_DISABLE_COPY_AND_MOVE(Data);
        private:
        protected:
        public:
            Data(const Key key, const RetainableCacheMetadata* const pRetainableCacheMetadata = nullptr);
            virtual ~Data();

            Key key;
        };

    private:
    protected:
        IMapKeyedDataProvider();
    public:
        virtual ~IMapKeyedDataProvider();

        virtual QList<Key> getProvidedDataKeys() const = 0;
        virtual bool obtainData(
            const Key key,
            std::shared_ptr<Data>& outKeyedData,
            std::shared_ptr<Metric>* pOutMetric = nullptr,
            const IQueryController* const queryController = nullptr) = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_KEYED_DATA_PROVIDER_H_)
