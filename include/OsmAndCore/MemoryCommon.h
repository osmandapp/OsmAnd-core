#ifndef _OSMAND_CORE_MEMORY_COMMON_H_
#define _OSMAND_CORE_MEMORY_COMMON_H_

#include <memory>
#include <new>

#include <OsmAndCore/QtExtensions.h>
#include <QtGlobal>

#include <OsmAndCore/IMemoryManager.h>
#include <OsmAndCore/MemoryManagerSelectors.h>

#define OSMAND_USE_MEMORY_MANAGER(class_name)                                                                       \
    public:                                                                                                         \
        static void* operator new(std::size_t count) throw(std::bad_alloc)                                          \
        {                                                                                                           \
            const auto ptr = OsmAnd::MemoryManagerSelector<class_name>::get()->allocate(count, #class_name);        \
            if (!ptr)                                                                                               \
                throw std::bad_alloc();                                                                             \
            return ptr;                                                                                             \
        }                                                                                                           \
        static void* operator new[](std::size_t count) throw(std::bad_alloc)                                        \
        {                                                                                                           \
            const auto ptr = OsmAnd::MemoryManagerSelector<class_name>::get()->allocate(count, #class_name);        \
            if (!ptr)                                                                                               \
                throw std::bad_alloc();                                                                             \
            return ptr;                                                                                             \
        }                                                                                                           \
        static void* operator new(std::size_t count, const std::nothrow_t& tag) Q_DECL_NOTHROW                      \
        {                                                                                                           \
            Q_UNUSED(tag);                                                                                          \
            return OsmAnd::MemoryManagerSelector<class_name>::get()->allocate(count, #class_name);                  \
        }                                                                                                           \
        static void* operator new[](std::size_t count, const std::nothrow_t& tag) Q_DECL_NOTHROW                    \
        {                                                                                                           \
            Q_UNUSED(tag);                                                                                          \
            return OsmAnd::MemoryManagerSelector<class_name>::get()->allocate(count, #class_name);                  \
        }                                                                                                           \
        /*virtual*/ void operator delete(void* ptr) Q_DECL_NOTHROW                                                  \
        {                                                                                                           \
            OsmAnd::MemoryManagerSelector<class_name>::get()->free(ptr, #class_name);                               \
        }                                                                                                           \
        /*virtual*/ void operator delete[](void* ptr) Q_DECL_NOTHROW                                                \
        {                                                                                                           \
            OsmAnd::MemoryManagerSelector<class_name>::get()->free(ptr, #class_name);                               \
        }                                                                                                           \
        /*virtual*/ void operator delete(void* ptr, const std::nothrow_t& tag) Q_DECL_NOTHROW                       \
        {                                                                                                           \
            Q_UNUSED(tag);                                                                                          \
            OsmAnd::MemoryManagerSelector<class_name>::get()->free(ptr, #class_name);                               \
        }                                                                                                           \
        /*virtual*/ void operator delete[](void* ptr, const std::nothrow_t& tag) Q_DECL_NOTHROW                     \
        {                                                                                                           \
            Q_UNUSED(tag);                                                                                          \
            OsmAnd::MemoryManagerSelector<class_name>::get()->free(ptr, #class_name);                               \
        }

#endif // !defined(_OSMAND_CORE_MEMORY_COMMON_H_)
