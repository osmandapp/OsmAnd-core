#ifndef _OSMAND_CORE_SYMBOL_RASTERIZER_H_
#define _OSMAND_CORE_SYMBOL_RASTERIZER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QList>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <SkImage.h>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/TextRasterizer.h>
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Map/MapPrimitiviser.h>


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
        public:
            RasterizedSymbolsGroup(const std::shared_ptr<const MapObject>& mapObject);
            virtual ~RasterizedSymbolsGroup();

            const std::shared_ptr<const MapObject> mapObject;
            QList< std::shared_ptr<const RasterizedSymbol> > symbols;
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

            sk_sp<const SkImage> image;
            int order;
            ContentType contentType;
            QString content;
            LanguageId languageId;
            float minDistance;
        };

        class OSMAND_CORE_API RasterizedSpriteSymbol : public RasterizedSymbol
        {
            Q_DISABLE_COPY_AND_MOVE(RasterizedSpriteSymbol);
        private:
        protected:
        public:
            RasterizedSpriteSymbol(
                const std::shared_ptr<const RasterizedSymbolsGroup>& group,
                const std::shared_ptr<const MapPrimitiviser::Symbol>& primitiveSymbol);
            virtual ~RasterizedSpriteSymbol();

            PointI location31;
            PointI offset;
            bool drawAlongPath;
            AreaI intersectionBBox;
        };

        class OSMAND_CORE_API RasterizedOnPathSymbol : public RasterizedSymbol
        {
            Q_DISABLE_COPY_AND_MOVE(RasterizedOnPathSymbol);
        private:
        protected:
        public:
            RasterizedOnPathSymbol(
                const std::shared_ptr<const RasterizedSymbolsGroup>& group,
                const std::shared_ptr<const MapPrimitiviser::Symbol>& primitiveSymbol);
            virtual ~RasterizedOnPathSymbol();

            QVector<SkScalar> glyphsWidth;
        };

        //NOTE: This won't work due to directors+shared_ptr are not supported. To summarize: it's currently impossible to use any %shared_ptr-marked type in a director declaration
        typedef std::function<bool(const std::shared_ptr<const MapObject>& mapObject)> FilterByMapObject;
        /*OSMAND_CALLABLE(FilterByMapObject,
            bool,
            const std::shared_ptr<const MapObject>& mapObject);*/

    private:
        PrivateImplementation<SymbolRasterizer_P> _p;
    protected:
    public:
        SymbolRasterizer(
            const std::shared_ptr<const TextRasterizer>& textRasterizer = TextRasterizer::getDefault());
        virtual ~SymbolRasterizer();

        const std::shared_ptr<const TextRasterizer> textRasterizer;

        virtual void rasterize(
            const std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects>& primitivisedObjects,
            QList< std::shared_ptr<const RasterizedSymbolsGroup> >& outSymbolsGroups,
            const FilterByMapObject filter = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr) const;
    };
}

#endif // !defined(_OSMAND_CORE_SYMBOL_RASTERIZER_H_)
