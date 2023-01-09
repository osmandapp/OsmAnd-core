#include "MapDataProviderHelpers.h"

#include "stdlib_common.h"
#include <cassert>

#include "QtExtensions.h"
#include <QMutex>
#include <QWaitCondition>
#include <QThreadPool>

#include "OsmAndCore.h"
#include "QRunnableFunctor.h"

OsmAnd::MapDataProviderHelpers::MapDataProviderHelpers()
{
}

OsmAnd::MapDataProviderHelpers::~MapDataProviderHelpers()
{
}

bool OsmAnd::MapDataProviderHelpers::nonNaturalObtainData(
    IMapDataProvider* const provider,
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    assert(provider->supportsNaturalObtainDataAsync());

    QMutex mutex;
    QWaitCondition callbackCalled;
    bool requestSucceeded = false;
    std::shared_ptr<IMapDataProvider::Data> data;
    std::shared_ptr<Metric> metric;
    const IMapDataProvider::ObtainDataAsyncCallback callback =
        [&mutex, &requestSucceeded, &data, &metric, &callbackCalled]
        (const IMapDataProvider* const provider_,
            const bool requestSucceeded_,
            const std::shared_ptr<IMapDataProvider::Data>& data_,
            const std::shared_ptr<Metric>& metric_)
        {
            QMutexLocker scopedLocker(&mutex);

            requestSucceeded = requestSucceeded_;
            data = data_;
            metric = metric_;

            callbackCalled.wakeOne();
        };

    QMutexLocker scopedLocker(&mutex);
    provider->obtainDataAsync(request, callback, pOutMetric != nullptr);
    REPEAT_UNTIL(callbackCalled.wait(&mutex));

    outData = data;
    if (pOutMetric)
        *pOutMetric = metric;

    return requestSucceeded;
}

void OsmAnd::MapDataProviderHelpers::nonNaturalObtainDataAsync(
    const std::shared_ptr<IMapDataProvider>& provider,
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    assert(provider->supportsNaturalObtainData());

    const auto weakProvider = std::weak_ptr<IMapDataProvider>(provider);
    const auto requestClone = request.clone();
    const QRunnableFunctor::Callback task =
        [weakProvider, requestClone, callback, collectMetric]
        (const QRunnableFunctor* const runnable)
        {
            const auto provider = weakProvider.lock();
            if (provider)
            {
                std::shared_ptr<IMapDataProvider::Data> data;
                std::shared_ptr<Metric> metric;
                const bool requestSucceeded =
                    provider->obtainData(*requestClone, data, collectMetric ? &metric : nullptr);
                callback(provider.get(), requestSucceeded, data, metric);
            }
        };

    const auto taskRunnable = new QRunnableFunctor(task);
    taskRunnable->setAutoDelete(true);
    QThreadPool::globalInstance()->start(taskRunnable);
}
