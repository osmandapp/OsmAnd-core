#include "IMemoryManager.h"
#include "MemoryManager.h"

#include <cstdlib>
#include <new>

OsmAnd::IMemoryManager::IMemoryManager()
{
}

OsmAnd::IMemoryManager::~IMemoryManager()
{
}

OsmAnd::IMemoryManager* OsmAnd::getMemoryManager()
{
    //NOTE: Known memory leak, manager will never be deallocated. Reason for such solution is that order of static
    //      variables destruction is undefined.
    static IMemoryManager* const pManager = new(std::malloc(sizeof(MemoryManager))) MemoryManager();
    return pManager;
}
