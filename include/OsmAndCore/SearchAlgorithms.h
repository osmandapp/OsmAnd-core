#ifndef _OSMAND_CORE_SEARCH_ALGORITHMS_H
#define _OSMAND_CORE_SEARCH_ALGORITHMS_H

#include "ICU.h"
#include <QString>
#include <QStringList>
#include <QVector>

namespace OsmAnd
{
    class SearchAlgorithms {
    public:
        static constexpr char16_t SUFFIX_DICT_MARKER_RAW_ESCAPE = u'\uE000';
        static constexpr int SUFFIX_DICT_MARKER_BASE = 0xE100;
        static constexpr int SUFFIX_DICT_MARKER_MAX = 0xF8FF;

        static QStringList splitAndNormalize(const QString& query);
        static QString canonicalizePunctuation(QString s);
        static QString removeQuotes(QString s);
        static QString removeApostrophes(QString s);
        static QString replaceGermanSS(QString fullText);
        static QString nameIndexDecodeDictionarySuffix(const QString& previousSuffix, const QString& encodedSuffix);
        static QString nameIndexEncodeSuffix(const QString& suffix, const QString& previousSuffix = QString());
        static QString normalizeToken(const QString& token);

    private:
        SearchAlgorithms() = delete;

        struct CodePointPrefixMatch {
            int leftOffset;
            int rightOffset;
            int commonPrefixCodePointLength;
        };

        static CodePointPrefixMatch startWith(const QString& token, const QString& prefix);
        static int suffixOffsetAfterPrefix(const QString& token, const QString& prefix);
        static QStringList split(const QString& name);
        static bool isTokenCharacter(const QString& value, int index, bool tokenAlreadyStarted);
        static QString decodeRawSuffix(const QString& encodedSuffix);
        static bool startsWithSuffixMarker(const QString& value);
        static int countCodePoints(const QString& s);
    };
}

#endif // _OSMAND_CORE_SEARCH_ALGORITHMS_H
