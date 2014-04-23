#ifndef _OSMAND_CORE_ACCESS_LOCK_COUNTER_P_H_
#define _OSMAND_CORE_ACCESS_LOCK_COUNTER_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QMutex>
#include <QWaitCondition>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"

namespace OsmAnd
{
    class AccessLockCounter;
    class AccessLockCounter_P
    {
        Q_DISABLE_COPY(AccessLockCounter_P)
    private:
        mutable QMutex _lockCounterMutex;
        mutable QWaitCondition _lockCounterWaitCondition;
        mutable volatile int _lockCounter;
    protected:
        AccessLockCounter_P(AccessLockCounter* const owner);
    public:
        virtual ~AccessLockCounter_P();

        ImplementationInterface<AccessLockCounter> owner;

        bool tryLockForReading() const;
        void lockForReading() const;
        void unlockFromReading() const;

        bool tryLockForWriting() const;
        void lockForWriting() const;
        void unlockFromWriting() const;

    friend class OsmAnd::AccessLockCounter;
    };
}

#endif // !defined(_OSMAND_CORE_ACCESS_LOCK_COUNTER_P_H_)
