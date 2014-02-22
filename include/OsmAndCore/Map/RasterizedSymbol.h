#ifndef _OSMAND_CORE_RASTERIZED_SYMBOL_H_
#define _OSMAND_CORE_RASTERIZED_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <SkBitmap.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/RasterizedSymbolsGroup.h>
#include <OsmAndCore/Data/Model/MapObject.h>

namespace OsmAnd
{
    class OSMAND_CORE_API RasterizedSymbol
    {
        Q_DISABLE_COPY(RasterizedSymbol);
    private:
    protected:
        RasterizedSymbol(
            const std::shared_ptr<const RasterizedSymbolsGroup>& group,
            const std::shared_ptr<const Model::MapObject>& mapObject,
            const std::shared_ptr<const SkBitmap>& bitmap,
            const int order,
            const QString& content,
            const LanguageId& langId,
            const PointI& minDistance);
    public:
        virtual ~RasterizedSymbol();

        const std::weak_ptr<const RasterizedSymbolsGroup> group;

        const std::shared_ptr<const Model::MapObject> mapObject;
        const std::shared_ptr<const SkBitmap> bitmap;
        const int order;
        const QString content;
        const LanguageId langId;
        const PointI minDistance;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_RASTERIZED_SYMBOL_H_)
