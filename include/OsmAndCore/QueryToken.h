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
            int offset;
            Prefix(QString k, int o) : key(std::move(k)), offset(o) {}
        };

        class SuffixMask 
        {
        public:
            SuffixMask(const Prefix& prefix, const QueryToken& parent) : prefix(prefix), parent(parent) {}
            void setDictionary(const QStringList& suffixDictionary);
            const QVector<int>& getMasks() const { return masks; }
            const Prefix& getPrefix() const { return prefix; }

        private:
            void addSuffix(int index, const QString& suffix);
            QVector<int> masks;
            const Prefix& prefix;
            const QueryToken& parent;
        };

        QueryToken(const QString& query, StringMatcherMode matcherMode, const QList<Prefix>& prefixes);

    private:
        QString query;
        StringMatcherMode matcherMode;
        QList<Prefix> prefixes;
    };

}

#endif // _OSMAND_CORE_QUERY_TOKEN_H
