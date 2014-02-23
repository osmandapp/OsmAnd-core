#ifndef _OSMAND_CORE_COMMON_TYPES_H_
#define _OSMAND_CORE_COMMON_TYPES_H_

#include <OsmAndCore/stdlib_common.h>
#include <type_traits>

#include <QtGlobal>
#include <QString>

#if defined(OSMAND_GLM_AVAILABLE)
#   include <glm/glm.hpp>
#endif // defined(OSMAND_GLM_AVAILABLE)

#include <OsmAndCore.h>

namespace OsmAnd
{
    template<typename T>
    struct Area;

    template<typename T>
    struct Point
    {
        typedef T CoordType;
        typedef Point<T> PointT;
        typedef typename std::conditional<std::is_integral<T>::value, int64_t, double>::type LargerSignedCoordType;
        typedef typename std::conditional<std::is_integral<T>::value, uint64_t, double>::type LargerUnsignedCoordType;

        T x, y;

        inline Point()
        {
            this->x = 0;
            this->y = 0;
        }

        template<typename T_>
        inline Point(const Point<T_>& that)
        {
            this->x = that.x;
            this->y = that.y;
        }

        inline Point(const T& x, const T& y)
        {
            this->x = x;
            this->y = y;
        }

#if !defined(SWIG)
        inline PointT operator+(const PointT& r) const
        {
            return PointT(x + r.x, y + r.y);
        }

        inline PointT& operator+=(const PointT& r)
        {
            x += r.x;
            y += r.y;
            return *this;
        }

        inline PointT operator-(const PointT& r) const
        {
            return PointT(x - r.x, y - r.y);
        }

        inline PointT& operator-=(const PointT& r)
        {
            x -= r.x;
            y -= r.y;
            return *this;
        }

        inline bool operator==(const PointT& r) const
        {
            return equal(x, r.x) && equal(y, r.y);
        }

        inline bool operator!=(const PointT& r) const
        {
            return !equal(x, r.x) || !equal(y, r.y);
        }

        inline PointT& operator=(const PointT& that)
        {
            if(this != &that)
            {
                x = that.x;
                y = that.y;
            }
            return *this;
        }

        template<typename T_>
        inline PointT operator*(const T_& r) const
        {
            return PointT(x * r, y * r);
        }

        template<typename T_>
        inline PointT& operator*=(const T_& r)
        {
            x *= r;
            y *= r;
            return *this;
        }

        template<typename T_>
        inline PointT operator/(const T_& r) const
        {
            return PointT(x / r, y / r);
        }

        template<typename T_>
        inline PointT& operator/=(const T_& r)
        {
            x /= r;
            y /= r;
            return *this;
        }
#endif // !defined(SWIG)

        inline LargerUnsignedCoordType squareNorm() const
        {
            return
                static_cast<LargerUnsignedCoordType>(static_cast<LargerSignedCoordType>(x)*static_cast<LargerSignedCoordType>(x)) +
                static_cast<LargerUnsignedCoordType>(static_cast<LargerSignedCoordType>(y)*static_cast<LargerSignedCoordType>(y));
        }

        inline T norm() const
        {
            return static_cast<T>(qSqrt(squareNorm()));
        }

#if defined(OSMAND_GLM_AVAILABLE)
        inline operator glm::detail::tvec2<T>() const
        {
            return glm::detail::tvec2<T>(x, y);
        }
#endif // defined(OSMAND_GLM_AVAILABLE)
    private:
        static inline bool equal(const double& a, const double& b)
        {
            return qFuzzyCompare(a, b);
        }

        static inline bool equal(const float a, const float& b)
        {
            return qFuzzyCompare(a, b);
        }

        static inline bool equal(const uint32_t& a, const uint32_t& b)
        {
            return a == b;
        }

        static inline bool equal(const int32_t& a, const int32_t& b)
        {
            return a == b;
        }

    friend struct OsmAnd::Area<T>;
    };

    template<typename T>
    class OOBB;

    template<typename T>
    struct Area
    {
        typedef T CoordType;
        typedef Point<T> PointT;
        typedef Area<T> AreaT;
        typedef OOBB<T> OOBBT;

        inline Area()
            : top(topLeft.y)
            , left(topLeft.x)
            , bottom(bottomRight.y)
            , right(bottomRight.x)
        {
            this->top = 0;
            this->left = 0;
            this->bottom = 0;
            this->right = 0;
        }

