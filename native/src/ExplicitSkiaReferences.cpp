#include <SkImageDecoder.h>
#include <SkImageEncoder.h>

struct SkiaExplicitReferences
{
    SkiaExplicitReferences()
    {
        auto gifDecoder = CreateGIFImageDecoder();
        auto jpgDecoder = CreateJPEGImageDecoder();
        auto jpgEncoder = CreateJPEGImageEncoder();
        auto pngDecoder = CreatePNGImageDecoder();
        auto pngEncoder = CreatePNGImageEncoder();

        delete gifDecoder;
        delete jpgDecoder;
        delete jpgEncoder;
        delete pngDecoder;
        delete pngEncoder;
    }
};

static SkiaExplicitReferences _skiaExplicitReferences;
