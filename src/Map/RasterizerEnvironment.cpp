#include "RasterizerEnvironment.h"
#include "RasterizerEnvironment_P.h"

#include "MapStyleValue.h"

OsmAnd::RasterizerEnvironment::RasterizerEnvironment( const std::shared_ptr<const MapStyle>& style_, const bool& basemapAvailable_, const float& displayDensityFactor_ )
    : _d(new RasterizerEnvironment_P(this))
    , style(style_)
    , basemapAvailable(basemapAvailable_)
    , displayDensityFactor(displayDensityFactor_)
{
    _d->initialize();
}

OsmAnd::RasterizerEnvironment::RasterizerEnvironment( const std::shared_ptr<const MapStyle>& style_, const bool& basemapAvailable_, const float& displayDensityFactor_, const QMap< std::shared_ptr<const MapStyleValueDefinition>, MapStyleValue >& settings_ )
    : _d(new RasterizerEnvironment_P(this))
    , style(style_)
    , basemapAvailable(basemapAvailable_)
    , displayDensityFactor(displayDensityFactor_)
    , settings(settings_)
{
    _d->initialize();
}

OsmAnd::RasterizerEnvironment::~RasterizerEnvironment()
{
}
