#ifndef _OSMAND_CORE_Q_RUNNABLE_FUNCTOR_H_
#define _OSMAND_CORE_Q_RUNNABLE_FUNCTOR_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QRunnable>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Callable.h>

namespace OsmAnd
{
    class OSMAND_CORE_API QRunnableFunctor : public QRunnable
    {
        Q_DISABLE_COPY_AND_MOVE(QRunnableFunctor);

    public:
        OSMAND_CALLABLE(Callback, void, const QRunnableFunctor* runnable);

    private:
    protected:
        const Callback _callback;
    public:
        QRunnableFunctor(const Callback callback);
        virtual ~QRunnableFunctor();

        virtual void run();
    };
}

#endif // !defined(_OSMAND_CORE_Q_RUNNABLE_FUNCTOR_H_)
