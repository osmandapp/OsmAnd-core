#ifndef _OSMAND_CORE_CHAINED_FONT_FINDER_H_
#define _OSMAND_CORE_CHAINED_FONT_FINDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/ITypefaceFinder.h>

namespace OsmAnd
{
    class OSMAND_CORE_API ChainedTypefaceFinder Q_DECL_FINAL : public ITypefaceFinder
    {
        Q_DISABLE_COPY_AND_MOVE(ChainedTypefaceFinder);

    private:
    protected:
    public:
        ChainedTypefaceFinder(const QList< std::shared_ptr<const ITypefaceFinder> >& chain);
        virtual ~ChainedTypefaceFinder();

        const QList< std::shared_ptr<const ITypefaceFinder> > chain;

        virtual std::shared_ptr<const Typeface> findTypefaceForCharacterUCS4(
            const uint32_t character,
            const SkFontStyle style = SkFontStyle()) const Q_DECL_OVERRIDE;
    };
}

#endif // !defined(_OSMAND_CORE_CHAINED_FONT_FINDER_H_)
