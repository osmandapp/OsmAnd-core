#ifndef _OSMAND_CORE_OBSERVABLE_H_
#define _OSMAND_CORE_OBSERVABLE_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QHash>
#include <QReadWriteLock>

#include <OsmAndCore.h>

namespace OsmAnd
{
    template<typename... ARGS>
    class Observable Q_DECL_FINAL
    {
        Q_DISABLE_COPY(Observable);
    public:
        typedef std::function<void (ARGS...)> Handler;
        typedef const void* Tag;

    private:
        mutable QReadWriteLock _observersLock;
        mutable QHash<Tag, Handler> _observers;
    public:
        Observable()
        {
        }
        ~Observable()
        {
        }

        bool attach(const Tag tag, const Handler handler) const
        {
            QWriteLocker scopedLocker(&_observersLock);

            if(_observers.contains(tag) || handler == null)
                return false;

            _observers.insert(tag, handler);

            return true;
        }

        bool detach(const Tag tag) const
        {
            QWriteLocker scopedLocker(&_observersLock);

            return (_observers.remove(tag) > 0);
        }

        void notify(ARGS... args) const
        {
            QHash<Tag, Handler> observers;
            {
                QReadLocker scopedLocker(&_observersLock);
                observers = _observers;
                observers.detach();
            }

            for(const auto& handler : constOf(observers))
                handler(args...);
        }
    };
}

#endif // !defined(_OSMAND_CORE_OBSERVABLE_H_)
