#ifndef _OSMAND_CORE_RASTERIZER_SHARED_CONTEXT_H_
#define _OSMAND_CORE_RASTERIZER_SHARED_CONTEXT_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QtGlobal>

#include <OsmAndCore.h>

namespace OsmAnd
{
    class Rasterizer;
    class Rasterizer_P;
    class RasterizerContext;
    class RasterizerContext_P;

    class RasterizerSharedContext_P;
    class OSMAND_CORE_API RasterizerSharedContext
    {
        Q_DISABLE_COPY(RasterizerSharedContext);
    private:
        const std::unique_ptr<RasterizerSharedContext_P> _d;
    protected:
    public:
        RasterizerSharedContext();
        virtual ~RasterizerSharedContext();

    friend class OsmAnd::Rasterizer;
    friend class OsmAnd::Rasterizer_P;
    friend class OsmAnd::RasterizerContext;
    friend class OsmAnd::RasterizerContext_P;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_RASTERIZER_SHARED_CONTEXT_H_)
