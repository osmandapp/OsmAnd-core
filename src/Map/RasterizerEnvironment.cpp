#include "RasterizerEnvironment.h"
#include "RasterizerEnvironment_P.h"

OsmAnd::RasterizerEnvironment::RasterizerEnvironment( const std::shared_ptr<const MapStyle>& style_, const bool& basemapAvailable_ )
    : _d(new RasterizerEnvironment_P(this))
    , style(style_)
    , basemapAvailable(basemapAvailable_)
{
    _d->initialize();
}

OsmAnd::RasterizerEnvironment::RasterizerEnvironment( const std::shared_ptr<const MapStyle>& style_, const bool& basemapAvailable_, const QMap< std::shared_ptr<const MapStyleValueDefinition>, MapStyleValue >& settings_ )
    : _d(new RasterizerEnvironment_P(this))
    , style(style_)
    , basemapAvailable(basemapAvailable_)
    , settings(settings_)
{
}

OsmAnd::RasterizerEnvironment::~RasterizerEnvironment()
{
}
