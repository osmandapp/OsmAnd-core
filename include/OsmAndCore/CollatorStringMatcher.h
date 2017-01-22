#ifndef _OSMAND_CORE_COLLATORSTRINGMATCHER_H
#define _OSMAND_CORE_COLLATORSTRINGMATCHER_H

#include <OsmAndCore.h>

#include <QString>

namespace OsmAnd
{
    class CollatorStringMatcher_P;

    class OSMAND_CORE_API CollatorStringMatcher
    {
        Q_DISABLE_COPY_AND_MOVE(CollatorStringMatcher);
    public:
        enum class StringMatcherMode
        {
            CHECK_ONLY_STARTS_WITH,
            CHECK_STARTS_FROM_SPACE,
            CHECK_STARTS_FROM_SPACE_NOT_BEGINNING,
            CHECK_EQUALS_FROM_SPACE,
            CHECK_CONTAINS
        };

    private:
        const QString _part;
        const StringMatcherMode _mode;
        
    protected:
        static const CollatorStringMatcher_P _p;

    public:
        CollatorStringMatcher(const QString& part, const StringMatcherMode mode);
        virtual ~CollatorStringMatcher();
        
        bool matches(const QString& name) const;

        static bool cmatches(const QString& _base, const QString& _part, StringMatcherMode _mode);
        static bool ccontains(const QString& _base, const QString& _part);
        static bool cstartsWith(const QString& _searchInParam, const QString& _theStart,
                         bool checkBeginning, bool checkSpaces, bool equals);
    };
}


#endif //_OSMAND_CORE_COLLATORSTRINGMATCHER_H
