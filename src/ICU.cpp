#include "ICU.h"
#include "ICU_private.h"
#include "CollatorStringMatcher.h"

#include <cassert>
#include <cstring>

#include <algorithm>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QByteArray>
#include <QVector>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <unicode/uclean.h>
#include <unicode/udata.h>
#include <unicode/ubidi.h>
#include <unicode/ushape.h>
#include <unicode/translit.h>
#include <unicode/brkiter.h>
#include <unicode/coll.h>
#include <unicode/urename.h>

#include "restore_internal_warnings.h"

#include "CoreResourcesEmbeddedBundle.h"
#include "Logging.h"
#include "utf8/unchecked.h"

std::unique_ptr<QByteArray> g_IcuData;
const Transliterator* g_pIcuAnyToLatinTransliterator = nullptr;
const Transliterator* g_pIcuAccentsAndDiacriticsConverter = nullptr;
const BreakIterator* g_pIcuLineBreakIterator = nullptr;
const Collator* g_pIcuCollator = nullptr;

bool OsmAnd::ICU::initialize()
{
    // Initialize ICU
    UErrorCode icuError = U_ZERO_ERROR;
    g_IcuData = qMove(std::unique_ptr<QByteArray>(new QByteArray(
        getCoreResourcesProvider()->getResource(QLatin1String("misc/icu4c/icu-data-l.dat")))));
    udata_setCommonData(g_IcuData->constData(), &icuError);
    if (U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to initialize ICU data: %d", icuError);
        return false;
    }
    u_init(&icuError);
    if (U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to initialize ICU: %d", icuError);
        return false;
    }

    // Allocate resources:
    g_pIcuAnyToLatinTransliterator = Transliterator::createInstance(UnicodeString("Any-Latin/BGN"), UTRANS_FORWARD, icuError);
    if (U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to create global ICU Any-to-Latin transliterator: %d", icuError);
        return false;
    }

    icuError = U_ZERO_ERROR;
    g_pIcuAccentsAndDiacriticsConverter = Transliterator::createInstance(UnicodeString("NFD; [:Mn:] Remove; NFC"), UTRANS_FORWARD, icuError);
    if (U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to create global ICU accents&diacritics converter: %d", icuError);
        return false;
    }

    // "Line-break Boundary" is best for breaking text into lines when wrapping text
    icuError = U_ZERO_ERROR;
    g_pIcuLineBreakIterator = BreakIterator::createLineInstance(Locale::getRoot(), icuError);
    if (U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to create global ICU line break iterator: %d", icuError);
        return false;
    }
    
    Collator *collator = nullptr;
    icuError = U_ZERO_ERROR;
    Locale locale = Locale::getDefault();
    if (std::strcmp(locale.getLanguage(), "ro") ||
        std::strcmp(locale.getLanguage(), "cs") ||
        std::strcmp(locale.getLanguage(), "sk"))
    {
        collator = Collator::createInstance(Locale("en", "US"), icuError);
    }
    else
    {
        collator = Collator::createInstance(icuError);
    }
    
    if (U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to create global ICU collator: %d", icuError);
        return false;
    }
    else
    {
        collator->setStrength(Collator::PRIMARY);
        g_pIcuCollator = collator;
    }
    
    return true;
}

void OsmAnd::ICU::release()
{
    // Release resources:

    delete g_pIcuCollator;
    g_pIcuCollator = nullptr;

    delete g_pIcuAccentsAndDiacriticsConverter;
    g_pIcuAccentsAndDiacriticsConverter = nullptr;
    
    delete g_pIcuAnyToLatinTransliterator;
    g_pIcuAnyToLatinTransliterator = nullptr;

    delete g_pIcuLineBreakIterator;
    g_pIcuLineBreakIterator = nullptr;

    g_IcuData.reset();

    // Release ICU
    u_cleanup();
}

template <typename T>
void logBytes(const T* buf, size_t sz)
{
    for (size_t i = 0; i < sz; ++i)
    {
        if (i > 0) printf(":");
        printf("0x%02X", buf[i]);
    }
    printf("\n");
}

