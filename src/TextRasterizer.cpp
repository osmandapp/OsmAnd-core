#include "TextRasterizer.h"
#include "TextRasterizer_P.h"
#include "TextRasterizer_private.h"

#include "CoreFontsCollection.h"

OsmAnd::TextRasterizer::TextRasterizer(
    const std::shared_ptr<const IFontsCollection>& fontsCollection_)
    : _p(new TextRasterizer_P(this))
    , fontsCollection(fontsCollection_)
{
}

OsmAnd::TextRasterizer::~TextRasterizer()
{
}

std::shared_ptr<SkBitmap> OsmAnd::TextRasterizer::rasterize(
    const QString& text,
    const Style& style /*= Style()*/,
    QVector<SkScalar>* const outGlyphWidths /*= nullptr*/,
    float* const outExtraTopSpace /*= nullptr*/,
    float* const outExtraBottomSpace /*= nullptr*/,
    float* const outLineSpacing /*= nullptr*/) const
{
    return _p->rasterize(
        text,
        style,
        outGlyphWidths,
        outExtraTopSpace,
        outExtraBottomSpace,
        outLineSpacing);
}

bool OsmAnd::TextRasterizer::rasterize(
    SkBitmap& targetBitmap,
    const QString& text,
    const Style& style /*= Style()*/,
    QVector<SkScalar>* const outGlyphWidths /*= nullptr*/,
    float* const outExtraTopSpace /*= nullptr*/,
    float* const outExtraBottomSpace /*= nullptr*/,
    float* const outLineSpacing /*= nullptr*/) const
{
    return _p->rasterize(
        targetBitmap,
        text,
        style,
        outGlyphWidths,
        outExtraTopSpace,
        outExtraBottomSpace,
        outLineSpacing);
}

static std::shared_ptr<const OsmAnd::TextRasterizer> s_defaultTextRasterizer;
std::shared_ptr<const OsmAnd::TextRasterizer> OsmAnd::TextRasterizer::getDefault()
{
    return s_defaultTextRasterizer;
}

void OsmAnd::TextRasterizer_initialize()
{
    s_defaultTextRasterizer.reset(new TextRasterizer(CoreFontsCollection::getDefaultInstance()));
}

void OsmAnd::TextRasterizer_release()
{
    s_defaultTextRasterizer.reset();
}
