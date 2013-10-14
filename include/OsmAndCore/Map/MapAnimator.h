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
#ifndef __MAP_ANIMATOR_H_
#define __MAP_ANIMATOR_H_

#include <cstdint>
#include <memory>
#include <functional>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd {

    class IMapRenderer;

    enum MapAnimatorEasingType
    {
        None = -1,

        Linear = 0,
        Quadratic,
        Cubic,
        Quartic,
        Quintic,
        Sinusoidal,
        Exponential,
        Circular
    };

    class MapAnimator_P;
    class OSMAND_CORE_API MapAnimator
    {
    private:
        const std::unique_ptr<MapAnimator_P> _d;
    protected:
    public:
        MapAnimator();
        virtual ~MapAnimator();

        void setMapRenderer(const std::shared_ptr<IMapRenderer>& mapRenderer);
        const std::shared_ptr<IMapRenderer>& mapRenderer;

        bool isAnimationPaused() const;
        bool isAnimationRunning() const;

        void pauseAnimation();
        void resumeAnimation();
        void cancelAnimation();

        void update(const float timePassed);

        void animateZoomBy(const float deltaValue, const float duration, MapAnimatorEasingType easingIn = MapAnimatorEasingType::Quadratic, MapAnimatorEasingType easingOut = MapAnimatorEasingType::Quadratic);
        void animateZoomWith(const float velocity, const float deceleration);

        void animateTargetBy(const PointI& deltaValue, const float duration, MapAnimatorEasingType easingIn = MapAnimatorEasingType::Quadratic, MapAnimatorEasingType easingOut = MapAnimatorEasingType::Quadratic);
        void animateTargetBy(const PointI64& deltaValue, const float duration, MapAnimatorEasingType easingIn = MapAnimatorEasingType::Quadratic, MapAnimatorEasingType easingOut = MapAnimatorEasingType::Quadratic);
        void animateTargetWith(const PointD& velocity, const PointD& deceleration);

        void animateAzimuthBy(const float deltaValue, const float duration, MapAnimatorEasingType easingIn = MapAnimatorEasingType::Quadratic, MapAnimatorEasingType easingOut = MapAnimatorEasingType::Quadratic);
        void animateAzimuthWith(const float velocity, const float deceleration);

        void animateElevationAngleBy(const float deltaValue, const float duration, MapAnimatorEasingType easingIn = MapAnimatorEasingType::Quadratic, MapAnimatorEasingType easingOut = MapAnimatorEasingType::Quadratic);
        void animateElevationAngleWith(const float velocity, const float deceleration);
    };

}

#endif // __MAP_ANIMATOR_H_
