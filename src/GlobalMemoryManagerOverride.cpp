#include "Memory.h"

void* operator new(std::size_t count)
{
    const auto ptr = getMemoryManager()->allocate(count, "global");
    if(!ptr)
        throw std::bad_alloc();
    return ptr;
}

void* operator new[](std::size_t count)
{
    const auto ptr = getMemoryManager()->allocate(count, "global");
    if(!ptr)
        throw std::bad_alloc();
    return ptr;
}

void* operator new(std::size_t count, const std::nothrow_t& tag)
{
    return getMemoryManager()->allocate(count, "global");
}

void* operator new[](std::size_t count, const std::nothrow_t& tag)
{
    return getMemoryManager()->allocate(count, "global");
}

void operator delete(void* ptr)
{
    getMemoryManager()->free(ptr, "global");
}

void operator delete[](void* ptr)
{
    getMemoryManager()->free(ptr, "global");
}

void operator delete(void* ptr, const std::nothrow_t& tag)
{
    getMemoryManager()->free(ptr, "global");
}

void operator delete[](void* ptr, const std::nothrow_t& tag)
{
    getMemoryManager()->free(ptr, "global");
}
