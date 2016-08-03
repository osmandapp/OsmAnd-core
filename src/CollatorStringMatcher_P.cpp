#include "CollatorStringMatcher_P.h"
#include "CollatorStringMatcher.h"

#include <cstring>
#include "unicode/unistr.h"
#include "unicode/coll.h"

static bool isSpace(UChar c);
static UnicodeString qStrToUniStr(QString inStr);

OsmAnd::CollatorStringMatcher_P::CollatorStringMatcher_P(CollatorStringMatcher* owner_)
    : owner(owner_)
{
    Locale locale = Locale::getDefault();

    UErrorCode status = U_ZERO_ERROR;
    if (std::strcmp(locale.getLanguage(), "ro") ||
        std::strcmp(locale.getLanguage(), "cs") ||
        std::strcmp(locale.getLanguage(), "sk"))
    {
        collator = Collator::createInstance(Locale("en", "US"), status);
    }
    else
    {
        collator = Collator::createInstance(status);
    }

    if (U_FAILURE(status)) {
        return;
    }

    collator->setStrength(Collator::PRIMARY);
}

OsmAnd::CollatorStringMatcher_P::~CollatorStringMatcher_P()
{
}

bool OsmAnd::CollatorStringMatcher_P::cmatches(QString _base, QString _part,
                                               CollatorStringMatcher::StringMatcherMode _mode) const
{
    switch (_mode)
    {
        case CollatorStringMatcher::StringMatcherMode::CHECK_CONTAINS:
            return ccontains(_base, _part);
        case CollatorStringMatcher::StringMatcherMode::CHECK_EQUALS_FROM_SPACE:
            return cstartsWith(_base, _part, true, true, true);
        case CollatorStringMatcher::StringMatcherMode::CHECK_STARTS_FROM_SPACE:
            return cstartsWith(_base, _part, true, true, false);
        case CollatorStringMatcher::StringMatcherMode::CHECK_STARTS_FROM_SPACE_NOT_BEGINNING:
            return cstartsWith(_base, _part, false, true, false);
        case CollatorStringMatcher::StringMatcherMode::CHECK_ONLY_STARTS_WITH:
            return cstartsWith(_base, _part, true, false, false);
        default:
            return false;
    }
}

bool OsmAnd::CollatorStringMatcher_P::ccontains(QString _base, QString _part) const
{

    UnicodeString baseString = qStrToUniStr(_base);
    UnicodeString partString = qStrToUniStr(_part);

    if (baseString.length() <= partString.length())
        return collator->equals(baseString, partString);

    for (int pos = 0; pos <= baseString.length() - partString.length() + 1; pos++)
    {
        UnicodeString temp = baseString.tempSubString(pos, baseString.length());

        for (int length = temp.length(); length >= 0; length--)
        {
            UnicodeString temp2 = temp.tempSubString(0, length);
            if (collator->equals(temp2, partString))
            {
                return true;
            }
        }

    }

    return false;
}

bool OsmAnd::CollatorStringMatcher_P::cstartsWith(QString _searchInParam, QString _theStart,
                                                  bool checkBeginning, bool checkSpaces, bool equals) const
{
    UnicodeString searchIn = qStrToUniStr(_searchInParam).toLower(Locale::getDefault());
    UnicodeString theStart = qStrToUniStr(_theStart);

    int startLength = theStart.length();
    int serchInLength = searchIn.length();

    if (startLength == 0) return true;
    if (startLength > serchInLength) return false;

    if (checkBeginning)
    {
        bool starts = collator->equals(searchIn.tempSubString(0, startLength), theStart);

        if (starts)
        {
            if (equals)
            {
                if (startLength == serchInLength || isSpace(searchIn.charAt(startLength)))
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
            if (isSpace(searchIn.charAt(i - 1)) && !isSpace(searchIn.charAt(i)))
            {
                if (collator->equals(searchIn.tempSubString(i, startLength), theStart))
                {
                    if (equals)
                    {
                        if (i + startLength == serchInLength || isSpace(searchIn.charAt(i + startLength)))
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

UnicodeString qStrToUniStr(QString inStr)
{
    const ushort *utf16 = inStr.utf16();
    int length = inStr.length();

    UnicodeString returnString(utf16, length);

    return returnString;
}

bool isSpace(UChar c)
{
    return !u_isalnum(c);
}
