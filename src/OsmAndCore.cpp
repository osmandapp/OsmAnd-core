#include "OsmAndCore.h"

#include "stdlib_common.h"
#include <memory>
#include <cstdio>
#include <clocale>

#include "QtExtensions.h"
#include <QCoreApplication>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QLocale>

#include "ignore_warnings_on_external_includes.h"
#include <gdal.h>
#include "restore_internal_warnings.h"

#include "Common.h"
#include "Logging.h"
#include "DefaultLogSink.h"
#include "OsmAndCore_private.h"
#include "ExplicitReferences.h"
#include "QMainThreadTaskHost.h"
#include "QMainThreadTaskEvent.h"
#include "SKIA_private.h"
#include "ICU_private.h"
#include "EmbeddedTypefaceFinder_internal.h"
#include "TextRasterizer_internal.h"
#include "MapSymbolIntersectionClassesRegistry_private.h"

#if defined(OSMAND_TARGET_OS_)
#   error CMAKE_TARGET_OS defined incorrectly
#endif // defined(OSMAND_TARGET_OS_)

#if defined(OSMAND_TARGET_CPU_ARCH_)
#   error CMAKE_TARGET_CPU_ARCH defined incorrectly
#endif // defined(OSMAND_TARGET_CPU_ARCH_)

#if defined(OSMAND_TARGET_CPU_ARCH_FAMILY_)
#   error CMAKE_TARGET_CPU_ARCH_FAMILY defined incorrectly
#endif // defined(OSMAND_TARGET_CPU_ARCH_FAMILY_)

#if defined(OSMAND_COMPILER_FAMILY_)
#   error CMAKE_COMPILER_FAMILY defined incorrectly
#endif // defined(OSMAND_COMPILER_FAMILY_)

namespace OsmAnd
{
    void initializeInAppThread();
    void releaseInAppThread();
    QThread* gMainThread;
    std::shared_ptr<QObject> gMainThreadRootObject;

    std::shared_ptr<const ICoreResourcesProvider> gCoreResourcesProvider;
    QString fontPath;
}

int _dummyArgc = 1;
const char* _dummyArgs[] = { "osmand.core" };
std::shared_ptr<QCoreApplication> _qCoreApplication;
QMutex _qCoreApplicationThreadMutex;
QWaitCondition _qCoreApplicationThreadWaitCondition;
struct QCoreApplicationThread : public QThread
{
    QCoreApplicationThread()
        : wasInitialized(false)
    {
    }

    volatile bool wasInitialized;

    void run()
    {
        _qCoreApplication.reset(new QCoreApplication(_dummyArgc, const_cast<char**>(&_dummyArgs[0])));
        OsmAnd::initializeInAppThread();
        {
            QMutexLocker scopedLocker(&_qCoreApplicationThreadMutex);

            wasInitialized = true;

            _qCoreApplicationThreadWaitCondition.wakeAll();
        }
        QCoreApplication::exec();
        OsmAnd::releaseInAppThread();
        _qCoreApplication.reset();
    }
};
std::shared_ptr<QCoreApplicationThread> _qCoreApplicationThread;

OSMAND_CORE_API bool OSMAND_CORE_CALL OsmAnd::InitializeCore(
    const std::shared_ptr<const ICoreResourcesProvider>& coreResourcesProvider,
    const char* appFontsPath /* = nullptr */)
{
    if (!coreResourcesProvider)
    {
        std::cerr << "OsmAnd core requires non-null core resources provider!" << std::endl;
        return false;
    }
    
    gCoreResourcesProvider = coreResourcesProvider;

    fontPath = QString(appFontsPath);

    Logger::get()->addLogSink(std::shared_ptr<ILogSink>(new DefaultLogSink()));

    InflateExplicitReferences();

    if (!QCoreApplication::instance())
    {
        LogPrintf(LogSeverityLevel::Verbose,
            "OsmAnd Core is initialized standalone, so going to create 'application' thread");
        _qCoreApplicationThread.reset(new QCoreApplicationThread());
        gMainThread = _qCoreApplicationThread.get();
        _qCoreApplicationThread->start();

        // Wait until global initialization will pass in that thread
        {
            QMutexLocker scopeLock(&_qCoreApplicationThreadMutex);
            while (!_qCoreApplicationThread->wasInitialized)
                REPEAT_UNTIL(_qCoreApplicationThreadWaitCondition.wait(&_qCoreApplicationThreadMutex));
        }
    }
    else
    {
        LogPrintf(LogSeverityLevel::Verbose,
            "OsmAnd Core is initialized inside a Qt application, so assuming that got called from application thread");
        gMainThread = QThread::currentThread();
        initializeInAppThread();
    }

    std::setlocale(LC_CTYPE, "UTF-8");
#if defined(OSMAND_TARGET_OS_linux) || defined(OSMAND_TARGET_OS_macosx) || defined(OSMAND_TARGET_OS_windows)
    // Fix parsing SVG via SkParse on desktop
    std::setlocale(LC_NUMERIC, "C");
#endif // defined(OSMAND_TARGET_OS_linux) || defined(OSMAND_TARGET_OS_macosx) || defined(OSMAND_TARGET_OS_windows)

    GDALAllRegister();
    if (!SKIA::initialize())
        return false;
    if (!ICU::initialize())
        return false;
    (void)QLocale::system(); // This will initialize system locale, since it fails to initialize concurrently
    EmbeddedTypefaceFinder_initialize();
    TextRasterizer_initialize();
    MapSymbolIntersectionClassesRegistry_initializeGlobalInstance();

    return true;
}

OSMAND_CORE_API const std::shared_ptr<const OsmAnd::ICoreResourcesProvider>& OSMAND_CORE_CALL OsmAnd::getCoreResourcesProvider()
{
    return gCoreResourcesProvider;
}

OSMAND_CORE_API const QString& OSMAND_CORE_CALL OsmAnd::getFontDirectory()
{
    return fontPath;
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

    MapSymbolIntersectionClassesRegistry_releaseGlobalInstance();
    TextRasterizer_release();
    EmbeddedTypefaceFinder_release();
    ICU::release();
    SKIA::release();

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
