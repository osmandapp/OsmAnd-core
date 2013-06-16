#include "QMainThreadTaskHost.h"

OsmAnd::QMainThreadTaskHost::QMainThreadTaskHost( QObject* parent /*= nullptr*/ )
    : QObject(parent)
{
}

OsmAnd::QMainThreadTaskHost::~QMainThreadTaskHost()
{
}

void OsmAnd::QMainThreadTaskHost::customEvent( QEvent *e )
{
    if(e->type() != OsmAnd::QMainThreadTaskEvent::Type)
        return;

    auto taskEvent = static_cast<QMainThreadTaskEvent*>(e);

    taskEvent->task();
}
