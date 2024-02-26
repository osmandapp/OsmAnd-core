#ifndef _OSMAND_CORE_SYMBOL_RASTERIZER_P_H_
#define _OSMAND_CORE_SYMBOL_RASTERIZER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QList>
#include <QVector>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkCanvas.h>
#include <SkPaint.h>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "MapCommonTypes.h"
#include "MapPrimitiviser.h"
#include "SymbolRasterizer.h"

namespace OsmAnd
{
    class MapObject;
    class IQueryController;

    class SymbolRasterizer_P Q_DECL_FINAL
    {
    public:
        typedef SymbolRasterizer::RasterizedSymbolsGroup RasterizedSymbolsGroup;
        typedef SymbolRasterizer::RasterizedSymbol RasterizedSymbol;
        typedef SymbolRasterizer::RasterizedSpriteSymbol RasterizedSpriteSymbol;
        typedef SymbolRasterizer::RasterizedOnPathSymbol RasterizedOnPathSymbol;
        typedef SymbolRasterizer::FilterBySymbolsGroup FilterBySymbolsGroup;
        typedef MapPrimitiviser::TextSymbol::Placement TextSymbolPlacement;

    private:
    protected:
        SymbolRasterizer_P(SymbolRasterizer* const owner);
    public:
        ~SymbolRasterizer_P();

        ImplementationInterface<SymbolRasterizer> owner;

        void rasterize(
            const MapPrimitiviser::SymbolsGroupsCollection& symbolsGroups,
            const std::shared_ptr<const MapPresentationEnvironment>& mapPresentationEnvironment,
            QList< std::shared_ptr<const RasterizedSymbolsGroup> >& outSymbolsGroups,
            const FilterBySymbolsGroup filter,
            const std::shared_ptr<const IQueryController>& queryController) const;

    friend class OsmAnd::SymbolRasterizer;
    };
}

#endif // !defined(_OSMAND_CORE_SYMBOL_RASTERIZER_P_H_)
