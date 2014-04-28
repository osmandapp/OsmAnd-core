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
    if (e->type() != static_cast<QEvent::Type>(QMainThreadTaskEvent::Type))
        return;

    static_cast<QMainThreadTaskEvent*>(e)->task();
}
