#ifndef _OSMAND_CORE_LIST_MAP_H_
#define _OSMAND_CORE_LIST_MAP_H_

#include <OsmAndCore/stdlib_common.h>
#include <vector>
#include <type_traits>

#include <QtGlobal>

#include <OsmAndCore.h>

namespace OsmAnd
{
    template<typename VALUE>
    class ListMap Q_DECL_FINAL
    {
    public:
        typedef ListMap<VALUE> ListMapT;
        typedef typename std::vector< std::shared_ptr<VALUE> >::size_type KeyType;
        typedef typename std::vector< std::shared_ptr<VALUE> >::size_type SizeType;

    private:
    protected:
        std::vector< std::shared_ptr<VALUE> > _storage;
    public:
        inline ListMap(const SizeType size = 0)
        {
            _storage.reserve(size);
        }

        inline ListMap(const ListMapT& that)
        {
            _storage.reserve(that._storage.size());
            for (const auto& thatItem : that._storage)
            {
                const auto thisItem = std::make_shared<VALUE>();
                *thisItem = *thatItem;
                _storage.push_back(thisItem);
            }
        }

#ifdef Q_COMPILER_RVALUE_REFS
        inline ListMap(ListMapT&& that)
            : _storage(qMove(that._storage))
        {
        }
#endif // Q_COMPILER_RVALUE_REFS

        inline ~ListMap()
        {
        }

        inline ListMapT& operator=(const ListMapT& that)
        {
            if (this != &that)
            {
                _storage.clear();
                _storage.reserve(that._storage.size());
                for (const auto& thatItem : that._storage)
                {
                    const auto thisItem = std::make_shared<VALUE>();
                    *thisItem = *thatItem;
                    _storage.push_back(thisItem);
                }
            }
            return *this;
        }

#ifdef Q_COMPILER_RVALUE_REFS
        inline ListMapT& operator=(ListMapT&& that)
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
            return _storage[key];
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

            auto& entry = _storage[key];
            if (!entry)
                entry = std::make_shared<VALUE>();
            *entry = value;
        }

        inline VALUE* insert(const KeyType key, const VALUE& value)
        {
            if (key >= _storage.size())
                _storage.resize(key + 1);

            auto& entry = _storage[key];
            if (!entry)
                entry = std::make_shared<VALUE>();
            *entry = value;
            return entry.get();
        }

        inline bool get(const KeyType key, VALUE& outValue) const
        {
            if (key >= _storage.size())
                return false;

            const auto& entry = _storage[key];
            if (!entry)
                return false;

            outValue = *entry;
            return true;
        }

        inline const VALUE* getRef(const KeyType key) const
        {
            if (key >= _storage.size())
                return nullptr;

            return _storage[key].get();
        }

        inline VALUE* getRef(const KeyType key)
        {
            if (key >= _storage.size())
                return nullptr;

            return _storage[key].get();
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
                (pValue++)->reset();
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
            auto pEntry = _storage.constData();
            for (SizeType index = 0; index < size; index++)
            {
                if (!*(pEntry++))
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

            auto pEntry = _storage.data() + (size - 1);
            for (SizeType index = 0; index < size; index++)
            {
                if (!*(pEntry--))
                    continue;

                outKey = size - index - 1;
                return true;
            }

            return false;
        }
    };
}

#endif // !defined(_OSMAND_CORE_LIST_MAP_H_)
