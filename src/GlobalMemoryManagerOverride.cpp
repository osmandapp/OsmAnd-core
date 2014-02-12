#include "Memory.h"

void* operator new(std::size_t count)
{
    const auto ptr = OsmAnd::getMemoryManager()->allocate(count, "global");
    if(!ptr)
        throw std::bad_alloc();
    return ptr;
}

void* operator new[](std::size_t count)
{
    const auto ptr = OsmAnd::getMemoryManager()->allocate(count, "global");
    if(!ptr)
        throw std::bad_alloc();
    return ptr;
}

void* operator new(std::size_t count, const std::nothrow_t& tag)
{
    return OsmAnd::getMemoryManager()->allocate(count, "global");
}

void* operator new[](std::size_t count, const std::nothrow_t& tag)
{
    return OsmAnd::getMemoryManager()->allocate(count, "global");
}

void operator delete(void* ptr)
{
    OsmAnd::getMemoryManager()->free(ptr, "global");
}

void operator delete[](void* ptr)
{
    OsmAnd::getMemoryManager()->free(ptr, "global");
}

void operator delete(void* ptr, const std::nothrow_t& tag)
{
    OsmAnd::getMemoryManager()->free(ptr, "global");
}

void operator delete[](void* ptr, const std::nothrow_t& tag)
{
    OsmAnd::getMemoryManager()->free(ptr, "global");
}
