#ifndef _OSMAND_CORE_I_FONTS_COLLECTION_H_
#define _OSMAND_CORE_I_FONTS_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/CommonSWIG.h>

class SkTypeface;

namespace OsmAnd
{
    class OSMAND_CORE_API IFontsCollection
    {
        Q_DISABLE_COPY_AND_MOVE(IFontsCollection);

    private:
    protected:
        IFontsCollection();
    public:
        virtual ~IFontsCollection();

        virtual QString findSuitableFont(
            const QString& text,
            const bool isBold,
            const bool isItalic) const = 0;
        virtual SkTypeface* obtainTypeface(
            const QString& fontName) const = 0;
    };

    SWIG_EMIT_DIRECTOR_BEGIN(IFontsCollection);
        SWIG_EMIT_DIRECTOR_CONST_METHOD(
            QString,
            findSuitableFont,
            const QString& text,
            const bool isBold,
            const bool isItalic);
        SWIG_EMIT_DIRECTOR_CONST_METHOD(
            SkTypeface*,
            obtainTypeface,
            const QString& fontName);
    SWIG_EMIT_DIRECTOR_END(IFontsCollection);
}

#endif // !defined(_OSMAND_CORE_I_FONTS_COLLECTION_H_)
