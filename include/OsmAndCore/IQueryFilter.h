#ifndef _OSMAND_CORE_I_QUERY_FILTER_H_
#define _OSMAND_CORE_I_QUERY_FILTER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IQueryFilter
    {
        Q_DISABLE_COPY(IQueryFilter);
    private:
    protected:
        IQueryFilter();
    public:
        virtual ~IQueryFilter();
    
        virtual bool acceptsZoom(ZoomLevel zoom) const = 0;
        virtual bool acceptsArea(const AreaI& area) const = 0;
        virtual bool acceptsPoint(const PointI& point) const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_QUERY_FILTER_H_)
