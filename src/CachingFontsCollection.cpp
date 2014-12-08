#include "CachingFontsCollection.h"
#include "CachingFontsCollection_P.h"

OsmAnd::CachingFontsCollection::CachingFontsCollection()
    : _p(new CachingFontsCollection_P(this))
{
}

OsmAnd::CachingFontsCollection::~CachingFontsCollection()
{
    _p->clearFontsCache();
}

SkTypeface* OsmAnd::CachingFontsCollection::obtainTypeface(const QString& fontName) const
{
    return _p->obtainTypeface(fontName);
}
