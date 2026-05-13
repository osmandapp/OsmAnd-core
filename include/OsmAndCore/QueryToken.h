#ifndef _OSMAND_CORE_QUERY_TOKEN_H
#define _OSMAND_CORE_QUERY_TOKEN_H

#include <OsmAndCore/CommonTypes.h>
#include <QString>
#include <QList>
#include <QVector>
#include <algorithm>

namespace OsmAnd
{
    class QueryToken 
    {
    public:
        struct Prefix 
        {
            QString key;
            uint32_t offset;
            Prefix(QString k, int o) : key(std::move(k)), offset(o) {}
        };

        class SuffixMask 
        {
        public:
            SuffixMask(const Prefix& prefix, const QueryToken* parent) : prefix(prefix), parent(parent) {}
            void setDictionary(const QStringList& suffixDictionary);
            const QVector<uint32_t>& getMasks() const { return masks; }
            const Prefix& getPrefix() const { return prefix; }
            bool shouldPassThrough() const { return passThrough; }
            bool isMatched(int maskIndex, uint32_t mask) const;

        private:
            void addSuffix(int index, const QString& suffix);
            QVector<uint32_t> masks;
            const Prefix& prefix;
            const QueryToken* parent;
            bool passThrough = false;
        };

        QueryToken(const QString& query, StringMatcherMode matcherMode, const QList<Prefix>& prefixes);
        QList<Prefix> prefixes;

    private:
        QString query;
        StringMatcherMode matcherMode;
    };

}

#endif // _OSMAND_CORE_QUERY_TOKEN_H
