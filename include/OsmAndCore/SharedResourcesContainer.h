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
#include <utility>

#include <OsmAndCore/QtExtensions.h>
#include <QHash>
#include <QMutableHashIterator>
#include <QSet>
#include <QReadWriteLock>
#include <QWaitCondition>

#include <OsmAndCore.h>

namespace OsmAnd
{
    template<typename KEY_TYPE, typename RESOURCE_TYPE>
    class SharedResourcesContainer Q_DECL_FINAL
    {
    private:
        typedef std::pair< uintmax_t, std::shared_ptr<RESOURCE_TYPE> > ResourceEntry;

        mutable QReadWriteLock _lock;
        QHash< KEY_TYPE, ResourceEntry > _container;
        QSet< KEY_TYPE > _pending;
        QWaitCondition _pendingWaitCondition;
    protected:
    public:
        SharedResourcesContainer()
        {
        }
        ~SharedResourcesContainer()
        {
        }

        // insert(key, data)
        // insertAndReference(key, data)
        // obtainReference
        // releaseReference

        // makePromise(key) == reserveAsPending
        // breakPromise(key) == cancelPending
        // fulfilPromise(key, data) == insertPending
        // fulfilPromiseAndReference(key, data) == insertPendingAndReference
        // obtainFutureReferenceOrMakePromise [future references are also counted]
        // releaseFutureReference

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        
        void insert(const KEY_TYPE& key, std::shared_ptr<RESOURCE_TYPE>& resource)
        {
            QWriteLocker scopedLocker(&_lock);

            // Check if this resource is not in pending
            assert(!_pending.contains(key));

            // Insert resource and set it's reference counter to 0
            assert(!_container.contains(key));
            _container.insert(key, qMove(ResourceEntry(0, resource)));

            // Since we've taken reference, reset it
            resource.reset();
        }

#ifdef Q_COMPILER_RVALUE_REFS
        void insert(const KEY_TYPE& key, std::shared_ptr<RESOURCE_TYPE>&& resource)
        {
            QWriteLocker scopedLocker(&_lock);

            // Check if this resource is not in pending
            assert(!_pending.contains(key));

            // Insert resource and set it's reference counter to 0
            assert(!_container.contains(key));
            _container.insert(key, qMove(ResourceEntry(0, std::forward(resource))));

            // Check that resource was reset as expected
            assert(resource.use_count() == 0);
        }
#endif // Q_COMPILER_RVALUE_REFS

        void insertAndReference(const KEY_TYPE& key, const std::shared_ptr<RESOURCE_TYPE>& resource)
        {
            QWriteLocker scopedLocker(&_lock);

            // Check if this resource is not in pending
            assert(!_pending.contains(key));

            // Insert resource and set it's reference counter to 1
            assert(!_container.contains(key));
            _container.insert(key, qMove(ResourceEntry(1, resource)));
        }

        void reserveAsPending(const KEY_TYPE& key)
        {
            QWriteLocker scopedLocker(&_lock);

            // Check if this resource is not in pending (yet)
            assert(!_pending.contains(key));

            // Check if this resource is not in container
            assert(!_container.contains(key));

            // Insert resource key into pending set
            _pending.insert(key);
        }

        void cancelPending(const KEY_TYPE& key)
        {
            QWriteLocker scopedLocker(&_lock);

            // Check if this resource is in pending (still)
            assert(_pending.contains(key));

            // Check if this resource is not in container
            assert(!_container.contains(key));

            // Insert resource key into pending set
            _pending.remove(key);

            // Notify that pending has changed
            _pendingWaitCondition.wakeAll();
        }

        void insertPending(const KEY_TYPE& key, std::shared_ptr<RESOURCE_TYPE>& resource)
        {
            QWriteLocker scopedLocker(&_lock);

            // Check if this resource is in pending
            assert(_pending.contains(key));
            _pending.remove(key);

            // Check if this resource is not in container
            assert(!_container.contains(key));

            // Insert resource and set it's reference counter to 0
            _container.insert(key, qMove(ResourceEntry(0, resource)));

            // Since we've taken reference, reset it
            resource.reset();

            // Notify that pending has changed
            _pendingWaitCondition.wakeAll();
        }

#ifdef Q_COMPILER_RVALUE_REFS
        void insertPending(const KEY_TYPE& key, std::shared_ptr<RESOURCE_TYPE>&& resource)
        {
            QWriteLocker scopedLocker(&_lock);

            // Check if this resource is in pending
            assert(_pending.contains(key));
            _pending.remove(key);

            // Check if this resource is not in container
            assert(!_container.contains(key));

            // Insert resource and set it's reference counter to 0
            _container.insert(key, qMove(ResourceEntry(0, std::forward(resource))));

            // Check that resource was reset as expected
            assert(resource.use_count() == 0);

            // Notify that pending has changed
            _pendingWaitCondition.wakeAll();
        }
#endif // Q_COMPILER_RVALUE_REFS

        void insertPendingAndReference(const KEY_TYPE& key, const std::shared_ptr<RESOURCE_TYPE>& resource)
        {
            QWriteLocker scopedLocker(&_lock);

            // Check if this resource is in pending
            assert(_pending.contains(key));
            _pending.remove(key);

            // Check if this resource is not in container
            assert(!_container.contains(key));

            // Insert resource and set it's reference counter to 1
            _container.insert(key, qMove(ResourceEntry(1, resource)));

            // Notify that pending has changed
            _pendingWaitCondition.wakeAll();
        }

        bool obtainReference(const KEY_TYPE& key, std::shared_ptr<RESOURCE_TYPE>& outResource)
        {
            QWriteLocker scopedLocker(&_lock);

            // Wait until resource is in pending state
            while(_pending.contains(key))
                _pendingWaitCondition.wait(&_lock);

            // Find entry
            const auto itEntry = _container.find(key);
            if(itEntry == _container.end())
                return false;

            // Increase internal reference counter
            itEntry->first++;

            // Set reference
            outResource = itEntry->second;
            return true;
        }

        bool releaseReference(const KEY_TYPE& key, std::shared_ptr<RESOURCE_TYPE>& resource, const bool autoClean = true)
        {
            QWriteLocker scopedLocker(&_lock);

            // Check if this resource is not in pending
            assert(!_pending.contains(key));

            // Find entry
            const auto itEntry = _container.find(key);
            if(itEntry == _container.end())
                return false;

            // Decrement internal reference counter
            assert(itEntry->first > 0);
            itEntry->first--;

            // Remove reference
            resource.reset();

            // If this is the last reference, remove the entire entry
            if(autoClean && itEntry->first == 0)
                _container.erase(itEntry);

            return true;
        }

        bool obtainReferenceOrReserveAsPending(const KEY_TYPE& key, std::shared_ptr<RESOURCE_TYPE>& outResource)
        {
            QWriteLocker scopedLocker(&_lock);

            // Wait until resource is in pending state
            while(_pending.contains(key))
                _pendingWaitCondition.wait(&_lock);

            // Find entry
            const auto itEntry = _container.find(key);
            if(itEntry == _container.end())
            {
                // Reserve as pending
                _pending.insert(key);

                return false;
            }

            // Increase internal reference counter
            itEntry->first++;

            // Set reference
            outResource = itEntry->second;
            return true;
        }
    };
}

#endif // _OSMAND_CORE_SHARED_RESOURCES_CONTAINER_H_
