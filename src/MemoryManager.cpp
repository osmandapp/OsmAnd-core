#include "MemoryManager.h"

#include <cstdlib>
#include <cassert>

OsmAnd::MemoryManager::MemoryManager()
{
}

OsmAnd::MemoryManager::~MemoryManager()
{
    //NOTE: Destructor will never be called due to known memory leak
    assert(false);
}

void* OsmAnd::MemoryManager::allocate(std::size_t size, const char* tag)
{
    return std::malloc(size);
}

void OsmAnd::MemoryManager::free(void* ptr, const char* tag)
{
    std::free(ptr);
}