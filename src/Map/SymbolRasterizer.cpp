#include "SymbolRasterizer.h"
#include "SymbolRasterizer_P.h"

OsmAnd::SymbolRasterizer::SymbolRasterizer()
    : _p(new SymbolRasterizer_P(this))
{
}

OsmAnd::SymbolRasterizer::~SymbolRasterizer()
{
}

void OsmAnd::SymbolRasterizer::rasterize(
    const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
    QList< std::shared_ptr<const RasterizedSymbolsGroup> >& outSymbolsGroups,
    std::function<bool (const std::shared_ptr<const Model::BinaryMapObject>& mapObject)> filter /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/)
{
    _p->rasterize(primitivizedArea, outSymbolsGroups, filter, controller);
}

OsmAnd::SymbolRasterizer::RasterizedSymbolsGroup::RasterizedSymbolsGroup(const std::shared_ptr<const Model::BinaryMapObject>& mapObject_)
    : mapObject(mapObject_)
{
}

OsmAnd::SymbolRasterizer::RasterizedSymbolsGroup::~RasterizedSymbolsGroup()
{
}

OsmAnd::SymbolRasterizer::RasterizedSymbol::RasterizedSymbol(
    const std::shared_ptr<const RasterizedSymbolsGroup>& group_,
    const std::shared_ptr<const Model::BinaryMapObject>& mapObject_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const int order_,
    const ContentType contentType_,
    const QString& content_,
    const LanguageId& languageId_,
    const PointI& minDistance_)
    : group(group_)
    , mapObject(mapObject_)
    , bitmap(bitmap_)
    , order(order_)
    , contentType(contentType_)
    , content(content_)
    , languageId(languageId_)
    , minDistance(minDistance_)
{
    assert(mapObject_);
}

OsmAnd::SymbolRasterizer::RasterizedSymbol::~RasterizedSymbol()
{
}

OsmAnd::SymbolRasterizer::RasterizedSpriteSymbol::RasterizedSpriteSymbol(
    const std::shared_ptr<const RasterizedSymbolsGroup>& group_,
    const std::shared_ptr<const Model::BinaryMapObject>& mapObject_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const int order_,
    const ContentType contentType_,
    const QString& content_,
    const LanguageId& languageId_,
    const PointI& minDistance_,
    const PointI& location31_,
    const PointI& offset_,
    const bool drawAlongPath_,
    const PointI intersectionSize_)
    : RasterizedSymbol(group_, mapObject_, bitmap_, order_, contentType_, content_, languageId_, minDistance_)
    , location31(location31_)
    , offset(offset_)
    , drawAlongPath(drawAlongPath_)
    , intersectionSize(intersectionSize_)
{
}

OsmAnd::SymbolRasterizer::RasterizedSpriteSymbol::~RasterizedSpriteSymbol()
{
}

OsmAnd::SymbolRasterizer::RasterizedOnPathSymbol::RasterizedOnPathSymbol(
    const std::shared_ptr<const RasterizedSymbolsGroup>& group_,
    const std::shared_ptr<const Model::BinaryMapObject>& mapObject_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const int order_,
    const ContentType contentType_,
    const QString& content_,
    const LanguageId& languageId_,
    const PointI& minDistance_,
    const QVector<SkScalar>& glyphsWidth_)
    : RasterizedSymbol(group_, mapObject_, bitmap_, order_, contentType_, content_, languageId_, minDistance_)
    , glyphsWidth(glyphsWidth_)
{
}

OsmAnd::SymbolRasterizer::RasterizedOnPathSymbol::~RasterizedOnPathSymbol()
{
}
