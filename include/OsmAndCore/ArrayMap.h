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
        inline ArrayMap(const SizeType size = 0)
        {
            _storage.reserve(size);
        }

        inline ArrayMap(const ArrayMapT& that)
            : _storage(that._storage)
        {
        }

#ifdef Q_COMPILER_RVALUE_REFS
        inline ArrayMap(ArrayMapT&& that)
            : _storage(qMove(that._storage))
        {
        }
#endif // Q_COMPILER_RVALUE_REFS

        inline ~ArrayMap()
        {
        }

        inline ArrayMapT& operator=(const ArrayMapT& that)
        {
            if (this != &that)
                _storage = that._storage;
            return *this;
        }

#ifdef Q_COMPILER_RVALUE_REFS
        inline ArrayMapT& operator=(ArrayMapT&& that)
        {
            if (this != &that)
                _storage = qMove(that._storage);
            return *this;
        }
#endif // Q_COMPILER_RVALUE_REFS

        inline bool contains(const KeyType key) const
        {
            if (key >= _storage.size())
                return false;
            return _storage[key].isSet();
        }

        inline const VALUE& operator[](const KeyType key) const
        {
            return *_storage[key];
        }

        inline VALUE& operator[](const KeyType key)
        {
            return *_storage[key];
        }

        inline void set(const KeyType key, const VALUE& value)
        {
            if (key >= _storage.size())
                _storage.resize(key + 1);

            _storage[key] = value;
        }

        inline VALUE* insert(const KeyType key, const VALUE& value)
        {
            if (key >= _storage.size())
                _storage.resize(key + 1);

            auto& entry = _storage[key];
            entry = value;
            return entry.getValuePtrOrNullptr();
        }

        inline bool get(const KeyType key, VALUE& outValue) const
        {
            if (key >= _storage.size())
                return false;

            const auto& value = _storage[key];
            if (!value.isSet())
                return false;

            outValue = *value;
            return true;
        }

        inline const VALUE* getRef(const KeyType key) const
        {
            if (key >= _storage.size())
                return nullptr;

            return _storage[key].getValuePtrOrNullptr();
        }

        inline VALUE* getRef(const KeyType key)
        {
            if (key >= _storage.size())
                return nullptr;

            return _storage[key].getValuePtrOrNullptr();
        }

        inline void reserve(const SizeType size)
        {
            if (_storage.size() >= size)
                return;

            _storage.resize(size);
        }

        inline void reset()
        {
            _storage.clear();
        }

        inline void clear()
        {
            const auto size = _storage.size();
            auto pValue = _storage.data();
            for (SizeType index = 0; index < size; index++)
                (pValue++)->unset();
        }

        inline SizeType size() const
        {
            return _storage.size();
        }

        inline bool isEmpty() const
        {
            return _storage.empty();
        }

        inline bool empty() const
        {
            return isEmpty();
        }

        inline void squeeze()
        {
            _storage.shrink_to_fit();
        }

        inline bool findMinKey(KeyType& outKey) const
        {
            const auto size = _storage.size();
            auto pValue = _storage.constData();
            for (SizeType index = 0; index < size; index++)
            {
                if (!(pValue++)->isSet())
                    continue;

                outKey = index;
                return true;
            }

            return false;
        }

        inline bool findMaxKey(KeyType& outKey) const
        {
            const auto size = _storage.size();
            if (size == 0)
                return false;

            auto pValue = _storage.data() + (size - 1);
            for (SizeType index = 0; index < size; index++)
            {
                if (!(pValue--)->isSet())
                    continue;

                outKey = size - index - 1;
                return true;
            }

            return false;
        }
    };
}

#endif // !defined(_OSMAND_CORE_ARRAY_MAP_H_)
