#ifndef _OSMAND_CORE_SHARED_BY_ZOOM_RESOURCES_CONTAINER_H_
#define _OSMAND_CORE_SHARED_BY_ZOOM_RESOURCES_CONTAINER_H_

#include <OsmAndCore/stdlib_common.h>
#include <array>

#include <OsmAndCore/QtExtensions.h>
#include <QHash>
#include <QSet>
#include <QReadWriteLock>
#include <QThread>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/SharedResourcesContainer.h>
#include <OsmAndCore/Logging.h>
#include <OsmAndCore/Utilities.h>

//#define OSMAND_LOG_SHARED_BY_ZOOM_RESOURCES_CONTAINER_CHANGE 1
#ifndef OSMAND_LOG_SHARED_BY_ZOOM_RESOURCES_CONTAINER_CHANGE
#   define OSMAND_LOG_SHARED_BY_ZOOM_RESOURCES_CONTAINER_CHANGE 0
#endif // !defined(OSMAND_LOG_SHARED_BY_ZOOM_RESOURCES_CONTAINER_CHANGE)

namespace OsmAnd
{
    // SharedByZoomResourcesContainer is similar to SharedResourcesContainer,
    // but also allows resources to be shared between multiple zoom levels.
    template<typename KEY_TYPE, typename RESOURCE_TYPE>
    class SharedByZoomResourcesContainer : protected SharedResourcesContainer<KEY_TYPE, RESOURCE_TYPE>
    {
        Q_DISABLE_COPY_AND_MOVE(SharedByZoomResourcesContainer);
    public:
        typedef typename SharedResourcesContainer<KEY_TYPE, RESOURCE_TYPE>::ResourcePtr ResourcePtr;
    protected:
        struct AvailableResourceEntry : public SharedResourcesContainer<KEY_TYPE, RESOURCE_TYPE>::AvailableResourceEntry
        {
            typedef typename SharedResourcesContainer<KEY_TYPE, RESOURCE_TYPE>::AvailableResourceEntry base;

            AvailableResourceEntry(const uintmax_t refCounter_, const ResourcePtr& resourcePtr_,
                const QSet<ZoomLevel>& zoomLevels_)
                : base(refCounter_, resourcePtr_)
                , zoomLevels(zoomLevels_)
            {
            }

#ifdef Q_COMPILER_RVALUE_REFS
            AvailableResourceEntry(const uintmax_t refCounter_, ResourcePtr&& resourcePtr_,
                const QSet<ZoomLevel>& zoomLevels_)
                : base(refCounter_, resourcePtr_)
                , zoomLevels(zoomLevels_)
            {
            }
#endif // Q_COMPILER_RVALUE_REFS

            const QSet<ZoomLevel> zoomLevels;

        private:
            Q_DISABLE_COPY_AND_MOVE(AvailableResourceEntry);
        };

        struct PromisedResourceEntry : public SharedResourcesContainer<KEY_TYPE, RESOURCE_TYPE>::PromisedResourceEntry
        {
            typedef typename SharedResourcesContainer<KEY_TYPE, RESOURCE_TYPE>::PromisedResourceEntry base;

            PromisedResourceEntry(const QSet<ZoomLevel>& zoomLevels_)
                : base()
                , zoomLevels(zoomLevels_)
            {
            }

            const QSet<ZoomLevel> zoomLevels;

        private:
            Q_DISABLE_COPY_AND_MOVE(PromisedResourceEntry);
        };
    private:
        typedef SharedResourcesContainer<KEY_TYPE, RESOURCE_TYPE> base;

        typedef std::shared_ptr<AvailableResourceEntry> AvailableResourceEntryPtr;
        QHash< ZoomLevel, QSet< AvailableResourceEntryPtr > > _availableResourceEntriesStorage;
        std::array< QHash< KEY_TYPE, AvailableResourceEntryPtr >, ZoomLevelsCount> _availableResources;

        typedef std::shared_ptr<PromisedResourceEntry> PromisedResourceEntryPtr;
        QHash< ZoomLevel, QHash< KEY_TYPE, PromisedResourceEntryPtr > > _promisedResourceEntriesStorage;
        std::array< QHash< KEY_TYPE, PromisedResourceEntryPtr >, ZoomLevelsCount> _promisedResources;