        inline Area(const T& t, const T& l, const T& b, const T& r)
            : top(topLeft.y)
            , left(topLeft.x)
            , bottom(bottomRight.y)
            , right(bottomRight.x)
        {
            this->top = t;
            this->left = l;
            this->bottom = b;
            this->right = r;
        }

        inline Area(const PointT& tl, const PointT& br)
            : top(topLeft.y)
            , left(topLeft.x)
            , bottom(bottomRight.y)
            , right(bottomRight.x)
        {
            this->topLeft = tl;
            this->bottomRight = br;
        }

        inline Area(const AreaT& that)
            : top(topLeft.y)
            , left(topLeft.x)
            , bottom(bottomRight.y)
            , right(bottomRight.x)
        {
            this->topLeft = that.topLeft;
            this->bottomRight = that.bottomRight;
        }

        template<typename T_>
        inline Area(const Area<T_>& that)
            : top(topLeft.y)
            , left(topLeft.x)
            , bottom(bottomRight.y)
            , right(bottomRight.x)
        {
            this->topLeft = that.topLeft;
            this->bottomRight = that.bottomRight;
        }

        PointT topLeft, bottomRight;

        T& top;
        T& left;
        T& bottom;
        T& right;

#if !defined(SWIG)
        inline AreaT& operator=(const AreaT& that)
        {
            if(this != &that)
            {
                topLeft = that.topLeft;
                bottomRight = that.bottomRight;
            }
            return *this;
        }

        inline bool operator==(const AreaT& r) const
        {
            return topLeft == r.topLeft && bottomRight == r.bottomRight;
        }

        inline bool operator!=(const AreaT& r) const
        {
            return topLeft != r.topLeft || bottomRight != r.bottomRight;
        }
#endif // !defined(SWIG)

        inline bool contains(const T& x, const T& y) const
        {
            return !(left > x || right < x || top > y || bottom < y);
        }

        inline bool contains(const PointT& p) const
        {
            return !(left > p.x || right < p.x || top > p.y || bottom < p.y);
        }

        inline bool contains(const T& t, const T& l, const T& b, const T& r) const
        {
            return
                l >= left &&
                r <= right &&
                t >= top &&
                b <= bottom;
        }

        inline bool contains(const AreaT& that) const
        {
            return
                that.left >= this->left &&
                that.right <= this->right &&
                that.top >= this->top &&
                that.bottom <= this->bottom;
        }

        template<typename T_>
        inline bool contains(const T_& x, const T_& y) const
        {
            return !(left > x || right < x || top > y || bottom < y);
        }

        template<typename T_>
        inline bool contains(const Point<T_>& p) const
        {
            return !(left > p.x || right < p.x || top > p.y || bottom < p.y);
        }

        template<typename T_>
        inline bool contains(const T_& t, const T_& l, const T_& b, const T_& r) const
        {
            return
                l >= left &&
                r <= right &&
                t >= top &&
                b <= bottom;
        }

        template<typename T_>
        inline bool contains(const Area<T_>& that) const
        {
            return
                that.left >= this->left &&
                that.right <= this->right &&
                that.top >= this->top &&
                that.bottom <= this->bottom;
        }

        inline bool intersects(const T& t, const T& l, const T& b, const T& r) const
        {
            return !(
                l > this->right ||
                r < this->left ||
                t > this->bottom ||
                b < this->top);
        }

        inline bool intersects(const AreaT& that) const
        {
            return !(
                that.left > this->right ||
                that.right < this->left ||
                that.top > this->bottom ||
                that.bottom < this->top);
        }

        template<typename T_>
        inline bool intersects(const T_& t, const T_& l, const T_& b, const T_& r) const
        {
            return !(
                l > this->right ||
                r < this->left ||
                t > this->bottom ||
                b < this->top);
        }

        template<typename T_>
        inline bool intersects(const Area<T_>& that) const
        {
            return !(
                that.left > this->right ||
                that.right < this->left ||
                that.top > this->bottom ||
                that.bottom < this->top);
        }

        inline bool contains(const OOBBT& that) const
        {
            return false;
        }

        inline bool intersects(const OOBBT& that) const
        {
            return false;
        }

        inline T width() const
        {
            return right - left;
        }

        inline T height() const
        {
            return bottom - top;
        }

        inline PointT center() const
        {
            return PointT(left + width() / 2, top + height() / 2);
        }

        inline PointT topRight() const
        {
            return PointT(right, top);
        }

        inline PointT bottomLeft() const
        {
            return PointT(left, bottom);
        }

