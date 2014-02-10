#ifndef _OSMAND_CORE_RASTERIZED_SYMBOLS_GROUP_H_
#define _OSMAND_CORE_RASTERIZED_SYMBOLS_GROUP_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    class RasterizedSymbol;
    namespace Model {
        class MapObject;
    } // namespace Model
    class Rasterizer_P;

    class OSMAND_CORE_API RasterizedSymbolsGroup
    {
        Q_DISABLE_COPY(RasterizedSymbolsGroup);
    private:
    protected:
        RasterizedSymbolsGroup(const std::shared_ptr<const Model::MapObject>& mapObject);
    public:
        virtual ~RasterizedSymbolsGroup();

        const std::shared_ptr<const Model::MapObject> mapObject;
        QList< std::shared_ptr<const RasterizedSymbol> > symbols;

        friend class OsmAnd::Rasterizer_P;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_RASTERIZED_SYMBOLS_GROUP_H_)
