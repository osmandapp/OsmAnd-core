#ifndef _OSMAND_CORE_RASTERIZED_PINNED_SYMBOL_H_
#define _OSMAND_CORE_RASTERIZED_PINNED_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/RasterizedSymbol.h>

namespace OsmAnd
{
    class Rasterizer_P;

    class OSMAND_CORE_API RasterizedSpriteSymbol : public RasterizedSymbol
    {
        Q_DISABLE_COPY(RasterizedSpriteSymbol);
    private:
    protected:
        RasterizedSpriteSymbol(
            const std::shared_ptr<const RasterizedSymbolsGroup>& group,
            const std::shared_ptr<const Model::MapObject>& mapObject,
            const std::shared_ptr<const SkBitmap>& bitmap,
            const int order,
            const QString& content,
            const LanguageId& languageId,
            const PointI& minDistance,
            const PointI& location31,
            const PointI& offset);
    public:
        virtual ~RasterizedSpriteSymbol();

        const PointI location31;
        const PointI offset;

    friend class OsmAnd::Rasterizer_P;
    };
}

#endif // !defined(_OSMAND_CORE_RASTERIZED_PINNED_SYMBOL_H_)
