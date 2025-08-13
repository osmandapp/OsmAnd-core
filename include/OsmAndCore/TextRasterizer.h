#ifndef _OSMAND_CORE_TEXT_RASTERIZER_H_
#define _OSMAND_CORE_TEXT_RASTERIZER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QList>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <SkBitmap.h>
#include <SkImage.h>
#include <SkTypeface.h>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/CommonSWIG.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/ITypefaceFinder.h>
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Map/MapPrimitiviser.h>

namespace OsmAnd
{
    class TextRasterizer_P;
    class OSMAND_CORE_API TextRasterizer
    {
        Q_DISABLE_COPY_AND_MOVE(TextRasterizer);
    public:
        struct Style Q_DECL_FINAL
        {
            Style()
                : wrapWidth(0)
                , maxLines(0)
                , size(16.0f)
                , bold(false)
                , italic(false)
                , haloRadius(0)
                , textAlignment(TextAlignment::Center)
            {
            }

            unsigned int wrapWidth;
#if !defined(SWIG)
            inline Style& setWrapWidth(const unsigned int newWrapWidth)
            {
                wrapWidth = newWrapWidth;

                return *this;
            }
#endif // !defined(SWIG)

            unsigned int maxLines;
#if !defined(SWIG)
            inline Style& setMaxLines(const unsigned int newMaxLines)
            {
                maxLines = newMaxLines;

                return *this;
            }
#endif // !defined(SWIG)
            
            float size;
#if !defined(SWIG)
            inline Style& setSize(const float newSize)
            {
                size = newSize;

                return *this;
            }
#endif // !defined(SWIG)

            bool bold;
#if !defined(SWIG)
            inline Style& setBold(const bool newBold)
            {
                bold = newBold;

                return *this;
            }
#endif // !defined(SWIG)

            bool italic;
#if !defined(SWIG)
            inline Style& setItalic(const bool newItalic)
            {
                italic = newItalic;

                return *this;
            }
#endif // !defined(SWIG)

            ColorARGB color;
#if !defined(SWIG)
            inline Style& setColor(const ColorARGB newColor)
            {
                color = newColor;

                return *this;
            }
#endif // !defined(SWIG)

            unsigned int haloRadius;
#if !defined(SWIG)
            inline Style& setHaloRadius(const unsigned int newHaloRadius)
            {
                haloRadius = newHaloRadius;

                return *this;
            }
#endif // !defined(SWIG)

            ColorARGB haloColor;
#if !defined(SWIG)
            inline Style& setHaloColor(const ColorARGB newHaloColor)
            {
                haloColor = newHaloColor;

                return *this;
            }
#endif // !defined(SWIG)

            sk_sp<const SkImage> backgroundImage;
#if !defined(SWIG)
            inline Style& setBackgroundImage(const sk_sp<const SkImage>& newBackgroundImage)
            {
                backgroundImage = newBackgroundImage;

                return *this;
            }
#endif // !defined(SWIG)

            enum class TextAlignment
            {
                Left,
                Center,
                Right,
                Under
            };
            TextAlignment textAlignment;
#if !defined(SWIG)
            inline Style& setTextAlignment(const TextAlignment newTextAlignment)
            {
                textAlignment = newTextAlignment;

                return *this;
            }
#endif // !defined(SWIG)

            inline bool operator==(const Style& other) const
            {
                return wrapWidth == other.wrapWidth &&
                       maxLines == other.maxLines &&
                       size == other.size &&
                       bold == other.bold &&
                       italic == other.italic &&
                       color == other.color &&
                       haloRadius == other.haloRadius &&
                       haloColor == other.haloColor &&
                       backgroundImage == other.backgroundImage &&
                       textAlignment == other.textAlignment;
            }

            inline bool operator!=(const Style& other) const
            {
                return !(*this == other);
            }
        };

    private:
        PrivateImplementation<TextRasterizer_P> _p;
    protected:
    public:
        TextRasterizer(
            const std::shared_ptr<const ITypefaceFinder>& typefaceFinder);
        virtual ~TextRasterizer();

        const std::shared_ptr<const ITypefaceFinder> typefaceFinder;

        sk_sp<SkImage> rasterize(
            const QString& text,
            const Style& style = Style(),
            QVector<SkScalar>* const outGlyphWidths = nullptr,
            float* const outExtraTopSpace = nullptr,
            float* const outExtraBottomSpace = nullptr,
            float* const outLineSpacing = nullptr,
            float* const outFontAscent = nullptr) const;
        
        bool rasterize(
            SkBitmap& targetBitmap,
            const QString& text,
            const Style& style = Style(),
            QVector<SkScalar>* const outGlyphWidths = nullptr,
            float* const outExtraTopSpace = nullptr,
            float* const outExtraBottomSpace = nullptr,
            float* const outLineSpacing = nullptr,
            float* const outFontAscent = nullptr) const;

        static std::shared_ptr<const TextRasterizer> getDefault();
        static std::shared_ptr<const TextRasterizer> getOnlySystemFonts();
    };
}

#endif // !defined(_OSMAND_CORE_TEXT_RASTERIZER_H_)
