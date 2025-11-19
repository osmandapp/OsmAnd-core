#ifndef _OSMAND_CORE_COLLATORSTRINGMATCHER_P_H
#define _OSMAND_CORE_COLLATORSTRINGMATCHER_P_H


#include <OsmAndCore.h>

#include <QString>
#include "OsmAndCore.h"
#include <CollatorStringMatcher.h>

namespace OsmAnd
{
    class CollatorStringMatcher;
    
    class OSMAND_CORE_API CollatorStringMatcher_P Q_DECL_FINAL
    {
    private:
        static QString lowercaseAndAlignChars(const QString& fullText);
        static QString alignChars(const QString& fullText);
    protected:
        CollatorStringMatcher_P(CollatorStringMatcher* const owner);
        
    public:
        virtual ~CollatorStringMatcher_P();
        
        ImplementationInterface<CollatorStringMatcher> owner;
        
        bool matches(const QString& _base, const QString& _part, StringMatcherMode _mode) const;
        bool contains(const QString& _base, const QString& _part) const;
        bool startsWith(const QString& _searchInParam, const QString& _theStart,
                         bool checkBeginning, bool checkSpaces, bool equals) const;
    
        friend class OsmAnd::CollatorStringMatcher;
    };
}


#endif //_OSMAND_CORE_COLLATORSTRINGMATCHER_P_H
