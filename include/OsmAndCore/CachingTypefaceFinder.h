#ifndef _OSMAND_CORE_CACHING_FONT_FINDER_H_
#define _OSMAND_CORE_CACHING_FONT_FINDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>
#include <QMutex>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/ITypefaceFinder.h>

namespace OsmAnd
{
    class OSMAND_CORE_API CachingTypefaceFinder Q_DECL_FINAL : public ITypefaceFinder
    {
        Q_DISABLE_COPY_AND_MOVE(CachingTypefaceFinder);

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
        mutable QHash< CacheKey, std::shared_ptr<const Typeface> > _cache;
    protected:
    public:
        CachingTypefaceFinder(const std::shared_ptr<const ITypefaceFinder>& typefaceFinder);
        virtual ~CachingTypefaceFinder();

        const std::shared_ptr<const ITypefaceFinder> typefaceFinder;

        virtual std::shared_ptr<const Typeface> findTypefaceForCharacterUCS4(
            const uint32_t character,
            const SkFontStyle style = SkFontStyle()) const Q_DECL_OVERRIDE;
    };
}

#endif // !defined(_OSMAND_CORE_CACHING_FONT_FINDER_H_)
