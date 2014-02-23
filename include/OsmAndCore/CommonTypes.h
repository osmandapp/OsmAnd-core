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
#include <OsmAndCore/Common.h>

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
    typedef Point<double> PointD;
    typedef Point<float> PointF;
    typedef Point<int32_t> PointI;
    typedef Point<int64_t> PointI64;
    inline int crossProductSign(const PointF& a, const PointF& b, const PointF& p)
    {
        return sign((b.y - a.y)*(p.x - a.x) - (b.x - a.x)*(p.y - a.y));
    }
    inline int crossProductSign(const PointI& a, const PointI& b, const PointI& p)
    {
        const int64_t bx_ax = b.x - a.x;
        const int64_t by_ay = b.y - a.y;
        const int64_t px_ax = p.x - a.x;
        const int64_t py_ay = p.y - a.y;
        return sign(by_ay*px_ax - bx_ax*py_ay);
    }
    inline bool testLineLineIntersection(const PointI& a0, const PointI& a1, const PointI& b0, const PointI& b1)
    {
        const auto a1x_a0x = a1.x - a0.x;
        const auto a1y_a0y = a1.y - a0.y;
        const auto b0y_b1y = b0.y - b1.y;
        const auto b0x_b1x = b0.x - b1.x;
        const auto b0x_a0x = b0.x - a0.x;
        const auto b0y_a0y = b0.y - a0.y;

        const auto d_ = static_cast<int64_t>(a1x_a0x*b0y_b1y) - static_cast<int64_t>(b0x_b1x*a1y_a0y);
        const auto t_ = static_cast<int64_t>(b0y_b1y*b0x_a0x) - static_cast<int64_t>(b0x_b1x*b0y_a0y);
        const auto u_ = static_cast<int64_t>(a1x_a0x*b0y_a0y) - static_cast<int64_t>(a1y_a0y*b0x_a0x);

        if(d_ == 0)
        {
            if(t_ == 0 && u_ == 0)
                return true;
            return false;
        }

        const auto t = (static_cast<double>(t_) / static_cast<double>(d_));
        const auto u = (static_cast<double>(u_) / static_cast<double>(d_));

        return
            t >= 0.0 && t <= 1.0 &&
            u >= 0.0 && u <= 1.0;
    }
    inline bool testLineLineIntersection(const PointF& a0, const PointF& a1, const PointF& b0, const PointF& b1)
    {
        const auto a1x_a0x = a1.x - a0.x;
        const auto a1y_a0y = a1.y - a0.y;
        const auto b0y_b1y = b0.y - b1.y;
        const auto b0x_b1x = b0.x - b1.x;
        const auto b0x_a0x = b0.x - a0.x;
        const auto b0y_a0y = b0.y - a0.y;

        const auto d_ = a1x_a0x*b0y_b1y - b0x_b1x*a1y_a0y;
        const auto t_ = b0y_b1y*b0x_a0x - b0x_b1x*b0y_a0y;
        const auto u_ = a1x_a0x*b0y_a0y - a1y_a0y*b0x_a0x;

        if(qFuzzyIsNull(d_))
        {
            if(qFuzzyIsNull(t_) && qFuzzyIsNull(u_))
                return true;
            return false;
        }

        const auto t = (t_ / d_);
        const auto u = (u_ / d_);

        return
            t >= 0.0f && t <= 1.0f &&
            u >= 0.0f && u <= 1.0f;
    }

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
            return that.contains(*this);
        }

        inline bool intersects(const OOBBT& that) const
        {
            return that.intersects(*this);
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

        inline AreaT& enlargeBy(const T& dt, const T& dl, const T& db, const T& dr)
        {
            top -= dt;
            left -= dl;
            bottom += db;
            right += dr;

            return *this;
        }

        inline AreaT enlargeBy(const T& dt, const T& dl, const T& db, const T& dr) const
        {
            return getEnlargedBy(dt, dl, db, dr);
        }

        inline AreaT getEnlargedBy(const T& dt, const T& dl, const T& db, const T& dr) const
        {
            return AreaT(
                top - dt,
                left - dl,
                bottom + db,
                right + dr);
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
    typedef Area<double> AreaD;
    typedef Area<float> AreaF;
    typedef Area<int32_t> AreaI;
    typedef Area<int64_t> AreaI64;

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
        PointT _pointInGlobalSpace0;
        PointT _pointInGlobalSpace1;
        PointT _pointInGlobalSpace2;
        PointT _pointInGlobalSpace3;

        inline bool isPointInside(const PointT& p) const
        {
            const auto& p0 = pointInGlobalSpace0;
            const auto& p1 = pointInGlobalSpace1;
            const auto& p2 = pointInGlobalSpace2;
            const auto& p3 = pointInGlobalSpace3;

            // Check if point 'p' is on the same 'side' of each edge
            const auto sign0 = crossProductSign(p0, p1, p);
            const auto sign1 = crossProductSign(p1, p2, p);
            const auto sign2 = crossProductSign(p2, p3, p);
            const auto sign3 = crossProductSign(p3, p0, p);
            int sign = sign0;
            if(sign1 != 0)
            {
                if(sign != 0 && sign != sign1)
                    return false;
                sign = sign1;
            }
            if(sign2 != 0)
            {
                if(sign != 0 && sign != sign2)
                    return false;
                sign = sign2;
            }
            if(sign3 != 0)
            {
                if(sign != 0 && sign != sign3)
                    return false;
            }
            return true;
        }

        void updateDerivedData()
        {
            //TODO:!!!!!!
        }
    public:
        inline OOBB()
            : bboxInObjectSpace(_bboxInObjectSpace)
            , rotation(_rotation)
            , aabb(_aabb)
            , pointInGlobalSpace0(_pointInGlobalSpace0)
            , pointInGlobalSpace1(_pointInGlobalSpace1)
            , pointInGlobalSpace2(_pointInGlobalSpace2)
            , pointInGlobalSpace3(_pointInGlobalSpace3)
        {
        }

        inline OOBB(const AreaT& bboxInObjectSpace_, const float rotation_)
            : OOBB()
        {
            this->_bboxInObjectSpace = bboxInObjectSpace_;
            this->_rotation = rotation_;
            updateDerivedData();
        }

        template<typename T_>
        inline OOBB(const OOBB<T_>& that)
            : OOBB()
        {
            this->_bboxInObjectSpace = that._bboxInObjectSpace;
            this->_rotation = that._rotation;
            this->_aabb = that._aabb;
            this->_pointInGlobalSpace0 = that._pointInGlobalSpace0;
            this->_pointInGlobalSpace1 = that._pointInGlobalSpace1;
            this->_pointInGlobalSpace2 = that._pointInGlobalSpace2;
            this->_pointInGlobalSpace3 = that._pointInGlobalSpace3;
        }

        const AreaT& bboxInObjectSpace;
        const float& rotation;
        const AreaT& aabb;
        const PointT& pointInGlobalSpace0;
        const PointT& pointInGlobalSpace1;
        const PointT& pointInGlobalSpace2;
        const PointT& pointInGlobalSpace3;

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
                this->_pointInGlobalSpace0 = that._pointInGlobalSpace0;
                this->_pointInGlobalSpace1 = that._pointInGlobalSpace1;
                this->_pointInGlobalSpace2 = that._pointInGlobalSpace2;
                this->_pointInGlobalSpace3 = that._pointInGlobalSpace3;
            }
            return *this;
        }
