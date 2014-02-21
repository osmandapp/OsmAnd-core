#include "ICU.h"

#include <QVector>

#include <unicode/ubidi.h>
#include <unicode/ushape.h>

#include "Logging.h"

OSMAND_CORE_API QString OSMAND_CORE_CALL OsmAnd::ICU::convertToVisualOrder(const QString& input)
{
    QString output;
    const auto len = input.length();
    UErrorCode icuError = U_ZERO_ERROR;
    bool ok = true;

    // Allocate ICU BiDi context
    const auto context = ubidi_openSized(len, 0, &icuError);
    if(!context || !U_SUCCESS(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        return input;
    }

    // Configure context to reorder from logical to visual
    ubidi_setReorderingMode(context, UBIDI_REORDER_DEFAULT);

    // Set data
    ubidi_setPara(context, reinterpret_cast<const UChar*>(input.unicode()), len, UBIDI_DEFAULT_RTL, nullptr, &icuError);
    ok = U_SUCCESS(icuError);

    if(ok)
    {
        QVector<UChar> reordered(len);
        ubidi_writeReordered(context, reordered.data(), len, UBIDI_DO_MIRRORING | UBIDI_REMOVE_BIDI_CONTROLS, &icuError);
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
    ubidi_close(context);

    if(!ok)
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        return input;
    }
    return output;
}