        const bool _useSeparateZoomLevels;
    protected:
    public:
        SharedByZoomResourcesContainer(bool useSeparateZoomLevels = false)
        : _useSeparateZoomLevels(useSeparateZoomLevels)
        {
        }
        virtual ~SharedByZoomResourcesContainer()
        {
        }

        void insert(const KEY_TYPE& key, const QSet<ZoomLevel>& levels, ResourcePtr& resourcePtr,
            const ZoomLevel zoomLevel = ZoomLevel0)
        {
            QMutexLocker scopedLocker(&this->_containerMutex);

#if OSMAND_LOG_SHARED_BY_ZOOM_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedByZoomResourcesContainer(%p)->insert(%s, [%s], %p)",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)),
                qPrintable(Utilities::stringifyZoomLevels(levels)),
                resourcePtr.get());
#endif

            const AvailableResourceEntryPtr newEntryPtr(new AvailableResourceEntry(0, qMove(resourcePtr), levels));
#ifndef Q_COMPILER_RVALUE_REFS
            resourcePtr.reset();
#else
            assert(resourcePtr.use_count() == 0);
#endif

            for(const auto& level : constOf(levels))
            {
                // Resource must not be promised and must not be already available.
                // Otherwise behavior is undefined
                assert(!_promisedResources[level].contains(key));
                assert(!_availableResources[level].contains(key));

                _availableResources[level].insert(key, newEntryPtr);
            }
            
            _availableResourceEntriesStorage[zoomLevel].insert(qMove(newEntryPtr));
        }

#ifdef Q_COMPILER_RVALUE_REFS
        void insert(const KEY_TYPE& key, const QSet<ZoomLevel>& levels, ResourcePtr&& resourcePtr,
            const ZoomLevel zoomLevel = ZoomLevel0)
        {
            QMutexLocker scopedLocker(&this->_containerMutex);

#if OSMAND_LOG_SHARED_BY_ZOOM_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedByZoomResourcesContainer(%p)->insert(%s, [%s], %p)",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)),
                qPrintable(Utilities::stringifyZoomLevels(levels)),
                resourcePtr.get());
#endif

            const AvailableResourceEntryPtr newEntryPtr(new AvailableResourceEntry(0, qMove(resourcePtr), levels));
            assert(resourcePtr.use_count() == 0);

            for(const auto& level : constOf(levels))
            {
                // Resource must not be promised and must not be already available.
                // Otherwise behavior is undefined
                assert(!_promisedResources[level].contains(key));
                assert(!_availableResources[level].contains(key));

                _availableResources[level].insert(key, newEntryPtr);
            }

            _availableResourceEntriesStorage[zoomLevel].insert(qMove(newEntryPtr));
        }
#endif // Q_COMPILER_RVALUE_REFS

        void insertAndReference(const KEY_TYPE& key, const QSet<ZoomLevel>& levels, const ResourcePtr& resourcePtr,
            const ZoomLevel zoomLevel = ZoomLevel0)
        {
            QMutexLocker scopedLocker(&this->_containerMutex);

#if OSMAND_LOG_SHARED_BY_ZOOM_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedByZoomResourcesContainer(%p)->insertAndReference(%s, [%s], %p)",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)),
                qPrintable(Utilities::stringifyZoomLevels(levels)),
                resourcePtr.get());
#endif

            const AvailableResourceEntryPtr newEntryPtr(new AvailableResourceEntry(1, resourcePtr, levels));

            for(const auto& level : constOf(levels))
            {
                // Resource must not be promised and must not be already available.
                // Otherwise behavior is undefined
                assert(!_promisedResources[level].contains(key));
                assert(!_availableResources[level].contains(key));

                _availableResources[level].insert(key, newEntryPtr);
            }

            _availableResourceEntriesStorage[zoomLevel].insert(qMove(newEntryPtr));
        }

        bool obtainReference(const KEY_TYPE& key, const ZoomLevel level, ResourcePtr& outResourcePtr)
        {
            QMutexLocker scopedLocker(&this->_containerMutex);

#if OSMAND_LOG_SHARED_BY_ZOOM_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedByZoomResourcesContainer(%p)->obtainReference(%s, [%d],...)",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)),
                level);
