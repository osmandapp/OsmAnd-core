#include "OsmAndCore/Search/Abbreviations.h"
#include "OsmAndCore/SearchAlgorithms.h"

#include <QStringList>

QHash<QString, QString> OsmAnd::Abbreviations::abbreviations;
QHash<QString, QString> OsmAnd::Abbreviations::searchAbbreviations;
QHash<QString, QString> OsmAnd::Abbreviations::buildingAbbreviations;
QSet<QString> OsmAnd::Abbreviations::conjunctions;
QSet<QString> OsmAnd::Abbreviations::commonSkipOtherCnt;

void OsmAnd::Abbreviations::addDirectionWord(const QString& key, const QString& full)
{
    abbreviations.insert(key, full);
    commonSkipOtherCnt.insert(key);
    commonSkipOtherCnt.insert(full.toLower());
}

void OsmAnd::Abbreviations::addStreetStatus(const QString& key, const QString& full)
{
    abbreviations.insert(key, full);
    commonSkipOtherCnt.insert(key);
    commonSkipOtherCnt.insert(full.toLower());
}

void OsmAnd::Abbreviations::addConjunction(const QString& key)
{
    conjunctions.insert(key);
    commonSkipOtherCnt.insert(key);
}

void OsmAnd::Abbreviations::initialize()
{
    static const bool initialized = []()
    {
        // articles
        addConjunction(QStringLiteral("the"));
        addConjunction(QStringLiteral("de"));
        addConjunction(QStringLiteral("du"));
        addConjunction(QStringLiteral("der"));
        addConjunction(QStringLiteral("den"));
        addConjunction(QStringLiteral("die"));
        addConjunction(QStringLiteral("das"));
        addConjunction(QStringLiteral("la"));
        addConjunction(QStringLiteral("le"));
        addConjunction(QStringLiteral("el"));
        addConjunction(QStringLiteral("il"));
        addConjunction(QStringLiteral("of"));

        // and
        addConjunction(QStringLiteral("and"));
        addConjunction(QStringLiteral("und"));
        addConjunction(QStringLiteral("en"));
        addConjunction(QStringLiteral("et"));
        addConjunction(QStringLiteral("y"));
        addConjunction(QStringLiteral("и"));

        // direction
        addDirectionWord(QStringLiteral("e"), QStringLiteral("East"));
        addDirectionWord(QStringLiteral("w"), QStringLiteral("West"));
        addDirectionWord(QStringLiteral("s"), QStringLiteral("South"));
        addDirectionWord(QStringLiteral("n"), QStringLiteral("North"));
        addDirectionWord(QStringLiteral("sw"), QStringLiteral("Southwest"));
        addDirectionWord(QStringLiteral("se"), QStringLiteral("Southeast"));
        addDirectionWord(QStringLiteral("nw"), QStringLiteral("Northwest"));
        addDirectionWord(QStringLiteral("ne"), QStringLiteral("Northeast"));

        // street status
        addStreetStatus(QStringLiteral("ln"), QStringLiteral("Lane"));
        addStreetStatus(QStringLiteral("dr"), QStringLiteral("Drive"));
        addStreetStatus(QStringLiteral("rd"), QStringLiteral("Road"));
        addStreetStatus(QStringLiteral("av"), QStringLiteral("Avenue"));
        addStreetStatus(QStringLiteral("st"), QStringLiteral("Street")); // 2 values could be saint
        addStreetStatus(QStringLiteral("hwy"), QStringLiteral("Highway"));
        addStreetStatus(QStringLiteral("blvd"), QStringLiteral("Boulevard"));

        searchAbbreviations = abbreviations;
        searchAbbreviations.insert(QStringLiteral("ave"), QStringLiteral("Avenue")); // extra
        searchAbbreviations.insert(QStringLiteral("st"), QStringLiteral("Street Saint")); // 2 values could be saint
        // duplicates - synonyms and not abbrevations actually
        searchAbbreviations.insert(QStringLiteral("о"), QStringLiteral("Остров"));
        searchAbbreviations.insert(QStringLiteral("остров"), QStringLiteral("о."));
        searchAbbreviations.insert(QStringLiteral("1st"), QStringLiteral("First"));
        searchAbbreviations.insert(QStringLiteral("2nd"), QStringLiteral("Second"));
        searchAbbreviations.insert(QStringLiteral("3rd"), QStringLiteral("Third"));
        searchAbbreviations.insert(QStringLiteral("first"), QStringLiteral("1st"));
        searchAbbreviations.insert(QStringLiteral("second"), QStringLiteral("2nd"));
        searchAbbreviations.insert(QStringLiteral("third"), QStringLiteral("3rd"));

        // common housenumber additions
        // french
        buildingAbbreviations.insert(QStringLiteral("bis"), QStringLiteral("Bis"));
        buildingAbbreviations.insert(QStringLiteral("ter"), QStringLiteral("Ter"));
        buildingAbbreviations.insert(QStringLiteral("quater"), QStringLiteral("Quater"));
        // american
        buildingAbbreviations.insert(QStringLiteral("bldg"), QStringLiteral("Building"));
        buildingAbbreviations.insert(QStringLiteral("ste"), QStringLiteral("Suite"));
        buildingAbbreviations.insert(QStringLiteral("unt"), QStringLiteral("Unit"));
        buildingAbbreviations.insert(QStringLiteral("apt"), QStringLiteral("Apartment"));
        buildingAbbreviations.insert(QStringLiteral("fl"), QStringLiteral("Floor"));
        buildingAbbreviations.insert(QStringLiteral("flr"), QStringLiteral("Floor"));
        buildingAbbreviations.insert(QStringLiteral("bsmt"), QStringLiteral("Basement"));

        return true;
    }();
    Q_UNUSED(initialized);
}

