#ifndef _OSMAND_CORE_SYMBOL_RASTERIZER_P_H_
#define _OSMAND_CORE_SYMBOL_RASTERIZER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include <QList>
#include <QVector>

#include <SkCanvas.h>
#include <SkPaint.h>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "MapTypes.h"
#include "Primitiviser.h"
#include "SymbolRasterizer.h"

namespace OsmAnd
{
    namespace Model
    {
        class BinaryMapObject;
    }
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
            const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
            QList< std::shared_ptr<const RasterizedSymbolsGroup> >& outSymbolsGroups,
            std::function<bool(const std::shared_ptr<const Model::BinaryMapObject>& mapObject)> filter,
            const IQueryController* const controller);

    friend class OsmAnd::SymbolRasterizer;
    };
}

#endif // !defined(_OSMAND_CORE_SYMBOL_RASTERIZER_P_H_)
