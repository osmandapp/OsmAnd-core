#include "SKIA_private.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkGraphics.h>
//#include <SkError.h>
#include "restore_internal_warnings.h"

#include "Logging.h"

/*namespace OsmAnd
{
    namespace SKIA
    {
        void errorLogger(SkError error, void* context);
    }
}*/

bool OsmAnd::SKIA::initialize()
{
    //SkSetErrorCallback(errorLogger, nullptr);
    SkGraphics::PurgeFontCache(); // This will initialize global glyph cache, since it fails to initialize concurrently

    return true;
}

void OsmAnd::SKIA::release()
{
    //SkSetErrorCallback(nullptr, nullptr);
}

/*void OsmAnd::SKIA::errorLogger(SkError error, void* context)
{
    Q_UNUSED(context);

    LogPrintf(LogSeverityLevel::Error,
        "SKIA error (%08x) : %s",
        error,
        SkGetLastErrorString());
}*/
