#ifndef _OSMAND_CORE_SMART_POD_H_
#define _OSMAND_CORE_SMART_POD_H_

#include <QtGlobal>

namespace OsmAnd
{
    template<typename T, T DEFAULT_VALUE>
    struct SmartPOD
    {
        typedef SmartPOD<T, DEFAULT_VALUE> SmartPODT;

        inline SmartPOD()
            : value(DEFAULT_VALUE)
        {
        }

        explicit inline SmartPOD(const T& value_)
            : value(value_)
        {
        }

        virtual ~SmartPOD()
        {
        }

        T value;

        inline operator T() const
        {
            return value;
        }

        inline operator T&()
        {
            return value;
        }

        inline bool operator==(const SmartPODT& r) const
        {
            return value == r.value;
        }

        inline bool operator!=(const SmartPODT& r) const
        {
            return value != r.value;
        }

        inline SmartPODT& operator=(const SmartPODT& that)
        {
            value = that.value;
            return *this;
        }

        inline SmartPODT& operator=(const T& that)
        {
            value = that;
            return *this;
        }

        inline T& operator*()
        {
            return value;
        }

        inline const T& operator*() const
        {
            return value;
        }

        inline T* operator&()
        {
            return &value;
        }

        inline const T* operator&() const
        {
            return &value;
        }

        inline void reset()
        {
            value = DEFAULT_VALUE;
        }
    };
}

#endif // !defined(_OSMAND_CORE_SMART_POD_H_)
