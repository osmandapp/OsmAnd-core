#ifndef _OSMAND_CORE_ARRAY_MAP_H_
#define _OSMAND_CORE_ARRAY_MAP_H_

#include <OsmAndCore/stdlib_common.h>
#include <vector>
#include <type_traits>

#include <QtGlobal>

#include <OsmAndCore.h>
#include <OsmAndCore/Nullable.h>

namespace OsmAnd
{
    template<typename VALUE>
    class ArrayMap Q_DECL_FINAL
    {
    public:
        typedef ArrayMap<VALUE> ArrayMapT;
        typedef Nullable<VALUE> NullableT;
        typedef typename std::vector<NullableT>::size_type KeyType;
        typedef typename std::vector<NullableT>::size_type SizeType;

    private:
    protected:
        std::vector<NullableT> _storage;
    public:
        ArrayMap(const SizeType size = 0)
            : _storage(size)
        {
        }

        ArrayMap(const ArrayMapT& that)
            : _storage(that._storage)
        {
        }

#ifdef Q_COMPILER_RVALUE_REFS
        ArrayMap(ArrayMapT&& that)
            : _storage(that._storage)
        {
        }
#endif // Q_COMPILER_RVALUE_REFS

        ~ArrayMap()
        {
        }

        ArrayMapT& operator=(const ArrayMapT& that)
        {
            if (this != &that)
                _storage = that._storage;
            return *this;
        }

#ifdef Q_COMPILER_RVALUE_REFS
        ArrayMapT& operator=(ArrayMapT&& that)
        {
            if (this != &that)
                _storage = qMove(that._storage);
            return *this;
        }
#endif // Q_COMPILER_RVALUE_REFS

        bool contains(const KeyType key) const
        {
            if (key >= _storage.size())
                return false;
            return _storage[key].isSet();
        }

        const VALUE& operator[](const KeyType key) const
        {
            return *_storage[key];
        }

        VALUE& operator[](const KeyType key)
        {
            return *_storage[key];
        }

        void set(const KeyType key, const VALUE& value)
        {
            if (key >= _storage.size())
                _storage.resize(key + 1);

            _storage[key] = value;
        }

        bool get(const KeyType key, VALUE& outValue) const
        {
            if (key >= _storage.size())
                return false;

            const auto& value = _storage[key];
            if (!value.isSet())
                return false;

            outValue = *value;
            return true;
        }

        const VALUE* getRef(const KeyType key) const
        {
            if (key >= _storage.size())
                return nullptr;

            const auto& value = _storage[key];
            return value.getValuePtrOrNullptr();
        }

        VALUE* getRef(const KeyType key)
        {
            if (key >= _storage.size())
                return nullptr;

            auto& value = _storage[key];
            return value.getValuePtrOrNullptr();
        }

        void reserve(const SizeType size)
        {
            if (_storage.size() >= size)
                return;

            _storage.resize(size);
        }

        void reset()
        {
            _storage.clear();
        }

        void clear()
        {
            const auto size = _storage.size();
            auto pValue = _storage.data();
            for (SizeType index = 0; index < size; index++)
                (pValue++)->unset();
        }

        SizeType size() const
        {
            return _storage.size();
        }
    };
}

#endif // !defined(_OSMAND_CORE_ARRAY_MAP_H_)
