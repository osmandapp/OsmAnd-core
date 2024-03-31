#include "IconsProvider.h"

#include <OsmAndCore/ICoreResourcesProvider.h>
#include "SkiaUtilities.h"
#include "Logging.h"

OsmAnd::IconsProvider::IconsProvider(
    const QString& pathFormat_,
    const std::shared_ptr<const ICoreResourcesProvider>& externalResourcesProvider_,
    const float displayDensityFactor_)
    : pathFormat(pathFormat_)
    , externalResourcesProvider(externalResourcesProvider_)
    , displayDensityFactor(displayDensityFactor_)
{
}

OsmAnd::IconsProvider::~IconsProvider()
{
}

bool OsmAnd::IconsProvider::obtainIcon(const QString& name, const float scale, const bool colorable, sk_sp<const SkImage>& outIcon) const
{
    QMutexLocker scopedLocker(&_mutex);

    const auto key = makeIconKey(name, scale, colorable);
    auto citIcon = _cache.constFind(key);
    if (citIcon == _cache.cend())
    {
        QByteArray data;
        if (!obtainResourceByPath(makeIconPath(name, colorable), data))
            return false;

        const auto image = SkiaUtilities::createImageFromVectorData(data, scale * displayDensityFactor);
        if (!image)
            return false;

        citIcon = _cache.insert(key, image);
    }

    outIcon = *citIcon;
    return true;
}

bool OsmAnd::IconsProvider::obtainResourceByPath(const QString& path, QByteArray& outResource) const
{
    bool ok = false;

    if (externalResourcesProvider)
    {
        outResource = externalResourcesProvider->containsResource(path, displayDensityFactor)
            ? externalResourcesProvider->getResource(path, displayDensityFactor, &ok)
            : externalResourcesProvider->getResource(path, &ok);

        if (ok)
            return true;
    }

    outResource = getCoreResourcesProvider()->containsResource(path, displayDensityFactor)
        ? getCoreResourcesProvider()->getResource(path, displayDensityFactor, &ok)
        : getCoreResourcesProvider()->getResource(path, &ok);
    if (!ok)
        LogPrintf(LogSeverityLevel::Warning,
            "Resource '%s' (requested by MapPresentationEnvironment) was not found", qPrintable(path));

    return ok;
}

bool OsmAnd::IconsProvider::containsResource(const QString& name, bool colorable) const
{
    const auto resourcePath = makeIconPath(name, colorable);
    bool ok = false;
    ok = ok || externalResourcesProvider && externalResourcesProvider->containsResource(resourcePath, displayDensityFactor);
    ok = ok || externalResourcesProvider && externalResourcesProvider->containsResource(resourcePath);
    ok = ok || getCoreResourcesProvider()->containsResource(resourcePath, displayDensityFactor);
    ok = ok || getCoreResourcesProvider()->containsResource(resourcePath);
    return ok;
}

QString OsmAnd::IconsProvider::makeIconKey(const QString& name, const float scale, const bool colorable) const
{
    return colorable
        ? QLatin1String("%1_%2").arg(name).arg(static_cast<int>(scale * 1000))
        : QLatin1String("c_%1_%2").arg(name).arg(static_cast<int>(scale * 1000));
}

QString OsmAnd::IconsProvider::makeIconPath(const QString& name, const bool colorable) const
{
    return colorable
        ? pathFormat.arg(name)
        : pathFormat.arg(QLatin1String("c_%1").arg(name));
}

bool OsmAnd::IconsProvider::obtainIcon(
    const QString& name,
    const float scale,
    sk_sp<const SkImage>& outIcon,
    bool* const outColorable /*= nullptr*/) const
{
    if (containsResource(name, false) && obtainIcon(name, scale, false, outIcon))
    {
        if (outColorable)
            *outColorable = false;
        return true;
    }

    if (obtainIcon(name, scale, true, outIcon))
    {
        if (outColorable)
            *outColorable = true;
        return true;
    }

    return false;
}

bool OsmAnd::IconsProvider::containsResource(const QString& name) const
{
    return containsResource(name, false) || containsResource(name, true);
}
