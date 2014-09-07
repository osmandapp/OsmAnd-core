#include "CoreResourcesEmbeddedBundle.h"
#include "CoreResourcesEmbeddedBundle_P.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkStream.h>
#include <SkImageDecoder.h>
#include "restore_internal_warnings.h"

#include "Logging.h"

OsmAnd::CoreResourcesEmbeddedBundle::CoreResourcesEmbeddedBundle()
    : _p(new CoreResourcesEmbeddedBundle_P(this))
{
}

OsmAnd::CoreResourcesEmbeddedBundle::CoreResourcesEmbeddedBundle(const QString& bundleLibraryName)
    : _p(new CoreResourcesEmbeddedBundle_P(this, bundleLibraryName))
{
}

OsmAnd::CoreResourcesEmbeddedBundle::~CoreResourcesEmbeddedBundle()
{
}

QByteArray OsmAnd::CoreResourcesEmbeddedBundle::getResource(const QString& name, const float displayDensityFactor, bool* ok /*= nullptr*/) const
{
    return _p->getResource(name, displayDensityFactor, ok);
}

QByteArray OsmAnd::CoreResourcesEmbeddedBundle::getResource(const QString& name, bool* ok /*= nullptr*/) const
{
    return _p->getResource(name, ok);
}

bool OsmAnd::CoreResourcesEmbeddedBundle::containsResource(const QString& name, const float displayDensityFactor) const
{
    return _p->containsResource(name, displayDensityFactor);
}

bool OsmAnd::CoreResourcesEmbeddedBundle::containsResource(const QString& name) const
{
    return _p->containsResource(name);
}
