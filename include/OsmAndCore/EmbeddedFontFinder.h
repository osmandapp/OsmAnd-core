#ifndef _OSMAND_CORE_EMBEDDED_FONT_FINDER_H_
#define _OSMAND_CORE_EMBEDDED_FONT_FINDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QStringList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/IFontFinder.h>

namespace OsmAnd
{
    class OSMAND_CORE_API EmbeddedFontFinder Q_DECL_FINAL : public IFontFinder
    {
        Q_DISABLE_COPY_AND_MOVE(EmbeddedFontFinder);

    private:
        QList<sk_sp<SkTypeface>> _fonts;
    protected:
    public:
        EmbeddedFontFinder(
            const std::shared_ptr<const ICoreResourcesProvider>& coreResourcesProvider = getCoreResourcesProvider());
        virtual ~EmbeddedFontFinder();

        const std::shared_ptr<const ICoreResourcesProvider> coreResourcesProvider;

        virtual SkTypeface* findFontForCharacterUCS4(
            const uint32_t character,
            const SkFontStyle style = SkFontStyle()) const Q_DECL_OVERRIDE;

        static const QStringList resources;
        static std::shared_ptr<const EmbeddedFontFinder> getDefaultInstance();
    };
}

#endif // !defined(_OSMAND_CORE_EMBEDDED_FONT_FINDER_H_)
