#ifndef _OSMAND_CORE_BITMASK_H_
#define _OSMAND_CORE_BITMASK_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    template<typename FLAGS_ENUM, typename STORAGE = unsigned int>
    class Bitmask Q_DECL_FINAL
    {
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

        inline bool isSet(const FLAGS_ENUM flag) const
        {
            assert(static_cast<unsigned int>(flag) <= sizeof(unsigned int) * 8);
            
            return (_storage & (1u << static_cast<unsigned int>(flag))) != 0;
        }

        inline bool operator&(const FLAGS_ENUM flag) const
        {
            return isSet(flag);
        }

        inline BitmaskT& set(const FLAGS_ENUM flag)
        {
            assert(static_cast<unsigned int>(flag) <= sizeof(unsigned int) * 8);

            _storage |= (STORAGE(1) << static_cast<unsigned int>(flag));

            return *this;
        }

        inline BitmaskT operator|(const FLAGS_ENUM flag) const
        {
            return BitmaskT(*this).set(flag);
        }

        inline BitmaskT& operator|=(const FLAGS_ENUM flag)
        {
            return set(flag);
        }

        inline BitmaskT& unset(const FLAGS_ENUM flag)
        {
            assert(static_cast<unsigned int>(flag) <= sizeof(unsigned int) * 8);

            _storage &= ~(STORAGE(1) << static_cast<unsigned int>(flag));
        }
    };
}

#endif // !defined(_OSMAND_CORE_BITMASK_H_)
