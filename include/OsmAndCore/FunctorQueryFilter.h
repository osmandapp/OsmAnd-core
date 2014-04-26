#ifndef _OSMAND_CORE_FUNCTOR_QUERY_FILTER_H_
#define _OSMAND_CORE_FUNCTOR_QUERY_FILTER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/IQueryFilter.h>

namespace OsmAnd
{
    class OSMAND_CORE_API FunctorQueryFilter : public IQueryFilter
    {
        Q_DISABLE_COPY(FunctorQueryFilter);

    public:
        OSMAND_CALLABLE(AcceptsZoomCallback, bool, const ZoomLevel zoom);
        OSMAND_CALLABLE(AcceptsAreaCallback, bool, const AreaI& area);
        OSMAND_CALLABLE(AcceptsPointCallback, bool, const PointI& point);

    private:
    protected:
        const AcceptsZoomCallback _acceptsZoomCallback;
        const AcceptsAreaCallback _acceptsAreaCallback;
        const AcceptsPointCallback _acceptsPointCallback;
    public:
        FunctorQueryFilter(
            const AcceptsZoomCallback acceptsZoomCallback,
            const AcceptsAreaCallback acceptsAreaCallback,
            const AcceptsPointCallback acceptsPointCallback);
        virtual ~FunctorQueryFilter();

        virtual bool acceptsZoom(const ZoomLevel zoom) const;
        virtual bool acceptsArea(const AreaI& area) const;
        virtual bool acceptsPoint(const PointI& point) const;
    };
}

#endif // !defined(_OSMAND_CORE_FUNCTOR_QUERY_FILTER_H_)
