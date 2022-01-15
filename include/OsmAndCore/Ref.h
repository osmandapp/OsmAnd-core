#ifndef _OSMAND_CORE_REF_H_
#define _OSMAND_CORE_REF_H_

#include <OsmAndCore/stdlib_common.h>
#include <type_traits>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>

namespace OsmAnd
{
    template<typename T>
    class Ref Q_DECL_FINAL
    {
    public:
        // Ensure that T is correct
        static_assert(std::is_void<T>::value == 0, "T must be non-void");
        static_assert(std::is_pointer<T>::value == 0, "T must be non-pointer");
        static_assert(std::is_const<T>::value == 0, "T must be pure type: remove const");
        static_assert(std::is_volatile<T>::value == 0, "T must be pure type: remove volatile");
        static_assert(std::is_lvalue_reference<T>::value == 0, "T must be pure type: remove lvalue reference");
        static_assert(std::is_rvalue_reference<T>::value == 0, "T must be pure type: remove rvalue reference");

        typedef T Type;
        typedef typename std::add_const<T>::type ConstType;
        typedef Ref<T> RefT;

    private:
    protected:
        template<typename Type, typename OtherType>
        struct Check
        {
            typedef typename std::enable_if<
                std::is_base_of<Type, OtherType>::value ||
                std::is_same<Type, OtherType>::value, OtherType
            >::type Valid;
        };

        std::shared_ptr<Type> _objectRef;
    public:
        inline Ref()
        {
        }

        inline Ref(std::nullptr_t)
        {
        }

        template<typename OtherType, typename Check<Type, OtherType>::Valid* = nullptr>
            inline Ref(OtherType* const pObject)
            : _objectRef(pObject)
        {
        }

        template<typename OtherType, typename Check<Type, OtherType>::Valid* = nullptr>
        inline Ref(const std::shared_ptr< OtherType >& objectRef)
            : _objectRef(objectRef)
        {
        }

#ifdef Q_COMPILER_RVALUE_REFS
        template<typename OtherType, typename Check<Type, OtherType>::Valid* = nullptr>
        inline Ref(std::shared_ptr< OtherType >&& objectRef)
            : _objectRef(qMove(objectRef))
        {
        }
#endif // defined(Q_COMPILER_RVALUE_REFS)

        template<typename OtherType, typename Check<Type, OtherType>::Valid* = nullptr>
        inline Ref(const Ref< OtherType >& that)
            : _objectRef(that._objectRef)
        {
        }

#ifdef Q_COMPILER_RVALUE_REFS
        template<typename OtherType, typename Check<Type, OtherType>::Valid* = nullptr>
        inline Ref(Ref< OtherType >&& that)
            : _objectRef(qMove(that._objectRef))
        {
        }
#endif // defined(Q_COMPILER_RVALUE_REFS)

        inline ~Ref()
        {
        }

        template<typename OtherType, typename Check<Type, OtherType>::Valid* = nullptr>
        inline bool operator==(const Ref< OtherType >& r) const
        {
            return (_objectRef == r._objectRef);
        }

        template<typename OtherType, typename Check<Type, OtherType>::Valid* = nullptr>
        inline bool operator==(const std::shared_ptr< const OtherType >& rObjectRef) const
        {
            return (_objectRef == rObjectRef);
        }

        template<typename OtherType, typename Check<Type, OtherType>::Valid* = nullptr>
        inline bool operator==(const OtherType* const pObject) const
        {
            return (_objectRef.get() == pObject);
        }

        inline bool operator==(std::nullptr_t) const
        {
            return static_cast<bool>(_objectRef);
        }

        template<typename OtherType, typename Check<Type, OtherType>::Valid* = nullptr>
        inline bool operator!=(const Ref< OtherType >& r) const
        {
            return (_objectRef != r._objectRef);
        }

        template<typename OtherType, typename Check<Type, OtherType>::Valid* = nullptr>
        inline bool operator!=(const std::shared_ptr< const OtherType >& rObjectRef) const
        {
            return (_objectRef != rObjectRef);
        }

        template<typename OtherType, typename Check<Type, OtherType>::Valid* = nullptr>
        inline bool operator!=(const OtherType* const pObject) const
        {
            return (_objectRef.get() == pObject);
        }

        inline bool operator!=(std::nullptr_t) const
        {
            return !static_cast<bool>(_objectRef);
        }

        template<typename OtherType, typename Check<Type, OtherType>::Valid* = nullptr>
        inline RefT& operator=(OtherType* const pObject)
        {
            _objectRef.reset(pObject);
            return *this;
        }

        template<typename OtherType, typename Check<Type, OtherType>::Valid* = nullptr>
        inline RefT& operator=(const std::shared_ptr< OtherType >& objectRef)
        {
            _objectRef = objectRef;
            return *this;
        }

        template<typename OtherType, typename Check<Type, OtherType>::Valid* = nullptr>
        inline RefT& operator=(const Ref< OtherType >& that)
        {
            _objectRef = that._objectRef;
            return *this;
        }

        inline RefT& operator=(std::nullptr_t)
        {
            _objectRef.reset();
            return *this;
        }

        inline operator bool() const
        {
            return static_cast<bool>(_objectRef);
        }

        inline Type* operator->()
        {
            return _objectRef.get();
        }

        inline Type* get()
        {
            return _objectRef.get();
        }

        inline operator Type*()
        {
            return _objectRef.get();
        }

        inline Type& operator*()
        {
            return *_objectRef;
        }

        inline operator std::shared_ptr<Type>()
        {
            return _objectRef;
        }

        inline std::shared_ptr<Type> shared_ptr()
        {
            return _objectRef;
        }

        inline ConstType* operator->() const
        {
            return const_cast<ConstType*>(_objectRef.get());
        }

        inline ConstType* get() const
        {
            return const_cast<ConstType*>(_objectRef.get());
        }

        inline operator ConstType*() const
        {
            return const_cast<ConstType*>(_objectRef.get());
        }

        inline ConstType& operator*() const
        {
            return *_objectRef;
        }

        inline operator std::shared_ptr<ConstType>() const
        {
            return _objectRef;
        }

        inline std::shared_ptr<ConstType> shared_ptr() const
        {
            return _objectRef;
        }

#if !defined(SWIG)
#   ifdef Q_COMPILER_RVALUE_REFS
        template<typename... Args>
        static inline RefT New(Args&&... args)
        {
            return RefT(new T(std::forward<Args>(args)...));
        }
#   else
        template<typename... Args>
        static inline RefT New(const Args&... args)
        {
            return RefT(new T(args...));
        }
#   endif
#endif // !defined(SWIG)

    template<typename OtherType>
    friend class OsmAnd::Ref;
    };
}

#endif // !defined(_OSMAND_CORE_REF_H_)
