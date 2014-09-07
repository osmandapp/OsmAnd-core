#include "CoreResourcesEmbeddedBundle_P.h"
#include "CoreResourcesEmbeddedBundle.h"

#if !defined(OSMAND_TARGET_OS_windows)
#   include <dlfcn.h>
#endif // !defined(OSMAND_TARGET_OS_windows)

#include <QRegularExpression>
#include <QStringList>
#include "QtCommon.h"

#include "Logging.h"
#include "QKeyValueIterator.h"

OsmAnd::CoreResourcesEmbeddedBundle_P::CoreResourcesEmbeddedBundle_P(
    CoreResourcesEmbeddedBundle* const owner_,
    const QString& bundleLibraryName /*= QString::null*/)
    : owner(owner_)
{
#if defined(OSMAND_TARGET_OS_windows)
    if (bundleLibraryName.isNull())
    {
        _bundleLibraryNeedsClose = false;
        _bundleLibrary = GetModuleHandleA(NULL);
    }
    else
    {
        _bundleLibraryNeedsClose = true;
        _bundleLibrary = LoadLibraryA(qPrintable(bundleLibraryName));
    }
#else
    _bundleLibraryNeedsClose = true;
    _bundleLibrary = dlopen(bundleLibraryName.isNull() ? NULL : qPrintable(bundleLibraryName), RTLD_NOW | RTLD_GLOBAL);
#endif
    assert(_bundleLibrary != nullptr);

    loadResources();
}

OsmAnd::CoreResourcesEmbeddedBundle_P::~CoreResourcesEmbeddedBundle_P()
{
    if (_bundleLibraryNeedsClose)
    {
#if defined(OSMAND_TARGET_OS_windows)
        CloseHandle(_bundleLibrary);
#else
        dlclose(_bundleLibrary);
#endif
    }
}

void* OsmAnd::CoreResourcesEmbeddedBundle_P::loadSymbol(const char* const symbolName)
{
    void* symbolPtr = nullptr;
#if defined(OSMAND_TARGET_OS_windows)
    symbolPtr = reinterpret_cast<void*>(GetProcAddress(_bundleLibrary, symbolName));
#else
    symbolPtr = dlsym(_bundleLibrary, symbolName);
#endif

    return symbolPtr;
}

void OsmAnd::CoreResourcesEmbeddedBundle_P::loadResources()
{
#if defined(OSMAND_TARGET_OS_windows)
    typedef const void* (*GetPointerFunctionPtr)();
#else
    typedef const void* (*GetPointerFunctionPtr)();
#endif
    typedef const char* NamePtr;
    typedef const uint8_t* DataPtr;

    // Regular expressions to extract pure resource name or qualifiers
    const QRegularExpression resourceNameWithQualifiersRegExp("(?:\\[(.*)\\])(.*)");

    // Find out what number of resources there is in the bundle
    const auto pGetResourcesCount = reinterpret_cast<GetPointerFunctionPtr>(loadSymbol("__get____CoreResourcesEmbeddedBundle__ResourcesCount"));
    assert(pGetResourcesCount != nullptr);
    const auto resourcesCount = *reinterpret_cast<const uint32_t*>(pGetResourcesCount());

    for (auto resourceIdx = 0u; resourceIdx < resourcesCount; resourceIdx++)
    {
        ResourceData resourceData;

        const auto pGetResourceName = reinterpret_cast<GetPointerFunctionPtr>(loadSymbol(
            QString(QLatin1String("__get____CoreResourcesEmbeddedBundle__ResourceName_%1")).arg(resourceIdx).toLatin1()));
        assert(pGetResourceName != nullptr);
        const auto resourceName = reinterpret_cast<const char*>(pGetResourceName());

        const auto pGetResourceSize = reinterpret_cast<GetPointerFunctionPtr>(loadSymbol(
            QString(QLatin1String("__get____CoreResourcesEmbeddedBundle__ResourceSize_%1")).arg(resourceIdx).toLatin1()));
        assert(pGetResourceSize != nullptr);
        resourceData.size = *reinterpret_cast<const size_t*>(pGetResourceSize());

        const auto pGetResourceData = reinterpret_cast<GetPointerFunctionPtr>(loadSymbol(
            QString(QLatin1String("__get____CoreResourcesEmbeddedBundle__ResourceData_%1")).arg(resourceIdx).toLatin1()));
        assert(pGetResourceData != nullptr);
        resourceData.data = reinterpret_cast<const uint8_t*>(pGetResourceData());

        // Process resource name
        QStringList qualifiers;
        QString pureResourceName;
        const auto resourceNameComponents = resourceNameWithQualifiersRegExp.match(QLatin1String(resourceName));
        if (resourceNameComponents.hasMatch())
        {
            qualifiers = resourceNameComponents.captured(1).split(QLatin1Char(';'), QString::SkipEmptyParts);
            pureResourceName = resourceNameComponents.captured(2);
        }
        else
        {
            pureResourceName = QLatin1String(resourceName);
        }

        // Get resource entry for this resource
        auto& resourceEntry = _resources[pureResourceName];        
        if (qualifiers.isEmpty())
        {
            resourceEntry.defaultVariant = resourceData;
        }
        else
        {
            for (const auto& qualifier : constOf(qualifiers))
            {
                const auto qualifierComponents = qualifier.trimmed().split(QLatin1Char('='), QString::SkipEmptyParts);
                
                bool ok = false;
                if (qualifierComponents.size() == 2 && qualifierComponents.first() == QLatin1String("ddf"))
                {
                    const auto ddfValue = qualifierComponents.last().toFloat(&ok);
                    if (!ok)
                    {
                        LogPrintf(LogSeverityLevel::Warning,
                            "Unsupported value '%s' for DDF qualifier",
                            qPrintable(qualifierComponents.last()));
                    }
                    resourceEntry.variantsByDisplayDensityFactor.insert(ddfValue, resourceData);
                }
                else
                {
                    LogPrintf(LogSeverityLevel::Warning,
                        "Unsupported qualifier '%s'",
                        qPrintable(qualifier.trimmed()));
                }
            }
        }
    }
}

