#include "OnPathMapSymbol.h"

OsmAnd::OnPathMapSymbol::OnPathMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_,
    const bool isShareable_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const int order_,
    const IntersectionModeFlags intersectionModeFlags_,
    const QString& content_,
    const LanguageId& languageId_,
    const PointI& minDistance_,
    const QVector<PointI>& path_,
    const QVector<float>& glyphsWidth_)
    : MapSymbol(group_, isShareable_, bitmap_, order_, intersectionModeFlags_, content_, languageId_, minDistance_)
    , path(path_)
    , glyphsWidth(glyphsWidth_)
{
}

OsmAnd::OnPathMapSymbol::~OnPathMapSymbol()
{
}

std::shared_ptr<OsmAnd::MapSymbol> OsmAnd::OnPathMapSymbol::clone() const
{
    return std::shared_ptr<MapSymbol>(new OnPathMapSymbol(
        nullptr,
        isShareable,
        bitmap,
        order,
        intersectionModeFlags,
        content,
        languageId,
        minDistance,
        path,
        glyphsWidth));
}
