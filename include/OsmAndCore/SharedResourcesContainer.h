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
#ifndef _OSMAND_CORE_SHARED_RESOURCES_CONTAINER_H_
#define _OSMAND_CORE_SHARED_RESOURCES_CONTAINER_H_

#include <OsmAndCore/stdlib_common.h>
#include <cassert>
#include <future>

#include <OsmAndCore/QtExtensions.h>
#include <QHash>
#include <QReadWriteLock>

#include <OsmAndCore.h>

namespace OsmAnd
{
    template<typename KEY_TYPE, typename RESOURCE_TYPE>
    class SharedResourcesContainer
    {
    public:
        typedef std::shared_ptr<RESOURCE_TYPE> ResourcePtr;
    protected:
        mutable QReadWriteLock _lock;

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
            std::promise<ResourcePtr> promise;
            const std::shared_future<ResourcePtr> sharedFuture;
        };
    private:
        QHash< KEY_TYPE, std::shared_ptr< AvailableResourceEntry > > _availableResources;
        QHash< KEY_TYPE, std::shared_ptr< PromisedResourceEntry > > _promisedResources;
    protected:
    public:
        SharedResourcesContainer()
            : _lock(QReadWriteLock::Recursive)
        {
        }
        ~SharedResourcesContainer()
        {
        }

        void insert(const KEY_TYPE& key, ResourcePtr& resourcePtr)
        {
            QWriteLocker scopedLocker(&_lock);

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
            QWriteLocker scopedLocker(&_lock);

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
            QWriteLocker scopedLocker(&_lock);

            // Resource must not be promised and must not be already available.
            // Otherwise behavior is undefined
            assert(!_promisedResources.contains(key));
            assert(!_availableResources.contains(key));

            const auto newEntry = new AvailableResourceEntry(1, resourcePtr);
            _availableResources.insert(key, qMove(std::shared_ptr<AvailableResourceEntry>(newEntry)));
        }

        bool obtainReference(const KEY_TYPE& key, ResourcePtr& outResourcePtr)
        {
            QWriteLocker scopedLocker(&_lock);

            // In case resource was promised, wait forever until promise is fulfilled
            const auto itPromisedResourceEntry = _promisedResources.constFind(key);
            if(itPromisedResourceEntry != _promisedResources.cend())
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
            if(itAvailableResourceEntry == _availableResources.end())
                return false;
            const auto& availableResourceEntry = *itAvailableResourceEntry;

            availableResourceEntry->refCounter++;
            outResourcePtr = availableResourceEntry->resourcePtr;

            return true;
        }

        bool releaseReference(const KEY_TYPE& key, ResourcePtr& resourcePtr, const bool autoClean = true, bool* outWasCleaned = nullptr, uintmax_t* outRemainingReferences = nullptr)
        {
            QWriteLocker scopedLocker(&_lock);

            // Resource must not be promised. Otherwise behavior is undefined
            assert(!_promisedResources.contains(key));
            
            const auto itAvailableResourceEntry = _availableResources.find(key);
            if(itAvailableResourceEntry == _availableResources.end())
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
                _availableResources.erase(itAvailableResourceEntry);

                if(outWasCleaned)
                    *outWasCleaned = true;
            }
            resourcePtr.reset();

