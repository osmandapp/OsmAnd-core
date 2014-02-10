#include "Memory.h"

void* operator new(std::size_t count)
{
    return OsmAnd::operator new(count);
}

void* operator new[](std::size_t count)
{
    return OsmAnd::operator new[](count);
}

void* operator new(std::size_t count, const std::nothrow_t& tag)
{
    return OsmAnd::operator new(count, tag);
}

void* operator new[](std::size_t count, const std::nothrow_t& tag)
{
    return OsmAnd::operator new[](count, tag);
}

void operator delete(void* ptr)
{
    OsmAnd::operator delete(ptr);
}

void operator delete[](void* ptr)
{
    OsmAnd::operator delete[](ptr);
}

void operator delete(void* ptr, const std::nothrow_t& tag)
{
    OsmAnd::operator delete(ptr, tag);
}

void operator delete[](void* ptr, const std::nothrow_t& tag)
{
    OsmAnd::operator delete[](ptr, tag);
}
