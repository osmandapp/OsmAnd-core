#ifndef _OSMAND_CORE_I_MEMORY_MANAGER_H_
#define _OSMAND_CORE_I_MEMORY_MANAGER_H_

#include <OsmAndCore/stdlib_common.h>
#include <new>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IMemoryManager
    {
        Q_DISABLE_COPY_AND_MOVE(IMemoryManager);
    private:
    protected:
        IMemoryManager();
    public:
        virtual ~IMemoryManager();

        virtual void* allocate(std::size_t size, const char* tag) = 0;
        virtual void free(void* ptr, const char* tag) = 0;
    };

    IMemoryManager* getMemoryManager();
}

#endif // !defined(_OSMAND_CORE_I_MEMORY_MANAGER_H_)
