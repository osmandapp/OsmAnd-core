#ifndef _OSMAND_CORE_SINGLESKIMAGE_H_
#define _OSMAND_CORE_SINGLESKIMAGE_H_

#include <OsmAndCore/stdlib_common.h>
#include <OsmAndCore/QtExtensions.h>
#include <SkImage.h>

namespace OsmAnd
{
    class SingleSkImage
    {
    
    public:
    
        sk_sp<const SkImage> sp;

        inline SingleSkImage()
        {
            this->sp.reset();
        }

        inline SingleSkImage(std::nullptr_t)
        {
            this->sp.reset();
        }

        inline SingleSkImage(const sk_sp<const SkImage>& that)
        {
            this->sp = that;
        }

        inline SingleSkImage(SingleSkImage& that)
        {
            if (that.sp.get())
                this->sp.reset(that.sp.release());
        }

        inline SingleSkImage(SingleSkImage&& that)
        {
            if (that.sp.get())
                this->sp.reset(that.sp.release());
        }

        inline ~SingleSkImage()
        {
            if (this->sp.get())
                this->sp.reset();
        }

        inline SingleSkImage &operator=(std::nullptr_t)
        {
            if (this->sp.get())
                this->sp.reset();
            return *this;
        }

        inline SingleSkImage &operator=(SingleSkImage& right)
        {
            if (right.sp.get())
                this->sp.reset(right.sp.release());
            return *this;
        }

        inline SingleSkImage &operator=(SingleSkImage&& right)
        {
            if (right.sp.get())
                this->sp.reset(right.sp.release());
            return *this;
        }
#if !defined(SWIG)
        inline bool operator==(const SingleSkImage& right) const
        {
            return this->sp == right.sp;
        }

        inline bool operator!=(const SingleSkImage& right) const
        {
            return this->sp != right.sp;
        }
#endif // !defined(SWIG)
    };
}

#endif // !defined(_OSMAND_CORE_SINGLESKIMAGE_H_)
