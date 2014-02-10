#ifndef _OSMAND_CORE_MEMORY_H_
#define _OSMAND_CORE_MEMORY_H_

#include <memory>
#include <new>

#include <OsmAndCore/IMemoryManager.h>
#include <OsmAndCore/MemoryManagerSelector.h>

namespace OsmAnd
{
    inline void* operator new(std::size_t count)
    {
        const auto ptr = getMemoryManager()->allocate(count, "global");
        if(!ptr)
            throw std::bad_alloc();
        return ptr;
    }

    inline void* operator new[](std::size_t count)
    {
        const auto ptr = getMemoryManager()->allocate(count, "global");
        if(!ptr)
            throw std::bad_alloc();
        return ptr;
    }

    inline void* operator new  (std::size_t count, const std::nothrow_t& tag)
    {
        return getMemoryManager()->allocate(count, "global");
    }

    inline void* operator new[](std::size_t count, const std::nothrow_t& tag)
    {
        return getMemoryManager()->allocate(count, "global");
    }

    inline void operator delete(void* ptr)
    {
        getMemoryManager()->free(ptr, "global");
    }

    inline void operator delete[](void* ptr)
    {
        getMemoryManager()->free(ptr, "global");
    }

    inline void operator delete(void* ptr, const std::nothrow_t& tag)
    {
        getMemoryManager()->free(ptr, "global");
    }

    inline void operator delete[](void* ptr, const std::nothrow_t& tag)
    {
        getMemoryManager()->free(ptr, "global");
    }
}

#define OSMAND_USE_MEMORY_MANAGER(class_name)                                                                       \
    public:                                                                                                         \
        static void* operator new(std::size_t count)                                                                \
        {                                                                                                           \
            const auto ptr = OsmAnd::MemoryManagerSelector<class_name>()->allocate(count, #class_name);             \
            if(!ptr)                                                                                                \
                throw std::bad_alloc();                                                                             \
            return ptr;                                                                                             \
        }                                                                                                           \
        static void* operator new[](std::size_t count)                                                              \
        {                                                                                                           \
            const auto ptr = OsmAnd::MemoryManagerSelector<class_name>::get()->allocate(count, #class_name);        \
            if(!ptr)                                                                                                \
                throw std::bad_alloc();                                                                             \
            return ptr;                                                                                             \
        }                                                                                                           \
        static void* operator new(std::size_t count, const std::nothrow_t& tag)                                     \
        {                                                                                                           \
            return OsmAnd::MemoryManagerSelector<class_name>::get()->allocate(count, #class_name);                  \
        }                                                                                                           \
        static void* operator new[](std::size_t count, const std::nothrow_t& tag)                                   \
        {                                                                                                           \
            return OsmAnd::MemoryManagerSelector<class_name>::get()->allocate(count, #class_name);                  \
        }                                                                                                           \
        /*virtual*/ void operator delete(void* ptr)                                                                 \
        {                                                                                                           \
            OsmAnd::MemoryManagerSelector<class_name>::get()->free(ptr, #class_name);                               \
        }                                                                                                           \
        /*virtual*/ void operator delete[](void* ptr)                                                               \
        {                                                                                                           \
            OsmAnd::MemoryManagerSelector<class_name>::get()->free(ptr, #class_name);                               \
        }                                                                                                           \
        /*virtual*/ void operator delete(void* ptr, const std::nothrow_t& tag)                                      \
        {                                                                                                           \
            OsmAnd::MemoryManagerSelector<class_name>::get()->free(ptr, #class_name);                               \
        }                                                                                                           \
        /*virtual*/ void operator delete[](void* ptr, const std::nothrow_t& tag)                                    \
        {                                                                                                           \
            OsmAnd::MemoryManagerSelector<class_name>::get()->free(ptr, #class_name);                               \
        }

//namespace OsmAnd
//{
//    template<class C>
//    class Fwd Q_DECL_FINAL
//    {
//    private:
//        Fwd();
//        ~Fwd();
//    public:
//        typedef C* Ptr;
//        typedef const C* PtrC;
//        typedef std::shared_ptr< C > Ref;
//        typedef std::shared_ptr< const C > RefC;
//        typedef std::weak_ptr< C > WeakRef;
//        typedef std::weak_ptr< const C > WeakRefC;
//    };
//}
//
////#if !defined(SWIG)
//#   define OSMAND_DECLARE_REFERENCEABLE_CLASS(name)                                                                 \
//        public:                                                                                                     \
//            typedef name* Ptr;                                                                                      \
//            typedef const name* PtrC;                                                                               \
//            typedef std::shared_ptr< name > Ref;                                                                    \
//            typedef std::shared_ptr< const name > RefC;                                                             \
//            typedef std::weak_ptr< name > WeakRef;                                                                  \
//            typedef std::weak_ptr< const name > WeakRefC;                                                           \
//                                                                                                                    \
//            inline Ref getRef()                                                                                     \
//            {                                                                                                       \
//                return shared_from_this();                                                                          \
//            }                                                                                                       \
//            inline RefC getRef() const                                                                              \
//            {                                                                                                       \
//                return shared_from_this();                                                                          \
//            }
//#   define OSMAND_REFERENCEABLE_CLASS(name) name : public std::enable_shared_from_this< name >
//#else
//#   define OSMAND_DECLARE_REFERENCEABLE_CLASS(name)
//#   define OSMAND_REFERENCEABLE_CLASS(name) name
//#endif

#endif // !defined(_OSMAND_CORE_MEMORY_H_)
