#ifndef _OSMAND_CORE_EMBEDDED_RESOURCES_P_H_
#define _OSMAND_CORE_EMBEDDED_RESOURCES_P_H_

#include "stdlib_common.h"

#if defined(OSMAND_TARGET_OS_windows)
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#endif

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QHash>
#include <QMap>
#include <QString>
#include <QByteArray>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"

namespace OsmAnd
{
    class CoreResourcesEmbeddedBundle;
    class CoreResourcesEmbeddedBundle_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(CoreResourcesEmbeddedBundle_P);

    private:
#if defined(OSMAND_TARGET_OS_windows)
        typedef HMODULE LibraryHandle;
#else
        typedef void* LibraryHandle;
#endif
        LibraryHandle _bundleLibrary;
        bool _bundleLibraryNeedsClose;
        void unloadLibrary();

        void* loadSymbol(const char* const symbolName);
        bool loadResources();

        struct ResourceData
        {
            ResourceData()
                : data(nullptr)
                , size(0)
            {
            }

            const uint8_t* data;
            size_t size;
        };

        struct ResourceEntry
        {
            ResourceData defaultVariant;
            QMap<float, ResourceData> variantsByDisplayDensityFactor;
        };
        QHash<QString, ResourceEntry> _resources;
    protected:
        CoreResourcesEmbeddedBundle_P(CoreResourcesEmbeddedBundle* const owner);

        bool loadFromCurrentExecutable();
        bool loadFromLibrary(const QString& libraryNameOrFilename);
        QByteArray getResourceBytes(const QString& resourceName, const ResourceData& resourceData) const;
    public:
        ~CoreResourcesEmbeddedBundle_P();

        ImplementationInterface<CoreResourcesEmbeddedBundle> owner;

        QByteArray getResource(
            const QString& name,
            const float displayDensityFactor,
            bool* ok = nullptr) const;
        QByteArray getResource(
            const QString& name,
            bool* ok = nullptr) const;

        bool containsResource(
            const QString& name,
            const float displayDensityFactor) const;
        bool containsResource(
            const QString& name) const;

    friend class OsmAnd::CoreResourcesEmbeddedBundle;
    };
}

#endif // !defined(_OSMAND_CORE_EMBEDDED_RESOURCES_P_H_)
