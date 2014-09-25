#ifndef _OSMAND_CORE_BITMASK_H_
#define _OSMAND_CORE_BITMASK_H_

#include <OsmAndCore/stdlib_common.h>
#include <type_traits>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    template<typename FLAGS_ENUM, typename STORAGE = unsigned int>
    class Bitmask Q_DECL_FINAL
    {
        static_assert(std::is_integral<STORAGE>::value, "STORAGE has to be integral type");

    public:
        typedef Bitmask<FLAGS_ENUM, STORAGE> BitmaskT;

    private:
    protected:
        STORAGE _storage;
    public:
        inline Bitmask()
            : _storage(0)
        {
        }

        inline Bitmask(const STORAGE storage_)
            : _storage(storage_)
        {
        }

        inline Bitmask(const BitmaskT& that)
            : _storage(that._storage)
        {
        }

        inline ~Bitmask()
        {
        }

#if !defined(SWIG)
        inline bool operator==(const BitmaskT& r) const
        {
            return (_storage == r._storage);
        }

        inline bool operator!=(const BitmaskT& r) const
        {
            return (_storage != r._storage);
        }

        inline BitmaskT& operator=(const BitmaskT& that)
        {
            if (this != &that)
            {
                _storage = that._storage;
            }
            return *this;
        }

        inline BitmaskT& operator=(const STORAGE storage_)
        {
            _storage = storage_;
            return *this;
        }

        inline operator STORAGE() const
        {
            return _storage;
        }
#endif // !defined(SWIG)

        inline bool isSet(const FLAGS_ENUM flag) const
        {
            assert(static_cast<unsigned int>(flag) <= sizeof(unsigned int) * 8);
            
            return (_storage & (1u << static_cast<unsigned int>(flag))) != 0;
        }

#if !defined(SWIG)
        inline bool operator&(const FLAGS_ENUM flag) const
        {
            return isSet(flag);
        }
#endif // !defined(SWIG)

        inline BitmaskT& set(const FLAGS_ENUM flag)
        {
            assert(static_cast<unsigned int>(flag) <= sizeof(unsigned int) * 8);

            _storage |= (STORAGE(1) << static_cast<unsigned int>(flag));

            return *this;
        }

#if !defined(SWIG)
        inline BitmaskT operator|(const FLAGS_ENUM flag) const
        {
            return BitmaskT(*this).set(flag);
        }

        inline BitmaskT& operator|=(const FLAGS_ENUM flag)
        {
            return set(flag);
        }
#endif // !defined(SWIG)

        inline BitmaskT& unset(const FLAGS_ENUM flag)
        {
            assert(static_cast<unsigned int>(flag) <= sizeof(unsigned int) * 8);

            _storage &= ~(STORAGE(1) << static_cast<unsigned int>(flag));

            return *this;
        }

        inline BitmaskT& unite(const BitmaskT& otherMask)
        {
            _storage |= otherMask._storage;

            return *this;
        }

#if !defined(SWIG)
        inline BitmaskT operator+(const BitmaskT& otherMask) const
        {
            return BitmaskT(*this).unite(otherMask);
        }

        inline BitmaskT& operator+=(const BitmaskT& otherMask)
        {
            return unite(otherMask);
        }
#endif // !defined(SWIG)
    };
}

#endif // !defined(_OSMAND_CORE_BITMASK_H_)
