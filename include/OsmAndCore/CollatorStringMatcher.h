#ifndef _OSMAND_CORE__COLLATORSTRINGMATCHER_H
#define _OSMAND_CORE__COLLATORSTRINGMATCHER_H


#include <OsmAndCore.h>

#include <unistd.h>
#include "unicode/unistr.h"
#include "unicode/uversion.h"
#include "unicode/coll.h"
#include "QtCore/QString"


namespace OsmAnd
{
    class OSMAND_CORE_API CollatorStringMatcher
    {
    public:
        enum StringMatcherMode {
            CHECK_ONLY_STARTS_WITH,
            CHECK_STARTS_FROM_SPACE,
            CHECK_STARTS_FROM_SPACE_NOT_BEGINNING,
            CHECK_EQUALS_FROM_SPACE,
            CHECK_CONTAINS
        };

    private:
        Collator *collator;
        QString part;
        StringMatcherMode mode;

        static int cindexOf(const Collator *_collator, int _start, QString _part, QString _base);
        static bool isSpace(UChar c);
        static UnicodeString qStrToUniStr(QString inStr);

    public:
        CollatorStringMatcher();
        CollatorStringMatcher(QString _part, StringMatcherMode _mode);
        ~CollatorStringMatcher();
        Collator * getCollator() const;
        bool matches(QString name);
        static bool cmatches(const Collator *_collator, QString _base, QString _part, StringMatcherMode _mode);
        static bool ccontains(const Collator *_collator, QString _base, QString _part);
        static bool cstartsWith(const Collator *_collator, QString _searchInParam, QString _theStart,
                                bool checkBeginning, bool checkSpaces, bool equals);
    };
}


#endif //_OSMAND_CORE__COLLATORSTRINGMATCHER_H
