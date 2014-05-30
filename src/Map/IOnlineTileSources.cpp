#include "IOnlineTileSources.h"

#include "OnlineRasterMapTileProvider.h"

OsmAnd::IOnlineTileSources::IOnlineTileSources()
{
}

OsmAnd::IOnlineTileSources::~IOnlineTileSources()
{
}

std::shared_ptr<OsmAnd::OnlineRasterMapTileProvider> OsmAnd::IOnlineTileSources::createProviderFor(const QString& sourceName) const
{
    const auto source = getSourceByName(sourceName);
    if (!source)
        return nullptr;

    return std::shared_ptr<OsmAnd::OnlineRasterMapTileProvider>(new OnlineRasterMapTileProvider(
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