#endif

            // In case resource was promised, wait forever until promise is fulfilled
            const auto& promisedResources = _promisedResources[level];
            const auto& itPromisedResourceEntry = promisedResources.constFind(key);
            if (itPromisedResourceEntry != promisedResources.cend())
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

            auto& availableResources = _availableResources[level];
            const auto& itAvailableResourceEntry = availableResources.find(key);
            if (itAvailableResourceEntry == availableResources.end())
                return false;
            const auto& availableResourceEntry = *itAvailableResourceEntry;

            availableResourceEntry->refCounter++;
            outResourcePtr = availableResourceEntry->resourcePtr;

            return true;
        }

        bool releaseReference(const KEY_TYPE& key, const ZoomLevel level, ResourcePtr& resourcePtr,
            const bool autoClean = true, bool* outWasCleaned = nullptr, uintmax_t* outRemainingReferences = nullptr)
        {
            QMutexLocker scopedLocker(&this->_containerMutex);

#if OSMAND_LOG_SHARED_BY_ZOOM_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedByZoomResourcesContainer(%p)->releaseReference(%s, [%d], %p, ...)",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)),
                level,
                resourcePtr.get());
#endif

            // Resource must not be promised. Otherwise behavior is undefined
            assert(!_promisedResources[level].contains(key));

            auto& availableResources = _availableResources[level];
            const auto& itAvailableResourceEntry = availableResources.find(key);
            if (itAvailableResourceEntry == availableResources.end())
                return false;
            const auto& availableResourceEntry = *itAvailableResourceEntry;
            assert(availableResourceEntry->refCounter > 0);
            assert(availableResourceEntry->resourcePtr == resourcePtr);

            availableResourceEntry->refCounter--;
            if (outRemainingReferences)
                *outRemainingReferences = availableResourceEntry->refCounter;
            if (autoClean && outWasCleaned)
                *outWasCleaned = false;
            if (autoClean && availableResourceEntry->refCounter == 0)
            {
                for(const auto& otherLevel : constOf(availableResourceEntry->zoomLevels))
                {
                    if (otherLevel == level)
                        continue;

                    const auto removedCount = _availableResources[otherLevel].remove(key);
                    assert(removedCount == 1);
                }

                _availableResourceEntriesStorage[_useSeparateZoomLevels ? level : ZoomLevel0].remove(
                    availableResourceEntry);
                availableResources.erase(itAvailableResourceEntry);

                if (outWasCleaned)
                    *outWasCleaned = true;
            }
            resourcePtr.reset();

            return true;
        }
        
        void makePromise(const KEY_TYPE& key, const QSet<ZoomLevel>& levels,
            const ZoomLevel zoomLevel = ZoomLevel0)
        {
            //QMutexLocker scopedLocker(&this->_containerMutex);

#if OSMAND_LOG_SHARED_BY_ZOOM_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedByZoomResourcesContainer(%p)->makePromise(%s, [%s])",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)),
                qPrintable(Utilities::stringifyZoomLevels(levels)));
#endif

            const PromisedResourceEntryPtr newEntryPtr(new PromisedResourceEntry(levels));

            for(const auto& level : constOf(levels))
            {
                // Resource must not be promised and must not be already available.
                // Otherwise behavior is undefined
                assert(!_promisedResources[level].contains(key));
                assert(!_availableResources[level].contains(key));

                _promisedResources[level].insert(key, newEntryPtr);
            }

            _promisedResourceEntriesStorage[zoomLevel].insert(key, qMove(newEntryPtr));
        }

        void breakPromise(const KEY_TYPE& key,
            const ZoomLevel zoomLevel = ZoomLevel0)
        {
            QMutexLocker scopedLocker(&this->_containerMutex);

#if OSMAND_LOG_SHARED_BY_ZOOM_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedByZoomResourcesContainer(%p)->breakPromise(%s, [%s])",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)),
                qPrintable(Utilities::stringifyZoomLevels(levels)));
