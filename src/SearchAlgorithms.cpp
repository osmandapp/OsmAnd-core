#include "SearchAlgorithms.h"
#include <unicode/normalizer2.h>
#include <unicode/utext.h>
#include <QChar>
#include <QSet>
#include "ArabicNormalizer.h"

QStringList OsmAnd::SearchAlgorithms::splitAndNormalize(const QString& query) {
    QString normalizedQuery = canonicalizePunctuation(query);
    QStringList queryTokens;
    QSet<QString> uniqueTokens;

    for (const QString& token : split(normalizedQuery))
    {
        QString norm = normalizeToken(token);
        if (!norm.isEmpty() && !uniqueTokens.contains(norm))
        {
            queryTokens.append(norm);
            uniqueTokens.insert(norm);
        }
    }

    if (OsmAnd::ArabicNormalizer::isSpecialArabic(normalizedQuery))
    {
        QString arabic = OsmAnd::ArabicNormalizer::normalize(normalizedQuery);
        if (!arabic.isEmpty() && arabic != normalizedQuery)
        {
            for (const QString& token : split(arabic))
            {
                QString normalizedToken = normalizeToken(token);
                if (!normalizedToken.isEmpty())
                {
                    queryTokens.append(normalizedToken);
                }
            }
        }
    }
    return queryTokens;
}

QString OsmAnd::SearchAlgorithms::normalizeToken(const QString& token)
{
    if (token.isEmpty()) return "";
    return OsmAnd::ICU::toNFC(token).toLower();
}

QString OsmAnd::SearchAlgorithms::canonicalizePunctuation(QString s)
{
    static const char16_t keys[] = {u'’', u'ʼ', u'(', u')', u'´', u'`', u'′', u'‵', u'ʹ'};
    static const char16_t values[] = {u'\'', u'\'', u' ', u' ', u'\'', u'\'', u'\'', u'\'', u'\''};
    const int size = sizeof(keys) / sizeof(char16_t);

    bool needNormalization = false;
    for (int i = 0; i < size; ++i) {
        if (s.contains(QChar(keys[i]))) {
            needNormalization = true;
            break;
        }
    }

    if (!needNormalization) return s;

    for (int i = 0; i < size; ++i)
    {
        s.replace(QChar(keys[i]), QChar(values[i]));
    }
    return s;
}

QString OsmAnd::SearchAlgorithms::removeQuotes(QString s)
{
    return s.replace(QStringLiteral("«"), "").replace(QStringLiteral("»"), "");
}

QString OsmAnd::SearchAlgorithms::removeApostrophes(QString s)
{
    static const char16_t apostrophes[] = {u'\'', u'’', u'ʼ', u'´', u'`', u'′', u'‵', u'ʹ'};
    const int size = sizeof(apostrophes) / sizeof(char16_t);

    QString result;
    result.reserve(s.length());
    for (int i = 0; i < s.length(); ++i) {
        QChar c = s[i];
        bool isApostroph = false;
        for (int j = 0; j < size; ++j) {
            if (c.unicode() == apostrophes[j]) {
                isApostroph = true;
                break;
            }
        }
        if (!isApostroph) result.append(c);
    }
    return result;
}

QString OsmAnd::SearchAlgorithms::replaceGermanSS(QString fullText)
{
    return fullText.replace(QStringLiteral("ß"), QStringLiteral("ss"));
}

OsmAnd::SearchAlgorithms::CodePointPrefixMatch OsmAnd::SearchAlgorithms::startWith(const QString& token, const QString& prefix)
{
    int leftOffset = 0;
    int rightOffset = 0;
    int commonPrefixCodePointLength = 0;

    while (leftOffset < token.length() && rightOffset < prefix.length())
    {
        uint32_t leftCp = token.at(leftOffset).unicode();
        if (QChar::isHighSurrogate(leftCp) && leftOffset + 1 < token.length())
        {
            leftCp = QChar::surrogateToUcs4(leftCp, token.at(leftOffset + 1).unicode());
        }

        uint32_t rightCp = prefix.at(rightOffset).unicode();
        if (QChar::isHighSurrogate(rightCp) && rightOffset + 1 < prefix.length())
        {
            rightCp = QChar::surrogateToUcs4(rightCp, prefix.at(rightOffset + 1).unicode());
        }

        if (leftCp != rightCp)
            break;

        int charCount = QChar::requiresSurrogates(leftCp) ? 2 : 1;
        leftOffset += charCount;
        rightOffset += charCount;
        commonPrefixCodePointLength++;
    }
    return {leftOffset, rightOffset, commonPrefixCodePointLength};
}

