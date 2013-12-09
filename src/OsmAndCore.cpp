#include "OsmAndCore.h"

#include <memory>

#include <OsmAndCore/QtExtensions.h>
#include <QCoreApplication>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include <gdal.h>
#include <SkGraphics.h>

#include "Logging.h"
#include "OsmAndCore_private.h"
#include "ExplicitReferences.h"
#include "QMainThreadTaskHost.h"
#include "QMainThreadTaskEvent.h"

namespace OsmAnd {
    void initializeGlobal();
    void releaseGlobal();
    std::shared_ptr<QObject> gMainThreadTaskHost;
}

int _dummyArgc = 1;
const char* _dummyArgs[] = { "osmand.core" };
std::shared_ptr<QCoreApplication> _qCoreApplication;
QMutex _qCoreApplicationThreadMutex;
QWaitCondition _qCoreApplicationThreadWaitCondition;
class QCoreApplicationThread : public QThread
{
    void run()
    {
        _qCoreApplication.reset(new QCoreApplication(_dummyArgc, const_cast<char**>(&_dummyArgs[0])));
        OsmAnd::initializeGlobal();
        {
            QMutexLocker scopedLocker(&_qCoreApplicationThreadMutex);
            _qCoreApplicationThreadWaitCondition.wakeAll();
        }
        QCoreApplication::exec();
        OsmAnd::releaseGlobal();
        _qCoreApplication.reset();
    }
};
std::shared_ptr<QCoreApplicationThread> _qCoreApplicationThread;

OSMAND_CORE_API void OSMAND_CORE_CALL OsmAnd::InitializeCore()
{
    InflateExplicitReferences();

    if(!QCoreApplication::instance())
    {
        _qCoreApplicationThread.reset(new QCoreApplicationThread());
        _qCoreApplicationThread->start();

        // Wait until global initialization will pass in that thread
        {
            QMutexLocker scopeLock(&_qCoreApplicationThreadMutex);

            _qCoreApplicationThreadWaitCondition.wait(&_qCoreApplicationThreadMutex);
        }
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
    GDALAllRegister();
    SkGraphics::PurgeFontCache(); // This will initialize global glyph cache
}

void OsmAnd::releaseGlobal()
{
    gMainThreadTaskHost.reset();
}