QByteArray OsmAnd::CoreResourcesEmbeddedBundle_P::getResource(const QString& name, const float displayDensityFactor, bool* ok /*= nullptr*/) const
{
    const auto citResourceEntry = _resources.constFind(name);
    if (citResourceEntry == _resources.cend())
    {
        if (ok)
            *ok = false;
        return QByteArray();
    }
    const auto& resourceEntry = *citResourceEntry;

    auto itByDisplayDensityFactor = iteratorOf(constOf(resourceEntry.variantsByDisplayDensityFactor));
    while (itByDisplayDensityFactor.hasNext())
    {
        const auto byDisplayDensityFactorEntry = itByDisplayDensityFactor.next();
        const auto& testDisplayDensityFactor = byDisplayDensityFactorEntry.key();
        const auto& resourceData = byDisplayDensityFactorEntry.value();

        if (qFuzzyCompare(displayDensityFactor, testDisplayDensityFactor) ||
            testDisplayDensityFactor >= displayDensityFactor ||
            !itByDisplayDensityFactor.hasNext())
        {
            if (ok)
                *ok = true;
            return qUncompress(resourceData.data, resourceData.size);
        }
    }

    if (ok)
        *ok = false;
    return QByteArray();
}

QByteArray OsmAnd::CoreResourcesEmbeddedBundle_P::getResource(const QString& name, bool* ok /*= nullptr*/) const
{
    const auto citResourceEntry = _resources.constFind(name);
    if (citResourceEntry == _resources.cend())
    {
        if (ok)
            *ok = false;
        return QByteArray();
    }
    const auto& resourceEntry = *citResourceEntry;
    if (!resourceEntry.defaultVariant.data)
    {
        if (ok)
            *ok = false;
        return QByteArray();
    }

    if (ok)
        *ok = true;
    return qUncompress(resourceEntry.defaultVariant.data, resourceEntry.defaultVariant.size);
}

bool OsmAnd::CoreResourcesEmbeddedBundle_P::containsResource(const QString& name, const float displayDensityFactor) const
{
    const auto citResourceEntry = _resources.constFind(name);
    if (citResourceEntry == _resources.cend())
        return false;
    const auto& resourceEntry = *citResourceEntry;

    auto itByDisplayDensityFactor = iteratorOf(constOf(resourceEntry.variantsByDisplayDensityFactor));
    while (itByDisplayDensityFactor.hasNext())
    {
        const auto byDisplayDensityFactorEntry = itByDisplayDensityFactor.next();
        const auto& testDisplayDensityFactor = byDisplayDensityFactorEntry.key();

        if (qFuzzyCompare(displayDensityFactor, testDisplayDensityFactor) ||
            testDisplayDensityFactor >= displayDensityFactor ||
            !itByDisplayDensityFactor.hasNext())
        {
            return true;
        }
    }

    return false;
}

bool OsmAnd::CoreResourcesEmbeddedBundle_P::containsResource(const QString& name) const
{
    const auto citResourceEntry = _resources.constFind(name);
    if (citResourceEntry == _resources.cend())
        return false;
    const auto& resourceEntry = *citResourceEntry;
    return (resourceEntry.defaultVariant.data != nullptr);
}
