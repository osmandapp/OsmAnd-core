#ifndef _OSMAND_CORE_EMBEDDED_TYPEFACE_FINDER_H_
#define _OSMAND_CORE_EMBEDDED_TYPEFACE_FINDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QStringList>
#include <SkRefCnt.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/ITypefaceFinder.h>

namespace OsmAnd
{
    class OSMAND_CORE_API EmbeddedTypefaceFinder Q_DECL_FINAL : public ITypefaceFinder
    {
        Q_DISABLE_COPY_AND_MOVE(EmbeddedTypefaceFinder);

    private:
        QList< std::shared_ptr<const Typeface> > _typefaces;
    protected:
    public:
        EmbeddedTypefaceFinder(
            const std::shared_ptr<const ICoreResourcesProvider>& coreResourcesProvider = getCoreResourcesProvider());
        virtual ~EmbeddedTypefaceFinder();

        const std::shared_ptr<const ICoreResourcesProvider> coreResourcesProvider;

        virtual std::shared_ptr<const Typeface> findTypefaceForCharacterUCS4(
            const uint32_t character,
            const SkFontStyle style = SkFontStyle()) const Q_DECL_OVERRIDE;

        static const QStringList resources;
        static std::shared_ptr<const EmbeddedTypefaceFinder> getDefaultInstance();
    };
}

#endif // !defined(_OSMAND_CORE_EMBEDDED_TYPEFACE_FINDER_H_)
