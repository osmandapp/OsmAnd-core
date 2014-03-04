#ifndef _OSMAND_CORE_I_EXTERNAL_RESOURCES_PROVIDER_H_
#define _OSMAND_CORE_I_EXTERNAL_RESOURCES_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QByteArray>

#include <OsmAndCore.h>
#include <OsmAndCore/MemoryCommon.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IExternalResourcesProvider
    {
        Q_DISABLE_COPY(IExternalResourcesProvider);
    public:
        IExternalResourcesProvider();
        virtual ~IExternalResourcesProvider();

        virtual QByteArray getResource(const QString& name, bool* ok = nullptr) const = 0;
        virtual bool containsResource(const QString& name) const = 0;
    };
} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_I_EXTERNAL_RESOURCES_PROVIDER_H_)