#define CUSTOM_IMPL (1)
#if CUSTOM_IMPL != 1
// modification
static QChar mirror(QChar ch)
{
    if (ch == '(') return QChar(')');
    if (ch == ')') return QChar('(');
    if (ch == '[') return QChar(']');
    if (ch == ']') return QChar('[');
    if (ch == '}') return QChar('{');
    if (ch == '{') return QChar('}');
    return ch;
}
OSMAND_CORE_API QString OSMAND_CORE_CALL OsmAnd::ICU::convertToVisualOrder(const QString& input)
{
    QString output;
    auto len = input.length();
    UErrorCode icuError = U_ZERO_ERROR;
    bool ok = true;

    // Allocate ICU BiDi context
    const auto pContext = ubidi_openSized(len, 0, &icuError);
    if (pContext == nullptr || U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        return input;
    }

    // Configure context to reorder from logical to visual
    ubidi_setReorderingMode(pContext, UBIDI_REORDER_DEFAULT);

    QVector<UChar> reshaped(len);
    const auto newLen = u_shapeArabic(reinterpret_cast<const UChar*>(input.unicode()), len, reshaped.data(), len, U_SHAPE_TEXT_DIRECTION_VISUAL_LTR | U_SHAPE_LETTERS_SHAPE | U_SHAPE_LENGTH_FIXED_SPACES_AT_END, &icuError);
    ok = U_SUCCESS(icuError);
    if (ok)
    {
        ubidi_setPara(pContext, reshaped.data(), len, UBIDI_DEFAULT_LTR, nullptr, &icuError);
        ok = U_SUCCESS(icuError);
        if (ok)
        {
            auto direction = ubidi_getDirection(pContext);
            if (direction == UBIDI_LTR || direction == UBIDI_RTL)
            {
                output = input;
            }
            else if (direction == UBIDI_MIXED)
            {
                int numberOfParts = ubidi_countRuns(pContext, &icuError);
                ok = U_SUCCESS(icuError);
                if(ok)
                {
                    QString outputReverted;
                    int start;
                    int subLength;
                    bool mirroring = true;
                    for(int i = 0; i < numberOfParts; ++i)
                    {
                        bool ltr = ubidi_getVisualRun(pContext, i, &start, &subLength) == UBIDI_RTL;
                        //outputReverted = input.mid(start, subLength);
                        int begin = ltr ? start : start + subLength - 1;
                        int end = ltr ? start + subLength : start - 1;
                        int ind = begin;
                        while (ind != end)
                        {
                            QChar ch = input[ind];
                            if (!ltr && mirroring)
                            { // !
                                //ch = mirror(ch);
                            }
                            outputReverted.append(ch);
                            if (ltr)
                            { // !
                                ind++;
                            } else {
                                ind--;
                            }
                        }
                    }
                    output += outputReverted;
                }
                //if (!mirror)
                {
                    std::reverse(output.begin(), output.end());
                }
            }
        }
    }
    // Release context
    ubidi_close(pContext);

    if (!ok)
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        return input;
    }
    return output;
}
#else
OSMAND_CORE_API QString OSMAND_CORE_CALL OsmAnd::ICU::convertToVisualOrder(const QString& input)
{
    QString output;
    auto len = input.length();
    UErrorCode icuError = U_ZERO_ERROR;
    bool ok = true;

    // Allocate ICU BiDi context
    const auto pContext = ubidi_openSized(len, 0, &icuError);
    if (pContext == nullptr || U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        return input;
    }

    // Configure context to reorder from logical to visual
    ubidi_setReorderingMode(pContext, UBIDI_REORDER_DEFAULT);

    ubidi_setPara(pContext, reinterpret_cast<const UChar*>(input.unicode()), len, UBIDI_DEFAULT_LTR, nullptr, &icuError);
    ok = U_SUCCESS(icuError);
    if (ok)
    {
        auto direction = ubidi_getDirection(pContext);
        if (direction == UBIDI_LTR || direction == UBIDI_RTL)
        {
            output = input;
        }
        else if (direction == UBIDI_MIXED)
        {
            int numberOfParts = ubidi_countRuns(pContext, &icuError);
            ok = U_SUCCESS(icuError);
            if(ok)
            {
                QString outputReverted;
                int start;
                int subLength;
                for(int i = 0; i < numberOfParts; ++i)
                {
                    direction = ubidi_getVisualRun(pContext, i, &start, &subLength);
                    outputReverted = input.mid(start, subLength);
                    if (UBIDI_RTL == direction)
                    {
                        std::reverse(outputReverted.begin(), outputReverted.end());
                        output += outputReverted;
                    }
                    else
                    {
                        output += outputReverted;
                    }
                }
            }
            std::reverse(output.begin(), output.end());
        }
    }

    // Release context
    ubidi_close(pContext);

    if (!ok)
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        return input;
    }
    return output;
}
#endif

