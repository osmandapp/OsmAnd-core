#include "ICU.h"
#include "ICU_private.h"

#include <QByteArray>
#include <QVector>

#include <unicode/uclean.h>
#include <unicode/udata.h>
#include <unicode/ubidi.h>
#include <unicode/ushape.h>
#include <unicode/translit.h>
#include <unicode/brkiter.h>
#include <unicode/locid.h>

#include "EmbeddedResources.h"
#include "Logging.h"

std::unique_ptr<QByteArray> g_IcuData;
const Transliterator* g_pIcuTransliterator = nullptr;
const BreakIterator* g_pIcuWordBreakIterator = nullptr;

void OsmAnd::ICU::initialize()
{
    // Initialize ICU
    UErrorCode icuError = U_ZERO_ERROR;
    g_IcuData = qMove(std::unique_ptr<QByteArray>(new QByteArray(EmbeddedResources::decompressResource(QLatin1String("icu4c/icu-data-l.xml")))));
    udata_setCommonData(g_IcuData->constData(), &icuError);
    if(!U_SUCCESS(icuError))
        LogPrintf(LogSeverityLevel::Error, "Failed to initialize ICU data: %d", icuError);
    u_init(&icuError);
    if(!U_SUCCESS(icuError))
        LogPrintf(LogSeverityLevel::Error, "Failed to initialize ICU: %d", icuError);

    // Allocate resources
    g_pIcuTransliterator = Transliterator::createInstance(UnicodeString(reinterpret_cast<const UChar*>(L"Any-Latin/UNGEGN; NFD; [:M:] Remove; NFC")), UTRANS_FORWARD, icuError);
    if(!U_SUCCESS(icuError))
        LogPrintf(LogSeverityLevel::Error, "Failed to create global ICU transliterator: %d", icuError);
    g_pIcuWordBreakIterator = BreakIterator::createWordInstance(Locale::getRoot(), icuError);
    if(!U_SUCCESS(icuError))
        LogPrintf(LogSeverityLevel::Error, "Failed to create global ICU word break iterator: %d", icuError);
}

void OsmAnd::ICU::release()
{
    // Release resources
    delete g_pIcuTransliterator;
    g_pIcuTransliterator = nullptr;
    delete g_pIcuWordBreakIterator;
    g_pIcuWordBreakIterator = nullptr;
    g_IcuData.reset();

    // Release ICU
    u_cleanup();
}

OSMAND_CORE_API QString OSMAND_CORE_CALL OsmAnd::ICU::convertToVisualOrder(const QString& input)
{
    QString output;
    const auto len = input.length();
    UErrorCode icuError = U_ZERO_ERROR;
    bool ok = true;

    // Allocate ICU BiDi context
    const auto pContext = ubidi_openSized(len, 0, &icuError);
    if(pContext == nullptr || !U_SUCCESS(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        return input;
    }

    // Configure context to reorder from logical to visual
    ubidi_setReorderingMode(pContext, UBIDI_REORDER_DEFAULT);

    // Set data
    ubidi_setPara(pContext, reinterpret_cast<const UChar*>(input.unicode()), len, UBIDI_DEFAULT_RTL, nullptr, &icuError);
    ok = U_SUCCESS(icuError);

    if(ok)
    {
        QVector<UChar> reordered(len);
        ubidi_writeReordered(pContext, reordered.data(), len, UBIDI_DO_MIRRORING | UBIDI_REMOVE_BIDI_CONTROLS, &icuError);
        ok = U_SUCCESS(icuError);

        if(ok)
        {
            QVector<UChar> reshaped(len);
            const auto newLen = u_shapeArabic(reordered.constData(), len, reshaped.data(), len, U_SHAPE_TEXT_DIRECTION_VISUAL_LTR | U_SHAPE_LETTERS_SHAPE | U_SHAPE_LENGTH_FIXED_SPACES_AT_END, &icuError);
            ok = U_SUCCESS(icuError);

            if(ok)
            {
                output = qMove(QString(reinterpret_cast<const QChar*>(reshaped.constData()), newLen));
            }
        }
    }

    // Release context
    ubidi_close(pContext);

    if(!ok)
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        return input;
    }
    return output;
}

