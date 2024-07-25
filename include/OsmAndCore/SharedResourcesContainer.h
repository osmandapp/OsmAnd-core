#ifndef _OSMAND_CORE_SHARED_RESOURCES_CONTAINER_H_
#define _OSMAND_CORE_SHARED_RESOURCES_CONTAINER_H_

#include <OsmAndCore/stdlib_common.h>
#include <proper/future.h>

#include <OsmAndCore/QtExtensions.h>
#include <QHash>
#include <QReadWriteLock>
#include <QThread>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Logging.h>
#include <OsmAndCore/Utilities.h>

//#define OSMAND_LOG_SHARED_RESOURCES_CONTAINER_CHANGE 1
#ifndef OSMAND_LOG_SHARED_RESOURCES_CONTAINER_CHANGE
#   define OSMAND_LOG_SHARED_RESOURCES_CONTAINER_CHANGE 0
#endif // !defined(OSMAND_LOG_SHARED_RESOURCES_CONTAINER_CHANGE)

namespace OsmAnd
{
    template<typename KEY_TYPE, typename RESOURCE_TYPE>
    class SharedResourcesContainer
    {
        Q_DISABLE_COPY_AND_MOVE(SharedResourcesContainer);

    public:
        typedef std::shared_ptr<RESOURCE_TYPE> ResourcePtr;
    protected:
        mutable QMutex _containerMutex;

        struct AvailableResourceEntry
        {
            AvailableResourceEntry(const uintmax_t refCounter_, const ResourcePtr& resourcePtr_)
                : refCounter(refCounter_)
                , resourcePtr(resourcePtr_)
            {
            }

#ifdef Q_COMPILER_RVALUE_REFS
            AvailableResourceEntry(const uintmax_t refCounter_, ResourcePtr&& resourcePtr_)
                : refCounter(refCounter_)
                , resourcePtr(resourcePtr_)
            {
            }
#endif // Q_COMPILER_RVALUE_REFS

            virtual ~AvailableResourceEntry()
            {
            }

            uintmax_t refCounter;
            const ResourcePtr resourcePtr;

        private:
            Q_DISABLE_COPY_AND_MOVE(AvailableResourceEntry);
        };

        struct PromisedResourceEntry
        {
            PromisedResourceEntry()
                : refCounter(0)
#ifdef Q_COMPILER_RVALUE_REFS
                , sharedFuture(qMove(promise.get_future()))
#else
                , sharedFuture(promise.get_future().share())
#endif
            {
            }

            virtual ~PromisedResourceEntry()
            {
            }

            uintmax_t refCounter;
            proper::promise<ResourcePtr> promise;
            const proper::shared_future<ResourcePtr> sharedFuture;

        private:
            Q_DISABLE_COPY_AND_MOVE(PromisedResourceEntry);
        };
    private:
        QHash< KEY_TYPE, std::shared_ptr< AvailableResourceEntry > > _availableResources;
        QHash< KEY_TYPE, std::shared_ptr< PromisedResourceEntry > > _promisedResources;
    protected:
    public:
        SharedResourcesContainer()
            : _containerMutex(QMutex::Recursive)
        {
        }
        virtual ~SharedResourcesContainer()
        {
        }

        void insert(const KEY_TYPE& key, ResourcePtr& resourcePtr)
        {
            QMutexLocker scopedLocker(&_containerMutex);

#if OSMAND_LOG_SHARED_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedResourcesContainer(%p)->insert(%s, %p)",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)),
                resourcePtr.get());
#endif

            // Resource must not be promised and must not be already available.
            // Otherwise behavior is undefined
            assert(!_promisedResources.contains(key));
            assert(!_availableResources.contains(key));