OSMAND_CORE_API QString OSMAND_CORE_CALL OsmAnd::ICU::transliterateToLatin(
    const QString& input,
    const bool keepAccentsAndDiacriticsInInput /*= true*/,
    const bool keepAccentsAndDiacriticsInOutput /*= true*/)
{
    QString output;
    UErrorCode icuError = U_ZERO_ERROR;
    bool ok = true;

    const auto pAnyToLatinTransliterator = g_pIcuAnyToLatinTransliterator->clone();
    if (pAnyToLatinTransliterator == nullptr || U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        if (pAnyToLatinTransliterator != nullptr)
            delete pAnyToLatinTransliterator;
        return input;
    }

    // Transliterate from any to latin
    UnicodeString icuString(reinterpret_cast<const UChar*>(input.unicode()), input.length());
    pAnyToLatinTransliterator->transliterate(icuString);
    output = qMove(QString(reinterpret_cast<const QChar*>(icuString.getBuffer()), icuString.length()));

    // If input and output differ at this point or accents/diacritics should be converted,
    // normalize the output again
    if ((input.compare(output, Qt::CaseInsensitive) != 0 || !keepAccentsAndDiacriticsInInput) && !keepAccentsAndDiacriticsInOutput)
    {
        const auto pIcuAccentsAndDiacriticsConverter = g_pIcuAccentsAndDiacriticsConverter->clone();
        ok = pIcuAccentsAndDiacriticsConverter != nullptr && U_SUCCESS(icuError);
        if (ok)
        {
            pIcuAccentsAndDiacriticsConverter->transliterate(icuString);
            output = QString(reinterpret_cast<const QChar*>(icuString.getBuffer()), icuString.length());
        }

        if (pIcuAccentsAndDiacriticsConverter != nullptr)
            delete pIcuAccentsAndDiacriticsConverter;
    }
    
    if (pAnyToLatinTransliterator != nullptr)
        delete pAnyToLatinTransliterator;

    if (!ok)
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        return input;
    }
    return output;
}

