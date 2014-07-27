#include "TextRasterizer.h"
#include "TextRasterizer_P.h"

OsmAnd::TextRasterizer::TextRasterizer()
    : _p(new TextRasterizer_P(this))
{
    _p->initialize();
}

OsmAnd::TextRasterizer::~TextRasterizer()
{
}

const OsmAnd::TextRasterizer& OsmAnd::TextRasterizer::getInstance()
{
    static const std::unique_ptr<TextRasterizer> instance(new TextRasterizer());
    return *instance;
}

std::shared_ptr<SkBitmap> OsmAnd::TextRasterizer::rasterize(
    const QString& text,
    const Style& style /*= Style()*/,
    QVector<SkScalar>* const outGlyphWidths /*= nullptr*/,
    float* const outExtraTopSpace /*= nullptr*/,
    float* const outExtraBottomSpace /*= nullptr*/) const
{
    return _p->rasterize(text, style, outGlyphWidths, outExtraTopSpace, outExtraBottomSpace);
}

void OsmAnd::TextRasterizer::rasterize(
    SkBitmap& targetBitmap,
    const QString& text,
    const Style& style /*= Style()*/,
    QVector<SkScalar>* const outGlyphWidths /*= nullptr*/,
    float* const outExtraTopSpace /*= nullptr*/,
    float* const outExtraBottomSpace /*= nullptr*/) const
{
    _p->rasterize(targetBitmap, text, style, outGlyphWidths, outExtraTopSpace, outExtraBottomSpace);
}
