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
}

#endif // !defined(_OSMAND_CORE_I_FONTS_COLLECTION_H_)