#endif // !defined(SWIG)

        inline bool contains(const AreaT& that) const
        {
            // If external AABB doesn't contain that AABB, surely inner doesn't contain also
            if(!aabb.contains(that))
                return false;

            // If angle of rotation is zero, check OOBB vs that AABB
            if(qFuzzyIsNull(rotation))
                return bboxInObjectSpace.contains(that);

            // If all of points of that AABB are inside, then it's totally inside
            const auto a0 = that.topLeft;
            const auto a1 = that.topRight();
            const auto a2 = that.bottomRight;
            const auto a3 = that.bottomLeft();
            return isPointInside(a0) && isPointInside(a1) && isPointInside(a2) && isPointInside(a3);
        }

        inline bool intersects(const AreaT& that) const
        {
            // If external AABB doesn't intersect that AABB, surely inner doesn't intersect also
            if(!aabb.intersects(that))
                return false;

            // If angle of rotation is zero, check OOBB vs that AABB
            if(qFuzzyIsNull(rotation))
                return bboxInObjectSpace.intersects(that);

            const auto& p0 = pointInGlobalSpace0;
            const auto& p1 = pointInGlobalSpace1;
            const auto& p2 = pointInGlobalSpace2;
            const auto& p3 = pointInGlobalSpace3;
            const auto a0 = that.topLeft;
            const auto a1 = that.topRight();
            const auto a2 = that.bottomRight;
            const auto a3 = that.bottomLeft();

            // Check if any points of that area is inside.
            // This case covers inner area and partially inner area (that has at least 1 point inside)
            if(isPointInside(a0) || isPointInside(a1) || isPointInside(a2) || isPointInside(a3))
                return true;

            // Check if any that area edge intersects any of OOBB edges.
            // This case covers intersecting area that has no vertex inside OOBB.
            return
                testLineLineIntersection(a0, a1, p0, p1) ||
                testLineLineIntersection(a0, a1, p1, p2) ||
                testLineLineIntersection(a0, a1, p2, p3) ||
                testLineLineIntersection(a0, a1, p3, p0) ||
                testLineLineIntersection(a1, a2, p0, p1) ||
                testLineLineIntersection(a1, a2, p1, p2) ||
                testLineLineIntersection(a1, a2, p2, p3) ||
                testLineLineIntersection(a1, a2, p3, p0) ||
                testLineLineIntersection(a2, a3, p0, p1) ||
                testLineLineIntersection(a2, a3, p1, p2) ||
                testLineLineIntersection(a2, a3, p2, p3) ||
                testLineLineIntersection(a2, a3, p3, p0) ||
                testLineLineIntersection(a3, a0, p0, p1) ||
                testLineLineIntersection(a3, a0, p1, p2) ||
                testLineLineIntersection(a3, a0, p2, p3) ||
                testLineLineIntersection(a3, a0, p3, p0);
        }

        inline OOBBT& enlargeBy(const PointT& delta)
        {
            _bboxInObjectSpace.top -= delta.y;
            _bboxInObjectSpace.left -= delta.x;
            _bboxInObjectSpace.bottom += delta.y;
            _bboxInObjectSpace.right += delta.x;
            updateDerivedData();

            return *this;
        }

        inline OOBBT enlargeBy(const PointT& delta) const
        {
            return getEnlargedBy(delta);
        }

        inline OOBBT getEnlargedBy(const PointT& delta) const
        {
            return OOBBT(
                AreaT(
                    top - delta.y,
                    left - delta.x,
                    bottom + delta.y,
                    right + delta.x),
                _rotation);
        }

        inline OOBBT& enlargeBy(const T& dt, const T& dl, const T& db, const T& dr)
        {
            _bboxInObjectSpace.top -= dt;
            _bboxInObjectSpace.left -= dl;
            _bboxInObjectSpace.bottom += db;
            _bboxInObjectSpace.right += dr;
            updateDerivedData();

            return *this;
        }

        inline OOBBT enlargeBy(const T& dt, const T& dl, const T& db, const T& dr) const
        {
            return getEnlargedBy(dt, dl, db, dr);
        }

        inline OOBBT getEnlargedBy(const T& dt, const T& dl, const T& db, const T& dr) const
        {
            return OOBBT(
                AreaT(
                    top - dt,
                    left - dl,
                    bottom + db,
                    right + dr),
                _rotation);
        }
    };
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
