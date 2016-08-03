#ifndef _OSMAND_CORE_COLLATORSTRINGMATCHER_H
#define _OSMAND_CORE_COLLATORSTRINGMATCHER_H

#include <OsmAndCore.h>

#include <QString>
#include <OsmAndCore/PrivateImplementation.h>

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
    protected:
        PrivateImplementation<CollatorStringMatcher_P> _p;

    public:
        CollatorStringMatcher();
        virtual ~CollatorStringMatcher();
        bool cmatches(QString _base, QString _part, StringMatcherMode _mode) const;
        bool ccontains(QString _base, QString _part) const;
        bool cstartsWith(QString _searchInParam, QString _theStart,
                         bool checkBeginning, bool checkSpaces, bool equals) const;
    };
}


#endif //_OSMAND_CORE_COLLATORSTRINGMATCHER_H
