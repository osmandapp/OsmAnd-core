#include "BillboardRasterMapSymbol.h"

OsmAnd::BillboardRasterMapSymbol::BillboardRasterMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_,
    const bool isShareable_,
    const int order_,
    const IntersectionModeFlags intersectionModeFlags_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const QString& content_,
    const LanguageId& languageId_,
    const PointI& minDistance_,
    const PointI& position31_,
    const PointI& offset_)
    : RasterMapSymbol(group_, isShareable_, order_, intersectionModeFlags_, bitmap_, content_, languageId_, minDistance_)
    , offset(offset_)
    , position31(position31_)
{
}

OsmAnd::BillboardRasterMapSymbol::~BillboardRasterMapSymbol()
{
}

OsmAnd::PointI OsmAnd::BillboardRasterMapSymbol::getOffset() const
{
    return offset;
}

void OsmAnd::BillboardRasterMapSymbol::setOffset(const PointI newOffset)
{
    offset = newOffset;
}

OsmAnd::PointI OsmAnd::BillboardRasterMapSymbol::getPosition31() const
{
    return position31;
}

void OsmAnd::BillboardRasterMapSymbol::setPosition31(const PointI position)
{
    position31 = position;
}
