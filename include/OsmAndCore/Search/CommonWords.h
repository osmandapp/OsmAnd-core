#ifndef _OSMAND_CORE_COMMON_WORDS_H_
#define _OSMAND_CORE_COMMON_WORDS_H_

//  OsmAnd-java/src/main/java/net/osmand/binary/CommonWords.java
//  git revision 5e3cda75836097f09709f8266c75bdb42efd7f23

#include <OsmAndCore/stdlib_common.h>
#include <OsmAndCore/Logging.h>
#include <OsmAndCore/Search/Abbreviations.h>
#include <OsmAndCore/SearchAlgorithms.h>

#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>

namespace OsmAnd
{
    // for ex: 100 bridge, ленина 30, but not potenitally name of street (31st road)
    const QString NUMBER_WITH_LESS_THAN_2_LETTERS = QStringLiteral("NUMBER_WITH_LESS_THAN_2_LETTERS");

    class CommonWords
    {
    public:
        static CommonWords& getInstance(const QStringList& regionNames = {})
        {
            static CommonWords instance(regionNames);
            return instance;
        }

        CommonWords(const CommonWords&) = delete;
        CommonWords& operator=(const CommonWords&) = delete;

    private:
        QHash<QString, int> commonWordsDictionary;
        QHash<QString, int> frequentlyUsedWordsDictionary;

        explicit CommonWords(const QStringList& initialRegionNames = {})
        {
            addCommon(NUMBER_WITH_LESS_THAN_2_LETTERS);
            addCalculatedAddrCommonWords();
            addCalculatedPoiCommonWords();
            addAbbrevationsToCommon(); // common words
            addManualAbbrevationsToFrequent();
            addRegionNames(initialRegionNames);
            addCalculatedAddrFrequentWords();
            addCalculatedPoiFrequentWords();
        }

        void addCommon(const QString& string)
        {
            const QString aligned = SearchAlgorithms::alignChars(string);
            commonWordsDictionary.insert(string, commonWordsDictionary.size());
            if (string != aligned)
            {
                commonWordsDictionary.insert(aligned, commonWordsDictionary.size());
            }
        }

        void addFrequent(const QString& string)
        {
            const QString aligned = SearchAlgorithms::alignChars(string);
            if (isCommon(string) || frequentlyUsedWordsDictionary.contains(string))
            {
                return;
            }
            frequentlyUsedWordsDictionary.insert(string, frequentlyUsedWordsDictionary.size());
            if (string != aligned)
            {
                frequentlyUsedWordsDictionary.insert(aligned, frequentlyUsedWordsDictionary.size());
            }
        }

        void addFrequentAbbrevation(const QString& string)
        {
            addFrequent(string);
        }

        bool isNumber2Letters(const QString& name) const
        {
            return SearchAlgorithms::isNumber2Letters(name);
        }

    public:
        bool isCommon(const QString& name) const
        {
            return commonWordsDictionary.contains(name) || isNumber2Letters(name);
        }

        int getCommon(const QString& name) const
        {
            const QString* lookupName = &name;
            QString tmp;
            if (isNumber2Letters(name))
            {
                tmp = NUMBER_WITH_LESS_THAN_2_LETTERS;
                lookupName = &tmp;
            }
            const auto it = commonWordsDictionary.constFind(*lookupName);
            if (it != commonWordsDictionary.constEnd())
            {
                return it.value();
            }
            return -1;
        }

        int getFrequentlyUsed(const QString& name) const
        {
            const auto it = frequentlyUsedWordsDictionary.constFind(name);
            return it == frequentlyUsedWordsDictionary.constEnd() ? -1 : it.value();
        }

        int getCommonSearch(const QString& name) const
        {
            const QString* lookupName = &name;
            QString tmp;
            if (isNumber2Letters(name))
            {
                tmp = NUMBER_WITH_LESS_THAN_2_LETTERS;
                lookupName = &tmp;
            }
            const auto it = commonWordsDictionary.constFind(*lookupName);
            // higher means better for search
            if (it != commonWordsDictionary.constEnd())
            {
                return it.value();
            }
            const int fq = getFrequentlyUsed(*lookupName);
            if (fq != -1)
            {
                return commonWordsDictionary.size() + fq + 1;
            }
            return -1;
        }

        int getCommonGeocoding(const QString& name) const
        {
            const auto it = commonWordsDictionary.constFind(name);
            if (it != commonWordsDictionary.constEnd())
            {
                return it.value();
            }
            return -1;
        }

    private:
        void addAbbrevationsToCommon()
        {
            const auto& abbreviations = Abbreviations::getAbbreviations();
            for (auto it = abbreviations.constBegin(); it != abbreviations.constEnd(); ++it)
            {
                const auto commonIt = commonWordsDictionary.constFind(it.value().toLower());
                if (commonIt != commonWordsDictionary.constEnd())
                {
                    commonWordsDictionary.insert(it.key().toLower(), commonIt.value());
                }
            }
        }
        void addManualAbbrevationsToFrequent();
        void addCalculatedAddrCommonWords();
        void addCalculatedPoiCommonWords();
        void addCalculatedPoiFrequentWords();
        void addCalculatedAddrFrequentWords();
        void addRegionNames(const QStringList& names)
        {
            for (const QString& name : names)
            {
                addFrequent(name);
//                regionNames.insert(name);
                if (name.contains(QStringLiteral(".")))
                {
                    addFrequent(QString(name).replace(QStringLiteral("."), QStringLiteral("")));
//                    regionNames.insert(QString(name).replace(QStringLiteral("."), QStringLiteral("")));
                }
            }
        }
    };
}

#endif // !defined(_OSMAND_CORE_COMMON_WORDS_H_)
