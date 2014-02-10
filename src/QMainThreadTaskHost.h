#ifndef _OSMAND_CORE_Q_MAIN_THREAD_TASK_HOST_H_
#define _OSMAND_CORE_Q_MAIN_THREAD_TASK_HOST_H_

#include <OsmAndCore/QtExtensions.h>
#include <QObject>

#include <QMainThreadTaskEvent.h>

namespace OsmAnd {

    class QMainThreadTaskHost : public QObject
    {
        Q_OBJECT
    public:
        QMainThreadTaskHost(QObject* parent = nullptr);
        virtual ~QMainThreadTaskHost();

        virtual void customEvent(QEvent *e);
    };

}

#endif // !defined(_OSMAND_CORE_Q_MAIN_THREAD_TASK_HOST_H_)