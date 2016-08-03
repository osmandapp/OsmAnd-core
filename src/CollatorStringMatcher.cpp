#include "CollatorStringMatcher.h"
#include "CollatorStringMatcher_P.h"

OsmAnd::CollatorStringMatcher::CollatorStringMatcher()
    : _p(new CollatorStringMatcher_P(this))
{
}

OsmAnd::CollatorStringMatcher::~CollatorStringMatcher()
{
}

bool OsmAnd::CollatorStringMatcher::cmatches(QString _base, QString _part,
                                             OsmAnd::CollatorStringMatcher::StringMatcherMode _mode) const
{
    return _p->CollatorStringMatcher_P::cmatches(
        _base,
        _part,
        _mode);
}

bool OsmAnd::CollatorStringMatcher::ccontains(QString _base, QString _part) const
{

    return _p->CollatorStringMatcher_P::ccontains(
        _base,
        _part);
}

bool OsmAnd::CollatorStringMatcher::cstartsWith(QString _searchInParam, QString _theStart,
                                                bool checkBeginning, bool checkSpaces, bool equals) const
{
    return _p->cstartsWith(
        _searchInParam,
        _theStart,
        checkBeginning,
        checkSpaces,
        equals);
}