            const auto newEntry = new AvailableResourceEntry(0, qMove(resourcePtr));
#ifndef Q_COMPILER_RVALUE_REFS
            resourcePtr.reset();
#else
            assert(resourcePtr.use_count() == 0);
#endif
            _availableResources.insert(key, qMove(std::shared_ptr<AvailableResourceEntry>(newEntry)));
        }

#ifdef Q_COMPILER_RVALUE_REFS
        void insert(const KEY_TYPE& key, ResourcePtr&& resourcePtr)
        {
            QMutexLocker scopedLocker(&_containerMutex);

#if OSMAND_LOG_SHARED_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedResourcesContainer(%p)->insert(%s, %p)",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)),
                resourcePtr.get());
#endif

            // Resource must not be promised and must not be already available.
            // Otherwise behavior is undefined
            assert(!_promisedResources.contains(key));
            assert(!_availableResources.contains(key));

            const auto newEntry = new AvailableResourceEntry(0, qMove(resourcePtr));
            assert(resourcePtr.use_count() == 0);
            _availableResources.insert(key, qMove(std::shared_ptr<AvailableResourceEntry>(newEntry)));
        }
#endif // Q_COMPILER_RVALUE_REFS
        
        void insertAndReference(const KEY_TYPE& key, const ResourcePtr& resourcePtr)
        {
            QMutexLocker scopedLocker(&_containerMutex);

#if OSMAND_LOG_SHARED_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedResourcesContainer(%p)->insertAndReference(%s, %p)",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)),
                resourcePtr.get());
#endif

            // Resource must not be promised and must not be already available.
            // Otherwise behavior is undefined
            assert(!_promisedResources.contains(key));
            assert(!_availableResources.contains(key));

            const auto newEntry = new AvailableResourceEntry(1, resourcePtr);
            _availableResources.insert(key, qMove(std::shared_ptr<AvailableResourceEntry>(newEntry)));
        }

        bool obtainReference(const KEY_TYPE& key, ResourcePtr& outResourcePtr)
        {
            QMutexLocker scopedLocker(&_containerMutex);

#if OSMAND_LOG_SHARED_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedResourcesContainer(%p)->obtainReference(%s)",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)));
#endif

            // In case resource was promised, wait forever until promise is fulfilled
            const auto itPromisedResourceEntry = _promisedResources.constFind(key);
            if (itPromisedResourceEntry != _promisedResources.cend())
            {
                const auto localFuture = (*itPromisedResourceEntry)->sharedFuture;
                scopedLocker.unlock();

                try
                {
                    outResourcePtr = localFuture.get();
                    return true;
                }
                catch(...)
                {
                }
                
                return false;
            }

            const auto itAvailableResourceEntry = _availableResources.find(key);
            if (itAvailableResourceEntry == _availableResources.end())
                return false;
            const auto& availableResourceEntry = *itAvailableResourceEntry;

            availableResourceEntry->refCounter++;
            outResourcePtr = availableResourceEntry->resourcePtr;

            return true;
        }

        bool releaseReference(const KEY_TYPE& key, ResourcePtr& resourcePtr, const bool autoClean = true, bool* outWasCleaned = nullptr, uintmax_t* outRemainingReferences = nullptr)
        {
            QMutexLocker scopedLocker(&_containerMutex);

            // Resource must not be promised. Otherwise behavior is undefined
            assert(!_promisedResources.contains(key));
            
            const auto itAvailableResourceEntry = _availableResources.find(key);
            if (itAvailableResourceEntry == _availableResources.end())
                return false;
            const auto& availableResourceEntry = *itAvailableResourceEntry;
            assert(availableResourceEntry->refCounter > 0);
            assert(availableResourceEntry->resourcePtr == resourcePtr);

#if OSMAND_LOG_SHARED_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedResourcesContainer(%p)->releaseReference(%s, %p): %" PRIu64 " -> %" PRIu64 "",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)),
                resourcePtr.get(),
                static_cast<uint64_t>(availableResourceEntry->refCounter),
                static_cast<uint64_t>(availableResourceEntry->refCounter) - 1);
