#include "SearchAlgorithms.h"
#include <unicode/normalizer2.h>
#include <unicode/utext.h>
#include <QChar>
#include <QSet>
#include "ArabicNormalizer.h"

const QString OsmAnd::SearchAlgorithms::EMPTY_SUFFIX_DICTIONARY_SENTINEL = QStringLiteral("\uE100");

QStringList OsmAnd::SearchAlgorithms::splitAndNormalize(const QString& query) {
    QString normalizedQuery = canonicalizePunctuation(query);
    QStringList queryTokens;
    QSet<QString> uniqueTokens;

    const QStringList tokens = split(normalizedQuery);
    for (const QString& token : tokens)
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
            const QStringList arabicTokens = split(arabic);
            for (const QString& token : arabicTokens)
            {
                QString normalizedToken = normalizeToken(token);
                if (!normalizedToken.isEmpty() && !uniqueTokens.contains(normalizedToken))
                {
                    queryTokens.append(normalizedToken);
                    uniqueTokens.insert(normalizedToken);
                }
            }
        }
    }
    return queryTokens;
}

QString OsmAnd::SearchAlgorithms::normalizeToken(const QString& token)
{
    if (token.isEmpty())
    {
        return QString();
    }
    return OsmAnd::ICU::toNFC(token).toLower();
}

QString OsmAnd::SearchAlgorithms::canonicalizePunctuation(QString s)
{
    const int n = s.length();
    for (int i = 0; i < n; ++i)
    {
        switch (s.at(i).unicode())
        {
            case u'’':
            case u'ʼ':
            case u'´':
            case u'`':
            case u'′':
            case u'‵':
            case u'ʹ':
                s[i] = QLatin1Char('\'');
                break;

            case u'(':
            case u')':
                s[i] = QLatin1Char(' ');
                break;

            default:
                break;
        }
    }

    return s;
}

QString OsmAnd::SearchAlgorithms::removeQuotes(QString s)
{
    if (!s.contains(QChar(u'«')) && !s.contains(QChar(u'»')))
    {
        return s;
    }
    s.remove(QChar(u'«'));
    s.remove(QChar(u'»'));
    return s;
}

QString OsmAnd::SearchAlgorithms::removeApostrophes(QString s)
{
    bool hasApostrophe = false;
    for (int i = 0, n = s.length(); i < n && !hasApostrophe; ++i)
    {
        if (isApostropheLike(s.at(i)))
        {
            hasApostrophe = true;
        }
    }
    if (!hasApostrophe)
    {
        return s;
    }

    const int n = s.length();
    QString result;
    result.reserve(n);
    for (int i = 0; i < n; ++i)
    {
        QChar c = s[i];
        if (isApostropheLike(c))
        {
            continue;
        }
        result.append(c);
    }
    return result;
}

QString OsmAnd::SearchAlgorithms::replaceGermanSS(QString fullText)
{
    return fullText.replace(QStringLiteral("ß"), QStringLiteral("ss"));
}

OsmAnd::SearchAlgorithms::CodePointPrefixMatch OsmAnd::SearchAlgorithms::startWith(const QString& token, const QString& prefix)
{
    const int tokenLen = token.length();
    const int prefixLen = prefix.length();
    int leftOffset = 0;
    int rightOffset = 0;
    int commonPrefixCodePointLength = 0;

    while (leftOffset < tokenLen && rightOffset < prefixLen)
    {
        uint32_t leftCp = token.at(leftOffset).unicode();
        if (QChar::isHighSurrogate(leftCp) && leftOffset + 1 < tokenLen)
        {
            leftCp = QChar::surrogateToUcs4(leftCp, token.at(leftOffset + 1).unicode());
        }

        uint32_t rightCp = prefix.at(rightOffset).unicode();
        if (QChar::isHighSurrogate(rightCp) && rightOffset + 1 < prefixLen)
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
    const int nameLen = name.length();
    int prev = -1;
    QStringList namesToAdd;
    QSet<QString> seen;

    for (int i = 0; i <= nameLen; )
    {
        bool tokenCharacter = false;
        int currentCpCount = 1;

        if (i != nameLen)
        {
            uint32_t cp = name.at(i).unicode();
            if (QChar::isHighSurrogate(cp) && i + 1 < nameLen)
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
    const int valueLen = value.length();
    uint32_t cp = value.at(index).unicode();
    int charLen = 1;
    if (QChar::isHighSurrogate(cp) && index + 1 < valueLen)
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
        if (nextIndex < valueLen)
        {
            uint32_t nextCp = value.at(nextIndex).unicode();
            if (QChar::isHighSurrogate(nextCp) && nextIndex + 1 < valueLen)
            {
                char16_t low = value.at(nextIndex + 1).unicode();
                if (QChar::isLowSurrogate(low))
                {
                    nextCp = QChar::surrogateToUcs4(nextCp, low);
                }
            }
            nextIsNumber = QChar::isNumber(nextCp);
        }

        bool prevIsNumber = false;
        if (index > 0) {
            int prevIndex = index - 1;
            uint32_t prevCp = value.at(prevIndex).unicode();
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
        const int prevLen = previousSuffix.length();
        int commonCpLen = marker - SUFFIX_DICT_MARKER_BASE;
        int prefixEndOffset = 0;
        for (int i = 0; i < commonCpLen && prefixEndOffset < prevLen; ++i)
        {
            prefixEndOffset += QChar::requiresSurrogates(previousSuffix.at(prefixEndOffset).unicode()) ? 2 : 1;
        }

        const int markerLen = QChar::requiresSurrogates(marker) ? 2 : 1;
        QString suffixRemainder = encodedSuffix.mid(markerLen);
        return OsmAnd::ICU::toNFC(previousSuffix.left(prefixEndOffset) + suffixRemainder);
    }

    return OsmAnd::ICU::toNFC(decodeRawSuffix(encodedSuffix));
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
    const uint32_t cp = value.at(0).unicode();
    return cp == SUFFIX_DICT_MARKER_RAW_ESCAPE || (cp >= SUFFIX_DICT_MARKER_BASE && cp <= SUFFIX_DICT_MARKER_MAX);
}

int OsmAnd::SearchAlgorithms::countCodePoints(const QString& s)
{
    const int n = s.length();
    int count = 0;
    for (int i = 0; i < n; ++count)
    {
        i += QChar::requiresSurrogates(s.at(i).unicode()) ? 2 : 1;
    }
    return count;
}

QString OsmAnd::SearchAlgorithms::nameIndexEncodeSuffix(const QString& suffix, const QString& previousSuffix)
{
    QString encodedRaw = suffix;
    if (startsWithSuffixMarker(suffix))
    {
        encodedRaw.prepend(QChar{SUFFIX_DICT_MARKER_RAW_ESCAPE});
    }
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

    const int rawCpCount = countCodePoints(encodedRaw);
    return (countCodePoints(deltaEncoded) < rawCpCount) ? deltaEncoded : encodedRaw;
}

bool OsmAnd::SearchAlgorithms::isApostropheLike(QChar ch)
{
    switch (ch.unicode())
    {
        case u'\'':
        case u'’':
        case u'ʼ':
        case u'´':
        case u'`':
        case u'′':
        case u'‵':
        case u'ʹ':
            return true;
        default:
            return false;
    }
}

