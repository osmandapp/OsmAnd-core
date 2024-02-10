#ifndef _OSMAND_CORE_TEXT_RASTERIZER_P_H_
#define _OSMAND_CORE_TEXT_RASTERIZER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QList>
#include <QVector>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkCanvas.h>
#include <SkPaint.h>
#include <SkFont.h>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <hb.h>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "MapCommonTypes.h"
#include "TextRasterizer.h"

namespace OsmAnd
{
    class BinaryMapObject;
    class IQueryController;

    class TextRasterizer;
    class TextRasterizer_P Q_DECL_FINAL
    {
    public:
        typedef TextRasterizer::Style Style;

        static constexpr auto HB_FONT_SCALE_FACTOR = 64.0f;

    private:
        SkPaint _defaultPaint;
        SkFont _defaultFont;

        struct TextPaint
        {
            inline TextPaint()
                : height(0)
                , width(0)
            {
            }

            QStringRef text;
            SkPaint paint;
            std::shared_ptr<const ITypefaceFinder::Typeface> typeface;
            SkFont skFont;
            std::shared_ptr<hb_font_t> hbFont;
            SkScalar height;
            SkScalar width;
            SkRect bounds;
            SkRect positionedBounds;
        };
        struct LinePaint
        {
            inline LinePaint()
                : maxFontHeight(0)
                , minFontHeight(std::numeric_limits<SkScalar>::max())
                , maxFontLineSpacing(0)
                , minFontLineSpacing(std::numeric_limits<SkScalar>::max())
                , maxFontTop(0)
                , minFontTop(std::numeric_limits<SkScalar>::max())
                , maxFontBottom(0)
                , minFontBottom(std::numeric_limits<SkScalar>::max())
                , fontAscent(0)
                , maxBoundsTop(0)
                , minBoundsTop(std::numeric_limits<SkScalar>::max())
                , width(0)
            {
            }

            QStringRef line;
            QVector<TextPaint> textPaints;
            SkScalar maxFontHeight;
            SkScalar minFontHeight;
            SkScalar maxFontLineSpacing;
            SkScalar minFontLineSpacing;
            SkScalar maxFontTop;
            SkScalar minFontTop;
            SkScalar maxFontBottom;
            SkScalar minFontBottom;
            SkScalar fontAscent;
            SkScalar maxBoundsTop;
            SkScalar minBoundsTop;
            SkScalar width;
        };
        QVector<LinePaint> evaluatePaints(const QVector<QStringRef>& lineRefs, const Style& style) const;
        void measureText(QVector<LinePaint>& paints, SkScalar& outMaxLineWidth) const;
        void measureGlyphs(const QVector<LinePaint>& paints, QVector<SkScalar>& outGlyphWidths) const;
        TextPaint getHaloTextPaint(const TextPaint& textPaint, const Style& style) const;
        void measureHalo(const Style& style, QVector<LinePaint>& paints) const;
        void measureHaloGlyphs(const Style& style, const QVector<LinePaint>& paints, QVector<SkScalar>& outGlyphWidths) const;
        SkRect positionText(
            QVector<LinePaint>& paints,
            const SkScalar maxLineWidth,
            const Style::TextAlignment textAlignment) const;
        bool drawPart(SkCanvas& canvas, const TextPaint& textPaint, const QString& text, bool rtl,
            const std::shared_ptr<hb_buffer_t>& hbBuffer, SkPoint& origin) const;
        bool drawText(SkCanvas& canvas, const TextPaint& textPaint) const;
        inline bool isRtlChar(const QChar& ch) const
        {
            auto d = ch.direction();
            bool result = d == QChar::DirR
                || d == QChar::DirAL
                || d == QChar::DirRLE
                || d == QChar::DirRLO
                || d == QChar::DirRLI;
            return result;
        }
        inline bool isNotRtlChar(const QChar& ch) const
        {
            return !isRtlChar(ch) && ch.direction() != QChar::DirAN;
        }
        inline bool isNotLtrChar(const QChar& ch) const
        {
            auto d = ch.direction();
            bool result = d == QChar::DirB
                || d == QChar::DirS
                || d == QChar::DirWS
                || d == QChar::DirON;
            return result;
        }

    protected:
        TextRasterizer_P(TextRasterizer* const owner);
    public:
        ~TextRasterizer_P();

        ImplementationInterface<TextRasterizer> owner;

        sk_sp<SkImage> rasterize(
            const QString& text,
            const Style& style,
            QVector<SkScalar>* const outGlyphWidths,
            float* const outExtraTopSpace,
            float* const outExtraBottomSpace,
            float* const outLineSpacing,
            float* const outFontAscent) const;

        bool rasterize(
            SkBitmap& targetBitmap,
            const QString& text,
            const Style& style,
            QVector<SkScalar>* const outGlyphWidths,
            float* const outExtraTopSpace,
            float* const outExtraBottomSpace,
            float* const outLineSpacing,
            float* const outFontAscent) const;

    friend class OsmAnd::TextRasterizer;
    };
}

#endif // !defined(_OSMAND_CORE_TEXT_RASTERIZER_P_H_)