#if CUSTOM_IMPL == 0
// modification
OSMAND_CORE_API QVector<int> OSMAND_CORE_CALL OsmAnd::ICU::getTextWrapping(const QString& input, const int maxCharsPerLine, const int maxLines /*= 0*/)
{
    if (input.isEmpty())
    {
        return {};
    }

    bool ltr = false;
    assert(maxCharsPerLine > 0);
    QVector<int> result;
    UErrorCode icuError = U_ZERO_ERROR;
    bool ok = true;

    // Create break iterator
    const auto pBreakIterator = g_pIcuLineBreakIterator->clone();
    if (pBreakIterator == nullptr || U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        if (pBreakIterator != nullptr)
            delete pBreakIterator;
        return (result << 0);
    }

    // Set text for breaking
    pBreakIterator->setText(UnicodeString(reinterpret_cast<const UChar*>(input.unicode()), input.length()));

    auto findBreakPos = [&input, ltr] (auto& cursor)
    {
        bool inBounds = ltr ? cursor < input.length() : cursor > 0;
        while(inBounds)
        {
            const auto c = static_cast<UChar>(input[cursor].unicode());
            if (!u_isspace(c) && u_charType(c) != U_CONTROL_CHAR && u_charType(c) != U_NON_SPACING_MARK)
            {
                break;
            }
            cursor = ltr ? cursor + 1 : cursor - 1;
            inBounds = ltr ? cursor < input.length() : cursor > 0;
        }
    };

    auto cursor = ltr ? 0 : input.length() - 1;
    while(ok && ltr ? cursor < input.length() : cursor >= 0)
    {
        if (maxLines > 0 && result.size() > maxLines)
            break;
        // Get next desired breaking position
        auto lookAheadCursor = ltr ? cursor + maxCharsPerLine : cursor - maxCharsPerLine;
        bool needsBreak = ltr ? lookAheadCursor >= input.length() : lookAheadCursor <= 0;
        if (needsBreak)
        {
            break;
        }

        // If look-ahead cursor is still in bounds of input, and is pointing to:
        //  - control character
        //  - space character
        //  - non-spacing mark
        // then move forward until a valuable character is found
        findBreakPos(lookAheadCursor);

        // Now locate last legal line-break at or before the look-ahead cursor
        const auto lastBreak = ltr ? pBreakIterator->preceding(lookAheadCursor + 1) : pBreakIterator->following(lookAheadCursor);

        // If last legal line-break wasn't found since current cursor, perform a hard-break
        bool hardBreak = ltr ? lastBreak <= cursor : lastBreak >= cursor;
        if (hardBreak)
        {
            result.push_back(lookAheadCursor);
            cursor = lookAheadCursor;
            continue;
        }

        // Otherwise a legal line-break was found, so move there and find next valuable character
        // and place line start there
        cursor = lastBreak;
        // while(cursor < input.length())
        // {
        //     const auto c = static_cast<UChar>(input[cursor].unicode());
        //     if (!u_isspace(c) && u_charType(c) != U_CONTROL_CHAR && u_charType(c) != U_NON_SPACING_MARK)
        //         break;
        //     cursor++;
        // }
        findBreakPos(cursor);
        result.push_back(cursor);
    }
    if (result.isEmpty() || result.first() != 0)
        result.prepend(0);

    if (pBreakIterator != nullptr)
        delete pBreakIterator;

    if (!ok)
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        return (result << 0);
    }
    return result;
}
#else
OSMAND_CORE_API QVector<int> OSMAND_CORE_CALL OsmAnd::ICU::getTextWrapping(const QString& input, const int maxCharsPerLine, const int maxLines /*= 0*/)
{
    assert(maxCharsPerLine > 0);
    QVector<int> result;
    UErrorCode icuError = U_ZERO_ERROR;
    bool ok = true;

    // Create break iterator
    const auto pBreakIterator = g_pIcuLineBreakIterator->clone();
    if (pBreakIterator == nullptr || U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        if (pBreakIterator != nullptr)
            delete pBreakIterator;
        return (result << 0);
    }

    // Set text for breaking
    pBreakIterator->setText(UnicodeString(reinterpret_cast<const UChar*>(input.unicode()), input.length()));

    auto cursor = 0;
    while(ok && cursor < input.length())
    {
        if (maxLines > 0 && result.size() > maxLines)
            break;
        // Get next desired breaking position
        auto lookAheadCursor = cursor + maxCharsPerLine;
        if (lookAheadCursor >= input.length())
            break;

        // If look-ahead cursor is still in bounds of input, and is pointing to:
        //  - control character
        //  - space character
        //  - non-spacing mark
        // then move forward until a valuable character is found
        while(lookAheadCursor < input.length())
        {
            const auto c = static_cast<UChar>(input[lookAheadCursor].unicode());
            if (!u_isspace(c) && u_charType(c) != U_CONTROL_CHAR && u_charType(c) != U_NON_SPACING_MARK)
                break;
            lookAheadCursor++;
        }

        // Now locate last legal line-break at or before the look-ahead cursor
        const auto lastBreak = pBreakIterator->preceding(lookAheadCursor + 1);

        // If last legal line-break wasn't found since current cursor, perform a hard-break
        if (lastBreak <= cursor)
        {
            result.push_back(lookAheadCursor);
            cursor = lookAheadCursor;
            continue;
        }

        // Otherwise a legal line-break was found, so move there and find next valuable character
        // and place line start there
        cursor = lastBreak;
        while(cursor < input.length())
        {
            const auto c = static_cast<UChar>(input[cursor].unicode());
            if (!u_isspace(c) && u_charType(c) != U_CONTROL_CHAR && u_charType(c) != U_NON_SPACING_MARK)
                break;
            cursor++;
        }
        result.push_back(cursor);
    }
    if (result.isEmpty() || result.first() != 0)
        result.prepend(0);

    if (pBreakIterator != nullptr)
        delete pBreakIterator;

    if (!ok)
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        return (result << 0);
    }
    return result;
}
#endif

