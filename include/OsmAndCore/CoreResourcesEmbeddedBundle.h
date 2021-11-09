#ifndef _OSMAND_CORE_EMBEDDED_RESOURCES_H_
#define _OSMAND_CORE_EMBEDDED_RESOURCES_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QByteArray>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/ICoreResourcesProvider.h>

namespace OsmAnd
{
    class CoreResourcesEmbeddedBundle_P;
    class OSMAND_CORE_API CoreResourcesEmbeddedBundle : public ICoreResourcesProvider
    {
        Q_DISABLE_COPY_AND_MOVE(CoreResourcesEmbeddedBundle);

    private:
        PrivateImplementation<CoreResourcesEmbeddedBundle_P> _p;
    protected:
        CoreResourcesEmbeddedBundle();
    public:
        virtual ~CoreResourcesEmbeddedBundle();

        virtual QByteArray getResource(
            const QString& name,
            const float displayDensityFactor,
            bool* ok = nullptr) const;
        virtual QByteArray getResource(
            const QString& name,
            bool* ok = nullptr) const;

        virtual bool containsResource(
            const QString& name,
            const float displayDensityFactor) const;
        virtual bool containsResource(
            const QString& name) const;

        static std::shared_ptr<const CoreResourcesEmbeddedBundle> loadFromCurrentExecutable();
        static std::shared_ptr<const CoreResourcesEmbeddedBundle> loadFromLibrary(const QString& libraryNameOrFilename);
        static std::shared_ptr<const CoreResourcesEmbeddedBundle> loadFromSharedResourcesBundle();
    };
}

#endif // !defined(_OSMAND_CORE_EMBEDDED_RESOURCES_H_)
