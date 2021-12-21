#include "SKIA_private.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkGraphics.h>
#include "restore_internal_warnings.h"

bool OsmAnd::SKIA::initialize()
{
    SkGraphics::PurgeFontCache(); // This will initialize global glyph cache, since it fails to initialize concurrently

    return true;
}

void OsmAnd::SKIA::release()
{
}
