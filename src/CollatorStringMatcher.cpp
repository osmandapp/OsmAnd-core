#include "CollatorStringMatcher.h"
#include "CollatorStringMatcher_P.h"

const OsmAnd::CollatorStringMatcher_P OsmAnd::CollatorStringMatcher::_p;

OsmAnd::CollatorStringMatcher::CollatorStringMatcher(const QString& part, const StringMatcherMode mode)
    : _part(part)
    , _mode(mode)
{
}

OsmAnd::CollatorStringMatcher::~CollatorStringMatcher()
{
}

bool OsmAnd::CollatorStringMatcher::matches(const QString& name) const
{
    return _p.matches(name, _part, _mode);
}

bool OsmAnd::CollatorStringMatcher::cmatches(const QString& _base, const QString& _part,
                                             OsmAnd::CollatorStringMatcher::StringMatcherMode _mode)
{
    return _p.matches(_base, _part, _mode);
}

bool OsmAnd::CollatorStringMatcher::ccontains(const QString& _base, const QString& _part)
{
    return _p.contains(_base, _part);
}

bool OsmAnd::CollatorStringMatcher::cstartsWith(const QString& _searchInParam, const QString& _theStart,
                                                bool checkBeginning, bool checkSpaces, bool equals)
{
    return _p.startsWith(_searchInParam, _theStart, checkBeginning, checkSpaces, equals);
}