#endif

            availableResourceEntry->refCounter--;
            if (outRemainingReferences)
                *outRemainingReferences = availableResourceEntry->refCounter;
            if (autoClean && outWasCleaned)
                *outWasCleaned = false;
            if (autoClean && availableResourceEntry->refCounter == 0)
            {
                _availableResources.erase(itAvailableResourceEntry);

                if (outWasCleaned)
                    *outWasCleaned = true;
            }
            resourcePtr.reset();

            return true;
        }

        void makePromise(const KEY_TYPE& key)
        {
            QMutexLocker scopedLocker(&_containerMutex);

#if OSMAND_LOG_SHARED_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedResourcesContainer(%p)->makePromise(%s)",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)));
#endif

            // Resource must not be promised and must not be already available.
            // Otherwise behavior is undefined
            assert(!_promisedResources.contains(key));
            assert(!_availableResources.contains(key));

            const auto newEntry = new PromisedResourceEntry();
            _promisedResources.insert(key, qMove(std::shared_ptr<PromisedResourceEntry>(newEntry)));
        }

        void breakPromise(const KEY_TYPE& key)
        {
            QMutexLocker scopedLocker(&_containerMutex);

#if OSMAND_LOG_SHARED_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedResourcesContainer(%p)->breakPromise(%s)",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)));
#endif

            // Resource must be promised and must not be already available.
            // Otherwise behavior is undefined
            assert(_promisedResources.contains(key));
            assert(!_availableResources.contains(key));

            const auto itPromisedResourceEntry = _promisedResources.find(key);
            const std::shared_ptr<PromisedResourceEntry> promisedResourceEntry
#ifdef Q_COMPILER_RVALUE_REFS
                (qMove(*itPromisedResourceEntry))
#else
                = itPromisedResourceEntry
#endif // Q_COMPILER_RVALUE_REFS
            ;
            _promisedResources.erase(itPromisedResourceEntry);

            promisedResourceEntry->promise.set_exception(proper::make_exception_ptr(std::runtime_error("Promise was broken")));
        }

        void fulfilPromise(const KEY_TYPE& key, ResourcePtr& resourcePtr)
        {
            QMutexLocker scopedLocker(&_containerMutex);

            // Resource must be promised and must not be already available.
            // Otherwise behavior is undefined
            assert(_promisedResources.contains(key));
            assert(!_availableResources.contains(key));

            const auto itPromisedResourceEntry = _promisedResources.find(key);
            const std::shared_ptr<PromisedResourceEntry> promisedResourceEntry
#ifdef Q_COMPILER_RVALUE_REFS
                (qMove(*itPromisedResourceEntry))
#else
                = itPromisedResourceEntry
#endif // Q_COMPILER_RVALUE_REFS
            ;
            _promisedResources.erase(itPromisedResourceEntry);

#if OSMAND_LOG_SHARED_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedResourcesContainer(%p)->fulfilPromise(%s, %p): %" PRIu64 "",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)),
                resourcePtr.get(),
                static_cast<uint64_t>(promisedResourceEntry->refCounter));
#endif

            if (promisedResourceEntry->refCounter <= 0)
                return;

            const auto newEntry = new AvailableResourceEntry(promisedResourceEntry->refCounter, qMove(resourcePtr));
#ifndef Q_COMPILER_RVALUE_REFS
            resourcePtr.reset();
#else
            assert(resourcePtr.use_count() == 0);
#endif
            _availableResources.insert(key, qMove(std::shared_ptr<AvailableResourceEntry>(newEntry)));
            promisedResourceEntry->promise.set_value(newEntry->resourcePtr);
        }

