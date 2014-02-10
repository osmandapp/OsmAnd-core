#ifndef _OSMAND_CORE_RASTERIZER_SHARED_CONTEXT_P_H_
#define _OSMAND_CORE_RASTERIZER_SHARED_CONTEXT_P_H_

#include <OsmAndCore/stdlib_common.h>
#include <array>

#include <OsmAndCore/QtExtensions.h>
#include <QtGlobal>

#include "OsmAndCore.h"
#include "Rasterizer_P.h"
#include "SharedResourcesContainer.h"

namespace OsmAnd
{
    class Rasterizer_P;
    class RasterizerContext;
    class RasterizerContext_P;

    class RasterizerSharedContext;
    class RasterizerSharedContext_P
    {
    private:
    protected:
        RasterizerSharedContext_P(RasterizerSharedContext* const owner);

        RasterizerSharedContext* const _owner;

        std::array< SharedResourcesContainer<uint64_t, const Rasterizer_P::PrimitivesGroup>, ZoomLevelsCount> _sharedPrimitivesGroups;
        std::array< SharedResourcesContainer<uint64_t, const Rasterizer_P::SymbolsGroup>, ZoomLevelsCount> _sharedSymbolGroups;
    public:
        virtual ~RasterizerSharedContext_P();

    friend class OsmAnd::RasterizerSharedContext;
    friend class OsmAnd::Rasterizer_P;
    friend class OsmAnd::RasterizerContext;
    friend class OsmAnd::RasterizerContext_P;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_RASTERIZER_SHARED_CONTEXT_P_H_)
