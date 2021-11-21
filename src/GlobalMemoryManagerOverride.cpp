#include "MemoryCommon.h"

#include <QtGlobal>

void* operator new(std::size_t count) Q_DECL_NOEXCEPT_EXPR(false)
{
    const auto ptr = OsmAnd::getMemoryManager()->allocate(count, "global");
    if (!ptr)
        throw std::bad_alloc();
    return ptr;
}

void* operator new[](std::size_t count) Q_DECL_NOEXCEPT_EXPR(false)
{
    const auto ptr = OsmAnd::getMemoryManager()->allocate(count, "global");
    if (!ptr)
        throw std::bad_alloc();
    return ptr;
}

void* operator new(std::size_t count, const std::nothrow_t& tag) Q_DECL_NOEXCEPT
{
    Q_UNUSED(tag);
    return OsmAnd::getMemoryManager()->allocate(count, "global");
}

void* operator new[](std::size_t count, const std::nothrow_t& tag) Q_DECL_NOEXCEPT
{
    Q_UNUSED(tag);
    return OsmAnd::getMemoryManager()->allocate(count, "global");
}

void operator delete(void* ptr) Q_DECL_NOEXCEPT
{
    OsmAnd::getMemoryManager()->free(ptr, "global");
}

void operator delete[](void* ptr) Q_DECL_NOEXCEPT
{
    OsmAnd::getMemoryManager()->free(ptr, "global");
}

void operator delete(void* ptr, const std::nothrow_t& tag) Q_DECL_NOEXCEPT
{
    Q_UNUSED(tag);
    OsmAnd::getMemoryManager()->free(ptr, "global");
}

void operator delete[](void* ptr, const std::nothrow_t& tag) Q_DECL_NOEXCEPT
{
    Q_UNUSED(tag);
    OsmAnd::getMemoryManager()->free(ptr, "global");
}