#ifdef Q_COMPILER_RVALUE_REFS
        void fulfilPromise(const KEY_TYPE& key, ResourcePtr&& resourcePtr)
        {
            QMutexLocker scopedLocker(&_containerMutex);

            // Resource must be promised and must not be already available.
            // Otherwise behavior is undefined
            assert(_promisedResources.contains(key));
            assert(!_availableResources.contains(key));

            const auto itPromisedResourceEntry = _promisedResources.find(key);
            const std::shared_ptr<PromisedResourceEntry> promisedResourceEntry(qMove(*itPromisedResourceEntry));
            _promisedResources.erase(itPromisedResourceEntry);

#if OSMAND_LOG_SHARED_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedResourcesContainer(%p)->fulfilPromise(%s, %p): %" PRIu64 "",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)),
                resourcePtr.get(),
                static_cast<uint64_t>(promisedResourceEntry->refCounter));
#endif

            if (promisedResourceEntry->refCounter <= 0)
                return;

            const auto newEntry = new AvailableResourceEntry(promisedResourceEntry->refCounter, qMove(resourcePtr));
            assert(resourcePtr.use_count() == 0);
            _availableResources.insert(key, qMove(std::shared_ptr<AvailableResourceEntry>(newEntry)));
            promisedResourceEntry->promise.set_value(newEntry->resourcePtr);
        }
#endif // Q_COMPILER_RVALUE_REFS

        void fulfilPromiseAndReference(const KEY_TYPE& key, const ResourcePtr& resourcePtr)
        {
            QMutexLocker scopedLocker(&_containerMutex);

            // Resource must be promised and must not be already available.
            // Otherwise behavior is undefined
            assert(_promisedResources.contains(key));
            assert(!_availableResources.contains(key));

            const auto itPromisedResourceEntry = _promisedResources.find(key);
            const std::shared_ptr<PromisedResourceEntry> promisedResourceEntry
#ifdef Q_COMPILER_RVALUE_REFS
                (qMove(*itPromisedResourceEntry))
#else
                = itPromisedResourceEntry
#endif // Q_COMPILER_RVALUE_REFS
            ;
            _promisedResources.erase(itPromisedResourceEntry);

#if OSMAND_LOG_SHARED_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedResourcesContainer(%p)->fulfilPromiseAndReference(%s, %p): %" PRIu64 " -> %" PRIu64 "",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)),
                resourcePtr.get(),
                static_cast<uint64_t>(promisedResourceEntry->refCounter),
                static_cast<uint64_t>(promisedResourceEntry->refCounter) + 1);
#endif

            const auto newEntry = new AvailableResourceEntry(promisedResourceEntry->refCounter + 1, resourcePtr);
            _availableResources.insert(key, qMove(std::shared_ptr<AvailableResourceEntry>(newEntry)));
            promisedResourceEntry->promise.set_value(newEntry->resourcePtr);
        }

        bool obtainFutureReference(const KEY_TYPE& key, proper::shared_future<ResourcePtr>& outFutureResourcePtr)
        {
            QMutexLocker scopedLocker(&_containerMutex);

            // Resource must not be already available.
            // Otherwise behavior is undefined
            assert(!_availableResources.contains(key));

            const auto itPromisedResourceEntry = _promisedResources.constFind(key);
            if (itPromisedResourceEntry == _promisedResources.cend())
            {
#if OSMAND_LOG_SHARED_RESOURCES_CONTAINER_CHANGE
                LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedResourcesContainer(%p)->obtainFutureReference(%s)",
                    QThread::currentThreadId(),
                    this,
                    qPrintable(QString::fromLatin1("%1").arg(key)));
#endif

                return false;
            }
            const auto& promisedResourceEntry = *itPromisedResourceEntry;

#if OSMAND_LOG_SHARED_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedResourcesContainer(%p)->obtainFutureReference(%s): %" PRIu64 " -> %" PRIu64 "",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)),
                static_cast<uint64_t>(promisedResourceEntry->refCounter),
                static_cast<uint64_t>(promisedResourceEntry->refCounter) + 1);