            return true;
        }

        void makePromise(const KEY_TYPE& key)
        {
            QWriteLocker scopedLocker(&_lock);

            // Resource must not be promised and must not be already available.
            // Otherwise behavior is undefined
            assert(!_promisedResources.contains(key));
            assert(!_availableResources.contains(key));

            const auto newEntry = new PromisedResourceEntry();
            _promisedResources.insert(key, qMove(std::shared_ptr<PromisedResourceEntry>(newEntry)));
        }

        void breakPromise(const KEY_TYPE& key)
        {
            QWriteLocker scopedLocker(&_lock);

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

            promisedResourceEntry->promise.set_exception(std::make_exception_ptr(std::runtime_error("Promise was broken")));
        }

        void fulfilPromise(const KEY_TYPE& key, ResourcePtr& resourcePtr)
        {
            QWriteLocker scopedLocker(&_lock);

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

            if(promisedResourceEntry->refCounter <= 0)
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
            QWriteLocker scopedLocker(&_lock);

            // Resource must be promised and must not be already available.
            // Otherwise behavior is undefined
            assert(_promisedResources.contains(key));
            assert(!_availableResources.contains(key));

            const auto itPromisedResourceEntry = _promisedResources.find(key);
            const std::shared_ptr<PromisedResourceEntry> promisedResourceEntry(qMove(*itPromisedResourceEntry));
            _promisedResources.erase(itPromisedResourceEntry);

            if(promisedResourceEntry->refCounter <= 0)
                return;

            const auto newEntry = new AvailableResourceEntry(promisedResourceEntry->refCounter, qMove(resourcePtr));
            assert(resourcePtr.use_count() == 0);
            _availableResources.insert(key, qMove(std::shared_ptr<AvailableResourceEntry>(newEntry)));
            promisedResourceEntry->promise.set_value(newEntry->resourcePtr);
        }
#endif // Q_COMPILER_RVALUE_REFS

        void fulfilPromiseAndReference(const KEY_TYPE& key, const ResourcePtr& resourcePtr)
        {
            QWriteLocker scopedLocker(&_lock);

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

            const auto newEntry = new AvailableResourceEntry(promisedResourceEntry->refCounter + 1, resourcePtr);
            _availableResources.insert(key, qMove(std::shared_ptr<AvailableResourceEntry>(newEntry)));
            promisedResourceEntry->promise.set_value(newEntry->resourcePtr);
        }

        bool obtainFutureReference(const KEY_TYPE& key, std::shared_future<ResourcePtr>& outFutureResourcePtr)
        {
            QWriteLocker scopedLocker(&_lock);

            // Resource must not be already available.
            // Otherwise behavior is undefined
            assert(!_availableResources.contains(key));

            const auto itPromisedResourceEntry = _promisedResources.constFind(key);
            if(itPromisedResourceEntry == _promisedResources.cend())
                return false;
            const auto& promisedResourceEntry = *itPromisedResourceEntry;

            promisedResourceEntry->refCounter++;
            outFutureResourcePtr = promisedResourceEntry->sharedFuture;

            return true;
        }

        bool releaseFutureReference(const KEY_TYPE& key)
        {
            QWriteLocker scopedLocker(&_lock);

            // Resource must not be already available.
            // Otherwise behavior is undefined
            assert(!_availableResources.contains(key));

            const auto itPromisedResourceEntry = _promisedResources.constFind(key);
            if(itPromisedResourceEntry == _promisedResources.cend())
                return false;
            const auto& promisedResourceEntry = *itPromisedResourceEntry;
            assert(promisedResourceEntry->refCounter > 0);
            
            promisedResourceEntry->refCounter--;

            return true;
        }

        bool obtainReferenceOrFutureReferenceOrMakePromise(const KEY_TYPE& key, ResourcePtr& outResourcePtr, std::shared_future<ResourcePtr>& outFutureResourcePtr)
        {
            QWriteLocker scopedLocker(&_lock);

            const auto itAvailableResourceEntry = _availableResources.find(key);
            if(itAvailableResourceEntry != _availableResources.end())
            {
                const auto& availableResourceEntry = *itAvailableResourceEntry;

                availableResourceEntry->refCounter++;
                outResourcePtr = availableResourceEntry->resourcePtr;

                return true;
            }
            
            const auto futureReferenceAvailable = obtainFutureReference(key, outFutureResourcePtr);
            if(futureReferenceAvailable)
                return true;
            
            makePromise(key);
            return false;
        }
    };
}

#endif // _OSMAND_CORE_SHARED_RESOURCES_CONTAINER_H_
