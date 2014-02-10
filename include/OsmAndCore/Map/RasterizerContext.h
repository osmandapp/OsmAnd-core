#ifndef _OSMAND_CORE_RASTERIZER_CONTEXT_H_
#define _OSMAND_CORE_RASTERIZER_CONTEXT_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QtGlobal>

#include <OsmAndCore.h>

namespace OsmAnd {

    class Rasterizer;
    class RasterizerEnvironment;
    class RasterizerSharedContext;

    class RasterizerContext_P;
    class OSMAND_CORE_API RasterizerContext
    {
        Q_DISABLE_COPY(RasterizerContext);
    private:
        const std::unique_ptr<RasterizerContext_P> _d;
    protected:
    public:
        RasterizerContext(const std::shared_ptr<RasterizerEnvironment>& environment, const std::shared_ptr<RasterizerSharedContext>& sharedContext = nullptr);
        virtual ~RasterizerContext();

        const std::shared_ptr<RasterizerEnvironment> environment;
        const std::shared_ptr<RasterizerSharedContext> sharedContext;

    friend class OsmAnd::Rasterizer;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_RASTERIZER_CONTEXT_H_)
