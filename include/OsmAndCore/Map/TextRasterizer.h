#ifndef _OSMAND_CORE_TEXT_RASTERIZER_H_
#define _OSMAND_CORE_TEXT_RASTERIZER_H_

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
    class TextRasterizer_P;
    class OSMAND_CORE_API TextRasterizer
    {
        Q_DISABLE_COPY(TextRasterizer);
    public:
        struct Style Q_DECL_FINAL
        {
            Style()
                : wrapWidth(0)
                , size(16.0f)
                , bold(false)
                , italic(false)
                , haloRadius(0)
            {
            }

            unsigned int wrapWidth;
            inline Style& setWrapWidth(const unsigned int newWrapWidth)
            {
                wrapWidth = newWrapWidth;

                return *this;
            }

            float size;
            inline Style& setSize(const float newSize)
            {
                size = newSize;

                return *this;
            }

            bool bold;
            inline Style& setBold(const bool newBold)
            {
                bold = newBold;

                return *this;
            }

            bool italic;
            inline Style& setItalic(const bool newItalic)
            {
                italic = newItalic;

                return *this;
            }

            ColorARGB color;
            inline Style& setColor(const ColorARGB newColor)
            {
                color = newColor;

                return *this;
            }

            unsigned int haloRadius;
            inline Style& setHaloRadius(const unsigned int newHaloRadius)
            {
                haloRadius = newHaloRadius;

                return *this;
            }

            ColorARGB haloColor;
            inline Style& setHaloColor(const ColorARGB newHaloColor)
            {
                haloColor = newHaloColor;

                return *this;
            }

            std::shared_ptr<const SkBitmap> backgroundBitmap;
            inline Style& setBackgroundBitmap(const std::shared_ptr<const SkBitmap>& newBackgroundBitmap)
            {
                backgroundBitmap = newBackgroundBitmap;

                return *this;
            }
        };

    private:
        PrivateImplementation<TextRasterizer_P> _p;
    protected:
        TextRasterizer();
    public:
        virtual ~TextRasterizer();

        static const TextRasterizer& getInstance();

        std::shared_ptr<SkBitmap> rasterize(
            const QString& text,
            const Style& style = Style(),
            QVector<SkScalar>* const outGlyphWidths = nullptr,
            float* const outExtraTopSpace = nullptr,
            float* const outExtraBottomSpace = nullptr,
            float* const outLineSpacing = nullptr) const;
        
        void rasterize(
            SkBitmap& targetBitmap,
            const QString& text,
            const Style& style = Style(),
            QVector<SkScalar>* const outGlyphWidths = nullptr,
            float* const outExtraTopSpace = nullptr,
            float* const outExtraBottomSpace = nullptr,
            float* const outLineSpacing = nullptr) const;
    };
}

#endif // !defined(_OSMAND_CORE_TEXT_RASTERIZER_H_)
