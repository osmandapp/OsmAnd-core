#ifndef _OSMAND_CORE_I_EXTERNAL_RESOURCES_PROVIDER_H_
#define _OSMAND_CORE_I_EXTERNAL_RESOURCES_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QByteArray>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/MemoryCommon.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IExternalResourcesProvider
    {
        Q_DISABLE_COPY_AND_MOVE(IExternalResourcesProvider);
    public:
        IExternalResourcesProvider();
        virtual ~IExternalResourcesProvider();

        virtual QByteArray getResource(const QString& name, bool* ok = nullptr) const = 0;
        virtual bool containsResource(const QString& name) const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_EXTERNAL_RESOURCES_PROVIDER_H_)
