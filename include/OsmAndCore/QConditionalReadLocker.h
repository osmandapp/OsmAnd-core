#ifndef _OSMAND_CORE_Q_CONDITIONAL_READ_LOCKER_H_
#define _OSMAND_CORE_Q_CONDITIONAL_READ_LOCKER_H_

#include <OsmAndCore/QtExtensions.h>
#include <QReadWriteLock>

namespace OsmAnd
{
    class OSMAND_CORE_API QConditionalReadLocker Q_DECL_FINAL
    {
    public:
        Q_DISABLE_COPY_AND_MOVE(QConditionalReadLocker);

    private:
        QReadWriteLock* const _lock;
        const bool _shouldUnlock;
    public:
        inline QConditionalReadLocker(QReadWriteLock* const lock_, const bool shouldLock_)
            : _lock(lock_)
            , _shouldUnlock(shouldLock_)
        {
            if (shouldLock_)
                _lock->lockForRead();
        }

        inline ~QConditionalReadLocker()
        {
            if (_shouldUnlock)
                _lock->unlock();
        }
    };
}

#endif // !defined(_OSMAND_CORE_Q_CONDITIONAL_READ_LOCKER_H_)
