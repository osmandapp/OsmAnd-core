#include "ITypefaceFinder.h"

OsmAnd::ITypefaceFinder::ITypefaceFinder()
{
}

OsmAnd::ITypefaceFinder::~ITypefaceFinder()
{
}

OsmAnd::ITypefaceFinder::Typeface::Typeface(const sk_sp<SkTypeface>& skTypeface_, hb_face_t* hbTypeface_)
	: skTypeface(skTypeface_)
	, hbTypeface(hbTypeface_, [](auto p) { hb_face_destroy(p); })
{
}

OsmAnd::ITypefaceFinder::Typeface::~Typeface()
{
}
