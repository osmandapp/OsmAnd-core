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
#include "Primitiviser.h"
#include "SymbolRasterizer.h"

namespace OsmAnd
{
    class MapObject;
    class IQueryController;

    class SymbolRasterizer;
    class SymbolRasterizer_P Q_DECL_FINAL
    {
    public:
        typedef SymbolRasterizer::RasterizedSymbolsGroup RasterizedSymbolsGroup;
        typedef SymbolRasterizer::RasterizedSymbol RasterizedSymbol;
        typedef SymbolRasterizer::RasterizedSpriteSymbol RasterizedSpriteSymbol;
        typedef SymbolRasterizer::RasterizedOnPathSymbol RasterizedOnPathSymbol;

    private:
    protected:
        SymbolRasterizer_P(SymbolRasterizer* const owner);
    public:
        ~SymbolRasterizer_P();

        ImplementationInterface<SymbolRasterizer> owner;

        void rasterize(
            const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivisedArea,
            QList< std::shared_ptr<const RasterizedSymbolsGroup> >& outSymbolsGroups,
            std::function<bool(const std::shared_ptr<const MapObject>& mapObject)> filter,
            const IQueryController* const controller);

    friend class OsmAnd::SymbolRasterizer;
    };
}

#endif // !defined(_OSMAND_CORE_SYMBOL_RASTERIZER_P_H_)
