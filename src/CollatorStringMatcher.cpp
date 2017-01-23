#include "CollatorStringMatcher.h"
#include "CollatorStringMatcher_P.h"
#include <ICU.h>

OsmAnd::CollatorStringMatcher::CollatorStringMatcher(const QString& part, const StringMatcherMode mode)
    : _p(new CollatorStringMatcher_P(this))
    ,_part(part)
    , _mode(mode)
{
}

OsmAnd::CollatorStringMatcher::~CollatorStringMatcher()
{
}

bool OsmAnd::CollatorStringMatcher::matches(const QString& name) const
{
    return _p->CollatorStringMatcher_P::matches(name, _part, _mode);
}

bool OsmAnd::CollatorStringMatcher::cmatches(const QString& _base, const QString& _part, StringMatcherMode _mode)
{
    return OsmAnd::ICU::cmatches(_base, _part, _mode);
}

bool OsmAnd::CollatorStringMatcher::ccontains(const QString& _base, const QString& _part)
{
    return OsmAnd::ICU::ccontains(_base, _part);
}

bool OsmAnd::CollatorStringMatcher::cstartsWith(const QString& _searchInParam, const QString& _theStart,
                                                bool checkBeginning, bool checkSpaces, bool equals)
{
    return OsmAnd::ICU::cstartsWith(_searchInParam, _theStart, checkBeginning, checkSpaces, equals);
}
