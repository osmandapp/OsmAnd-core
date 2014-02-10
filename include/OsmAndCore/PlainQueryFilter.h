#ifndef _OSMAND_CORE_PLAIN_QUERY_FILTER_H_
#define _OSMAND_CORE_PLAIN_QUERY_FILTER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/IQueryFilter.h>

namespace OsmAnd {

    class OSMAND_CORE_API PlainQueryFilter : public IQueryFilter
    {
    private:
    protected:
        bool _isZoomFiltered;
        ZoomLevel _zoom;
        bool _isAreaFiltered;
        AreaI _area;
    public:
        PlainQueryFilter(const ZoomLevel* zoom = nullptr, const AreaI* area = nullptr);
        virtual ~PlainQueryFilter();

        virtual bool acceptsZoom(ZoomLevel zoom);
        virtual bool acceptsArea(const AreaI& area);
        virtual bool acceptsPoint(const PointI& point);
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_PLAIN_QUERY_FILTER_H_)
