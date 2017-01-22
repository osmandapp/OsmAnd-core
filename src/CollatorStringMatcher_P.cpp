#include "CollatorStringMatcher_P.h"
#include "CollatorStringMatcher.h"
#include <QLocale>


static bool isSpace(QChar c);

OsmAnd::CollatorStringMatcher_P::CollatorStringMatcher_P()
{
}

OsmAnd::CollatorStringMatcher_P::~CollatorStringMatcher_P()
{
}

bool OsmAnd::CollatorStringMatcher_P::matches(const QString& _base, const QString& _part,
                                               OsmAnd::CollatorStringMatcher::StringMatcherMode _mode) const
{
    switch (_mode)
    {
        case CollatorStringMatcher::StringMatcherMode::CHECK_CONTAINS:
            return contains(_base, _part);
        case CollatorStringMatcher::StringMatcherMode::CHECK_EQUALS_FROM_SPACE:
            return startsWith(_base, _part, true, true, true);
        case CollatorStringMatcher::StringMatcherMode::CHECK_STARTS_FROM_SPACE:
            return startsWith(_base, _part, true, true, false);
        case CollatorStringMatcher::StringMatcherMode::CHECK_STARTS_FROM_SPACE_NOT_BEGINNING:
            return startsWith(_base, _part, false, true, false);
        case CollatorStringMatcher::StringMatcherMode::CHECK_ONLY_STARTS_WITH:
            return startsWith(_base, _part, true, false, false);
        default:
            return false;
    }
}

bool OsmAnd::CollatorStringMatcher_P::contains(const QString& _base, const QString& _part) const
{

    if (_base.length() <= _part.length())
        return _base.localeAwareCompare(_part) == 0;

    for (int pos = 0; pos <= _base.length() - _part.length() + 1; pos++)
    {
        QString temp = _base.mid(pos, _base.length());

        for (int length = temp.length(); length >= 0; length--)
        {
            QString temp2 = temp.mid(0, length);
            if (temp2.localeAwareCompare(_part) == 0)
            {
                return true;
            }
        }
    }

    return false;
}

bool OsmAnd::CollatorStringMatcher_P::startsWith(const QString& _searchInParam, const QString& _theStart,
                                                  bool checkBeginning, bool checkSpaces, bool equals) const
{
    QLocale locale = QLocale();
    QString searchIn = locale.toLower(_searchInParam);

    int startLength = _theStart.length();
    int serchInLength = searchIn.length();

    if (startLength == 0) return true;
    if (startLength > serchInLength) return false;

    if (checkBeginning)
    {
        bool starts = searchIn.mid(0, startLength).localeAwareCompare(_theStart) == 0;

        if (starts)
        {
            if (equals)
            {
                if (startLength == serchInLength || isSpace(searchIn.at(startLength)))
                {
                    return true;
                }
            }
            else
            {
                return true;
            }
        }
    }

    if (checkSpaces)
    {
        for (int i = 1; i <= serchInLength - startLength; i++)
        {
            if (isSpace(searchIn.at(i - 1)) && !isSpace(searchIn.at(i)))
            {
                if (searchIn.mid(i, startLength).localeAwareCompare(_theStart) == 0)
                {
                    if (equals)
                    {
                        if (i + startLength == serchInLength || isSpace(searchIn.at(i + startLength)))
                        {
                            return true;
                        }
                    }
                    else
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool isSpace(QChar c)
{
    return !c.isDigit() && !c.isLetter();
}