#endif

            const auto& itPromisedEntryPtr = _promisedResourceEntriesStorage[zoomLevel].find(key);
            if (itPromisedEntryPtr == _promisedResourceEntriesStorage[zoomLevel].end())
                return;
            const auto& levels = itPromisedEntryPtr.value()->zoomLevels;
            PromisedResourceEntryPtr promisedEntryPtr;
            for(const auto& level : constOf(levels))
            {
                // Resource must be promised and must not be already available.
                // Otherwise behavior is undefined
                assert(_promisedResources[level].contains(key));
                assert(!_availableResources[level].contains(key));

                auto& promisedResources = _promisedResources[level];
                const auto& itPromisedResourceEntry = promisedResources.find(key);
                if (!promisedEntryPtr)
                    promisedEntryPtr = *itPromisedResourceEntry;
                promisedResources.erase(itPromisedResourceEntry);
            }

            _promisedResourceEntriesStorage[zoomLevel].remove(key);
            promisedEntryPtr->promise.set_exception(proper::make_exception_ptr(std::runtime_error("Promise was broken")));
        }

        void fulfilPromise(const KEY_TYPE& key, const QSet<ZoomLevel>& levels, ResourcePtr& resourcePtr,
            const ZoomLevel zoomLevel = ZoomLevel0)
        {
            QMutexLocker scopedLocker(&this->_containerMutex);

#if OSMAND_LOG_SHARED_BY_ZOOM_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedByZoomResourcesContainer(%p)->fulfilPromise(%s, [%s], %p)",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)),
                qPrintable(Utilities::stringifyZoomLevels(levels)),
                resourcePtr.get());
#endif

            PromisedResourceEntryPtr promisedEntryPtr;
            for(const auto& level : constOf(levels))
            {
                // Resource must be promised and must not be already available.
                // Otherwise behavior is undefined
                assert(_promisedResources[level].contains(key));
                assert(!_availableResources[level].contains(key));

                auto& promisedResources = _promisedResources[level];
                const auto& itPromisedResourceEntry = promisedResources.find(key);
                if (!promisedEntryPtr)
                    promisedEntryPtr = *itPromisedResourceEntry;
                promisedResources.erase(itPromisedResourceEntry);
            }
            _promisedResourceEntriesStorage[zoomLevel].remove(key);

            if (promisedEntryPtr->refCounter <= 0)
                return;

            const AvailableResourceEntryPtr newEntryPtr(
                new AvailableResourceEntry(promisedEntryPtr->refCounter, qMove(resourcePtr), levels));
#ifndef Q_COMPILER_RVALUE_REFS
            resourcePtr.reset();
#else
            assert(resourcePtr.use_count() == 0);
#endif

            for(const auto& level : constOf(levels))
            {
                // Resource must not be promised and must not be already available.
                // Otherwise behavior is undefined
                assert(!_promisedResources[level].contains(key));
                assert(!_availableResources[level].contains(key));

                _availableResources[level].insert(key, newEntryPtr);
            }

            _availableResourceEntriesStorage[zoomLevel].insert(newEntryPtr);
            promisedEntryPtr->promise.set_value(newEntryPtr->resourcePtr);
        }

#ifdef Q_COMPILER_RVALUE_REFS
        void fulfilPromise(const KEY_TYPE& key, const QSet<ZoomLevel>& levels, ResourcePtr&& resourcePtr,
            const ZoomLevel zoomLevel = ZoomLevel0)
        {
            QMutexLocker scopedLocker(&this->_containerMutex);

#if OSMAND_LOG_SHARED_BY_ZOOM_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedByZoomResourcesContainer(%p)->fulfilPromise(%s, [%s], %p)",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)),
                qPrintable(Utilities::stringifyZoomLevels(levels)),
                resourcePtr.get());
