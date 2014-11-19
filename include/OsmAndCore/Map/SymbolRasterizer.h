#ifndef _OSMAND_CORE_SYMBOL_RASTERIZER_H_
#define _OSMAND_CORE_SYMBOL_RASTERIZER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Map/MapPrimitiviser.h>

class SkCanvas;
class SkBitmap;

namespace OsmAnd
{
    class MapObject;

    class SymbolRasterizer_P;
    class OSMAND_CORE_API SymbolRasterizer
    {
        Q_DISABLE_COPY_AND_MOVE(SymbolRasterizer);
    public:
        class RasterizedSymbol;

        class OSMAND_CORE_API RasterizedSymbolsGroup
        {
            Q_DISABLE_COPY_AND_MOVE(RasterizedSymbolsGroup);
        private:
        protected:
            RasterizedSymbolsGroup(const std::shared_ptr<const MapObject>& mapObject);
        public:
            virtual ~RasterizedSymbolsGroup();

            const std::shared_ptr<const MapObject> mapObject;
            QList< std::shared_ptr<const RasterizedSymbol> > symbols;

        friend class OsmAnd::SymbolRasterizer_P;
        };

        class OSMAND_CORE_API RasterizedSymbol
        {
            Q_DISABLE_COPY_AND_MOVE(RasterizedSymbol);

        public:
            enum class ContentType
            {
                Icon,
                Text
            };

        private:
        protected:
            RasterizedSymbol(
                const std::shared_ptr<const RasterizedSymbolsGroup>& group,
                const std::shared_ptr<const MapPrimitiviser::Symbol>& primitiveSymbol);
        public:
            virtual ~RasterizedSymbol();

            const std::weak_ptr<const RasterizedSymbolsGroup> group;
            const std::shared_ptr<const MapPrimitiviser::Symbol> primitiveSymbol;

            std::shared_ptr<const SkBitmap> bitmap;
            int order;
            ContentType contentType;
            QString content;
            LanguageId languageId;
            float minDistance;
            float pathPaddingLeft;
            float pathPaddingRight;
        };

        class OSMAND_CORE_API RasterizedSpriteSymbol : public RasterizedSymbol
        {
            Q_DISABLE_COPY_AND_MOVE(RasterizedSpriteSymbol);
        private:
        protected:
            RasterizedSpriteSymbol(
                const std::shared_ptr<const RasterizedSymbolsGroup>& group,
                const std::shared_ptr<const MapPrimitiviser::Symbol>& primitiveSymbol);
        public:
            virtual ~RasterizedSpriteSymbol();

            PointI location31;
            PointI offset;
            bool drawAlongPath;
            PointI intersectionSize;

        friend class OsmAnd::SymbolRasterizer_P;
        };

        class OSMAND_CORE_API RasterizedOnPathSymbol : public RasterizedSymbol
        {
            Q_DISABLE_COPY_AND_MOVE(RasterizedOnPathSymbol);
        private:
        protected:
            RasterizedOnPathSymbol(
                const std::shared_ptr<const RasterizedSymbolsGroup>& group,
                const std::shared_ptr<const MapPrimitiviser::Symbol>& primitiveSymbol);
        public:
            virtual ~RasterizedOnPathSymbol();

            QVector<SkScalar> glyphsWidth;

        friend class OsmAnd::SymbolRasterizer_P;
        };

    private:
        PrivateImplementation<SymbolRasterizer_P> _p;
    protected:
    public:
        SymbolRasterizer();
        virtual ~SymbolRasterizer();

        void rasterize(
            const std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects>& primitivisedObjects,
            QList< std::shared_ptr<const RasterizedSymbolsGroup> >& outSymbolsGroups,
            std::function<bool(const std::shared_ptr<const MapObject>& mapObject)> filter = nullptr,
            const IQueryController* const controller = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_SYMBOL_RASTERIZER_H_)
