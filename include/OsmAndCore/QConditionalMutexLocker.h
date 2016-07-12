#ifndef _OSMAND_CORE_Q_CONDITIONAL_MUTEX_LOCKER_H_
#define _OSMAND_CORE_Q_CONDITIONAL_MUTEX_LOCKER_H_

#include <OsmAndCore/QtExtensions.h>
#include <QMutex>

namespace OsmAnd
{
    class OSMAND_CORE_API QConditionalMutexLocker Q_DECL_FINAL
    {
    public:
        Q_DISABLE_COPY_AND_MOVE(QConditionalMutexLocker);

    private:
        QMutex* const _mutex;
        const bool _shouldUnlock;
    public:
        inline QConditionalMutexLocker(QMutex* const mutex_, const bool shouldLock_)
            : _mutex(mutex_)
            , _shouldUnlock(shouldLock_)
        {
            if (shouldLock_)
                _mutex->lock();
        }

        inline ~QConditionalMutexLocker()
        {
            if (_shouldUnlock)
                _mutex->unlock();
        }
    };
}

#endif // !defined(_OSMAND_CORE_Q_CONDITIONAL_MUTEX_LOCKER_H_)
