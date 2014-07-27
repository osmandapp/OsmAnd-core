#ifndef _OSMAND_CORE_TEXT_RASTERIZER_P_H_
#define _OSMAND_CORE_TEXT_RASTERIZER_P_H_

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
#include "TextRasterizer.h"

namespace OsmAnd
{
    namespace Model
    {
        class BinaryMapObject;
    }
    class IQueryController;

    class TextRasterizer;
    class TextRasterizer_P Q_DECL_FINAL
    {
    public:
        typedef TextRasterizer::Style Style;

    private:
    protected:
        TextRasterizer_P(TextRasterizer* const owner);

        void initialize();

        SkPaint _defaultPaint;

        struct FontsRegisterEntry
        {
            QString resource;
            bool bold;
            bool italic;
        };
        QList< FontsRegisterEntry > _fontsRegister;

        mutable QMutex _fontTypefacesCacheMutex;
        mutable QHash< QString, SkTypeface* > _fontTypefacesCache;
        void clearFontsCache();
        SkTypeface* getTypefaceForFontResource(const QString& fontResource) const;

        void configurePaintForText(SkPaint& paint, const QString& text, const bool bold, const bool italic) const;
    public:
        ~TextRasterizer_P();

        ImplementationInterface<TextRasterizer> owner;

        std::shared_ptr<SkBitmap> rasterize(
            const QString& text,
            const Style& style,
            QVector<SkScalar>* const outGlyphWidths,
            float* const outExtraTopSpace,
            float* const outExtraBottomSpace) const;

        void rasterize(
            SkBitmap& targetBitmap,
            const QString& text,
            const Style& style,
            QVector<SkScalar>* const outGlyphWidths,
            float* const outExtraTopSpace,
            float* const outExtraBottomSpace) const;

    friend class OsmAnd::TextRasterizer;
    };
}

#endif // !defined(_OSMAND_CORE_TEXT_RASTERIZER_P_H_)
