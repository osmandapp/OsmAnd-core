#ifndef _OSMAND_CORE_MEMORY_MANAGER_H_
#define _OSMAND_CORE_MEMORY_MANAGER_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "IMemoryManager.h"

namespace OsmAnd
{
    class MemoryManager : public IMemoryManager
    {
        Q_DISABLE_COPY(MemoryManager);
    private:
    protected:
    public:
        MemoryManager();
        virtual ~MemoryManager();

        virtual void* allocate(std::size_t size, const char* tag);
        virtual void free(void* ptr, const char* tag);
    };
}

#endif // !defined(_OSMAND_CORE_MEMORY_MANAGER_H_)
