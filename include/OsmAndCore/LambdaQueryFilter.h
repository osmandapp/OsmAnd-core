#ifndef _OSMAND_CORE_LAMBDA_QUERY_FILTER_H_
#define _OSMAND_CORE_LAMBDA_QUERY_FILTER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/IQueryFilter.h>

namespace OsmAnd
{
    class OSMAND_CORE_API LambdaQueryFilter : public IQueryFilter
    {
        Q_DISABLE_COPY(LambdaQueryFilter);
    public:
        typedef std::function<bool (ZoomLevel zoom)> ZoomFunctionSignature;
        typedef std::function<bool (const AreaI& area)> AreaFunctionSignature;
        typedef std::function<bool (const PointI& point)> PointFunctionSignature;
    private:
    protected:
        const ZoomFunctionSignature _zoomFunction;
        const AreaFunctionSignature _areaFunction;
        const PointFunctionSignature _pointFunction;
    public:
        LambdaQueryFilter(ZoomFunctionSignature zoomFunction, AreaFunctionSignature areaFunction, PointFunctionSignature pointFunction);
        virtual ~LambdaQueryFilter();

        virtual bool acceptsZoom(ZoomLevel zoom) const;
        virtual bool acceptsArea(const AreaI& area) const;
        virtual bool acceptsPoint(const PointI& point) const;
    };
}

#endif // !defined(_OSMAND_CORE_LAMBDA_QUERY_FILTER_H_)