#if CUSTOM_IMPL == 1
int nextWord(uint8_t* s, uint* charRead)
{
    uint8_t* init = s;
    while ((*s) != 0) {
        uint32_t tp = utf8::unchecked::next(s);
        (*charRead)++;
        if (tp == ' ' || tp == '\t') {
            return (s - init);
        }
    }
    return -1;
}
OSMAND_CORE_API QVector<QStringRef> OSMAND_CORE_CALL OsmAnd::ICU::getTextWrappingRefs(
    const QString& input,
    const int maxCharsPerLine,
    const int maxLines /*= 0*/)
{
    QVector<QStringRef> result = {};
    auto inpSize = input.size();
    std::string text = input.toUtf8().constData();
    logBytes(text.c_str(), text.length());
    if (text.length() > maxCharsPerLine) {
        const char* c_str = text.c_str();

        int end = text.length();
        int endCh = input.length();
        int line = 0;
        int pos = 0;
        int start = 0;
        int startCh = 0;
        int posCh = 0;
        while (start < end)
        {
            const char* p_str = c_str;
            uint charRead = 0;
            uint charInLine = 0;
            do
            {
                int lastSpace = nextWord((uint8_t*)p_str, &charRead);
                if (lastSpace == -1){
                    pos = end;
                    posCh = endCh;
                } else {
                    p_str += lastSpace;
                    charInLine += charRead;
                    if (pos != start && charInLine >= maxCharsPerLine) {
                        break;
                    }
                    posCh += charRead;
                    pos += lastSpace;
                }
            }
            while (pos < end && charRead < maxCharsPerLine);

            // PROFILE_NATIVE_OPERATION(
            //     rc, drawTextOnCanvas(rc, cv, c_str, pos - start, text->centerX, text->centerY + line * (textSize + 2),
            //                             paintText, text->textShadowColor, text->textShadow, skFontText, fontEntry));
            result.push_back(input.midRef(startCh, posCh - startCh));
            c_str += (pos - start);
            start = pos;
            startCh = posCh;
            line++;
        }
    }
    else
    {
        result.push_back(input.midRef(0));
    }

    return result;
}
// {
//     bool ltr = false;
//     QVector<QStringRef> result;
//     const auto lineStartIndices = getTextWrapping(input, maxCharsPerLine, maxLines);
//     result.reserve(lineStartIndices.size());
//     auto lastStartIndex = ltr ? lineStartIndices[0] : input.length() - 1;
//     int linesCount = 0;
//     for(auto idx = 1, count = lineStartIndices.size(); idx < count; idx++)
//     {
//         auto startIndex = lineStartIndices[idx];
//         result.push_back(ltr ?
//             input.midRef(lastStartIndex, startIndex - lastStartIndex) :
//             input.midRef(lastStartIndex - startIndex, startIndex));
//         lastStartIndex = ltr ? startIndex : input.length() - startIndex - 1;
//         linesCount++;
//         if (maxLines > 0 && linesCount == maxLines)
//             break;
//     }
//     if (maxLines <= 0 || linesCount < maxLines)
//         result.push_back(ltr ? input.midRef(lastStartIndex) : input.midRef(0, lastStartIndex));
    
