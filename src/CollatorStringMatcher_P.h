#ifndef _OSMAND_CORE_COLLATORSTRINGMATCHER_P_H
#define _OSMAND_CORE_COLLATORSTRINGMATCHER_P_H


#include <OsmAndCore.h>

#include <QString>
#include "OsmAndCore.h"
#include <CollatorStringMatcher.h>

namespace OsmAnd
{
    class OSMAND_CORE_API CollatorStringMatcher_P Q_DECL_FINAL
    {
    private:
    public:
        CollatorStringMatcher_P();
        virtual ~CollatorStringMatcher_P();

        bool matches(const QString& _base, const QString& _part, CollatorStringMatcher::StringMatcherMode _mode) const;
        bool contains(const QString& _base, const QString& _part) const;
        bool startsWith(const QString& _searchInParam, const QString& _theStart,
                         bool checkBeginning, bool checkSpaces, bool equals) const;
    };
}


#endif //_OSMAND_CORE_COLLATORSTRINGMATCHER_P_H
