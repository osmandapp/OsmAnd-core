/**
 * @file
 *
 * @section LICENSE
 *
 * OsmAnd - Android navigation software based on OSM maps.
 * Copyright (C) 2010-2013  OsmAnd Authors listed in AUTHORS file
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _OSMAND_CORE_SHARED_BY_ZOOM_RESOURCES_CONTAINER_H_
#define _OSMAND_CORE_SHARED_BY_ZOOM_RESOURCES_CONTAINER_H_

#include <OsmAndCore/stdlib_common.h>
#include <cassert>
#include <array>

#include <OsmAndCore/QtExtensions.h>
#include <QHash>
#include <QSet>
#include <QReadWriteLock>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/SharedResourcesContainer.h>

namespace OsmAnd
{
    // SharedByZoomResourcesContainer is similar to SharedResourcesContainer,
    // but also allows resources to be shared between multiple zoom levels.
    template<typename KEY_TYPE, typename RESOURCE_TYPE>
    class SharedByZoomResourcesContainer : protected SharedResourcesContainer<KEY_TYPE, RESOURCE_TYPE>
    {
    public:
        typedef typename SharedResourcesContainer<KEY_TYPE, RESOURCE_TYPE>::ResourcePtr ResourcePtr;
    protected:
        struct AvailableResourceEntry : public SharedResourcesContainer<KEY_TYPE, RESOURCE_TYPE>::AvailableResourceEntry
        {
            typedef typename SharedResourcesContainer<KEY_TYPE, RESOURCE_TYPE>::AvailableResourceEntry base;

            AvailableResourceEntry(const uintmax_t refCounter_, const ResourcePtr& resourcePtr_, const QSet<ZoomLevel>& zoomLevels_)
                : base(refCounter_, resourcePtr_)
                , zoomLevels(zoomLevels_)
            {
            }

#ifdef Q_COMPILER_RVALUE_REFS
            AvailableResourceEntry(const uintmax_t refCounter_, ResourcePtr&& resourcePtr_, const QSet<ZoomLevel>& zoomLevels_)
                : base(refCounter_, resourcePtr_)
                , zoomLevels(zoomLevels_)
            {
            }
#endif // Q_COMPILER_RVALUE_REFS

            const QSet<ZoomLevel> zoomLevels;
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
        };
    private:
        typedef SharedResourcesContainer<KEY_TYPE, RESOURCE_TYPE> base;

        typedef std::shared_ptr<AvailableResourceEntry> AvailableResourceEntryPtr;
        QSet< AvailableResourceEntryPtr > _availableResourceEntriesStorage;
        std::array< QHash< KEY_TYPE, AvailableResourceEntryPtr >, ZoomLevelsCount> _availableResources;

        typedef std::shared_ptr<PromisedResourceEntry> PromisedResourceEntryPtr;
        QSet< PromisedResourceEntryPtr > _promisedResourceEntriesStorage;
        std::array< QHash< KEY_TYPE, PromisedResourceEntryPtr >, ZoomLevelsCount> _promisedResources;
    protected:
    public:
        SharedByZoomResourcesContainer()
        {
        }
        ~SharedByZoomResourcesContainer()
        {
        }

        void insert(const KEY_TYPE& key, const QSet<ZoomLevel>& levels, ResourcePtr& resourcePtr)
        {
            QWriteLocker scopedLocker(&this->_lock);

            const AvailableResourceEntryPtr newEntryPtr(new AvailableResourceEntry(0, qMove(resourcePtr), levels));
#ifndef Q_COMPILER_RVALUE_REFS
            resourcePtr.reset();
#else
            assert(resourcePtr.use_count() == 0);
#endif

            for(auto itLevel = levels.cbegin(); itLevel != levels.cend(); ++itLevel)
            {
                const auto level = *itLevel;

                // Resource must not be promised and must not be already available.
                // Otherwise behavior is undefined
                assert(!_promisedResources[level].contains(key));
                assert(!_availableResources[level].contains(key));

                _availableResources[level].insert(key, newEntryPtr);
            }
            
            _availableResourceEntriesStorage.insert(qMove(newEntryPtr));
        }

#ifdef Q_COMPILER_RVALUE_REFS
        void insert(const KEY_TYPE& key, const QSet<ZoomLevel>& levels, ResourcePtr&& resourcePtr)
        {
            QWriteLocker scopedLocker(&this->_lock);

            const AvailableResourceEntryPtr newEntryPtr(new AvailableResourceEntry(0, qMove(resourcePtr), levels));
            assert(resourcePtr.use_count() == 0);

            for(auto itLevel = levels.cbegin(); itLevel != levels.cend(); ++itLevel)
            {
                const auto level = *itLevel;

                // Resource must not be promised and must not be already available.
                // Otherwise behavior is undefined
                assert(!_promisedResources[level].contains(key));
                assert(!_availableResources[level].contains(key));

                _availableResources[level].insert(key, newEntryPtr);
            }

            _availableResourceEntriesStorage.insert(qMove(newEntryPtr));
        }
#endif // Q_COMPILER_RVALUE_REFS

        void insertAndReference(const KEY_TYPE& key, const QSet<ZoomLevel>& levels, const ResourcePtr& resourcePtr)
        {
            QWriteLocker scopedLocker(&this->_lock);

            const AvailableResourceEntryPtr newEntryPtr(new AvailableResourceEntry(1, resourcePtr, levels));

            for(auto itLevel = levels.cbegin(); itLevel != levels.cend(); ++itLevel)
            {
                const auto level = *itLevel;

                // Resource must not be promised and must not be already available.
                // Otherwise behavior is undefined
                assert(!_promisedResources[level].contains(key));
                assert(!_availableResources[level].contains(key));

                _availableResources[level].insert(key, newEntryPtr);
            }

            _availableResourceEntriesStorage.insert(qMove(newEntryPtr));
        }

        bool obtainReference(const KEY_TYPE& key, const ZoomLevel level, ResourcePtr& outResourcePtr)
        {
            QWriteLocker scopedLocker(&this->_lock);

            // In case resource was promised, wait forever until promise is fulfilled
            const auto itPromisedResourceEntry = _promisedResources[level].constFind(key);
            if(itPromisedResourceEntry != _promisedResources[level].cend())
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

            const auto itAvailableResourceEntry = _availableResources[level].find(key);
            if(itAvailableResourceEntry == _availableResources[level].end())
                return false;
            const auto& availableResourceEntry = *itAvailableResourceEntry;

            availableResourceEntry->refCounter++;
            outResourcePtr = availableResourceEntry->resourcePtr;

            return true;
        }

        bool releaseReference(const KEY_TYPE& key, const ZoomLevel level, ResourcePtr& resourcePtr, const bool autoClean = true, bool* outWasCleaned = nullptr, uintmax_t* outRemainingReferences = nullptr)
        {
            QWriteLocker scopedLocker(&this->_lock);

            // Resource must not be promised. Otherwise behavior is undefined
            assert(!_promisedResources[level].contains(key));

            const auto itAvailableResourceEntry = _availableResources[level].find(key);
            if(itAvailableResourceEntry == _availableResources[level].end())
                return false;
            const auto& availableResourceEntry = *itAvailableResourceEntry;
            assert(availableResourceEntry->refCounter > 0);
            assert(availableResourceEntry->resourcePtr == resourcePtr);

            availableResourceEntry->refCounter--;
            if(outRemainingReferences)
                *outRemainingReferences = availableResourceEntry->refCounter;
            if(autoClean && outWasCleaned)
                *outWasCleaned = false;
            if(autoClean && availableResourceEntry->refCounter == 0)
            {
                for(auto itOtherLevel = availableResourceEntry->zoomLevels.cbegin(); itOtherLevel != availableResourceEntry->zoomLevels.cend(); ++itOtherLevel)
                {
                    const auto otherLevel = *itOtherLevel;
                    if(otherLevel == level)
                        continue;

                    const auto removedCount = _availableResources[otherLevel].remove(key);
                    assert(removedCount == 1);
                }

                _availableResourceEntriesStorage.remove(availableResourceEntry);
                _availableResources[level].erase(itAvailableResourceEntry);

                if(outWasCleaned)
                    *outWasCleaned = true;
            }
            resourcePtr.reset();

            return true;
        }
        
        void makePromise(const KEY_TYPE& key, const QSet<ZoomLevel>& levels)
        {
            QWriteLocker scopedLocker(&this->_lock);

            const PromisedResourceEntryPtr newEntryPtr(new PromisedResourceEntry(levels));

            for(auto itLevel = levels.cbegin(); itLevel != levels.cend(); ++itLevel)
            {
                const auto level = *itLevel;

                // Resource must not be promised and must not be already available.
                // Otherwise behavior is undefined
                assert(!_promisedResources[level].contains(key));
                assert(!_availableResources[level].contains(key));

                _promisedResources[level].insert(key, newEntryPtr);
            }

            _promisedResourceEntriesStorage.insert(qMove(newEntryPtr));
        }

        void breakPromise(const KEY_TYPE& key, const QSet<ZoomLevel>& levels)
        {
            QWriteLocker scopedLocker(&this->_lock);

            PromisedResourceEntryPtr promisedEntryPtr;
            for(auto itLevel = levels.cbegin(); itLevel != levels.cend(); ++itLevel)
            {
                const auto level = *itLevel;

                // Resource must be promised and must not be already available.
                // Otherwise behavior is undefined
                assert(_promisedResources[level].contains(key));
                assert(!_availableResources[level].contains(key));

                const auto itPromisedResourceEntry = _promisedResources[level].find(key);
                if(!promisedEntryPtr)
                    promisedEntryPtr = *itPromisedResourceEntry;
                _promisedResources[level].erase(itPromisedResourceEntry);

            }

            _promisedResourceEntriesStorage.erase(promisedEntryPtr);
            promisedEntryPtr->promise.set_exception(std::make_exception_ptr(std::runtime_error("Promise was broken")));
        }

        void fulfilPromise(const KEY_TYPE& key, const QSet<ZoomLevel>& levels, ResourcePtr& resourcePtr)
        {
            QWriteLocker scopedLocker(&this->_lock);

            PromisedResourceEntryPtr promisedEntryPtr;
            for(auto itLevel = levels.cbegin(); itLevel != levels.cend(); ++itLevel)
            {
                const auto level = *itLevel;

                // Resource must be promised and must not be already available.
                // Otherwise behavior is undefined
                assert(_promisedResources[level].contains(key));
                assert(!_availableResources[level].contains(key));

                const auto itPromisedResourceEntry = _promisedResources[level].find(key);
                if(!promisedEntryPtr)
                    promisedEntryPtr = *itPromisedResourceEntry;
                _promisedResources[level].erase(itPromisedResourceEntry);

            }
            _promisedResourceEntriesStorage.erase(promisedEntryPtr);

            if(promisedEntryPtr->refCounter <= 0)
                return;

            const AvailableResourceEntryPtr newEntryPtr(new AvailableResourceEntry(promisedEntryPtr->refCounter, qMove(resourcePtr), levels));
#ifndef Q_COMPILER_RVALUE_REFS
            resourcePtr.reset();
#else
            assert(resourcePtr.use_count() == 0);
#endif

            for(auto itLevel = levels.cbegin(); itLevel != levels.cend(); ++itLevel)
            {
                const auto level = *itLevel;

                // Resource must not be promised and must not be already available.
                // Otherwise behavior is undefined
                assert(!_promisedResources[level].contains(key));
                assert(!_availableResources[level].contains(key));

                _availableResources[level].insert(key, newEntryPtr);
            }

            _availableResourceEntriesStorage.insert(newEntryPtr);
            promisedEntryPtr->promise.set_value(newEntryPtr->resourcePtr);
        }

#ifdef Q_COMPILER_RVALUE_REFS
        void fulfilPromise(const KEY_TYPE& key, const QSet<ZoomLevel>& levels, ResourcePtr&& resourcePtr)
        {
            QWriteLocker scopedLocker(&this->_lock);

            PromisedResourceEntryPtr promisedEntryPtr;
            for(auto itLevel = levels.cbegin(); itLevel != levels.cend(); ++itLevel)
            {
                const auto level = *itLevel;

                // Resource must be promised and must not be already available.
                // Otherwise behavior is undefined
                assert(_promisedResources[level].contains(key));
                assert(!_availableResources[level].contains(key));

                const auto itPromisedResourceEntry = _promisedResources[level].find(key);
                if(!promisedEntryPtr)
                    promisedEntryPtr = *itPromisedResourceEntry;
                _promisedResources[level].erase(itPromisedResourceEntry);

            }
            _promisedResourceEntriesStorage.erase(promisedEntryPtr);

            if(promisedEntryPtr->refCounter <= 0)
                return;

            const AvailableResourceEntryPtr newEntryPtr(new AvailableResourceEntry(promisedEntryPtr->refCounter, qMove(resourcePtr), levels));
            assert(resourcePtr.use_count() == 0);

            for(auto itLevel = levels.cbegin(); itLevel != levels.cend(); ++itLevel)
            {
                const auto level = *itLevel;

                // Resource must not be promised and must not be already available.
                // Otherwise behavior is undefined
                assert(!_promisedResources[level].contains(key));
                assert(!_availableResources[level].contains(key));

                _availableResources[level].insert(key, newEntryPtr);
            }

            _availableResourceEntriesStorage.insert(newEntryPtr);
            promisedEntryPtr->promise.set_value(newEntryPtr->resourcePtr);
        }
#endif // Q_COMPILER_RVALUE_REFS

        void fulfilPromiseAndReference(const KEY_TYPE& key, const QSet<ZoomLevel>& levels, const ResourcePtr& resourcePtr)
        {
            QWriteLocker scopedLocker(&this->_lock);

            PromisedResourceEntryPtr promisedEntryPtr;
            for(auto itLevel = levels.cbegin(); itLevel != levels.cend(); ++itLevel)
            {
                const auto level = *itLevel;

                // Resource must be promised and must not be already available.
                // Otherwise behavior is undefined
                assert(_promisedResources[level].contains(key));
                assert(!_availableResources[level].contains(key));

                const auto itPromisedResourceEntry = _promisedResources[level].find(key);
                if(!promisedEntryPtr)
                    promisedEntryPtr = *itPromisedResourceEntry;
                _promisedResources[level].erase(itPromisedResourceEntry);

            }
            _promisedResourceEntriesStorage.remove(promisedEntryPtr);

            const AvailableResourceEntryPtr newEntryPtr(new AvailableResourceEntry(promisedEntryPtr->refCounter + 1, resourcePtr, levels));

            for(auto itLevel = levels.cbegin(); itLevel != levels.cend(); ++itLevel)
            {
                const auto level = *itLevel;

                // Resource must not be promised and must not be already available.
                // Otherwise behavior is undefined
                assert(!_promisedResources[level].contains(key));
                assert(!_availableResources[level].contains(key));

                _availableResources[level].insert(key, newEntryPtr);
            }

            _availableResourceEntriesStorage.insert(newEntryPtr);
            promisedEntryPtr->promise.set_value(newEntryPtr->resourcePtr);
        }

        bool obtainFutureReference(const KEY_TYPE& key, const ZoomLevel level, std::shared_future<ResourcePtr>& outFutureResourcePtr)
        {
            QWriteLocker scopedLocker(&this->_lock);

            // Resource must not be already available.
            // Otherwise behavior is undefined
            assert(!_availableResources[level].contains(key));

            const auto itPromisedResourceEntry = _promisedResources[level].constFind(key);
            if(itPromisedResourceEntry == _promisedResources[level].cend())
                return false;
            const auto& promisedResourceEntry = *itPromisedResourceEntry;

            promisedResourceEntry->refCounter++;
            outFutureResourcePtr = promisedResourceEntry->sharedFuture;

            return true;
        }

        bool releaseFutureReference(const KEY_TYPE& key, const ZoomLevel level)
        {
            QWriteLocker scopedLocker(&this->_lock);

            // Resource must not be already available.
            // Otherwise behavior is undefined
            assert(!_availableResources[level].contains(key));

            const auto itPromisedResourceEntry = _promisedResources[level].constFind(key);
            if(itPromisedResourceEntry == _promisedResources[level].cend())
                return false;
            const auto& promisedResourceEntry = *itPromisedResourceEntry;
            assert(promisedResourceEntry->refCounter > 0);

            promisedResourceEntry->refCounter--;

            return true;
        }

        bool obtainReferenceOrFutureReferenceOrMakePromise(const KEY_TYPE& key, const ZoomLevel level, const QSet<ZoomLevel>& levels, ResourcePtr& outResourcePtr, std::shared_future<ResourcePtr>& outFutureResourcePtr)
        {
            QWriteLocker scopedLocker(&this->_lock);

            assert(levels.contains(level));

            const auto itAvailableResourceEntry = _availableResources[level].find(key);
            if(itAvailableResourceEntry != _availableResources[level].end())
            {
                const auto& availableResourceEntry = *itAvailableResourceEntry;

                availableResourceEntry->refCounter++;
                outResourcePtr = availableResourceEntry->resourcePtr;

                return true;
            }

            const auto futureReferenceAvailable = obtainFutureReference(key, level, outFutureResourcePtr);
            if(futureReferenceAvailable)
                return true;

            makePromise(key, levels);
            return false;
        }

    };
}

#endif // _OSMAND_CORE_SHARED_BY_ZOOM_RESOURCES_CONTAINER_H_
