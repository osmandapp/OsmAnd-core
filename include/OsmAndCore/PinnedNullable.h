#ifndef _OSMAND_CORE_PINNED_PinnedNullable_H_
#define _OSMAND_CORE_PINNED_PinnedNullable_H_

#include <OsmAndCore/stdlib_common.h>
#include <type_traits>

#include <QtGlobal>

namespace OsmAnd
{
    template<typename T>
    class PinnedNullable Q_DECL_FINAL
    {
    public:
        typedef PinnedNullable<T> PinnedNullableT;

    private:
    protected:
        T* _pValue;
        bool _isSet;

        T& valueRef()
        {
            if (!_pValue)
                _pValue = new T();
            return *_pValue;
        }
    public:
        inline PinnedNullable()
            : _pValue(nullptr)
            , _isSet(false)
        {
        }

        inline PinnedNullable(const T& value)
            : _pValue(nullptr)
            , _isSet(true)
        {
            valueRef() = value;
        }

#ifdef Q_COMPILER_RVALUE_REFS
        inline PinnedNullable(T&& value)
            : _pValue(nullptr)
            , _isSet(true)
        {
            valueRef() = qMove(value);
        }
#endif // Q_COMPILER_RVALUE_REFS

        inline PinnedNullable(const PinnedNullableT& that)
            : _pValue(nullptr)
            , _isSet(that._isSet)
        {
            if (that._isSet)
                valueRef() = *that._pValue;
        }

#ifdef Q_COMPILER_RVALUE_REFS
        inline PinnedNullable(PinnedNullableT&& that)
            : _pValue(nullptr)
            , _isSet(that._isSet)
        {
            if (that._isSet)
            {
                _pValue = that._pValue;
                that._pValue = nullptr;
                that._isSet = false;
            }
        }
#endif // Q_COMPILER_RVALUE_REFS

        inline ~PinnedNullable()
        {
            if (_pValue)
            {
                delete _pValue;
                _pValue = nullptr;
            }
        }

#if !defined(SWIG)
        inline bool operator==(const PinnedNullableT& r) const
        {
            if (!_isSet && !r._isSet)
                return true;
            if (_isSet != r._isSet)
                return false;
            if (_pValue == r._pValue)
                return true;
            return (*_pValue == *r._pValue);
        }

        inline bool operator!=(const PinnedNullableT& r) const
        {
            return !(*this == r);
        }

        inline PinnedNullableT& operator=(const PinnedNullableT& that)
        {
            if (this != &that)
            {
                _isSet = that._isSet;
                if (that._isSet)
                    valueRef() = *that._pValue;
            }
            return *this;
        }

#ifdef Q_COMPILER_RVALUE_REFS
        inline PinnedNullableT& operator=(PinnedNullableT&& that)
        {
            if (this != &that)
            {
                _isSet = that._isSet;
                if (that._isSet)
                {
                    _pValue = that._pValue;
                    that._pValue = nullptr;
                    that._isSet = false;
                }
            }
            return *this;
        }
#endif // Q_COMPILER_RVALUE_REFS

        inline PinnedNullableT& operator=(const T& value)
        {
            _isSet = true;
            valueRef() = value;
            return *this;
        }

#ifdef Q_COMPILER_RVALUE_REFS
        inline PinnedNullableT& operator=(T&& value)
        {
            _isSet = true;
            valueRef() = qMove(value);
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
            return _isSet ? _pValue : nullptr;
        }

        inline const T* getValuePtrOrNullptr() const
        {
            return _isSet ? _pValue : nullptr;
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

#endif // !defined(_OSMAND_CORE_PINNED_PinnedNullable_H_)
