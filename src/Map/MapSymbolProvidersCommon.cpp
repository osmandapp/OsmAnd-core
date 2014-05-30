#include "MapSymbolProvidersCommon.h"

#include "ObjectWithId.h"

OsmAnd::MapSymbolsGroup::MapSymbolsGroup()
{
}

OsmAnd::MapSymbolsGroup::~MapSymbolsGroup()
{
}

QString OsmAnd::MapSymbolsGroup::getDebugTitle() const
{
    static QString noDebugTitle(QLatin1String("?"));

    return noDebugTitle;
}

OsmAnd::MapSymbolsGroupShareableById::MapSymbolsGroupShareableById(const uint64_t id_)
    : id(id_)
{
}

OsmAnd::MapSymbolsGroupShareableById::~MapSymbolsGroupShareableById()
{
}

QString OsmAnd::MapSymbolsGroupShareableById::getDebugTitle() const
{
    return QString(QLatin1String("%1")).arg(id);
}

OsmAnd::MapSymbol::MapSymbol(
    const std::shared_ptr<const MapSymbolsGroup>& group_,
    const bool isShareable_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const int order_,
    const QString& content_,
    const LanguageId& languageId_,
    const PointI& minDistance_)
    : _bitmap(bitmap_)
    , group(group_)
    , groupPtr(group_.get())
    , isShareable(isShareable_)
    , bitmap(_bitmap)
    , order(order_)
    , content(content_)
    , languageId(languageId_)
    , minDistance(minDistance_)
{
}

OsmAnd::MapSymbol::~MapSymbol()
{
}

void OsmAnd::MapSymbol::releaseNonRetainedData()
{
    _bitmap.reset();
}

OsmAnd::BoundToPointMapSymbol::BoundToPointMapSymbol(
    const std::shared_ptr<const MapSymbolsGroup>& group_,
    const bool isShareable_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const int order_,
    const QString& content_,
    const LanguageId& languageId_,
    const PointI& minDistance_,
    const PointI& location31_)
    : MapSymbol(group_, isShareable_, bitmap_, order_, content_, languageId_, minDistance_)
    , location31(location31_)
{
}

OsmAnd::BoundToPointMapSymbol::~BoundToPointMapSymbol()
{
}

OsmAnd::PinnedMapSymbol::PinnedMapSymbol(
    const std::shared_ptr<const MapSymbolsGroup>& group_,
    const bool isShareable_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const int order_,
    const QString& content_,
    const LanguageId& languageId_,
    const PointI& minDistance_,
    const PointI& location31_,
    const PointI& offset_)
    : BoundToPointMapSymbol(group_, isShareable_, bitmap_, order_, content_, languageId_, minDistance_, location31_)
    , offset(offset_)
{
}

OsmAnd::PinnedMapSymbol::~PinnedMapSymbol()
{
}

OsmAnd::MapSymbol* OsmAnd::PinnedMapSymbol::cloneWithReplacedBitmap(const std::shared_ptr<const SkBitmap>& replacementBitmap) const
{
    return new PinnedMapSymbol(
        group.lock(),
        isShareable,
        replacementBitmap,
        order,
        content,
        languageId,
        minDistance,
        location31,
        offset);
}

OsmAnd::OnSurfaceMapSymbol::OnSurfaceMapSymbol(
    const std::shared_ptr<const MapSymbolsGroup>& group_,
    const bool isShareable_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const int order_,
    const QString& content_,
    const LanguageId& languageId_,
    const PointI& minDistance_,
    const PointI& location31_)
    : BoundToPointMapSymbol(group_, isShareable_, bitmap_, order_, content_, languageId_, minDistance_, location31_)
{
}

OsmAnd::OnSurfaceMapSymbol::~OnSurfaceMapSymbol()
{
}

OsmAnd::MapSymbol* OsmAnd::OnSurfaceMapSymbol::cloneWithReplacedBitmap(const std::shared_ptr<const SkBitmap>& replacementBitmap) const
{
    return new OnSurfaceMapSymbol(
        group.lock(),
        isShareable,
        replacementBitmap,
        order,
        content,
        languageId,
        minDistance,
        location31);
}

OsmAnd::OnPathMapSymbol::OnPathMapSymbol(
    const std::shared_ptr<const MapSymbolsGroup>& group_,
    const bool isShareable_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const int order_,
    const QString& content_,
    const LanguageId& languageId_,
    const PointI& minDistance_,
    const QVector<PointI>& path_,
    const QVector<float>& glyphsWidth_)
    : MapSymbol(group_, isShareable_, bitmap_, order_, content_, languageId_, minDistance_)
    , path(path_)
    , glyphsWidth(glyphsWidth_)
{
}

OsmAnd::OnPathMapSymbol::~OnPathMapSymbol()
{
}

OsmAnd::MapSymbol* OsmAnd::OnPathMapSymbol::cloneWithReplacedBitmap(const std::shared_ptr<const SkBitmap>& replacementBitmap) const
{
    return new OnPathMapSymbol(
        group.lock(),
        isShareable,
        replacementBitmap,
        order,
        content,
        languageId,
        minDistance,
        path,
        glyphsWidth);
}
