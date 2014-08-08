#include "OsmAndCore.h"

#include "stdlib_common.h"
#include <memory>

#include "QtExtensions.h"
#include <QCoreApplication>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QLocale>

#include "ignore_warnings_on_external_includes.h"
#include <gdal.h>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkGraphics.h>
#include "restore_internal_warnings.h"

#include "Common.h"
#include "Logging.h"
#include "DefaultLogSink.h"
#include "OsmAndCore_private.h"
#include "ExplicitReferences.h"
#include "QMainThreadTaskHost.h"
#include "QMainThreadTaskEvent.h"
#include "ICU_private.h"

namespace OsmAnd
{
    void initializeInAppThread();
    void releaseInAppThread();
    QThread* gMainThread;
    std::shared_ptr<QObject> gMainThreadRootObject;
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
        OsmAnd::initializeInAppThread();
        {
            QMutexLocker scopedLocker(&_qCoreApplicationThreadMutex);
            _qCoreApplicationThreadWaitCondition.wakeAll();
        }
        QCoreApplication::exec();
        OsmAnd::releaseInAppThread();
        _qCoreApplication.reset();
    }
};
std::shared_ptr<QCoreApplicationThread> _qCoreApplicationThread;

OSMAND_CORE_API void OSMAND_CORE_CALL OsmAnd::InitializeCore()
{
    Logger::get()->addLogSink(std::shared_ptr<ILogSink>(new DefaultLogSink()));

    InflateExplicitReferences();

    if (!QCoreApplication::instance())
    {
        _qCoreApplicationThread.reset(new QCoreApplicationThread());
        gMainThread = _qCoreApplicationThread.get();
        _qCoreApplicationThread->start();

        // Wait until global initialization will pass in that thread
        {
            QMutexLocker scopeLock(&_qCoreApplicationThreadMutex);
            REPEAT_UNTIL(_qCoreApplicationThreadWaitCondition.wait(&_qCoreApplicationThreadMutex));
        }
    }
    else
    {
        gMainThread = QThread::currentThread();
        initializeInAppThread();
    }

    // GDAL
    GDALAllRegister();

    // SKIA
    SkGraphics::PurgeFontCache(); // This will initialize global glyph cache, since it fails to initialize concurrently

    // ICU
    ICU::initialize();

    // Qt
    (void)QLocale::system(); // This will initialize system locale, since it fails to initialize concurrently
}

OSMAND_CORE_API void OSMAND_CORE_CALL OsmAnd::ReleaseCore()
{
    if (_qCoreApplicationThread)
    {
        QCoreApplication::exit();
        REPEAT_UNTIL(_qCoreApplicationThread->wait());
        _qCoreApplicationThread.reset();
    }
    else
    {
        releaseInAppThread();
    }

    // ICU
    ICU::release();

    Logger::get()->flush();
    Logger::get()->removeAllLogSinks();
}

void OsmAnd::initializeInAppThread()
{
    gMainThreadRootObject.reset(new QMainThreadTaskHost());
}

void OsmAnd::releaseInAppThread()
{
    gMainThreadRootObject.reset();
}

#if defined(OSMAND_TARGET_OS_android)
//HACK: https://code.google.com/p/android/issues/detail?id=43819
//TODO: Remove this mess when it's going to be fixed
#include <iostream>
::std::ios_base::Init::Init()
{
}
#endif
