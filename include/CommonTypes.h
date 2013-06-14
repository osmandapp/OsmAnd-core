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

#if !defined(__COMMON_TYPES_H_)
#define __COMMON_TYPES_H_

#include <stdint.h>

#include <QtGlobal>
#include <QString>
#include <QList>

#include <OsmAndCore.h>

namespace OsmAnd
{
    struct TagValue
    {
        TagValue()
        {
        }

        TagValue(const QString& tag_, const QString& value_)
            : tag(tag_)
            , value(value_)
        {
        }

        TagValue(const char* tag_, const char* value_)
            : tag(QString::fromLatin1(tag_))
            , value(QString::fromLatin1(value_))
        {
        }

        QString tag;
        QString value;
    };

    template<typename T>
    struct Point
    {
        typedef Point<T> PointT;

        T x, y;

        Point()
        {
            this->x = 0;
            this->y = 0;
        }

        Point(const T& x, const T& y)
        {
            this->x = x;
            this->y = y;
        }

        PointT operator+ (const PointT& r) const
        {
            return PointT(x + r.x, y + r.y);
        }

        PointT& operator+= (const PointT& r)
        {
            x += r.x;
            y += r.y;
            return *this;
        }

        PointT operator- (const PointT& r) const
        {
            return PointT(x - r.x, y - r.y);
        }

        PointT& operator-= (const PointT& r)
        {
            x -= r.x;
            y -= r.y;
            return *this;
        }

        bool operator== (const PointT& r) const
        {
            return equal(x, r.x) && equal(y, r.y);
        }

        bool operator!= (const PointT& r) const
        {
            return !equal(x, r.x) || !equal(y, r.y);
        }

        PointT& operator=(const PointT& that)
        {
            if(this != &that)
            {
                x = that.x;
                y = that.y;
            }
            return *this;
        }

    private:
        static inline bool equal(const double& a, const double& b)
        {
            return qFuzzyCompare(a, b);
        }

        static inline bool equal(const float& a, const float& b)
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
    };

    template<typename T>
    struct Area
    {
        typedef Point<T> PointT;
        typedef Area<T> AreaT;

        Area()
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

        Area(const T& t, const T& l, const T& b, const T& r)
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

        Area(const PointT& tl, const PointT& br)
            : top(topLeft.y)
            , left(topLeft.x)
            , bottom(bottomRight.y)
            , right(bottomRight.x)
        {
            this->topLeft = tl;
            this->bottomRight = br;
        }

        Area(const AreaT& that)
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

        AreaT& operator=(const AreaT& that)
        {
            if(this != &that)
            {
                topLeft = that.topLeft;
                bottomRight = that.bottomRight;
            }
            return *this;
        }

        bool operator== (const AreaT& r) const
        {
            return topLeft == r.topLeft && bottomRight == r.bottomRight;
        }

        bool operator!= (const AreaT& r) const
        {
            return topLeft != r.topLeft || bottomRight != r.bottomRight;
        }

        bool contains(const T& x, const T& y) const
        {
            return !(left > x || right < x || top > y || bottom < y);
        }

        bool contains(const PointT& p) const
        {
            return !(left > p.x || right < p.x || top > p.y || bottom < p.y);
        }

        bool contains(const T& t, const T& l, const T& b, const T& r) const
        {
            return !(left > l || right < r || top > t || bottom < b);
        }

        bool contains(const AreaT& a) const
        {
            return !(left > a.left || right < a.right || top > a.top|| bottom < a.bottom);
        }

        bool intersects(const T& t, const T& l, const T& b, const T& r) const
        {
            return !(r < left || l > right || t > bottom || b < top);
        }

        bool intersects(const AreaT& a) const
        {
            return !(a.right < left || a.left > right || a.top > bottom || a.bottom < top);
        }

        T width() const
        {
            return left > right ? left - right : right - left;
        }

        T height() const
        {
            return top > bottom ? top - bottom : bottom - top;
        }

        Point<T> center() const
        {
            return Point<T>(left + width() / 2, bottom + height() / 2);
        }
    };

    typedef Point<double> PointD;
    typedef Point<float> PointF;
    typedef Point<int32_t> PointI;
    typedef Area<double> AreaD;
    typedef Area<float> AreaF;
    typedef Area<int32_t> AreaI;
}

#endif // __COMMON_TYPES_H_
