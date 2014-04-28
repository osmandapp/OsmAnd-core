#ifndef _OSMAND_CORE_ACCESS_LOCK_COUNTER_H_
#define _OSMAND_CORE_ACCESS_LOCK_COUNTER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>

namespace OsmAnd
{
    class AccessLockCounter_P;
    class OSMAND_CORE_API AccessLockCounter
    {
        Q_DISABLE_COPY(AccessLockCounter)
    private:
        PrivateImplementation<AccessLockCounter_P> _p;
    protected:
    public:
        AccessLockCounter();
        virtual ~AccessLockCounter();

        bool tryLockForReading() const;
        bool lockForReading() const;
        void unlockFromReading() const;

        bool tryLockForWriting() const;
        bool lockForWriting() const;
        void unlockFromWriting() const;
    };
}

#endif // !defined(_OSMAND_CORE_ACCESS_LOCK_COUNTER_H_)
