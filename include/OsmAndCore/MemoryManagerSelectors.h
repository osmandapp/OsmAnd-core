#ifndef _OSMAND_CORE_MEMORY_MANAGER_SELECTORS_H_
#define _OSMAND_CORE_MEMORY_MANAGER_SELECTORS_H_

#include <OsmAndCore/stdlib_common.h>
#include <new>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/IMemoryManager.h>

namespace OsmAnd
{
    template<class CLASS>
    struct MemoryManagerSelector
    {
        static IMemoryManager* get()
        {
            return getMemoryManager();
        }
    };

    namespace Model
    {
        class MapObject;
    }
    template<>
    struct MemoryManagerSelector<OsmAnd::Model::MapObject>
    {
        static IMemoryManager* get()
        {
            return getMemoryManager();
        }
    };
}

#endif // !defined(_OSMAND_CORE_MEMORY_MANAGER_SELECTORS_H_)