int OsmAnd::SearchAlgorithms::suffixOffsetAfterPrefix(const QString& token, const QString& prefix)
{
    CodePointPrefixMatch match = startWith(token, prefix);
    if (match.rightOffset != prefix.length())
        return -1;
    return (match.leftOffset < token.length()) ? match.leftOffset : -1;
}

QStringList OsmAnd::SearchAlgorithms::split(const QString& name)
{
    int prev = -1;
    QStringList namesToAdd;
    QSet<QString> seen;

    for (int i = 0; i <= name.length(); )
    {
        bool tokenCharacter = false;
        int currentCpCount = 1;

        if (i != name.length())
        {
            uint32_t cp = name.at(i).unicode();
            if (QChar::isHighSurrogate(cp) && i + 1 < name.length())
            {
                cp = QChar::surrogateToUcs4(cp, name.at(i + 1).unicode());
            }
            currentCpCount = QChar::requiresSurrogates(cp) ? 2 : 1;
            tokenCharacter = isTokenCharacter(name, i, prev != -1) || cp == '\'';
        }

        if (!tokenCharacter)
        {
            if (prev != -1)
            {
                QString substr = name.mid(prev, i - prev).toLower();
                if (!seen.contains(substr))
                {
                    namesToAdd << substr;
                    seen.insert(substr);
                }
                prev = -1;
            }
        }
        else if (prev == -1)
        {
            prev = i;
        }
        i += currentCpCount;
    }
    return namesToAdd;
}

bool OsmAnd::SearchAlgorithms::isTokenCharacter(const QString& value, int index, bool tokenAlreadyStarted)
{
    uint32_t cp = value.at(index).unicode();
    int charLen = 1;
    if (QChar::isHighSurrogate(cp) && index + 1 < value.length())
    {
        char16_t low = value.at(index + 1).unicode();
        if (QChar::isLowSurrogate(low)) {
            cp = QChar::surrogateToUcs4(cp, low);
            charLen = 2;
        }
    }

    if (QChar::isLetter(cp) || QChar::isNumber(cp))
        return true;

    if (cp == '-') {
        int nextIndex = index + charLen;
        bool nextIsNumber = false;
        if (nextIndex < value.length())
        {
            uint32_t nextCp = value.at(nextIndex).unicode();
            if (QChar::isHighSurrogate(nextCp) && nextIndex + 1 < value.length())
            {
                char16_t low = value.at(nextIndex + 1).unicode();
                if (QChar::isLowSurrogate(low))
                {
                    nextCp = QChar::surrogateToUcs4(nextCp, low);
                }
            }
            nextIsNumber = QChar::isNumber(nextCp);
        }

        // Перевірка попереднього символу (аналог Java offsetByCodePoints(index, -1))
        bool prevIsNumber = false;
        if (index > 0) {
            int prevIndex = index - 1;
            uint32_t prevCp = value.at(prevIndex).unicode();
            // Якщо ми потрапили на low surrogate, значить символ перед ним — high surrogate
            if (QChar::isLowSurrogate(prevCp) && prevIndex > 0) {
                char16_t high = value.at(prevIndex - 1).unicode();
                if (QChar::isHighSurrogate(high)) {
                    prevIndex--;
                    prevCp = QChar::surrogateToUcs4(high, prevCp);
                }
            }
            prevIsNumber = QChar::isNumber(prevCp);
        }

        if (nextIsNumber || prevIsNumber)
            return true;
    }

    // Перевірка на діакритичні знаки (Combining Marks)
    // Працює тільки якщо токен уже розпочався
    if (tokenAlreadyStarted) {
        QChar::Category cat = QChar::category(cp);
        if (cat == QChar::Mark_NonSpacing ||
            cat == QChar::Mark_SpacingCombining ||
            cat == QChar::Mark_Enclosing) {
            return true;
        }
    }

    return false;
}

