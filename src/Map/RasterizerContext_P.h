#ifndef _OSMAND_CORE_RASTERIZER_CONTEXT_P_H_
#define _OSMAND_CORE_RASTERIZER_CONTEXT_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QList>
#include <QVector>

#include <SkColor.h>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "Rasterizer_P.h"

namespace OsmAnd
{
    class Rasterizer_P;

    class RasterizerContext;
    class RasterizerContext_P Q_DECL_FINAL
    {
    private:
    protected:
        RasterizerContext_P(RasterizerContext* owner);

        ImplementationInterface<RasterizerContext> owner;

        SkColor _defaultBgColor;
        int _shadowRenderingMode;
        SkColor _shadowRenderingColor;
        double _polygonMinSizeToDisplay;
        uint32_t _roadDensityZoomTile;
        uint32_t _roadsDensityLimitPerTile;

        AreaI _area31;
        AreaI _largerArea31;
        ZoomLevel _zoom;
        double _tileDivisor;
        uint32_t _shadowLevelMin;
        uint32_t _shadowLevelMax;

        QVector< std::shared_ptr<const Rasterizer_P::PrimitivesGroup> > _primitivesGroups;
        QVector< std::shared_ptr<const Rasterizer_P::Primitive> > _polygons, _polylines, _points;

        QVector< std::shared_ptr<const Rasterizer_P::SymbolsGroup> > _symbolsGroups;
        typedef std::pair<
            std::shared_ptr<const Model::BinaryMapObject>,
            QVector< std::shared_ptr<const Rasterizer_P::PrimitiveSymbol> >
        > SymbolsEntry;
        QVector<SymbolsEntry> _symbols;

        void clear();
    public:
        virtual ~RasterizerContext_P();

    friend class OsmAnd::RasterizerContext;
    friend class OsmAnd::Rasterizer_P;
    };
}

#endif // !defined(_OSMAND_CORE_RASTERIZER_CONTEXT_P_H_)