bool OsmAnd::Abbreviations::likelyPartOfRef(const QString& word, const QSet<QString>& wordSplit)
{
    int letters = SearchAlgorithms::letters(word);
    if (letters < 2 || (letters == 2 && SearchAlgorithms::startsWithDigit(word)))
    {
        return true;
    }
    for (const QString& s : wordSplit)
    {
        letters = SearchAlgorithms::letters(s);
        if (!(letters < 2 || (letters == 2 && SearchAlgorithms::startsWithDigit(s))))
        {
            return false;
        }
    }
    return true;
}

// search v-2
bool OsmAnd::Abbreviations::likelyPartOfBuilding(const QString& word, const QSet<QString>* wordSplit)
{
    initialize();
    const bool bldNum = SearchAlgorithms::isNumber2Letters(word)
        || word.length() == 1
        || buildingAbbreviations.contains(word);
    if (bldNum)
    {
        return true;
    }
    if (wordSplit != nullptr)
    {
        // recursion for 2bis
        for (const QString& w : *wordSplit)
        {
            if (!likelyPartOfBuilding(w))
            {
                return false;
            }
        }
        return true;
    }
    return false;
}

// search-v2
const QHash<QString, QString>& OsmAnd::Abbreviations::getSearchabbreviations()
{
    initialize();
    return searchAbbreviations;
}

// search-v2
bool OsmAnd::Abbreviations::isCommonSkipOtherCnt(const QString& lowerCase)
{
    initialize();
    return commonSkipOtherCnt.contains(lowerCase);
}

// Indexing data
QString OsmAnd::Abbreviations::replaceAll(const QString& phrase)
{
    initialize();
    const QString delimiter = QStringLiteral(" ");
    const QStringList words = phrase.split(delimiter, Qt::KeepEmptyParts);
    QString result;
    bool changed = false;
    for (const QString& word : words)
    {
        if (!result.isEmpty())
        {
            result.append(delimiter);
        }
        const auto it = abbreviations.constFind(word.toLower());
        if (it == abbreviations.constEnd())
        {
            result.append(word);
        }
        else
        {
            changed = true;
            result.append(it.value());
        }
    }
    return changed ? result : phrase;
}

// search-v1
const QHash<QString, QString>& OsmAnd::Abbreviations::getAbbreviations()
{
    initialize();
    return abbreviations;
}

// search-v1
QString OsmAnd::Abbreviations::replace(const QString& word)
{
    initialize();
    const auto it = abbreviations.constFind(word.toLower());
    return it == abbreviations.constEnd() ? word : it.value();
}

// search-v1
bool OsmAnd::Abbreviations::isConjunction(const QString& lowerCase)
{
    initialize();
    return conjunctions.contains(lowerCase);
}
