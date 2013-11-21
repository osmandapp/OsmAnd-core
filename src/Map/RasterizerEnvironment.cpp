#include "RasterizerEnvironment.h"
#include "RasterizerEnvironment_P.h"

#include "MapStyleValue.h"

OsmAnd::RasterizerEnvironment::RasterizerEnvironment( const std::shared_ptr<const MapStyle>& style_, const float displayDensityFactor_ )
    : _d(new RasterizerEnvironment_P(this))
    , style(style_)
    , displayDensityFactor(displayDensityFactor_)
{
    _d->initialize();
}

OsmAnd::RasterizerEnvironment::~RasterizerEnvironment()
{
}

QHash< std::shared_ptr<const OsmAnd::MapStyleValueDefinition>, OsmAnd::MapStyleValue > OsmAnd::RasterizerEnvironment::getSettings() const
{
    return _d->getSettings();
}

void OsmAnd::RasterizerEnvironment::setSettings(const QHash< std::shared_ptr<const MapStyleValueDefinition>, MapStyleValue >& newSettings)
{
    _d->setSettings(newSettings);
}