#endif

            PromisedResourceEntryPtr promisedEntryPtr;
            for(const auto& level : constOf(levels))
            {
                // Resource must be promised and must not be already available.
                // Otherwise behavior is undefined
                assert(_promisedResources[level].contains(key));
                assert(!_availableResources[level].contains(key));

                auto& promisedResources = _promisedResources[level];
                const auto& itPromisedResourceEntry = promisedResources.find(key);
                if (!promisedEntryPtr)
                    promisedEntryPtr = *itPromisedResourceEntry;
                promisedResources.erase(itPromisedResourceEntry);
            }
            _promisedResourceEntriesStorage[zoomLevel].remove(key);

            if (promisedEntryPtr->refCounter <= 0)
                return;

            const AvailableResourceEntryPtr newEntryPtr(
                new AvailableResourceEntry(promisedEntryPtr->refCounter, qMove(resourcePtr), levels));
            assert(resourcePtr.use_count() == 0);

            for(const auto& level : constOf(levels))
            {
                // Resource must not be promised and must not be already available.
                // Otherwise behavior is undefined
                assert(!_promisedResources[level].contains(key));
                assert(!_availableResources[level].contains(key));

                _availableResources[level].insert(key, newEntryPtr);
            }

            _availableResourceEntriesStorage[zoomLevel].insert(newEntryPtr);
            promisedEntryPtr->promise.set_value(newEntryPtr->resourcePtr);
        }
#endif // Q_COMPILER_RVALUE_REFS

        void fulfilPromiseAndReference(const KEY_TYPE& key, const QSet<ZoomLevel>& levels,
            const ResourcePtr& resourcePtr, const ZoomLevel zoomLevel = ZoomLevel0)
        {
            QMutexLocker scopedLocker(&this->_containerMutex);

#if OSMAND_LOG_SHARED_BY_ZOOM_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedByZoomResourcesContainer(%p)->fulfilPromiseAndReference(%s, [%s], %p)",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)),
                qPrintable(Utilities::stringifyZoomLevels(levels)),
                resourcePtr.get());
#endif

            PromisedResourceEntryPtr promisedEntryPtr;
            for(const auto& level : constOf(levels))
            {
                // Resource must be promised and must not be already available.
                // Otherwise behavior is undefined
                assert(_promisedResources[level].contains(key));
                assert(!_availableResources[level].contains(key));

                auto& promisedResources = _promisedResources[level];
                const auto& itPromisedResourceEntry = promisedResources.find(key);
                if (!promisedEntryPtr)
                    promisedEntryPtr = *itPromisedResourceEntry;
                promisedResources.erase(itPromisedResourceEntry);
            }
            _promisedResourceEntriesStorage[zoomLevel].remove(key);

            const AvailableResourceEntryPtr newEntryPtr(
                new AvailableResourceEntry(promisedEntryPtr->refCounter + 1, resourcePtr, levels));

            for(const auto& level : constOf(levels))
            {
                // Resource must not be promised and must not be already available.
                // Otherwise behavior is undefined
                assert(!_promisedResources[level].contains(key));
                assert(!_availableResources[level].contains(key));

                _availableResources[level].insert(key, newEntryPtr);
            }

            _availableResourceEntriesStorage[zoomLevel].insert(newEntryPtr);
            promisedEntryPtr->promise.set_value(newEntryPtr->resourcePtr);
        }

        bool obtainFutureReference(const KEY_TYPE& key, const ZoomLevel level,
            proper::shared_future<ResourcePtr>& outFutureResourcePtr)
        {
            //QMutexLocker scopedLocker(&this->_containerMutex);

#if OSMAND_LOG_SHARED_BY_ZOOM_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedByZoomResourcesContainer(%p)->obtainFutureReference(%s, [%d], ...)",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)),
                level);
#endif

            // Resource must not be already available.
            // Otherwise behavior is undefined
            assert(!_availableResources[level].contains(key));

            const auto& promisedResources = _promisedResources[level];
            const auto& itPromisedResourceEntry = promisedResources.constFind(key);
            if (itPromisedResourceEntry == promisedResources.cend())
                return false;
            const auto& promisedResourceEntry = *itPromisedResourceEntry;

            promisedResourceEntry->refCounter++;
            outFutureResourcePtr = promisedResourceEntry->sharedFuture;

            return true;
        }

        bool releaseFutureReference(const KEY_TYPE& key, const ZoomLevel level)
        {
            QMutexLocker scopedLocker(&this->_containerMutex);

#if OSMAND_LOG_SHARED_BY_ZOOM_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedByZoomResourcesContainer(%p)->releaseFutureReference(%s, [%d])",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)),
                level);