        STRONG_ENUM(Edge)
        {
            Invalid = -1,

            Left = 0,
            Top = 1,
            Right = 2,
            Bottom = 3
        } STRONG_ENUM_TERMINATOR;

        bool isOnEdge(const PointT& p, Edge* edge = nullptr) const
        {
            bool res = false;

            if(!res && PointT::equal(p.x, left))
            {
                res = true;
                if(edge)
                    *edge = Edge::Left;
            }

            if(!res && PointT::equal(p.x, right))
            {
                res = true;
                if(edge)
                    *edge = Edge::Right;
            }

            if(!res && PointT::equal(p.y, top))
            {
                res = true;
                if(edge)
                    *edge = Edge::Top;
            }

            if(!res && PointT::equal(p.y, bottom))
            {
                res = true;
                if(edge)
                    *edge = Edge::Bottom;
            }

            return res;
        }

        STRONG_ENUM(Quadrant)
        {
            NE = 0,
            SE,
            SW,
            NW
        } STRONG_ENUM_TERMINATOR;
        inline AreaT getQuadrant(const Quadrant quadrant) const
        {
            const auto center_ = center();

            switch(quadrant)
            {
            case Quadrant::NE:
                return AreaT(top, center_.x, center_.y, right);
            case Quadrant::SE:
                return AreaT(center_.y, center_.x, bottom, right);
            case Quadrant::SW:
                return AreaT(center_.y, left, bottom, center_.x);
            case Quadrant::NW:
                return AreaT(top, left, center_.y, center_.x);
            }

            return *this;
        }

        inline AreaT& enlargeToInclude(const PointT& p)
        {
            top = std::min(top, p.y);
            left = std::min(left, p.x);
            bottom = std::max(bottom, p.y);
            right = std::max(right, p.x);

            return *this;
        }

        inline AreaT enlargeToInclude(const PointT& p) const
        {
            return getEnlargedToInclude(p);
        }

        inline AreaT getEnlargedToInclude(const PointT& p) const
        {
            return AreaT(
                std::min(top, p.y),
                std::min(left, p.x),
                std::max(bottom, p.y),
                std::max(right, p.x));
        }

        inline AreaT& enlargeBy(const PointT& delta)
        {
            top -= delta.y;
            left -= delta.x;
            bottom += delta.y;
            right += delta.x;

            return *this;
        }

        inline AreaT enlargeBy(const PointT& delta) const
        {
            return getEnlargedBy(delta);
        }

        inline AreaT getEnlargedBy(const PointT& delta) const
        {
            return AreaT(
                top - delta.y,
                left - delta.x,
                bottom + delta.y,
                right + delta.x);
        }

        static AreaT largest()
        {
            AreaT res;
            res.top = res.left = std::numeric_limits<T>::min();
            res.bottom = res.right = std::numeric_limits<T>::max();
            return res;
        }

        static AreaT fromCenterAndSize(const T& cx, const T& cy, const T& width, const T& height)
        {
            const T halfWidth = width / static_cast<T>(2);
            const T halfHeight = height / static_cast<T>(2);
            return AreaT(cy - halfHeight, cx - halfWidth, cy + halfHeight, cx + halfWidth);
        }
    };

    template<typename T>
    class OOBB
    {
    public:
        typedef Area<T> AreaT;
        typedef Point<T> PointT;
        typedef OOBB<T> OOBBT;

    private:
    protected:
        AreaT _bboxInObjectSpace;
        float _rotation;
        AreaT _aabb;
    public:
        inline OOBB()
            : bboxInObjectSpace(_bboxInObjectSpace)
            , rotation(_rotation)
            , aabb(_aabb)
        {
        }

        inline OOBB(const AreaT& bboxInObjectSpace_, const float rotation_, const AreaT& aabb_)
            : OOBB()
        {
            this->_bboxInObjectSpace = bboxInObjectSpace_;
            this->_rotation = rotation_;
            this->_aabb = aabb_;
        }

        template<typename T_>
        inline OOBB(const OOBB<T_>& that)
            : OOBB()
        {
            this->_bboxInObjectSpace = that._bboxInObjectSpace;
            this->_rotation = that._rotation;
            this->_aabb = that._aabb;
        }

        const AreaT& bboxInObjectSpace;
        const float& rotation;
        const AreaT& aabb;

#if !defined(SWIG)
        inline bool operator==(const OOBBT& r) const
        {
            return (_bboxInObjectSpace == r._b) && qFuzzyCompare(_rotation, r._rotation);
        }

