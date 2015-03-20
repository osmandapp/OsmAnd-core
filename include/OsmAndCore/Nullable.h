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
        inline Nullable()
            : _isSet(false)
        {
        }

        inline Nullable(const T& value_)
            : _value(value_)
            , _isSet(true)
        {
        }

        inline Nullable(const NullableT& that)
            : _isSet(that._isSet)
        {
            if (_isSet)
                _value = that._value;
        }

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

        inline NullableT& operator=(const T& value)
        {
            _isSet = true;
            _value = value;
            return *this;
        }

        inline operator bool() const
        {
            return _isSet;
        }
#endif // !defined(SWIG)

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
