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
#include <utility>
#include <array>
#include <tuple>

#include <OsmAndCore/QtExtensions.h>
#include <QHash>
#include <QSet>
#include <QMutableSetIterator>
#include <QReadWriteLock>
#include <QWaitCondition>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    // SharedByZoomResourcesContainer is similar to SharedResourcesContainer,
    // but also allows resources to be shared between multiple zoom levels.
    template<typename KEY_TYPE, typename RESOURCE_TYPE>
    class SharedByZoomResourcesContainer Q_DECL_FINAL
    {
    private:
        typedef std::tuple< int, std::shared_ptr< RESOURCE_TYPE >, QSet<ZoomLevel> > ResourceEntry;
        typedef std::shared_ptr<ResourceEntry> ResourceEntryRef;

        mutable QReadWriteLock _lock;
        std::array< QHash< KEY_TYPE, ResourceEntryRef >, ZoomLevelsCount> _container;
        QSet< ResourceEntryRef > _resources;
        std::array< QSet< KEY_TYPE >, ZoomLevelsCount> _pending;
        QWaitCondition _pendingWaitCondition;
    protected:
    public:
        SharedByZoomResourcesContainer()
        {
        }
        ~SharedByZoomResourcesContainer()
        {
        }

        void insert(const KEY_TYPE& key, const QSet<ZoomLevel>& levels, std::shared_ptr<RESOURCE_TYPE>& resource)
        {
            QWriteLocker scopedLocker(&_lock);

            // Insert resource and set it's reference counter to 0
            const ResourceEntryRef resourceEntry(new ResourceEntry(0, resource, levels));
            _resources.insert(resourceEntry);

            for(auto itLevel = levels.cbegin(); itLevel != levels.cend(); ++itLevel)
            {
                const auto level = *itLevel;

                // Check if this resource is not in pending
                assert(!_pending[level].contains(key));

                // Insert resource reference
                assert(!_container[level].contains(key));
                _container[level].insert(key, resourceEntry);
            }
            
            // Since we've taken reference, reset it
            resource.reset();
        }

#ifdef Q_COMPILER_RVALUE_REFS
        void insert(const KEY_TYPE& key, const QSet<ZoomLevel>& levels, std::shared_ptr<RESOURCE_TYPE>&& resource)
        {
            QWriteLocker scopedLocker(&_lock);

            // Insert resource and set it's reference counter to 0
            const ResourceEntryRef resourceEntry(new ResourceEntry(0, std::forward(resource), levels));
            _resources.insert(resourceEntry);

            for(auto itLevel = levels.cbegin(); itLevel != levels.cend(); ++itLevel)
            {
                const auto level = *itLevel;

                // Check if this resource is not in pending
                assert(!_pending[level].contains(key));

                // Insert resource reference
                assert(!_container[level].contains(key));
                _container[level].insert(key, resourceEntry);
            }

            // Check that resource was reset as expected
            assert(resource.use_count() == 0);
        }
#endif // Q_COMPILER_RVALUE_REFS

        void insertAndReference(const KEY_TYPE& key, const QSet<ZoomLevel>& levels, const std::shared_ptr<RESOURCE_TYPE>& resource)
        {
            QWriteLocker scopedLocker(&_lock);

            // Insert resource and set it's reference counter to 1
            const ResourceEntryRef resourceEntry(new ResourceEntry(1, resource, levels));
            _resources.insert(resourceEntry);

            for(auto itLevel = levels.cbegin(); itLevel != levels.cend(); ++itLevel)
            {
                const auto level = *itLevel;

                // Check if this resource is not in pending
                assert(!_pending[level].contains(key));

                // Insert resource reference
                assert(!_container[level].contains(key));
                _container[level].insert(key, resourceEntry);
            }

            // Since we've taken reference, reset it
            resource.reset();
        }

        void reserveAsPending(const KEY_TYPE& key, const QSet<ZoomLevel>& levels)
        {
            QWriteLocker scopedLocker(&_lock);

            for(auto itLevel = levels.cbegin(); itLevel != levels.cend(); ++itLevel)
            {
                const auto level = *itLevel;

                // Check if this resource is not in pending (yet)
                assert(!_pending[level].contains(key));

                // Check if this resource is not in container
                assert(!_container[level].contains(key));

                // Insert resource key into pending set
                _pending[level].insert(key);
            }
        }

        void insertPending(const KEY_TYPE& key, const QSet<ZoomLevel>& levels, std::shared_ptr<RESOURCE_TYPE>& resource)
        {
            QWriteLocker scopedLocker(&_lock);

            // Insert resource and set it's reference counter to 0
            const ResourceEntryRef resourceEntry(new ResourceEntry(0, resource, levels));
            _resources.insert(resourceEntry);

            for(auto itLevel = levels.cbegin(); itLevel != levels.cend(); ++itLevel)
            {
                const auto level = *itLevel;

                // Check if this resource is in pending
                assert(_pending[level].contains(key));
                _pending[level].remove(key);

                // Check if this resource is not in container
                assert(!_container[level].contains(key));

                // Insert resource reference
                _container[level].insert(key, resourceEntry);
            }

            // Since we've taken reference, reset it
            resource.reset();

            // Notify that pending has changed
            _pendingWaitCondition.wakeAll();
        }

#ifdef Q_COMPILER_RVALUE_REFS
        void insertPending(const KEY_TYPE& key, const QSet<ZoomLevel>& levels, std::shared_ptr<RESOURCE_TYPE>&& resource)
        {
            QWriteLocker scopedLocker(&_lock);

            // Insert resource and set it's reference counter to 0
            const ResourceEntryRef resourceEntry(new ResourceEntry(0, std::forward(resource), levels));
            _resources.insert(resourceEntry);

            for(auto itLevel = levels.cbegin(); itLevel != levels.cend(); ++itLevel)
            {
                const auto level = *itLevel;

                // Check if this resource is in pending
                assert(_pending[level].contains(key));
                _pending[level].remove(key);

                // Check if this resource is not in container
                assert(!_container[level].contains(key));

                // Insert resource reference
                _container[level].insert(key, resourceEntry);
            }

            // Check that resource was reset as expected
            assert(resource.use_count() == 0);

            // Notify that pending has changed
            _pendingWaitCondition.wakeAll();
        }
#endif // Q_COMPILER_RVALUE_REFS

        void insertPendingAndReference(const KEY_TYPE& key, const QSet<ZoomLevel>& levels, const std::shared_ptr<RESOURCE_TYPE>& resource)
        {
            QWriteLocker scopedLocker(&_lock);

            // Insert resource and set it's reference counter to 1
            const ResourceEntryRef resourceEntry(new ResourceEntry(1, resource, levels));
            _resources.insert(resourceEntry);

            for(auto itLevel = levels.cbegin(); itLevel != levels.cend(); ++itLevel)
            {
                const auto level = *itLevel;

                // Check if this resource is in pending
                assert(_pending[level].contains(key));
                _pending[level].remove(key);

                // Check if this resource is not in container
                assert(!_container[level].contains(key));

                // Insert resource reference
                _container[level].insert(key, resourceEntry);
            }

            // Notify that pending has changed
            _pendingWaitCondition.wakeAll();
        }

        bool obtainReference(const KEY_TYPE& key, const ZoomLevel level, std::shared_ptr<RESOURCE_TYPE>& outResource)
        {
            QWriteLocker scopedLocker(&_lock);

            // Wait until resource is in pending state
            while(_pending[level].contains(key))
                _pendingWaitCondition.wait(&_lock);

            // Find entry
            const auto itEntry = _container[level].find(key);
            if(itEntry == _container[level].end())
                return false;

            // Increase internal reference counter
            auto& entry = **itEntry;
            auto& refCounter = std::get<0>(entry);
            refCounter++;

            // Set reference
            outResource = std::get<1>(entry);
            return true;
        }

        bool releaseReference(const KEY_TYPE& key, const ZoomLevel level, std::shared_ptr<RESOURCE_TYPE>& resource, const bool autoClean = true)
        {
            QWriteLocker scopedLocker(&_lock);

            // Check if this resource is not in pending
            assert(!_pending[level].contains(key));

            // Find entry
            const auto itEntry = _container[level].find(key);
            if(itEntry == _container[level].end())
                return false;
            const auto& entryRef = *itEntry;
            auto& entry = *entryRef;

            // Decrement internal reference counter
            auto& refCounter = std::get<0>(entry);
            assert(refCounter > 0);
            refCounter--;

            // Remove reference
            resource.reset();

            // If this is the last reference, remove the entire entry
            if(autoClean && refCounter == 0)
            {
                // Erase all other links
                const auto& levels = std::get<2>(entry);
                for(auto itLevel = levels.cbegin(); itLevel != levels.cend(); ++itLevel)
                {
                    const auto otherLevel = *itLevel;
                    if(level == otherLevel)
                        continue;

                    const auto removedCount = _container[otherLevel].remove(key);
                    assert(removedCount == 1);
                }

                // Remove the resource entry
                _resources.remove(entryRef);

                // Erase current link
                _container[level].erase(itEntry);
            }

            return true;
        }

        bool obtainReferenceOrReserveAsPending(const KEY_TYPE& key, const ZoomLevel level, const QSet<ZoomLevel>& levels, std::shared_ptr<RESOURCE_TYPE>& outResource)
        {
            QWriteLocker scopedLocker(&_lock);

            // Wait until resource is in pending state
            while(_pending[level].contains(key))
                _pendingWaitCondition.wait(&_lock);

            // Find entry
            const auto itEntry = _container[level].find(key);
            if(itEntry == _container[level].end())
            {
                // Reserve as pending
                for(auto itLevel = levels.cbegin(); itLevel != levels.cend(); ++itLevel)
                {
                    const auto level = *itLevel;

                    // Check if this resource is not in pending (yet)
                    assert(!_pending[level].contains(key));

                    // Check if this resource is not in container
                    assert(!_container[level].contains(key));

                    // Insert resource key into pending set
                    _pending[level].insert(key);
                }

                return false;
            }

            // Increase internal reference counter
            auto& entry = **itEntry;
            auto& refCounter = std::get<0>(entry);
            refCounter++;

            // Set reference
            outResource = std::get<1>(entry);
            return true;
        }

        void cleanUpUnreferencedResources()
        {
            QWriteLocker scopedLocker(&_lock);

            QMutableSetIterator itResourceEntry(_resources);
            while(itResourceEntry.hasNext())
            {
                itResourceEntry.next();

                const auto doRemove = (std::get<0>(itResourceEntry.value()) == 0);
                if(doRemove)
                {
                    // Erase all the links
                    const auto& levels = std::get<2>(itResourceEntry.value());
                    for(auto itLevel = levels.cbegin(); itLevel != levels.cend(); ++itLevel)
                        _container[*itLevel].remove(itResourceEntry);

                    // Remove the resource entry
                    itResourceEntry.remove();
                }
            }
        }
    };
}

#endif // _OSMAND_CORE_SHARED_BY_ZOOM_RESOURCES_CONTAINER_H_
