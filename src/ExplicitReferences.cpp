#include "ExplicitReferences.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkImageDecoder.h>
#include <SkImageEncoder.h>
#include "restore_internal_warnings.h"

void OsmAnd::InflateExplicitReferences()
{
    struct ExplicitReferences
    {
        ExplicitReferences()
        {
            // SKIA
            delete CreateGIFImageDecoder();
            delete CreateJPEGImageDecoder();
            delete CreateJPEGImageEncoder();
            delete CreatePNGImageDecoder();
            delete CreatePNGImageEncoder();
        }
    };

    static ExplicitReferences _explicitReferences;
}
