#include "QMainThreadTaskEvent.h"

OsmAnd::QMainThreadTaskEvent::QMainThreadTaskEvent( Task task_ )
    : QEvent(static_cast<QEvent::Type>(Type))
    , task(task_)
{
}

OsmAnd::QMainThreadTaskEvent::~QMainThreadTaskEvent()
{
}
