#ifndef _OSMAND_CORE_CORE_FONTS_COLLECTION_H_
#define _OSMAND_CORE_CORE_FONTS_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QByteArray>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/CachingFontsCollection.h>

namespace OsmAnd
{
    class OSMAND_CORE_API CoreFontsCollection Q_DECL_FINAL : public CachingFontsCollection
    {
        Q_DISABLE_COPY_AND_MOVE(CoreFontsCollection);
    public:
        struct CoreResourcesFont
        {
            QString resource;
            bool bold;
            bool italic;
        };

    private:
    protected:
    public:
        CoreFontsCollection();
        virtual ~CoreFontsCollection();

        static QList<CoreResourcesFont> fonts;

        virtual QString findSuitableFont(
            const QString& text,
            const bool isBold,
            const bool isItalic) const;

        virtual QByteArray obtainFont(
            const QString& fontName) const;

        static std::shared_ptr<const CoreFontsCollection> getDefaultInstance();
    };
}

#endif // !defined(_OSMAND_CORE_CORE_FONTS_COLLECTION_H_)
