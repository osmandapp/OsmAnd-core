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
            SkFont font;
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
                , maxBoundsTop(0)
                , fontAscent(0)
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
        SkPaint getHaloPaint(const SkPaint& paint, const Style& style) const;
        SkFont getHaloFont(const SkFont& font, const Style& style) const;
        void measureHalo(const Style& style, QVector<LinePaint>& paints) const;
        void measureHaloGlyphs(const Style& style, const QVector<LinePaint>& paints, QVector<SkScalar>& outGlyphWidths) const;
        SkRect positionText(
            QVector<LinePaint>& paints,
            const SkScalar maxLineWidth,
            const Style::TextAlignment textAlignment) const;
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