//     return result;
// }
#else
OSMAND_CORE_API QVector<QStringRef> OSMAND_CORE_CALL OsmAnd::ICU::getTextWrappingRefs(
    const QString& input,
    const int maxCharsPerLine,
    const int maxLines /*= 0*/)
{
    QVector<QStringRef> result;
    const auto lineStartIndices = getTextWrapping(input, maxCharsPerLine, maxLines);
    result.reserve(lineStartIndices.size());
    auto lastStartIndex = lineStartIndices[0];
    int linesCount = 0;
    for(auto idx = 1, count = lineStartIndices.size(); idx < count; idx++)
    {
        auto startIndex = lineStartIndices[idx];
        result.push_back(input.midRef(lastStartIndex, startIndex - lastStartIndex));
        lastStartIndex = startIndex;
        linesCount++;
        if (maxLines > 0 && linesCount == maxLines)
            break;
    }
    if (maxLines <= 0 || linesCount < maxLines)
        result.push_back(input.midRef(lastStartIndex));
    
    return result;
}
#endif

OSMAND_CORE_API QStringList OSMAND_CORE_CALL OsmAnd::ICU::wrapText(const QString& input, const int maxCharsPerLine, const int maxLines /*= 0*/)
{
    QStringList result;
    const auto lineStartIndices = getTextWrapping(input, maxCharsPerLine, maxLines);
    auto lastStartIndex = lineStartIndices[0];
    int linesCount = 0;
    for(auto idx = 1, count = lineStartIndices.size(); idx < count; idx++)
    {
        auto startIndex = lineStartIndices[idx];
        result.push_back(input.mid(lastStartIndex, startIndex - lastStartIndex));
        lastStartIndex = startIndex;
        linesCount++;
        if (maxLines > 0 && linesCount == maxLines)
            break;
    }
    if (maxLines <= 0 || linesCount < maxLines)
        result.push_back(input.mid(lastStartIndex));

    return result;
}

OSMAND_CORE_API QString OSMAND_CORE_CALL OsmAnd::ICU::stripAccentsAndDiacritics(const QString& input)
{
    QString output;
    UErrorCode icuError = U_ZERO_ERROR;
    bool ok = true;

    const auto pIcuAccentsAndDiacriticsConverter = g_pIcuAccentsAndDiacriticsConverter->clone();
    if (pIcuAccentsAndDiacriticsConverter == nullptr || U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        if (pIcuAccentsAndDiacriticsConverter != nullptr)
            delete pIcuAccentsAndDiacriticsConverter;
        return input;
    }

    // Remove accents and diacritics
    UnicodeString icuString(reinterpret_cast<const UChar*>(input.unicode()), input.length());
    pIcuAccentsAndDiacriticsConverter->transliterate(icuString);
    output = qMove(QString(reinterpret_cast<const QChar*>(icuString.getBuffer()), icuString.length()));

    if (pIcuAccentsAndDiacriticsConverter != nullptr)
        delete pIcuAccentsAndDiacriticsConverter;

    if (!ok)
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        return input;
    }
    return output;
}

UnicodeString qStrToUniStr(QString input)
{
    UnicodeString icuString(reinterpret_cast<const UChar*>(input.unicode()), input.length());
    return icuString;
}

bool isSpace(UChar c)
{
    return !u_isalnum(c);
}