#endif

            promisedResourceEntry->refCounter++;
            outFutureResourcePtr = promisedResourceEntry->sharedFuture;

            return true;
        }

        bool releaseFutureReference(const KEY_TYPE& key)
        {
            QMutexLocker scopedLocker(&_containerMutex);

            // Resource must not be already available.
            // Otherwise behavior is undefined
            assert(!_availableResources.contains(key));

            const auto itPromisedResourceEntry = _promisedResources.constFind(key);
            if (itPromisedResourceEntry == _promisedResources.cend())
            {
#if OSMAND_LOG_SHARED_RESOURCES_CONTAINER_CHANGE
                LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedResourcesContainer(%p)->releaseFutureReference(%s)",
                    QThread::currentThreadId(),
                    this,
                    qPrintable(QString::fromLatin1("%1").arg(key)));
#endif

                return false;
            }
            const auto& promisedResourceEntry = *itPromisedResourceEntry;
            assert(promisedResourceEntry->refCounter > 0);

#if OSMAND_LOG_SHARED_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedResourcesContainer(%p)->releaseFutureReference(%s): %" PRIu64 " -> %" PRIu64 "",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)),
                static_cast<uint64_t>(promisedResourceEntry->refCounter),
                static_cast<uint64_t>(promisedResourceEntry->refCounter) - 1);
#endif
            
            promisedResourceEntry->refCounter--;

            return true;
        }

        bool obtainReferenceOrFutureReferenceOrMakePromise(const KEY_TYPE& key, ResourcePtr& outResourcePtr, proper::shared_future<ResourcePtr>& outFutureResourcePtr, bool withPromise = true)
        {
            QMutexLocker scopedLocker(&_containerMutex);

            const auto itAvailableResourceEntry = _availableResources.find(key);
            if (itAvailableResourceEntry != _availableResources.end())
            {
                const auto& availableResourceEntry = *itAvailableResourceEntry;

#if OSMAND_LOG_SHARED_RESOURCES_CONTAINER_CHANGE
                LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedResourcesContainer(%p)->obtainReferenceOrFutureReferenceOrMakePromise(%s): reference %" PRIu64 " -> %" PRIu64 "",
                    QThread::currentThreadId(),
                    this,
                    qPrintable(QString::fromLatin1("%1").arg(key)),
                    static_cast<uint64_t>(availableResourceEntry->refCounter),
                    static_cast<uint64_t>(availableResourceEntry->refCounter) + 1);
#endif

                availableResourceEntry->refCounter++;
                outResourcePtr = availableResourceEntry->resourcePtr;

                return true;
            }
            
            const auto futureReferenceAvailable = obtainFutureReference(key, outFutureResourcePtr);
            if (futureReferenceAvailable)
            {
#if OSMAND_LOG_SHARED_RESOURCES_CONTAINER_CHANGE
                LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedResourcesContainer(%p)->obtainReferenceOrFutureReferenceOrMakePromise(%s): future reference",
                    QThread::currentThreadId(),
                    this,
                    qPrintable(QString::fromLatin1("%1").arg(key)));
#endif

                return true;
            }
            
#if OSMAND_LOG_SHARED_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedResourcesContainer(%p)->obtainReferenceOrFutureReferenceOrMakePromise(%s): promise",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)));
#endif

            if (withPromise)
                makePromise(key);
            return false;
        }

        uintmax_t getReferencesCount(const KEY_TYPE& key) const
        {
            QMutexLocker scopedLocker(&_containerMutex);

            const auto citAvailableResourceEntry = _availableResources.constFind(key);
            if (citAvailableResourceEntry != _availableResources.cend())
            {
                const auto& availableResourceEntry = *citAvailableResourceEntry;

                return availableResourceEntry->refCounter;
            }

            const auto citPromisedResourceEntry = _promisedResources.constFind(key);
            if (citPromisedResourceEntry != _promisedResources.cend())
            {
                const auto& promisedResourceEntry = *citPromisedResourceEntry;

                return promisedResourceEntry->refCounter;
            }

            return 0;
        }
    };
}

#endif // !defined(_OSMAND_CORE_SHARED_RESOURCES_CONTAINER_H_)
