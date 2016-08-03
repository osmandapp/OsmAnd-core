#ifndef _OSMAND_CORE_COLLATORSTRINGMATCHER_P_H
#define _OSMAND_CORE_COLLATORSTRINGMATCHER_P_H


#include <OsmAndCore.h>

#include <QString>
#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include <unicode/coll.h>
#include <CollatorStringMatcher.h>

namespace OsmAnd
{
    class CollatorStringMatcher;

    class OSMAND_CORE_API CollatorStringMatcher_P Q_DECL_FINAL
    {

    protected:
        CollatorStringMatcher_P(CollatorStringMatcher* const owner);

    private:
        Collator *collator;

    public:
        virtual ~CollatorStringMatcher_P();

        ImplementationInterface<CollatorStringMatcher> owner;
        bool cmatches(QString _base, QString _part, CollatorStringMatcher::StringMatcherMode _mode) const;
        bool ccontains(QString _base, QString _part) const;
        bool cstartsWith(QString _searchInParam, QString _theStart,
                         bool checkBeginning, bool checkSpaces, bool equals) const;

        friend class OsmAnd::CollatorStringMatcher;
    };
}


#endif //_OSMAND_CORE_COLLATORSTRINGMATCHER_P_H
