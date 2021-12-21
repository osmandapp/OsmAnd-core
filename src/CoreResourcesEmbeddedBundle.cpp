#include "CoreResourcesEmbeddedBundle.h"
#include "CoreResourcesEmbeddedBundle_P.h"

#include "Logging.h"

OsmAnd::CoreResourcesEmbeddedBundle::CoreResourcesEmbeddedBundle()
    : _p(new CoreResourcesEmbeddedBundle_P(this))
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

std::shared_ptr<const OsmAnd::CoreResourcesEmbeddedBundle> OsmAnd::CoreResourcesEmbeddedBundle::loadFromCurrentExecutable()
{
    const std::shared_ptr<CoreResourcesEmbeddedBundle> bundle(new CoreResourcesEmbeddedBundle());
    if (!bundle->_p->loadFromCurrentExecutable())
        return nullptr;
    return bundle;
}

std::shared_ptr<const OsmAnd::CoreResourcesEmbeddedBundle> OsmAnd::CoreResourcesEmbeddedBundle::loadFromLibrary(const QString& libraryNameOrFilename)
{
    const std::shared_ptr<CoreResourcesEmbeddedBundle> bundle(new CoreResourcesEmbeddedBundle());
    if (!bundle->_p->loadFromLibrary(libraryNameOrFilename))
        return nullptr;
    return bundle;
}

std::shared_ptr<const OsmAnd::CoreResourcesEmbeddedBundle> OsmAnd::CoreResourcesEmbeddedBundle::loadFromSharedResourcesBundle()
{
    const QLatin1String libraryName(
#if defined(OSMAND_TARGET_OS_macosx) || defined(OSMAND_TARGET_OS_ios)
        "libOsmAndCore_ResourcesBundle_shared.dylib"
#else
        "OsmAndCore_ResourcesBundle_shared"
#endif
    );
    return loadFromLibrary(libraryName);
}
