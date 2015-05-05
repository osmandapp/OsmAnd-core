#ifndef _OSMAND_CORE_ON_DEMAND_H_
#define _OSMAND_CORE_ON_DEMAND_H_

#include <OsmAndCore/stdlib_common.h>
#include <type_traits>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>

namespace OsmAnd
{
    template<typename T>
    class OnDemand Q_DECL_FINAL
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
        typedef OnDemand<T> OnDemandT;
        typedef std::function<T*()> Allocator;

    private:
    protected:
        template<typename Type, typename OtherType>
        struct Check
        {
            typedef typename std::enable_if <
                std::is_base_of<Type, OtherType>::value ||
                std::is_same<Type, OtherType>::value, OtherType
            > ::type Valid;
        };

        std::shared_ptr<Type> _objectRef;
        Allocator _allocator;

        void allocate()
        {
            if (_allocator)
                _objectRef.reset(_allocator());
            else
                _objectRef.reset(new T());
        }
    public:
        inline OnDemand(const Allocator allocator = nullptr)
            : _allocator(allocator)
        {
        }

        template<typename OtherType, typename Check<Type, OtherType>::Valid* = nullptr>
        inline OnDemand(const OnDemand< OtherType >& that)
            : _objectRef(that._objectRef)
            , _allocator(that._allocator)
        {
        }

#ifdef Q_COMPILER_RVALUE_REFS
        template<typename OtherType, typename Check<Type, OtherType>::Valid* = nullptr>
        inline OnDemand(OnDemand< OtherType >&& that)
            : _objectRef(qMove(that._objectRef))
            , _allocator(that._allocator)
        {
        }
#endif // defined(Q_COMPILER_RVALUE_REFS)

        inline ~OnDemand()
        {
        }

        template<typename OtherType, typename Check<Type, OtherType>::Valid* = nullptr>
        inline bool operator==(const OnDemand< OtherType >& r) const
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

        template<typename OtherType, typename Check<Type, OtherType>::Valid* = nullptr>
        inline bool operator!=(const OnDemand< OtherType >& r) const
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

        template<typename OtherType, typename Check<Type, OtherType>::Valid* = nullptr>
        inline OnDemandT& operator=(OtherType* const pObject)
        {
            _objectRef.reset(pObject);
            return *this;
        }

        template<typename OtherType, typename Check<Type, OtherType>::Valid* = nullptr>
        inline OnDemandT& operator=(const std::shared_ptr< OtherType >& objectRef)
        {
            _objectRef = objectRef;
            return *this;
        }

        template<typename OtherType, typename Check<Type, OtherType>::Valid* = nullptr>
        inline OnDemandT& operator=(const OnDemand< OtherType >& that)
        {
            _objectRef = that._objectRef;
            return *this;
        }

        inline operator bool() const
        {
            return static_cast<bool>(_objectRef);
        }

        inline Type* operator->()
        {
            if (!_objectRef)
                allocate();
            return _objectRef.get();
        }

        inline Type* get()
        {
            if (!_objectRef)
                allocate();
            return _objectRef.get();
        }

        inline operator Type*()
        {
            if (!_objectRef)
                allocate();
            return _objectRef.get();
        }

        inline Type& operator*()
        {
            if (!_objectRef)
                allocate();
            return *_objectRef;
        }

        inline operator std::shared_ptr<Type>()
        {
            if (!_objectRef)
                allocate();
            return _objectRef;
        }

        inline std::shared_ptr<Type> shared_ptr()
        {
            if (!_objectRef)
                allocate();
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

    template<typename OtherType>
    friend class OsmAnd::OnDemand;
    };
}

#endif // !defined(_OSMAND_CORE_ON_DEMAND_H_)
