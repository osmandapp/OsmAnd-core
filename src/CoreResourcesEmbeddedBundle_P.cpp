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
    CoreResourcesEmbeddedBundle* const owner_)
    : owner(owner_)
{
}

OsmAnd::CoreResourcesEmbeddedBundle_P::~CoreResourcesEmbeddedBundle_P()
{
    unloadLibrary();
}

bool OsmAnd::CoreResourcesEmbeddedBundle_P::loadFromCurrentExecutable()
{
#if defined(OSMAND_TARGET_OS_windows)
    _bundleLibraryNeedsClose = false;
    _bundleLibrary = GetModuleHandleA(NULL);
    if (_bundleLibrary == NULL)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to open main program executable as library");
        return false;
    }
#else
    _bundleLibraryNeedsClose = true;
    _bundleLibrary = dlopen(NULL, RTLD_NOW | RTLD_GLOBAL);
    if (_bundleLibrary == NULL)
    {
        const auto error = dlerror();
        LogPrintf(LogSeverityLevel::Error,
            "Failed to open main program executable as library: %s", error ? error : "unknown");
        return false;
    }
#endif

    if (!loadResources())
    {
        unloadLibrary();
        return false;
    }

    return true;
}

bool OsmAnd::CoreResourcesEmbeddedBundle_P::loadFromLibrary(const QString& libraryNameOrFilename)
{
#if defined(OSMAND_TARGET_OS_windows)
    _bundleLibraryNeedsClose = true;
    _bundleLibrary = LoadLibraryA(qPrintable(libraryNameOrFilename));
    if (_bundleLibrary == NULL)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to load library from '%s'", qPrintable(libraryNameOrFilename));
        return false;
    }
#else
    _bundleLibraryNeedsClose = true;
    _bundleLibrary = dlopen(qPrintable(libraryNameOrFilename), RTLD_NOW | RTLD_GLOBAL);
    if (_bundleLibrary == NULL)
    {
        const auto error = dlerror();
        LogPrintf(LogSeverityLevel::Error,
            "Failed to load library from '%s': %s",
            qPrintable(libraryNameOrFilename),
            error ? error : "unknown");
        return false;
    }
#endif

    if (!loadResources())
    {
        unloadLibrary();
        return false;
    }

    return true;
}

void OsmAnd::CoreResourcesEmbeddedBundle_P::unloadLibrary()
{
    if (_bundleLibraryNeedsClose)
    {
#if defined(OSMAND_TARGET_OS_windows)
        if (_bundleLibrary != NULL)
        {
            CloseHandle(_bundleLibrary);
            _bundleLibrary = NULL;
        }
#else
        if (_bundleLibrary != NULL)
        {
            dlclose(_bundleLibrary);
            _bundleLibrary = NULL;
        }
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
    if (!symbolPtr)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to find '%s' when loading CoreResourcesEmbeddedBundle",
            symbolName);
    }

    return symbolPtr;
}

bool OsmAnd::CoreResourcesEmbeddedBundle_P::loadResources()
{
    typedef const void* (*GetPointerFunctionPtr)();
    // typedef const char* NamePtr;
    // typedef const uint8_t* DataPtr;

    // Regular expressions to extract pure resource name or qualifiers
    const QRegularExpression resourceNameWithQualifiersRegExp("(?:\\[(.*)\\])(.*)");

    // Find out what number of resources there is in the bundle
    const auto pGetResourcesCount = reinterpret_cast<GetPointerFunctionPtr>(loadSymbol("__get____CoreResourcesEmbeddedBundle__ResourcesCount"));
    if (pGetResourcesCount == nullptr) {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to load resources bundle: no resorces count");
        return false;
    }
    const auto resourcesCount = *reinterpret_cast<const uint32_t*>(pGetResourcesCount());

    for (auto resourceIdx = 0u; resourceIdx < resourcesCount; resourceIdx++)
    {
        ResourceData resourceData;

        const auto pGetResourceName = reinterpret_cast<GetPointerFunctionPtr>(loadSymbol(
            QString(QLatin1String("__get____CoreResourcesEmbeddedBundle__ResourceName_%1")).arg(resourceIdx).toLatin1()));
        if (pGetResourceName == nullptr) {
            LogPrintf(LogSeverityLevel::Error,
                "Failed load resources bundle: resource #%d name not found", resourceIdx);
            return false;
        }
        const auto resourceName = reinterpret_cast<const char*>(pGetResourceName());

        const auto pGetResourceSize = reinterpret_cast<GetPointerFunctionPtr>(loadSymbol(
            QString(QLatin1String("__get____CoreResourcesEmbeddedBundle__ResourceSize_%1")).arg(resourceIdx).toLatin1()));
        if (pGetResourceSize == nullptr) {
            LogPrintf(LogSeverityLevel::Error,
                "Failed load resources bundle: resource #%d size not found", resourceIdx);
            return false;
        }
        resourceData.size = *reinterpret_cast<const size_t*>(pGetResourceSize());

        const auto pGetResourceData = reinterpret_cast<GetPointerFunctionPtr>(loadSymbol(
            QString(QLatin1String("__get____CoreResourcesEmbeddedBundle__ResourceData_%1")).arg(resourceIdx).toLatin1()));
        if (pGetResourceData == nullptr) {
            LogPrintf(LogSeverityLevel::Error,
                "Failed load resources bundle: resource #%d data not found", resourceIdx);
            return false;
        }
        resourceData.data = reinterpret_cast<const uint8_t*>(pGetResourceData());

        // Process resource name
        QStringList qualifiers;
        QString pureResourceName;
        const auto resourceNameComponents = resourceNameWithQualifiersRegExp.match(QLatin1String(resourceName));
        if (resourceNameComponents.hasMatch())
        {
            qualifiers = resourceNameComponents.captured(1).split(QLatin1Char(';'), Qt::SkipEmptyParts);
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
                const auto qualifierComponents = qualifier.trimmed().split(QLatin1Char('='), Qt::SkipEmptyParts);

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

    return true;
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
            if (!name.endsWith(QString(".png")))
                return qUncompress(resourceData.data, resourceData.size);
            else {
                return QByteArray(reinterpret_cast<const char*>(resourceData.data) + 4, resourceData.size - 4);
            }
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
    if (!name.endsWith(QString(".png")))
        return qUncompress(resourceEntry.defaultVariant.data, resourceEntry.defaultVariant.size);
    else
        return QByteArray(reinterpret_cast<const char*>(resourceEntry.defaultVariant.data) + 4, resourceEntry.defaultVariant.size - 4);
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