QString OsmAnd::SearchAlgorithms::nameIndexDecodeDictionarySuffix(const QString& previousSuffix, const QString& encodedSuffix)
{
    if (encodedSuffix.isEmpty())
        return "";

    uint32_t marker = encodedSuffix.at(0).unicode();
    if (QChar::isHighSurrogate(marker) && encodedSuffix.length() > 1)
    {
        marker = QChar::surrogateToUcs4(marker, encodedSuffix.at(1).unicode());
    }

    if (marker >= SUFFIX_DICT_MARKER_BASE && marker <= SUFFIX_DICT_MARKER_MAX)
    {
        int commonCpLen = marker - SUFFIX_DICT_MARKER_BASE;
        int prefixEndOffset = 0;
        for (int i = 0; i < commonCpLen && prefixEndOffset < previousSuffix.length(); ++i)
        {
            prefixEndOffset += QChar::requiresSurrogates(previousSuffix.at(prefixEndOffset).unicode()) ? 2 : 1;
        }

        QString suffixRemainder = encodedSuffix.mid(QChar::requiresSurrogates(marker) ? 2 : 1);
        return ICU::toNFC(previousSuffix.left(prefixEndOffset) + suffixRemainder);
    }

    return ICU::toNFC(decodeRawSuffix(encodedSuffix));
}

QString OsmAnd::SearchAlgorithms::decodeRawSuffix(const QString& encodedSuffix)
{
    if (!encodedSuffix.isEmpty() && encodedSuffix.at(0).unicode() == SUFFIX_DICT_MARKER_RAW_ESCAPE)
    {
        return encodedSuffix.mid(1);
    }
    return encodedSuffix;
}

bool OsmAnd::SearchAlgorithms::startsWithSuffixMarker(const QString& value)
{
    if (value.isEmpty())
        return false;
    uint32_t cp = value.at(0).unicode();
    return cp == SUFFIX_DICT_MARKER_RAW_ESCAPE || (cp >= SUFFIX_DICT_MARKER_BASE && cp <= SUFFIX_DICT_MARKER_MAX);
}

int OsmAnd::SearchAlgorithms::countCodePoints(const QString& s)
{
    int count = 0;
    for (int i = 0; i < s.length(); ++count)
    {
        i += QChar::requiresSurrogates(s.at(i).unicode()) ? 2 : 1;
    }
    return count;
}

QString OsmAnd::SearchAlgorithms::nameIndexEncodeSuffix(const QString& suffix, const QString& previousSuffix)
{
    QString encodedRaw = startsWithSuffixMarker(suffix) ? (QString(QChar(SUFFIX_DICT_MARKER_RAW_ESCAPE)) + suffix) : suffix;
    if (previousSuffix.isNull())
        return encodedRaw;

    CodePointPrefixMatch match = startWith(previousSuffix, suffix);
    int commonLen = match.commonPrefixCodePointLength;

    if (commonLen > (SUFFIX_DICT_MARKER_MAX - SUFFIX_DICT_MARKER_BASE))
        return encodedRaw;

    int offset = 0;
    for (int i = 0; i < commonLen; ++i)
    {
        offset += QChar::requiresSurrogates(suffix.at(offset).unicode()) ? 2 : 1;
    }

    uint32_t markerCp = SUFFIX_DICT_MARKER_BASE + commonLen;
    QString deltaEncoded = QString::fromUcs4(&markerCp, 1) + suffix.mid(offset);

    return (countCodePoints(deltaEncoded) < countCodePoints(encodedRaw)) ? deltaEncoded : encodedRaw;
}
