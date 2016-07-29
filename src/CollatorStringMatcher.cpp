#include <cstring>
#include "CollatorStringMatcher.h"

CollatorStringMatcher::CollatorStringMatcher()
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

CollatorStringMatcher::CollatorStringMatcher(QString _part, StringMatcherMode _mode) : CollatorStringMatcher()
{
    part = _part;
    mode = _mode;
}

CollatorStringMatcher::~CollatorStringMatcher()
{
}

Collator* CollatorStringMatcher::getCollator()
{
    return collator;
}

bool CollatorStringMatcher::matches(QString name)
{
    return CollatorStringMatcher::cmatches(collator, part, name, mode);
}

UnicodeString CollatorStringMatcher::qStrToUniStr(QString inStr)
{
    const ushort *utf16 = inStr.utf16();
    int length = inStr.length();

    UnicodeString returnString(utf16, length);

    return returnString;
}

bool CollatorStringMatcher::cmatches(Collator *_collator, QString _base, QString _part, CollatorStringMatcher::StringMatcherMode _mode){
    switch (_mode) {
        case CollatorStringMatcher::CHECK_CONTAINS:
            return CollatorStringMatcher::ccontains(_collator, _base, _part);
        case CollatorStringMatcher::CHECK_EQUALS_FROM_SPACE:
            return CollatorStringMatcher::cstartsWith(_collator, _base, _part, true, true, true);
        case CollatorStringMatcher::CHECK_STARTS_FROM_SPACE:
            return CollatorStringMatcher::cstartsWith(_collator, _base, _part, true, true, false);
        case CollatorStringMatcher::CHECK_STARTS_FROM_SPACE_NOT_BEGINNING:
            return CollatorStringMatcher::cstartsWith(_collator, _base, _part, false, true, false);
        case CollatorStringMatcher::CHECK_ONLY_STARTS_WITH:
            return CollatorStringMatcher::cstartsWith(_collator, _base, _part, true, false, false);
    }
    return false;
}

int CollatorStringMatcher::cindexOf(Collator *_collator, int _start, QString _part, QString _base)
{
    UnicodeString part = qStrToUniStr(_part);
    UnicodeString base = qStrToUniStr(_base);

    for (int pos = _start; pos <= base.length() - part.length(); pos++)
    {
        UnicodeString temp = base.tempSubString(pos, part.length());
        if (_collator->equals(temp, part))
        {
            return pos;
        }
    }

    return -1;

}

bool CollatorStringMatcher::ccontains(Collator *_collator, QString _base, QString _part)
{
    int pos = 0;

    if (_part.length() > 3)
    {
        pos = cindexOf(_collator, pos, _part.left(3), _base);
        if (pos == -1)
        {
            return false;
        }
    }

    pos = cindexOf(_collator, pos, _part, _base);
    if (pos == -1)
    {
        return false;
    }

    return true;
}

bool CollatorStringMatcher::cstartsWith(Collator *_collator, QString _searchInParam, QString _theStart,
                                        bool checkBeginning, bool checkSpaces, bool equals)
{
    UnicodeString searchIn = qStrToUniStr(_searchInParam).toLower(Locale::getDefault());
    UnicodeString theStart = qStrToUniStr(_theStart);

    int startLength = theStart.length();
    int serchInLength = searchIn.length();

    if (startLength == 0) return true;
    if (startLength > serchInLength) return false;

    if (checkBeginning)
    {
        bool starts = _collator->equals(searchIn.tempSubString(0, startLength), theStart);

        if (starts)
        {
            if (equals)
            {
                if (startLength == serchInLength || isSpace(searchIn.charAt(startLength))) {
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
                if (_collator->equals(searchIn.tempSubString(i, startLength), theStart))
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


bool CollatorStringMatcher::isSpace(UChar c)
{
    return !u_isalnum(c);
}
