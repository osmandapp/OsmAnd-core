#ifndef _OSMAND_CORE_Q_MAIN_THREAD_TASK_EVENT_H_
#define _OSMAND_CORE_Q_MAIN_THREAD_TASK_EVENT_H_

#include <functional>

#include "QtExtensions.h"
#include <QEvent>

namespace OsmAnd
{
    class QMainThreadTaskEvent : public QEvent
    {
    public:
        typedef std::function<void ()> Task;
        enum {
            Type = QEvent::Type::User + 1,
        };
    private:
    protected:
    public:
        QMainThreadTaskEvent(Task task);
        virtual ~QMainThreadTaskEvent();

        const Task task;
    };
}

#endif // !defined(_OSMAND_CORE_Q_MAIN_THREAD_TASK_EVENT_H_)