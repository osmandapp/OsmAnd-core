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
    class AccessLockCounter_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(AccessLockCounter_P)
    private:
        mutable QMutex _lockCounterMutex;
        mutable QWaitCondition _lockCounterWaitCondition;
        mutable volatile int _lockCounter;

        mutable volatile bool _isBeingDestroyed;
    protected:
        AccessLockCounter_P(AccessLockCounter* const owner);

        void notifyAboutDestruction();
    public:
        virtual ~AccessLockCounter_P();

        ImplementationInterface<AccessLockCounter> owner;

        bool tryLockForReading() const;
        bool lockForReading() const;
        void unlockFromReading() const;

        bool tryLockForWriting() const;
        bool lockForWriting() const;
        void unlockFromWriting() const;

    friend class OsmAnd::AccessLockCounter;
    };
}

#endif // !defined(_OSMAND_CORE_ACCESS_LOCK_COUNTER_P_H_)
