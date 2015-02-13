#include "IOnlineTileSources.h"

#include "OnlineRasterMapLayerProvider.h"

OsmAnd::IOnlineTileSources::IOnlineTileSources()
{
}

OsmAnd::IOnlineTileSources::~IOnlineTileSources()
{
}

std::shared_ptr<OsmAnd::OnlineRasterMapLayerProvider> OsmAnd::IOnlineTileSources::createProviderFor(
    const QString& sourceName) const
{
    const auto source = getSourceByName(sourceName);
    if (!source)
        return nullptr;

    return std::shared_ptr<OsmAnd::OnlineRasterMapLayerProvider>(new OnlineRasterMapLayerProvider(
        source->name,
        source->urlPattern,
        source->minZoom,
        source->maxZoom,
        source->maxConcurrentDownloads,
        source->tileSize,
        source->alphaChannelPresence));
}

OsmAnd::IOnlineTileSources::Source::Source(const QString& name_)
    : name(name_)
    , title(name)
{
}

OsmAnd::IOnlineTileSources::Source::Source(const QString& name_, const QString& title_)
    : name(name_)
    , title(title_)
{
}

OsmAnd::IOnlineTileSources::Source::~Source()
{
}
