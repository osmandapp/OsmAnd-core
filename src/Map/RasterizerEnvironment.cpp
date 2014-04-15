#include "RasterizerEnvironment.h"
#include "RasterizerEnvironment_P.h"

#include "MapStyleValue.h"
#include "IExternalResourcesProvider.h"

OsmAnd::RasterizerEnvironment::RasterizerEnvironment(
    const std::shared_ptr<const MapStyle>& style_,
    const float displayDensityFactor_,
    const std::shared_ptr<const IExternalResourcesProvider>& externalResourcesProvider_ /*= nullptr*/ )
    : _p(new RasterizerEnvironment_P(this))
    , style(style_)
    , displayDensityFactor(displayDensityFactor_)
    , externalResourcesProvider(externalResourcesProvider_)
    , dummyMapSection(_p->dummyMapSection)
{
    _p->initialize();
}

OsmAnd::RasterizerEnvironment::~RasterizerEnvironment()
{
}

QHash< std::shared_ptr<const OsmAnd::MapStyleValueDefinition>, OsmAnd::MapStyleValue > OsmAnd::RasterizerEnvironment::getSettings() const
{
    return _p->getSettings();
}

void OsmAnd::RasterizerEnvironment::setSettings(const QHash< std::shared_ptr<const MapStyleValueDefinition>, MapStyleValue >& newSettings)
{
    _p->setSettings(newSettings);
}

void OsmAnd::RasterizerEnvironment::setSettings(const QHash< QString, QString >& newSettings)
{
    _p->setSettings(newSettings);
}
