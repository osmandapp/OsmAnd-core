#ifndef _OSMAND_CORE_CACHING_FONT_FINDER_H_
#define _OSMAND_CORE_CACHING_FONT_FINDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>
#include <QMutex>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/IFontFinder.h>

namespace OsmAnd
{
    class OSMAND_CORE_API CachingFontFinder Q_DECL_FINAL : public IFontFinder
    {
        Q_DISABLE_COPY_AND_MOVE(CachingFontFinder);

    private:
        static_assert(sizeof(SkFontStyle) == 4, "Check size of SkFontStyle");
        typedef uint32_t StyleId;
        union CacheKey
        {
            uint64_t key;
            struct 
            {
                StyleId styleId;
                uint32_t character;
            };

#if !defined(SWIG)
            inline operator uint64_t() const
            {
                return key;
            }

            inline bool operator==(const CacheKey& that)
            {
                return this->key == that.key;
            }

            inline bool operator!=(const CacheKey& that)
            {
                return this->key != that.key;
            }

            inline bool operator==(const uint64_t& that)
            {
                return this->key == that;
            }

            inline bool operator!=(const uint64_t& that)
            {
                return this->key != that;
            }
#endif // !defined(SWIG)
        };

        mutable QMutex _lock;
        mutable QHash< CacheKey, sk_sp<SkTypeface> > _cache;
    protected:
    public:
        CachingFontFinder(const std::shared_ptr<const IFontFinder>& fontFinder);
        virtual ~CachingFontFinder();

        const std::shared_ptr<const IFontFinder> fontFinder;

        virtual sk_sp<SkTypeface> findFontForCharacterUCS4(
            const uint32_t character,
            const SkFontStyle style = SkFontStyle()) const Q_DECL_OVERRIDE;
    };
}

#endif // !defined(_OSMAND_CORE_CACHING_FONT_FINDER_H_)
