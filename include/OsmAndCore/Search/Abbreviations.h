#ifndef _OSMAND_CORE_ABBREVIATIONS_H_
#define _OSMAND_CORE_ABBREVIATIONS_H_

//  OsmAnd-java/src/main/java/net/osmand/binary/Abbreviations.java
//  git revision 383f15bc221f56ee5a60072f8226898221c20076

#include <OsmAndCore/stdlib_common.h>
#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore.h>

#include <QHash>
#include <QSet>
#include <QString>

namespace OsmAnd
{
    class OSMAND_CORE_API Abbreviations
    {
    private:
        Abbreviations() = delete;

        static QHash<QString, QString> abbreviations;
        // 2nd version search abbrevations for spatial search
        static QHash<QString, QString> searchAbbreviations;
        // set of words to check for buidlings
        static QHash<QString, QString> buildingAbbreviations;
        static QSet<QString> conjunctions;
        static QSet<QString> commonSkipOtherCnt;

        static void addDirectionWord(const QString& key, const QString& full);
        static void addStreetStatus(const QString& key, const QString& full);
        static void addConjunction(const QString& key);

        static void initialize();

    public:
        static bool likelyPartOfRef(const QString& word, const QSet<QString>& wordSplit);

        // search v-2
        static bool likelyPartOfBuilding(const QString& word, const QSet<QString>* wordSplit = nullptr);

        // search-v2
        static const QHash<QString, QString>& getSearchAbbreviations();

        // search-v2
        static bool isCommonSkipOtherCnt(const QString& lowerCase);

        // Indexing data
        static QString replaceAll(const QString& phrase);

        // search-v1
        static const QHash<QString, QString>& getAbbreviations();

        // search v-1
        static QString replace(const QString& word);

        // search-v1
        static bool isConjunction(const QString& lowerCase);
    };
}

#endif // !defined(_OSMAND_CORE_ABBREVIATIONS_H_)
