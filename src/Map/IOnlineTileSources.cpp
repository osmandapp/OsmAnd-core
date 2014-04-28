#include "IOnlineTileSources.h"

#include "OnlineMapRasterTileProvider.h"

OsmAnd::IOnlineTileSources::IOnlineTileSources()
{
}

OsmAnd::IOnlineTileSources::~IOnlineTileSources()
{
}

std::shared_ptr<OsmAnd::OnlineMapRasterTileProvider> OsmAnd::IOnlineTileSources::createProviderFor(const QString& sourceName) const
{
    const auto source = getSourceByName(sourceName);
    if (!source)
        return nullptr;

    return std::shared_ptr<OsmAnd::OnlineMapRasterTileProvider>(new OnlineMapRasterTileProvider(
        source->name,
        source->urlPattern,
        source->minZoom,
        source->maxZoom,
        source->maxConcurrentDownloads,
        source->tileSize,
        source->alphaChannelData));
}

OsmAnd::IOnlineTileSources::Source::Source(const QString& name_)
    : name(name_)
{
}

OsmAnd::IOnlineTileSources::Source::~Source()
{
}
