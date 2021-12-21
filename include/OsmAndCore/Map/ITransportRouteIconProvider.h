#ifndef _OSMAND_CORE_I_TRANSPORT_STOP_ICON_PROVIDER_H_
#define _OSMAND_CORE_I_TRANSPORT_STOP_ICON_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <SkImage.h>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/CommonSWIG.h>
#include <OsmAndCore/MemoryCommon.h>

namespace OsmAnd
{
    class TransportRoute;

    class OSMAND_CORE_API ITransportRouteIconProvider
    {
        Q_DISABLE_COPY_AND_MOVE(ITransportRouteIconProvider);

    protected:
        ITransportRouteIconProvider();
    public:
        virtual ~ITransportRouteIconProvider();

        virtual sk_sp<const SkImage> getIcon(
            const std::shared_ptr<const TransportRoute>& transportRoute = nullptr,
            const ZoomLevel zoomLevel = ZoomLevel12,
            const bool largeIcon = false) const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_TRANSPORT_STOP_ICON_PROVIDER_H_)
