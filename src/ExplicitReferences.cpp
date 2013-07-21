#include "ExplicitReferences.h"

#include <SkImageDecoder.h>
#include <SkImageEncoder.h>

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
