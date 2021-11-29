#include "ITypefaceFinder.h"


/*static */const char OsmAnd::ITypefaceFinder::Typeface::sRepChars[] = "\xE2\x80\x8B\x41\xE2\x80\x8C";//just add code to the end with \x41 divider

OsmAnd::ITypefaceFinder::ITypefaceFinder()
{
}

OsmAnd::ITypefaceFinder::~ITypefaceFinder()
{
}

OsmAnd::ITypefaceFinder::Typeface::Typeface(const sk_sp<SkTypeface>& skTypeface_,
                                            hb_face_t* hbTypeface_,
                                            std::set<uint32_t> delCodePoints_,
                                            uint32_t repCodePoint_)
	: skTypeface(skTypeface_)
	, hbTypeface(hbTypeface_, [](auto p) { hb_face_destroy(p); })
    , delCodePoints(delCodePoints_)
    , repCodePoint(repCodePoint_)
{
}

OsmAnd::ITypefaceFinder::Typeface::~Typeface()
{
}
