#ifndef _OSMAND_CORE_LINK_H_
#define _OSMAND_CORE_LINK_H_

#include <OsmAndCore/stdlib_common.h>
#include <type_traits>
#include <stdexcept>

#include <OsmAndCore/QtExtensions.h>
#include <QAtomicInt>
#include <QWaitCondition>
#include <QMutex>
#include <QReadWriteLock>

namespace OsmAnd
{
    template<typename ENTITY>
    class Link Q_DECL_FINAL : public std::enable_shared_from_this < Link<ENTITY> >
    {
        Q_DISABLE_COPY_AND_MOVE(Link)

    public:
        typedef Link<ENTITY> LinkT;
        class WeakEnd;
        template<typename ENTITY_> class LinkLock;
        typedef LinkLock<ENTITY> LinkLockT;

        template<typename ENTITY_>
        class LinkLock Q_DECL_FINAL
        {
        private:
            const std::shared_ptr<LinkT> _link;
            const bool _locked;

            LinkLockT& operator=(const LinkLockT& other) Q_DECL_EQ_DELETE;
        protected:
            LinkLock(const std::weak_ptr<LinkT>& link)
                : _link(link.lock())
                , _locked(_link && _link->lock())
            {
            }
        public:
            LinkLock(const LinkLockT& other)
                : _link(other._link)
                , _locked(_link && _link->lock())
            {
            }

            ~LinkLock()
            {
                if (_locked)
                    _link->unlock();
            }

            inline operator bool() const
            {
                return _locked;
            }

            inline ENTITY_ operator->() const
            {
                return _link->_linkedEntity;
            }

        friend class WeakEnd;
        };

        class WeakEnd Q_DECL_FINAL
        {
        private:
            std::weak_ptr<LinkT> _linkWeakReference;
        protected:
        public:
            WeakEnd()
            {
            }

            WeakEnd(LinkT& link)
                : _linkWeakReference(link.shared_from_this())
            {
            }

            WeakEnd(const WeakEnd& other)
                : _linkWeakReference(other._linkWeakReference)
            {
            }

            ~WeakEnd()
            {
            }

            inline WeakEnd& operator=(const WeakEnd& other)
            {
                _linkWeakReference = other._linkWeakReference;

                return *this;
            }

            inline operator bool() const
            {
                return !_linkWeakReference.expired();
            }

            inline const LinkLockT lock() const
            {
                return LinkLockT(_linkWeakReference);
            }

            inline void reset()
            {
                _linkWeakReference.reset();
            }
        };

    private:
    protected:
        volatile bool _isLockable;
        mutable QReadWriteLock _isLockableLock;
        QAtomicInt _locksCounter;

        mutable QMutex _lockCounterDecrementedMutex;
        QWaitCondition _lockCounterDecremented;

        inline bool lock()
        {
            QReadLocker scopedLocker(&_isLockableLock);

            if (!_isLockable)
                return false;

            _locksCounter.fetchAndAddOrdered(1);
            return true;
        }

        inline void unlock()
        {
            const auto prevLocksCount = _locksCounter.fetchAndSubOrdered(1);

            // Previous locks count was less or equal to zero - unpaired unlock() call
            if (prevLocksCount <= 0)
            {
                assert(false);
                throw new std::runtime_error("Previous locks count was less or equal to zero - unpaired unlock() call");
            }

            QMutexLocker scopedLocker(&_lockCounterDecrementedMutex);
            _lockCounterDecremented.wakeAll();
        }

        const ENTITY _linkedEntity;
    public:
        Link(ENTITY linkedEntity)
            : _isLockable(true)
            , _linkedEntity(linkedEntity)
        {
        }

        ~Link()
        {
            QWriteLocker scopedLocker(&_isLockableLock);

            // Cause crash if link is destroyed while not released properly
            if (_isLockable)
            {
                assert(false);
                throw new std::runtime_error("Link is destroyed while not released properly");
            }
        }

        void release()
        {
            QWriteLocker scopedLocker(&_isLockableLock);
            _isLockable = false;

            // Wait until all ref-counters are released
            while (_locksCounter.loadAcquire() > 0)
            {
                QMutexLocker scopedLocker(&_lockCounterDecrementedMutex);
                REPEAT_UNTIL(_lockCounterDecremented.wait(&_lockCounterDecrementedMutex));
            }
        }

        inline operator WeakEnd()
        {
            return WeakEnd(*this);
        }

        inline WeakEnd getWeak()
        {
            return WeakEnd(*this);
        }
    };
}

#endif // !defined(_OSMAND_CORE_LINK_H_)
