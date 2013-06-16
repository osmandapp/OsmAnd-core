#include "OsmAndCore.h"

#include <memory>

#include <QCoreApplication>
#include <QThread>

#include "OsmAndLogging.h"
#include "OsmAndCore_private.h"
#include "QMainThreadTaskHost.h"
#include "QMainThreadTaskEvent.h"

namespace OsmAnd {
    void initializeGlobal();
    void releaseGlobal();
    std::shared_ptr<QNetworkAccessManager> gNetworkAccessManager;
    std::shared_ptr<QObject> gMainThreadTaskHost;
}

int _dummyArgc = 1;
const char* _dummyArgs[] = { "osmand.core" };
std::shared_ptr<QCoreApplication> _qCoreApplication;
class QCoreApplicationThread : public QThread
{
    void run()
    {
        _qCoreApplication.reset(new QCoreApplication(_dummyArgc, const_cast<char**>(&_dummyArgs[0])));
        OsmAnd::initializeGlobal();
        QCoreApplication::exec();
        OsmAnd::releaseGlobal();
        _qCoreApplication.reset();
    }
};
std::shared_ptr<QCoreApplicationThread> _qCoreApplicationThread;

OSMAND_CORE_API void OSMAND_CORE_CALL OsmAnd::InitializeCore()
{
    if(!QCoreApplication::instance())
    {
        _qCoreApplicationThread.reset(new QCoreApplicationThread());
        _qCoreApplicationThread->start();
        //TODO: wait for initialization to complete
    }
    else
    {
        initializeGlobal();
    }
}

OSMAND_CORE_API void OSMAND_CORE_CALL OsmAnd::ReleaseCore()
{
    if(_qCoreApplicationThread)
    {
        QCoreApplication::exit();
        _qCoreApplicationThread->wait();
        _qCoreApplicationThread.reset();
    }
    else
    {
        releaseGlobal();
    }
}

void OsmAnd::initializeGlobal()
{
    gMainThreadTaskHost.reset(new QMainThreadTaskHost());
    gNetworkAccessManager.reset(new QNetworkAccessManager());
}

void OsmAnd::releaseGlobal()
{
    gNetworkAccessManager.reset();
    gMainThreadTaskHost.reset();
}