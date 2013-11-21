/**
* @file
*
* @section LICENSE
*
* OsmAnd - Android navigation software based on OSM maps.
* Copyright (C) 2010-2013  OsmAnd Authors listed in AUTHORS file
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _OSMAND_CORE_COMMON_TYPES_H_
#define _OSMAND_CORE_COMMON_TYPES_H_

#include <cstdint>
#include <memory>

#include <QtGlobal>
#include <QString>

#include <OsmAndCore.h>

namespace OsmAnd
{

    template<typename T>
    struct Area;

    template<typename T>
    struct Point
    {
        typedef Point<T> PointT;

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

        inline PointT operator+ (const PointT& r) const
        {
            return PointT(x + r.x, y + r.y);
        }

        inline PointT& operator+= (const PointT& r)
        {
            x += r.x;
            y += r.y;
            return *this;
        }

        inline PointT operator- (const PointT& r) const
        {
            return PointT(x - r.x, y - r.y);
        }

        inline PointT& operator-= (const PointT& r)
        {
            x -= r.x;
            y -= r.y;
            return *this;
        }

        inline bool operator== (const PointT& r) const
        {
            return equal(x, r.x) && equal(y, r.y);
        }

        inline bool operator!= (const PointT& r) const
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

    private:
        static inline bool equal(const double a, const double b)
        {
            return qFuzzyCompare(a, b);
        }

        static inline bool equal(const float a, const float b)
        {
            return qFuzzyCompare(a, b);
        }

        static inline bool equal(const uint32_t a, const uint32_t b)
        {
            return a == b;
        }

        static inline bool equal(const int32_t a, const int32_t b)
        {
            return a == b;
        }

    friend struct OsmAnd::Area<T>;
    };

    template<typename T>
    struct Area
    {
        typedef Point<T> PointT;
        typedef Area<T> AreaT;

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

        inline AreaT& operator=(const AreaT& that)
        {
            if(this != &that)
            {
                topLeft = that.topLeft;
                bottomRight = that.bottomRight;
            }
            return *this;
        }

        inline bool operator== (const AreaT& r) const
        {
            return topLeft == r.topLeft && bottomRight == r.bottomRight;
        }

        inline bool operator!= (const AreaT& r) const
        {
            return topLeft != r.topLeft || bottomRight != r.bottomRight;
        }

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

        inline T width() const
        {
            return left > right ? left - right : right - left;
        }

        inline T height() const
        {
            return top > bottom ? top - bottom : bottom - top;
        }

        inline Point<T> center() const
        {
            return Point<T>(left + width() / 2, bottom + height() / 2);
        }

#ifndef SWIG
        STRONG_ENUM(Edge)
        {
            Invalid = -1,

            Left = 0,
            Top = 1,
            Right = 2,
            Bottom = 3
        };

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
#endif // !SWIG

        inline AreaT& enlargeToInclude(const PointT& p)
        {
            top = std::min(top, p.y);
            left = std::min(left, p.x);
            bottom = std::max(bottom, p.y);
            right = std::max(right, p.x);

            return *this;
        }

        static AreaT largest()
        {
            AreaT res;
            res.top = res.left = std::numeric_limits<T>::min();
            res.bottom = res.right = std::numeric_limits<T>::max();
            return res;
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

    union TileId
    {
        uint64_t id;
        struct
        {
            int32_t x;
            int32_t y;
        };

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
    };

#ifndef SWIG
    static_assert(sizeof(TileId) == 8, "TileId must be 8 bytes in size");
#endif

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
        {}

        inline FColorRGB(const float r_, const float g_, const float b_)
            : r(r_)
            , g(g_)
            , b(b_)
        {}

        float value[3];
        struct
        {
            float r;
            float g;
            float b;
        };

        inline bool operator== (const FColorRGB& other) const
        {
            return qFuzzyCompare(r, other.r) && qFuzzyCompare(g, other.g) && qFuzzyCompare(b, other.b);
        }

        inline bool operator!= (const FColorRGB& other) const
        {
            return !qFuzzyCompare(r, other.r) || !qFuzzyCompare(g, other.g) || !qFuzzyCompare(b, other.b);
        }
    };

}

#endif // _OSMAND_CORE_COMMON_TYPES_H_
