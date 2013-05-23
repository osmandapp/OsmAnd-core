#include <SkImageDecoder.h>
#include <SkImageEncoder.h>

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
