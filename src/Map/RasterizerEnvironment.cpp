#include "RasterizerEnvironment.h"
#include "RasterizerEnvironment_P.h"

#include "MapStyleValue.h"
#include "IExternalResourcesProvider.h"

OsmAnd::RasterizerEnvironment::RasterizerEnvironment( const std::shared_ptr<const MapStyle>& style_, const float displayDensityFactor_, const std::shared_ptr<const IExternalResourcesProvider>& externalResourcesProvider_ /*= nullptr*/ )
    : _d(new RasterizerEnvironment_P(this))
    , style(style_)
    , displayDensityFactor(displayDensityFactor_)
    , externalResourcesProvider(externalResourcesProvider_)
    , dummyMapSection(_d->dummyMapSection)
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

void OsmAnd::RasterizerEnvironment::setSettings(const QHash< QString, QString >& newSettings)
{
    _d->setSettings(newSettings);
}
