#include "TextRasterizer.h"
#include "TextRasterizer_P.h"
#include "TextRasterizer_internal.h"

#include "EmbeddedFontFinder.h"
#include "SystemFontFinder.h"
#include "CachingFontFinder.h"
#include "ChainedFontFinder.h"

OsmAnd::TextRasterizer::TextRasterizer(
    const std::shared_ptr<const IFontFinder>& fontFinder_)
    : _p(new TextRasterizer_P(this))
    , fontFinder(fontFinder_)
{
}

OsmAnd::TextRasterizer::~TextRasterizer()
{
}

sk_sp<SkImage> OsmAnd::TextRasterizer::rasterize(
    const QString& text,
    const Style& style /*= Style()*/,
    QVector<SkScalar>* const outGlyphWidths /*= nullptr*/,
    float* const outExtraTopSpace /*= nullptr*/,
    float* const outExtraBottomSpace /*= nullptr*/,
    float* const outLineSpacing /*= nullptr*/,
    float* const outFontAscent /*= nullptr*/) const
{
    return _p->rasterize(
        text,
        style,
        outGlyphWidths,
        outExtraTopSpace,
        outExtraBottomSpace,
        outLineSpacing,
        outFontAscent);
}

bool OsmAnd::TextRasterizer::rasterize(
    SkBitmap& targetBitmap,
    const QString& text,
    const Style& style /*= Style()*/,
    QVector<SkScalar>* const outGlyphWidths /*= nullptr*/,
    float* const outExtraTopSpace /*= nullptr*/,
    float* const outExtraBottomSpace /*= nullptr*/,
    float* const outLineSpacing /*= nullptr*/,
    float* const outFontAscent /*= nullptr*/) const
{
    return _p->rasterize(
        targetBitmap,
        text,
        style,
        outGlyphWidths,
        outExtraTopSpace,
        outExtraBottomSpace,
        outLineSpacing,
        outFontAscent);
}

static std::shared_ptr<const OsmAnd::TextRasterizer> s_defaultTextRasterizer;
std::shared_ptr<const OsmAnd::TextRasterizer> OsmAnd::TextRasterizer::getDefault()
{
    return s_defaultTextRasterizer;
}

static std::shared_ptr<const OsmAnd::TextRasterizer> s_onlySystemFontsTextRasterizer;
std::shared_ptr<const OsmAnd::TextRasterizer> OsmAnd::TextRasterizer::getOnlySystemFonts()
{
    return s_onlySystemFontsTextRasterizer;
}

void OsmAnd::TextRasterizer_initialize()
{
    s_defaultTextRasterizer.reset(new TextRasterizer(
        std::shared_ptr<const IFontFinder>(new CachingFontFinder(
            std::shared_ptr<const IFontFinder>(new ChainedFontFinder(
                QList< std::shared_ptr<const IFontFinder> >()
                    << EmbeddedFontFinder::getDefaultInstance()
                    << std::shared_ptr<const IFontFinder>(new SystemFontFinder())))))));
    s_onlySystemFontsTextRasterizer.reset(new TextRasterizer(std::shared_ptr<const IFontFinder>(new CachingFontFinder(
        std::shared_ptr<const IFontFinder>(new SystemFontFinder())))));
}

void OsmAnd::TextRasterizer_release()
{
    s_defaultTextRasterizer.reset();
    s_onlySystemFontsTextRasterizer.reset();
}
