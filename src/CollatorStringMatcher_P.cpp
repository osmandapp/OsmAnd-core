#include "CollatorStringMatcher_P.h"
#include "CollatorStringMatcher.h"

#include <ICU.h>
#include <QLocale>

OsmAnd::CollatorStringMatcher_P::CollatorStringMatcher_P(CollatorStringMatcher* owner_)
    : owner(owner_)
{
}

OsmAnd::CollatorStringMatcher_P::~CollatorStringMatcher_P()
{
}

bool OsmAnd::CollatorStringMatcher_P::matches(const QString& _base, const QString& _part, StringMatcherMode _mode) const
{
    return OsmAnd::ICU::cmatches(_base, _part, _mode);
}

bool OsmAnd::CollatorStringMatcher_P::contains(const QString& _base, const QString& _part) const
{
    return OsmAnd::ICU::ccontains(_base, _part);
}

bool OsmAnd::CollatorStringMatcher_P::startsWith(const QString& _searchInParam, const QString& _theStart,
                                                  bool checkBeginning, bool checkSpaces, bool equals) const
{
    return OsmAnd::ICU::cstartsWith(_searchInParam, _theStart, checkBeginning, checkSpaces, equals);
}

QString OsmAnd::CollatorStringMatcher_P::lowercaseAndAlignChars(const QString& fullText)
{
    QLocale defaultLocale;
    QString res = defaultLocale.toLower(fullText);
    res = alignChars(res);
    return res;
}

QString OsmAnd::CollatorStringMatcher_P::alignChars(const QString& fullText)
{
    int i;
    QString res = fullText;
    while( (i = res.indexOf(QStringLiteral("ß")) ) != -1 )
    {
        res = res.mid(0, i) + QStringLiteral("ss") + res.mid(i + 1);
    }
    return res;
}
