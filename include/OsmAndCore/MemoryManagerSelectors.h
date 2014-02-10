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
    struct getMemoryManager
    {
        static operator()
        {
            return getMemoryManager();
        }
    };

    namespace Model
    {
        class MapObject;
    }
    template<>
    struct getMemoryManager<OsmAnd::Model::MapObject>
    {
        static operator()
        {
            return getMemoryManager();
        }
    };
}

#endif // !defined(_OSMAND_CORE_MEMORY_MANAGER_SELECTORS_H_)