OSMAND_CORE_API QString OSMAND_CORE_CALL OsmAnd::ICU::transliterateToLatin(const QString& input)
{
    QString output;
    UErrorCode icuError = U_ZERO_ERROR;
    bool ok = true;

    const auto pTransliterator = g_pIcuTransliterator->clone();
    if(pTransliterator == nullptr || !U_SUCCESS(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        if(pTransliterator != nullptr)
            delete pTransliterator;
        return input;
    }

    UnicodeString icuString(reinterpret_cast<const UChar*>(input.unicode()), input.length());
    pTransliterator->transliterate(icuString);
    output = qMove(QString(reinterpret_cast<const QChar*>(icuString.getBuffer()), icuString.length()));

    if(pTransliterator != nullptr)
        delete pTransliterator;

    if(!ok)
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        return input;
    }
    return output;
}

OSMAND_CORE_API QVector<int> OSMAND_CORE_CALL OsmAnd::ICU::getTextWrapping(const QString& input, const int maxCharsPerLine)
{
    assert(maxCharsPerLine > 0);
    QVector<int> result;
    UErrorCode icuError = U_ZERO_ERROR;
    bool ok = true;

    // Create break iterator
    const auto pBreakIterator = g_pIcuWordBreakIterator->clone();
    if(pBreakIterator == nullptr || !U_SUCCESS(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        if(pBreakIterator != nullptr)
            delete pBreakIterator;
        return (result << 0);
    }

    // Set text for breaking
    pBreakIterator->setText(UnicodeString(reinterpret_cast<const UChar*>(input.unicode()), input.length()));

    auto cursor = 0;
    while(ok && cursor < input.length())
    {
        // Get next desired breaking position
        auto lookAheadCursor = cursor + maxCharsPerLine;
        if(lookAheadCursor >= input.length())
            break;

        // If look-ahead cursor is still in bounds of input, and is pointing to:
        //  - control character
        //  - space character
        //  - non-spacing mark
        // then move forward until a valuable character is found
        while(lookAheadCursor < input.length())
        {
            const auto c = static_cast<UChar>(input[lookAheadCursor].unicode());
            if(!u_isspace(c) && u_charType(c) != U_CONTROL_CHAR && u_charType(c) != U_NON_SPACING_MARK)
                break;
            lookAheadCursor++;
        }

        // Now locate last legal word-break at or before the look-ahead cursor
        const auto lastBreak = pBreakIterator->preceding(lookAheadCursor + 1);

        // If last legal word-break wasn't found since current cursor, perform a hard-break
        if(lastBreak <= cursor)
        {
            result.push_back(lookAheadCursor);
            cursor = lookAheadCursor;
            continue;
        }

        // Otherwise a legal word-break was found, so move there and find next valuable character
        // and place line start there
        cursor = lastBreak;
        while(cursor < input.length())
        {
            const auto c = static_cast<UChar>(input[cursor].unicode());
            if(!u_isspace(c) && u_charType(c) != U_CONTROL_CHAR && u_charType(c) != U_NON_SPACING_MARK)
                break;
            cursor++;
        }
        result.push_back(cursor);
    }
    if(result.isEmpty())
        result.push_back(0);

    if(pBreakIterator != nullptr)
        delete pBreakIterator;

    if(!ok)
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        return (result << 0);
    }
    return result;
}

OSMAND_CORE_API QVector<QStringRef> OSMAND_CORE_CALL OsmAnd::ICU::getTextWrappingRefs(const QString& input, const int maxCharsPerLine)
{
    QVector<QStringRef> result;
    const auto lineStartIndices = getTextWrapping(input, maxCharsPerLine);
    result.reserve(lineStartIndices.size());
    auto lastStartIndex = lineStartIndices[0];
    for(auto idx = 1, count = lineStartIndices.size(); idx < count; idx++)
    {
        auto startIndex = lineStartIndices[idx];
        result.push_back(input.midRef(lastStartIndex, startIndex - lastStartIndex));
        lastStartIndex = startIndex;
    }
    result.push_back(input.midRef(lastStartIndex));
    return result;
}

OSMAND_CORE_API QStringList OSMAND_CORE_CALL OsmAnd::ICU::wrapText(const QString& input, const int maxCharsPerLine)
{
    QStringList result;
    const auto lineStartIndices = getTextWrapping(input, maxCharsPerLine);
    auto lastStartIndex = lineStartIndices[0];
    for(auto idx = 1, count = lineStartIndices.size(); idx < count; idx++)
    {
        auto startIndex = lineStartIndices[idx];
        result.push_back(input.mid(lastStartIndex, startIndex - lastStartIndex));
        lastStartIndex = startIndex;
    }
    result.push_back(input.mid(lastStartIndex));
    return result;
}
