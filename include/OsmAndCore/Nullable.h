#ifndef _OSMAND_CORE_NULLABLE_H_
#define _OSMAND_CORE_NULLABLE_H_

#include <OsmAndCore/stdlib_common.h>
#include <type_traits>

#include <QtGlobal>

namespace OsmAnd
{
    template<typename T>
    class Nullable Q_DECL_FINAL
    {
    public:
        typedef Nullable<T> NullableT;

    private:
    protected:
        T _value;
        bool _isSet;
    public:
        inline Nullable(const T* const pValue = nullptr)
            : _isSet(pValue != nullptr)
        {
            if (pValue != nullptr)
                _value = *pValue;
        }

        inline Nullable(const T& value)
            : _value(value)
            , _isSet(true)
        {
        }

#ifdef Q_COMPILER_RVALUE_REFS
        inline Nullable(T&& value)
            : _value(qMove(value))
            , _isSet(true)
        {
        }
#endif // Q_COMPILER_RVALUE_REFS

        inline Nullable(const NullableT& that)
            : _isSet(that._isSet)
        {
            if (_isSet)
                _value = that._value;
        }

#ifdef Q_COMPILER_RVALUE_REFS
        inline Nullable(NullableT&& that)
            : _isSet(that._isSet)
        {
            if (_isSet)
                _value = qMove(that._value);

            that._isSet = false;
        }
#endif // Q_COMPILER_RVALUE_REFS

        inline ~Nullable()
        {
        }

#if !defined(SWIG)
        inline bool operator==(const NullableT& r) const
        {
            if (!_isSet && !r._isSet)
                return true;
            if (_isSet != r._isSet)
                return false;
            return (_value == r._value);
        }

        inline bool operator!=(const NullableT& r) const
        {
            return !(*this == r);
        }

        inline NullableT& operator=(const NullableT& that)
        {
            if (this != &that)
            {
                _isSet = that._isSet;
                if (_isSet)
                    _value = that._value;
            }
            return *this;
        }

#ifdef Q_COMPILER_RVALUE_REFS
        inline NullableT& operator=(NullableT&& that)
        {
            if (this != &that)
            {
                _isSet = that._isSet;
                if (_isSet)
                    _value = qMove(that._value);

                that._isSet = false;
            }
            return *this;
        }
#endif // Q_COMPILER_RVALUE_REFS

        inline NullableT& operator=(const T& value)
        {
            _isSet = true;
            _value = value;
            return *this;
        }

#ifdef Q_COMPILER_RVALUE_REFS
        inline NullableT& operator=(T&& value)
        {
            _isSet = true;
            _value = qMove(value);
            return *this;
        }
#endif // Q_COMPILER_RVALUE_REFS

        explicit inline operator bool() const
        {
            return _isSet;
        }

        inline T& operator*()
        {
            return *getValuePtrOrNullptr();
        }

        inline const T& operator*() const
        {
            return *getValuePtrOrNullptr();
        }

        inline T* operator->()
        {
            return getValuePtrOrNullptr();
        }

        inline const T* operator->() const
        {
            return getValuePtrOrNullptr();
        }
#endif // !defined(SWIG)

        inline T* getValuePtrOrNullptr()
        {
            return _isSet ? &_value : nullptr;
        }

        inline const T* getValuePtrOrNullptr() const
        {
            return _isSet ? &_value : nullptr;
        }

        inline void unset()
        {
            _isSet = false;
        }

        inline bool isSet() const
        {
            return _isSet;
        }
    };
}

#endif // !defined(_OSMAND_CORE_NULLABLE_H_)
