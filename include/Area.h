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

#ifndef __AREA_H_
#define __AREA_H_

#include <stdint.h>

#include <OsmAndCore.h>

namespace OsmAnd {

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
            return x == r.x && y == r.y;
        }

        bool operator!= (const PointT& r) const
        {
            return x != r.x || y != r.y;
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
    };

    typedef Point<double> PointD;
    typedef Point<uint32_t> PointI;
    typedef Area<double> AreaD;
    typedef Area<uint32_t> AreaI;

} // namespace OsmAnd

#endif // __AREA_H_
