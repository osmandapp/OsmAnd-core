#ifndef _OSMAND_CORE_CACHING_FONTS_COLLECTION_H_
#define _OSMAND_CORE_CACHING_FONTS_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/CommonSWIG.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/IFontsCollection.h>

namespace OsmAnd
{
    class CachingFontsCollection_P;
    class OSMAND_CORE_API CachingFontsCollection : public IFontsCollection
    {
        Q_DISABLE_COPY_AND_MOVE(CachingFontsCollection);

    private:
        PrivateImplementation<CachingFontsCollection_P> _p;
    protected:
        CachingFontsCollection();
    public:
        virtual ~CachingFontsCollection();

        virtual SkTypeface* obtainTypeface(
            const QString& fontName) const;

        virtual QByteArray obtainFont(
            const QString& fontName) const = 0;
    };

    SWIG_EMIT_DIRECTOR_BEGIN(CachingFontsCollection);
        SWIG_EMIT_DIRECTOR_CONST_METHOD(
            QString,
            findSuitableFont,
            const QString& text,
            const bool isBold,
            const bool isItalic);
        SWIG_EMIT_DIRECTOR_CONST_METHOD(
            QByteArray,
            obtainFont,
            const QString& fontName);
    SWIG_EMIT_DIRECTOR_END(CachingFontsCollection);
}

#endif // !defined(_OSMAND_CORE_CACHING_FONTS_COLLECTION_H_)
