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
        ZoomFunctionSignature _zoomFunction;
        AreaFunctionSignature _areaFunction;
        PointFunctionSignature _pointFunction;
    public:
        LambdaQueryFilter(ZoomFunctionSignature zoomFunction, AreaFunctionSignature areaFunction, PointFunctionSignature pointFunction);
        virtual ~LambdaQueryFilter();

        virtual bool acceptsZoom(ZoomLevel zoom);
        virtual bool acceptsArea(const AreaI& area);
        virtual bool acceptsPoint(const PointI& point);
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_LAMBDA_QUERY_FILTER_H_)
