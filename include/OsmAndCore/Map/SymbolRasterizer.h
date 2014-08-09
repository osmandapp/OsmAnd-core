#ifndef _OSMAND_CORE_SYMBOL_RASTERIZER_H_
#define _OSMAND_CORE_SYMBOL_RASTERIZER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/MapTypes.h>
#include <OsmAndCore/Map/Primitiviser.h>

class SkCanvas;
class SkBitmap;

namespace OsmAnd
{
    class SymbolRasterizer_P;
    class OSMAND_CORE_API SymbolRasterizer
    {
        Q_DISABLE_COPY(SymbolRasterizer);
    public:
        class RasterizedSymbol;

        class OSMAND_CORE_API RasterizedSymbolsGroup
        {
            Q_DISABLE_COPY(RasterizedSymbolsGroup);
        private:
        protected:
            RasterizedSymbolsGroup(const std::shared_ptr<const Model::BinaryMapObject>& mapObject);
        public:
            virtual ~RasterizedSymbolsGroup();

            const std::shared_ptr<const Model::BinaryMapObject> mapObject;
            QList< std::shared_ptr<const RasterizedSymbol> > symbols;

        friend class OsmAnd::SymbolRasterizer_P;
        };

        class OSMAND_CORE_API RasterizedSymbol
        {
            Q_DISABLE_COPY(RasterizedSymbol);

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
                const std::shared_ptr<const Model::BinaryMapObject>& mapObject,
                const std::shared_ptr<const SkBitmap>& bitmap,
                const int order,
                const ContentType contentType,
                const QString& content,
                const LanguageId& languageId,
                const PointI& minDistance);
        public:
            virtual ~RasterizedSymbol();

            const std::weak_ptr<const RasterizedSymbolsGroup> group;

            const std::shared_ptr<const Model::BinaryMapObject> mapObject;
            const std::shared_ptr<const SkBitmap> bitmap;
            const int order;
            const ContentType contentType;
            const QString content;
            const LanguageId languageId;
            const PointI minDistance;
        };

        class OSMAND_CORE_API RasterizedSpriteSymbol : public RasterizedSymbol
        {
            Q_DISABLE_COPY(RasterizedSpriteSymbol);
        private:
        protected:
            RasterizedSpriteSymbol(
                const std::shared_ptr<const RasterizedSymbolsGroup>& group,
                const std::shared_ptr<const Model::BinaryMapObject>& mapObject,
                const std::shared_ptr<const SkBitmap>& bitmap,
                const int order,
                const ContentType contentType,
                const QString& content,
                const LanguageId& languageId,
                const PointI& minDistance,
                const PointI& location31,
                const PointI& offset);
        public:
            virtual ~RasterizedSpriteSymbol();

            const PointI location31;
            const PointI offset;

        friend class OsmAnd::SymbolRasterizer_P;
        };

        class OSMAND_CORE_API RasterizedOnPathSymbol : public RasterizedSymbol
        {
            Q_DISABLE_COPY(RasterizedOnPathSymbol);
        private:
        protected:
            RasterizedOnPathSymbol(
                const std::shared_ptr<const RasterizedSymbolsGroup>& group,
                const std::shared_ptr<const Model::BinaryMapObject>& mapObject,
                const std::shared_ptr<const SkBitmap>& bitmap,
                const int order,
                const ContentType contentType,
                const QString& content,
                const LanguageId& languageId,
                const PointI& minDistance,
                const QVector<SkScalar>& glyphsWidth);
        public:
            virtual ~RasterizedOnPathSymbol();

            const QVector<SkScalar> glyphsWidth;

        friend class OsmAnd::SymbolRasterizer_P;
        };

    private:
        PrivateImplementation<SymbolRasterizer_P> _p;
    protected:
    public:
        SymbolRasterizer();
        virtual ~SymbolRasterizer();

        void rasterize(
            const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
            QList< std::shared_ptr<const RasterizedSymbolsGroup> >& outSymbolsGroups,
            std::function<bool(const std::shared_ptr<const Model::BinaryMapObject>& mapObject)> filter = nullptr,
            const IQueryController* const controller = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_SYMBOL_RASTERIZER_H_)