#endif

            // Resource must not be already available.
            // Otherwise behavior is undefined
            assert(!_availableResources[level].contains(key));

            const auto& promisedResources = _promisedResources[level];
            const auto& itPromisedResourceEntry = promisedResources.constFind(key);
            if (itPromisedResourceEntry == promisedResources.cend())
                return false;
            const auto& promisedResourceEntry = *itPromisedResourceEntry;
            assert(promisedResourceEntry->refCounter > 0);

            promisedResourceEntry->refCounter--;

            return true;
        }

        bool obtainReferenceOrFutureReferenceOrMakePromise(
            const KEY_TYPE& key,
            const ZoomLevel level,
            const QSet<ZoomLevel>& levels,
            ResourcePtr& outResourcePtr,
            proper::shared_future<ResourcePtr>& outFutureResourcePtr,
            bool withPromise = true)
        {
            QMutexLocker scopedLocker(&this->_containerMutex);

#if OSMAND_LOG_SHARED_BY_ZOOM_RESOURCES_CONTAINER_CHANGE
            LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedByZoomResourcesContainer(%p)->obtainReferenceOrFutureReferenceOrMakePromise(%s, [%d], [%s], ...)",
                QThread::currentThreadId(),
                this,
                qPrintable(QString::fromLatin1("%1").arg(key)),
                level,
                qPrintable(Utilities::stringifyZoomLevels(levels)));
#endif

            assert(levels.contains(level));

            auto& availableResources = _availableResources[level];
            const auto& itAvailableResourceEntry = availableResources.find(key);
            if (itAvailableResourceEntry != availableResources.end())
            {
                const auto& availableResourceEntry = *itAvailableResourceEntry;

                availableResourceEntry->refCounter++;
                outResourcePtr = availableResourceEntry->resourcePtr;

#if OSMAND_LOG_SHARED_BY_ZOOM_RESOURCES_CONTAINER_CHANGE
                LogPrintf(LogSeverityLevel::Debug, "[thread:%p] SharedByZoomResourcesContainer(%p)->obtainReferenceOrFutureReferenceOrMakePromise(%s, [%d], [%s], ...) = true, found %p",
                    QThread::currentThreadId(),
                    this,
                    qPrintable(QString::fromLatin1("%1").arg(key)),
                    level,
                    qPrintable(Utilities::stringifyZoomLevels(levels)),
                    outResourcePtr.get());
#endif

                return true;
            }

            const auto futureReferenceAvailable = obtainFutureReference(key, level, outFutureResourcePtr);
            if (futureReferenceAvailable)
                return true;

            if (withPromise)
                makePromise(key, levels, _useSeparateZoomLevels ? level : ZoomLevel0);
            return false;
        }

        uintmax_t getReferencesCount(const KEY_TYPE& key, const ZoomLevel level = InvalidZoomLevel) const
        {
            QMutexLocker scopedLocker(&this->_containerMutex);

            uintmax_t result = 0;
            const auto firstZoom = (level == InvalidZoomLevel) ? MinZoomLevel : level;
            const auto lastZoom = (level == InvalidZoomLevel) ? MaxZoomLevel : level;
            for (int currentLevel = firstZoom; currentLevel <= lastZoom; currentLevel++)
            {
                const auto& availableResources = _availableResources[currentLevel];
                const auto citAvailableResourceEntry = availableResources.constFind(key);
                if (citAvailableResourceEntry != availableResources.cend())
                {
                    const auto& availableResourceEntry = *citAvailableResourceEntry;

                    result += availableResourceEntry->refCounter;
                    continue;
                }

                const auto& promisedResources = _promisedResources[currentLevel];
                const auto citPromisedResourceEntry = promisedResources.constFind(key);
                if (citPromisedResourceEntry != promisedResources.cend())
                {
                    const auto& promisedResourceEntry = *citPromisedResourceEntry;

                    result += promisedResourceEntry->refCounter;
                    continue;
                }
            }

            return result;
        }
    };
}

#endif // !defined(_OSMAND_CORE_SHARED_BY_ZOOM_RESOURCES_CONTAINER_H_)
