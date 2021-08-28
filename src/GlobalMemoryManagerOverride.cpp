#include "MemoryCommon.h"

#include <QtGlobal>

void* operator new(std::size_t count)
{
    const auto ptr = OsmAnd::getMemoryManager()->allocate(count, "global");
    if (!ptr)
        throw std::bad_alloc();
    return ptr;
}

void* operator new[](std::size_t count)
{
    const auto ptr = OsmAnd::getMemoryManager()->allocate(count, "global");
    if (!ptr)
        throw std::bad_alloc();
    return ptr;
}

void* operator new(std::size_t count, const std::nothrow_t& tag) Q_DECL_NOTHROW
{
    Q_UNUSED(tag);
    return OsmAnd::getMemoryManager()->allocate(count, "global");
}

void* operator new[](std::size_t count, const std::nothrow_t& tag) Q_DECL_NOTHROW
{
    Q_UNUSED(tag);
    return OsmAnd::getMemoryManager()->allocate(count, "global");
}

void operator delete(void* ptr) Q_DECL_NOTHROW
{
    OsmAnd::getMemoryManager()->free(ptr, "global");
}

void operator delete[](void* ptr) Q_DECL_NOTHROW
{
    OsmAnd::getMemoryManager()->free(ptr, "global");
}

void operator delete(void* ptr, const std::nothrow_t& tag) Q_DECL_NOTHROW
{
    Q_UNUSED(tag);
    OsmAnd::getMemoryManager()->free(ptr, "global");
}

void operator delete[](void* ptr, const std::nothrow_t& tag) Q_DECL_NOTHROW
{
    Q_UNUSED(tag);
    OsmAnd::getMemoryManager()->free(ptr, "global");
}