OSMAND_CORE_API bool OSMAND_CORE_CALL OsmAnd::ICU::cmatches(const QString& _base, const QString& _part, StringMatcherMode _mode)
{
    switch (_mode)
    {
        case StringMatcherMode::CHECK_CONTAINS:
            return ccontains(_base, _part);
        case StringMatcherMode::CHECK_EQUALS_FROM_SPACE:
            return cstartsWith(_base, _part, true, true, true);
        case StringMatcherMode::CHECK_STARTS_FROM_SPACE:
            return cstartsWith(_base, _part, true, true, false);
        case StringMatcherMode::CHECK_STARTS_FROM_SPACE_NOT_BEGINNING:
            return cstartsWith(_base, _part, false, true, false);
        case StringMatcherMode::CHECK_ONLY_STARTS_WITH:
            return cstartsWith(_base, _part, true, false, false);
        case StringMatcherMode::CHECK_EQUALS:
            return cstartsWith(_base, _part, false, false, true);
        default:
            return false;
    }
}
OSMAND_CORE_API bool OSMAND_CORE_CALL OsmAnd::ICU::ccontains(const QString& _base, const QString& _part)
{
    UErrorCode icuError = U_ZERO_ERROR;
    bool result = false;
    const auto collator = g_pIcuCollator->clone();
    if (collator == nullptr || U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        if (collator != nullptr)
            delete collator;
        return false;
    }
    else
    {
        UnicodeString baseString = qStrToUniStr(_base);
        UnicodeString partString = qStrToUniStr(_part);
        
        if (baseString.length() <= partString.length())
            return collator->equals(baseString, partString);
        
        for (int pos = 0; pos <= baseString.length() - partString.length() + 1; pos++)
        {
            UnicodeString temp = baseString.tempSubString(pos, baseString.length());
            
            for (int length = temp.length(); length >= 0; length--)
            {
                UnicodeString temp2 = temp.tempSubString(0, length);
                if (collator->equals(temp2, partString))
                {
                    result = true;
                    break;
                }
            }
            if (result)
                break;
        }
    }
    if (collator != nullptr)
        delete collator;
    return result;
}
OSMAND_CORE_API bool OSMAND_CORE_CALL OsmAnd::ICU::cstartsWith(const QString& _searchInParam, const QString& _theStart,
                                                  bool checkBeginning, bool checkSpaces, bool equals)
{
    UErrorCode icuError = U_ZERO_ERROR;
    bool result = false;
    const auto collator = g_pIcuCollator->clone();
    if (collator == nullptr || U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        if (collator != nullptr)
            delete collator;
        return false;
    }
    else
    {
        // FUTURE: This is not effective code, it runs on each comparision
        // It would be more efficient to normalize all strings in file and normalize search string before collator
        UnicodeString searchIn = qStrToUniStr(OsmAnd::CollatorStringMatcher::simplifyStringAndAlignChars(_searchInParam));
        QString theStartAligned = OsmAnd::CollatorStringMatcher::alignChars(_theStart);
        UnicodeString theStart = qStrToUniStr(theStartAligned);
        
        int startLength = theStart.length();
        int serchInLength = searchIn.length();
        
        if (startLength == 0)
        {
            result = true;
        }
        // this is not correct without (simplifyStringAndAlignChars) because of Auhofstrasse != Auhofstraße
        if (startLength > serchInLength)
        {
            result = false;
        }
        
        if (checkBeginning)
        {
            bool starts = collator->equals(searchIn.tempSubString(0, startLength), theStart);
            if (starts)
            {
                if (equals)
                {
                    if (startLength == serchInLength || isSpace(searchIn.charAt(startLength)))
                    {
                        result = true;
                    }
                }
                else
                {
                    result = true;
                }
            }
        }
        
        if (!result && checkSpaces)
        {
            for (int i = 1; i <= serchInLength - startLength; i++)
            {
                if (isSpace(searchIn.charAt(i - 1)) && !isSpace(searchIn.charAt(i)))
                {
                    if (collator->equals(searchIn.tempSubString(i, startLength), theStart))
                    {
                        if (equals)
                        {
                            if (i + startLength == serchInLength || isSpace(searchIn.charAt(i + startLength)))
                            {
                                result = true;
                                break;
                            }
                        }
                        else
                        {
                            result = true;
                            break;
                        }
                    }
                }
            }
        }
        if (!checkBeginning && !checkSpaces && equals)
            result = collator->equals(searchIn, theStart);
    }
    
    if (collator != nullptr)
        delete collator;
    return result;
}

OSMAND_CORE_API int OSMAND_CORE_CALL OsmAnd::ICU::ccompare(const QString& _s1, const QString& _s2)
{
    UErrorCode icuError = U_ZERO_ERROR;
    int result = 0;
    const auto collator = g_pIcuCollator->clone();
    if (collator == nullptr || U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        if (collator != nullptr)
            delete collator;
        return result;
    }
    else
    {
        UnicodeString s1 = qStrToUniStr(_s1);
        UnicodeString s2 = qStrToUniStr(_s2);
        result = collator->compare(s1, s2);
    }
    if (collator != nullptr)
        delete collator;
    return result;
}
