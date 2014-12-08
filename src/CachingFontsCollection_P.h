#ifndef _OSMAND_CORE_CACHING_FONTS_COLLECTION_P_H_
#define _OSMAND_CORE_CACHING_FONTS_COLLECTION_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QString>
#include <QMutex>
#include <QHash>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "IFontsCollection.h"

class SkTypeface;

namespace OsmAnd
{
    class CachingFontsCollection;
    class CachingFontsCollection_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(CachingFontsCollection_P);

    private:
        mutable QMutex _fontTypefacesCacheMutex;
        mutable QHash< QString, SkTypeface* > _fontTypefacesCache;
        SkTypeface* getTypefaceForFont(const QString& fontName) const;
    protected:
        CachingFontsCollection_P(CachingFontsCollection* const owner);

        void clearFontsCache();
    public:
        virtual ~CachingFontsCollection_P();

        ImplementationInterface<CachingFontsCollection> owner;

        SkTypeface* obtainTypeface(
            const QString& fontName) const;

    friend class OsmAnd::CachingFontsCollection;
    };
}

#endif // !defined(_OSMAND_CORE_CACHING_FONTS_COLLECTION_P_H_)
