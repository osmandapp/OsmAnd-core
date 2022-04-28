#ifndef _OSMAND_CORE_OBSERVABLE_H_
#define _OSMAND_CORE_OBSERVABLE_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QHash>
#include <QReadWriteLock>
#include <QThreadPool>
#include <QRunnable>
#include <OsmAndCore/QtCommon.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    class IObservable
    {
        Q_DISABLE_COPY_AND_MOVE(IObservable);

    public:
        static_assert(sizeof(int64_t) >= sizeof(intptr_t), "int64_t can't fit a pointer");
        typedef int64_t Tag;

        IObservable()
        {
        }
        virtual ~IObservable()
        {
        }
    };

#if !defined(SWIG)
    template<typename... ARGS>
    class Observable : public IObservable
    {
        Q_DISABLE_COPY_AND_MOVE(Observable);
    public:
        typedef std::function<void (ARGS...)> Handler;
        typedef IObservable::Tag Tag;

    private:
        mutable QReadWriteLock _observersLock;
        mutable QHash<Tag, Handler> _observers;

        struct NotifyRunnable : public QRunnable
        {
            Q_DISABLE_COPY_AND_MOVE(NotifyRunnable);

            typedef std::function<void()> NotifyFunctor;
            const NotifyFunctor notifyFunctor;

            NotifyRunnable(const NotifyFunctor notifyFunctor_)
                : notifyFunctor(notifyFunctor_)
            {
            }

            virtual ~NotifyRunnable()
            {
            }

            virtual void run()
            {
                notifyFunctor();
            }
        };
    public:
        Observable()
        {
        }
        virtual ~Observable()
        {
        }

        bool attach(Tag tag, const Handler handler) const
        {
            QWriteLocker scopedLocker(&_observersLock);

            if (_observers.contains(tag) || handler == nullptr)
                return false;

            _observers.insert(tag, handler);

            return true;
        }

        bool detach(Tag tag) const
        {
            QWriteLocker scopedLocker(&_observersLock);

            return (_observers.remove(tag) > 0);
        }

        void notify(ARGS... args) const
        {
            QHash<Tag, Handler> observers;
            {
                QReadLocker scopedLocker(&_observersLock);
                observers = detachedOf(_observers);
            }

            for(const auto& handler : constOf(observers))
                handler(args...);
        }

        void postNotify(ARGS... args) const
        {
            QHash<Tag, Handler> observers;
            {
                QReadLocker scopedLocker(&_observersLock);
                observers = detachedOf(_observers);
            }

#if defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 9)) && !defined(__clang__)
            //WORKAROUND: Ugly workaround for https://gcc.gnu.org/bugzilla/show_bug.cgi?id=41933
            const auto runnableFunction =
                [observers]
                (ARGS... wrappedArgs) -> void
                {
                    for (const auto& handler : constOf(observers))
                        handler(wrappedArgs...);
                };
            const std::function<void ()> workaroudHandler = std::bind(runnableFunction, args...);
            QThreadPool::globalInstance()->start(new NotifyRunnable(workaroudHandler));
#else
            QThreadPool::globalInstance()->start(new NotifyRunnable(
                [observers, args...]
                ()
                {
                    for(const auto& handler : constOf(observers))
                        handler(args...);
                }));
#endif
        }
    };

    template<typename _>
    class ObservableAs;

    // ObservableAs for function pointer
    template<typename RETURN_TYPE, typename... ARGS>
    class ObservableAs<RETURN_TYPE(*)(ARGS...)> : public Observable< ARGS... >
    {
        static_assert(std::is_same<RETURN_TYPE, void>::value, "RETURN_TYPE has to be 'void'");

        Q_DISABLE_COPY_AND_MOVE(ObservableAs);
    public:
        typedef typename Observable< ARGS... >::Tag Tag;

    private:
    protected:
    public:
        ObservableAs()
        {
        }
        virtual ~ObservableAs()
        {
        }
    };

    // ObservableAs for member function pointer
    template<typename CLASS, typename RETURN_TYPE, typename... ARGS>
    class ObservableAs<RETURN_TYPE(CLASS::*)(ARGS...)> : public Observable< ARGS... >
    {
        static_assert(std::is_same<RETURN_TYPE, void>::value, "RETURN_TYPE has to be 'void'");

        Q_DISABLE_COPY_AND_MOVE(ObservableAs);
    public:
        typedef typename Observable< ARGS... >::Tag Tag;

    private:
    protected:
    public:
        ObservableAs()
        {
        }
        virtual ~ObservableAs()
        {
        }
    };

    // ObservableAs for const member function pointer
    template<typename CLASS, typename RETURN_TYPE, typename... ARGS>
    class ObservableAs<RETURN_TYPE(CLASS::*)(ARGS...) const> : public Observable< ARGS... >
    {
        static_assert(std::is_same<RETURN_TYPE, void>::value, "RETURN_TYPE has to be 'void'");

        Q_DISABLE_COPY_AND_MOVE(ObservableAs);
    public:
        typedef typename Observable< ARGS... >::Tag Tag;

    private:
    protected:
    public:
        ObservableAs()
        {
        }
        virtual ~ObservableAs()
        {
        }
    };

    // ObservableAs for member object pointer
    template<typename CLASS, typename RETURN_TYPE>
    class ObservableAs<RETURN_TYPE(CLASS::*)> : public Observable<>
    {
        static_assert(std::is_same<RETURN_TYPE, void>::value, "RETURN_TYPE has to be 'void'");

        Q_DISABLE_COPY_AND_MOVE(ObservableAs);
    public:
        typedef typename Observable<>::Tag Tag;

    private:
    protected:
    public:
        ObservableAs()
        {
        }
        virtual ~ObservableAs()
        {
        }
    };

    // ObservableAs for functor
    template<class F>
    class ObservableAs : public ObservableAs< decltype(&F::operator()) >
    {
        Q_DISABLE_COPY_AND_MOVE(ObservableAs);
    public:
        typedef typename ObservableAs< decltype(&F::operator()) >::Tag Tag;

    private:
    protected:
    public:
        ObservableAs()
        {
        }
        virtual ~ObservableAs()
        {
        }
    };

#else

    // Just a stub for SWIG
    template<typename T>
    struct ObservableAs : public IObservable
    {
        ObservableAs();
        virtual ~ObservableAs();

        bool detach(Tag tag) const;
    };

#endif

}

#endif // !defined(_OSMAND_CORE_OBSERVABLE_H_)
