#include "TextRasterizer.h"
#include "TextRasterizer_P.h"

#include "TextRasterizer_private.h"

static std::shared_ptr<const OsmAnd::TextRasterizer> s_globalTextRasterizer;

OsmAnd::TextRasterizer::TextRasterizer()
    : _p(new TextRasterizer_P(this))
{
    _p->initialize();
}

OsmAnd::TextRasterizer::~TextRasterizer()
{
}

const OsmAnd::TextRasterizer& OsmAnd::TextRasterizer::globalInstance()
{
    return *s_globalTextRasterizer;
}

std::shared_ptr<SkBitmap> OsmAnd::TextRasterizer::rasterize(
    const QString& text,
    const Style& style /*= Style()*/,
    QVector<SkScalar>* const outGlyphWidths /*= nullptr*/,
    float* const outExtraTopSpace /*= nullptr*/,
    float* const outExtraBottomSpace /*= nullptr*/,
    float* const outLineSpacing /*= nullptr*/) const
{
    return _p->rasterize(text, style, outGlyphWidths, outExtraTopSpace, outExtraBottomSpace, outLineSpacing);
}

void OsmAnd::TextRasterizer::rasterize(
    SkBitmap& targetBitmap,
    const QString& text,
    const Style& style /*= Style()*/,
    QVector<SkScalar>* const outGlyphWidths /*= nullptr*/,
    float* const outExtraTopSpace /*= nullptr*/,
    float* const outExtraBottomSpace /*= nullptr*/,
    float* const outLineSpacing /*= nullptr*/) const
{
    _p->rasterize(targetBitmap, text, style, outGlyphWidths, outExtraTopSpace, outExtraBottomSpace, outLineSpacing);
}

void OsmAnd::TextRasterizer_initializeGlobalInstance()
{
    s_globalTextRasterizer.reset(new TextRasterizer());
}

void OsmAnd::TextRasterizer_releaseGlobalInstance()
{
    s_globalTextRasterizer.reset();
}