        inline bool operator!=(const OOBBT& r) const
        {
            return (_bboxInObjectSpace != r._b) || !qFuzzyCompare(_rotation, r._rotation);
        }

        inline OOBBT& operator=(const OOBBT& that)
        {
            if(this != &that)
            {
                this->_bboxInObjectSpace = that._bboxInObjectSpace;
                this->_rotation = that._rotation;
                this->_aabb = that._aabb;
            }
            return *this;
        }
#endif // !defined(SWIG)

        inline bool contains(const AreaT& that) const
        {
            return false;
        }

        inline bool intersects(const AreaT& that) const
        {
            return false;
        }
    };

    typedef Point<double> PointD;
    typedef Point<float> PointF;
    typedef Point<int32_t> PointI;
    typedef Point<int64_t> PointI64;
    typedef Area<double> AreaD;
    typedef Area<float> AreaF;
    typedef Area<int32_t> AreaI;
    typedef Area<int64_t> AreaI64;
    typedef OOBB<double> OOBBD;
    typedef OOBB<float> OOBBF;
    typedef OOBB<int32_t> OOBBI;
    typedef OOBB<int64_t> OOBBI64;

    union TileId
    {
        uint64_t id;
        struct {
            int32_t x;
            int32_t y;
        };

#if !defined(SWIG)
        inline operator uint64_t() const
        {
            return id;
        }

        inline TileId& operator=( const uint64_t& that )
        {
            id = that;
            return *this;
        }

        inline bool operator==( const TileId& that )
        {
            return this->id == that.id;
        }

        inline bool operator!=( const TileId& that )
        {
            return this->id != that.id;
        }

        inline bool operator==( const uint64_t& that )
        {
            return this->id == that;
        }

        inline bool operator!=( const uint64_t& that )
        {
            return this->id != that;
        }
#endif // !defined(SWIG)
    };

#if !defined(SWIG)
    static_assert(sizeof(TileId) == 8, "TileId must be 8 bytes in size");
#endif // !defined(SWIG)

    WEAK_ENUM_EX(ZoomLevel, int32_t)
    {
        ZoomLevel0 = 0,
        ZoomLevel1,
        ZoomLevel2,
        ZoomLevel3,
        ZoomLevel4,
        ZoomLevel5,
        ZoomLevel6,
        ZoomLevel7,
        ZoomLevel8,
        ZoomLevel9,
        ZoomLevel10,
        ZoomLevel11,
        ZoomLevel12,
        ZoomLevel13,
        ZoomLevel14,
        ZoomLevel15,
        ZoomLevel16,
        ZoomLevel17,
        ZoomLevel18,
        ZoomLevel19,
        ZoomLevel20,
        ZoomLevel21,
        ZoomLevel22,
        ZoomLevel23,
        ZoomLevel24,
        ZoomLevel25,
        ZoomLevel26,
        ZoomLevel27,
        ZoomLevel28,
        ZoomLevel29,
        ZoomLevel30,
        ZoomLevel31 = 31,

        InvalidZoom = -1,
        MinZoomLevel = ZoomLevel0,
        MaxZoomLevel = ZoomLevel31,
    };
    enum {
        ZoomLevelsCount = static_cast<unsigned>(ZoomLevel::MaxZoomLevel) + 1u
    };

    union FColorRGB
    {
        inline FColorRGB()
            : r(1.0f)
            , g(1.0f)
            , b(1.0f)
        {
        }

        inline FColorRGB(const float& r_, const float& g_, const float& b_)
            : r(r_)
            , g(g_)
            , b(b_)
        {
        }

        float value[3];
        struct {
            float r;
            float g;
            float b;
        };

#if !defined(SWIG)
        inline bool operator== (const FColorRGB& other) const
        {
            return qFuzzyCompare(r, other.r) && qFuzzyCompare(g, other.g) && qFuzzyCompare(b, other.b);
        }

        inline bool operator!= (const FColorRGB& other) const
        {
            return !qFuzzyCompare(r, other.r) || !qFuzzyCompare(g, other.g) || !qFuzzyCompare(b, other.b);
        }
#endif // !defined(SWIG)
    };

    STRONG_ENUM_EX(LanguageId, int32_t)
    {
        Invariant = -1,

        Latin,
        Native
    } STRONG_ENUM_TERMINATOR;
}

#endif // !defined(_OSMAND_CORE_COMMON_TYPES_H_)
