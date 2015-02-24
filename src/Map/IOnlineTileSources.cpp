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
        source->alphaChannelPresence,
        source->tileDensityFactor));
}

OsmAnd::IOnlineTileSources::Source::Source(const QString& name_)
    : Source(name_, name_)
{
}

OsmAnd::IOnlineTileSources::Source::Source(const QString& name_, const QString& title_)
    : name(name_)
    , title(title_)
    , minZoom(MinZoomLevel)
    , maxZoom(MaxZoomLevel)
    , maxConcurrentDownloads(0)
    , tileSize(256)
    , alphaChannelPresence(AlphaChannelPresence::Unknown)
    , tileDensityFactor(1.0f)
{
}

OsmAnd::IOnlineTileSources::Source::~Source()
{
}
