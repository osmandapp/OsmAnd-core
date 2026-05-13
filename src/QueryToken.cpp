#include "CollatorStringMatcher.h"
#include "QueryToken.h"


OsmAnd::QueryToken::QueryToken(const QString& query, 
                       StringMatcherMode matcherMode, 
                       const QList<Prefix>& prefixes)
    : query(query), matcherMode(matcherMode) 
    {

    if (!prefixes.isEmpty())
    {
        this->prefixes = prefixes;        
        // Sorting: Primary by length descending, Secondary by string content ascending
        std::sort(this->prefixes.begin(), this->prefixes.end(), [](const Prefix& a, const Prefix& b) 
        {
            if (a.key.length() != b.key.length()) 
            {
                return a.key.length() > b.key.length();
            }
            return a.key < b.key;
        });
    }
}

void OsmAnd::QueryToken::SuffixMask::setDictionary(const QStringList& suffixDictionary)
{
    if (prefix.key.isNull() || suffixDictionary.isEmpty())
    {
        return;
    }
    if (suffixDictionary.size() == 1 && suffixDictionary.at(0).isEmpty())
    {
        passThrough = !parent->query.isNull() && CollatorStringMatcher::cmatches(prefix.key, parent->query, parent->matcherMode);
        return;
    }
    if (parent->query.isNull())
    {
        return;
    }
    masks.clear();
    for (int i = 0; i < suffixDictionary.size(); ++i)
    {
        addSuffix(i, suffixDictionary.at(i));
    }
}

void OsmAnd::QueryToken::SuffixMask::addSuffix(int index, const QString& suffix)
{
    if (suffix.isNull() || index < 0)
    {
        return;
    }

    QString fullKey = prefix.key + suffix;

    if (CollatorStringMatcher::cmatches(fullKey, parent->query, parent->matcherMode))
    {
        int intWordIndex = index >> 5; // index / 32
        while (masks.size() <= intWordIndex)
        {
            masks.append(0);
        }
        int bitOffset = index & 31;    // index % 32
        int wordMask = 1 << bitOffset;
        masks[intWordIndex] |= wordMask;
    }
}

bool OsmAnd::QueryToken::SuffixMask::isMatched(int maskIndex, uint32_t mask) const
{
    return !masks.isEmpty() && maskIndex < masks.size() && (masks.at(maskIndex) & mask) != 0;
}
