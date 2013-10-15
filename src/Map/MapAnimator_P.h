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
#ifndef __MAP_ANIMATOR_P_H_
#define __MAP_ANIMATOR_P_H_

#include <cstdint>
#include <memory>
#include <cassert>
#include <functional>

#include <QList>
#include <QMutex>

#include <OsmAndCore.h>
#include <MapAnimator.h>

namespace OsmAnd {

    class IMapRenderer;

    class MapAnimator;
    class MapAnimator_P
    {
    private:
    protected:
        MapAnimator_P(MapAnimator* const owner);

        MapAnimator* const owner;
        std::shared_ptr<IMapRenderer> _renderer;

        class AbstractAnimation
        {
        private:
        protected:
            float _timePassed;
        
            AbstractAnimation(float duration_, MapAnimatorEasingType easingIn_, MapAnimatorEasingType easingOut_)
                : _timePassed(0.0f)
                , duration(duration_)
                , easingIn(easingIn_)
                , easingOut(easingOut_)
            {}

            template <typename T>
            static void calculateValue(const float t, const T initial, const T delta, const float duration, MapAnimatorEasingType easingIn, MapAnimatorEasingType easingOut, T& value)
            {
                if(easingIn == MapAnimatorEasingType::None)
                {
                    switch (easingOut)
                    {
                    case MapAnimatorEasingType::Linear:
                        value = linearTween(t, initial, delta, duration);
                        break;
                    case MapAnimatorEasingType::Quadratic:
                        value = easeOut_Quadratic(t, initial, delta, duration);
                        break;
                    case MapAnimatorEasingType::Cubic:
                        value = easeOut_Cubic(t, initial, delta, duration);
                        break;
                    case MapAnimatorEasingType::Quartic:
                        value = easeOut_Quartic(t, initial, delta, duration);
                        break;
                    case MapAnimatorEasingType::Quintic:
                        value = easeOut_Quintic(t, initial, delta, duration);
                        break;
                    case MapAnimatorEasingType::Sinusoidal:
                        value = easeOut_Sinusoidal(t, initial, delta, duration);
                        break;
                    case MapAnimatorEasingType::Exponential:
                        value = easeOut_Exponential(t, initial, delta, duration);
                        break;
                    case MapAnimatorEasingType::Circular:
                        value = easeOut_Circular(t, initial, delta, duration);
                        break;
                    case MapAnimatorEasingType::None:
                    default:
                        assert(false);
                        break;
                    }
                }
                else if(easingOut == MapAnimatorEasingType::None)
                {
                    switch (easingIn)
                    {
                    case MapAnimatorEasingType::Linear:
                        value = linearTween(t, initial, delta, duration);
                        break;
                    case MapAnimatorEasingType::Quadratic:
                        value = easeIn_Quadratic(t, initial, delta, duration);
                        break;
                    case MapAnimatorEasingType::Cubic:
                        value = easeIn_Cubic(t, initial, delta, duration);
                        break;
                    case MapAnimatorEasingType::Quartic:
                        value = easeIn_Quartic(t, initial, delta, duration);
                        break;
                    case MapAnimatorEasingType::Quintic:
                        value = easeIn_Quintic(t, initial, delta, duration);
                        break;
                    case MapAnimatorEasingType::Sinusoidal:
                        value = easeIn_Sinusoidal(t, initial, delta, duration);
                        break;
                    case MapAnimatorEasingType::Exponential:
                        value = easeIn_Exponential(t, initial, delta, duration);
                        break;
                    case MapAnimatorEasingType::Circular:
                        value = easeIn_Circular(t, initial, delta, duration);
                        break;
                    case MapAnimatorEasingType::None:
                    default:
                        assert(false);
                        break;
                    }
                }
                else
                {
                    const auto halfDuration = 0.5f * duration;
                    const auto halfDelta = 0.5f * delta;
                    const auto tn = t / halfDuration;
                    if(tn < 1.0f)
                    {
                        // Ease-in part
                        calculateValue(tn * halfDuration, initial, halfDelta, halfDuration, easingIn, MapAnimatorEasingType::None, value);
                    }
                    else
                    {
                        // Ease-out part
                        calculateValue((tn - 1.0f) * halfDuration, initial + halfDelta, halfDelta, halfDuration, MapAnimatorEasingType::None, easingOut, value);
                    }
                }
            }

            template <typename T>
            static void calculateValue(const float t, const Point<T>& initial, const Point<T>& delta, const float duration, MapAnimatorEasingType easingIn, MapAnimatorEasingType easingOut, Point<T>& value)
            {
                if(easingIn == MapAnimatorEasingType::None)
                {
                    switch (easingOut)
                    {
                    case MapAnimatorEasingType::Linear:
                        value.x = linearTween(t, initial.x, delta.x, duration);
                        value.y = linearTween(t, initial.y, delta.y, duration);
                        break;
                    case MapAnimatorEasingType::Quadratic:
                        value.x = easeOut_Quadratic(t, initial.x, delta.x, duration);
                        value.y = easeOut_Quadratic(t, initial.y, delta.y, duration);
                        break;
                    case MapAnimatorEasingType::Cubic:
                        value.x = easeOut_Cubic(t, initial.x, delta.x, duration);
                        value.y = easeOut_Cubic(t, initial.y, delta.y, duration);
                        break;
                    case MapAnimatorEasingType::Quartic:
                        value.x = easeOut_Quartic(t, initial.x, delta.x, duration);
                        value.y = easeOut_Quartic(t, initial.y, delta.y, duration);
                        break;
                    case MapAnimatorEasingType::Quintic:
                        value.x = easeOut_Quintic(t, initial.x, delta.x, duration);
                        value.y = easeOut_Quintic(t, initial.y, delta.y, duration);
                        break;
                    case MapAnimatorEasingType::Sinusoidal:
                        value.x = easeOut_Sinusoidal(t, initial.x, delta.x, duration);
                        value.y = easeOut_Sinusoidal(t, initial.y, delta.y, duration);
                        break;
                    case MapAnimatorEasingType::Exponential:
                        value.x = easeOut_Exponential(t, initial.x, delta.x, duration);
                        value.y = easeOut_Exponential(t, initial.y, delta.y, duration);
                        break;
                    case MapAnimatorEasingType::Circular:
                        value.x = easeOut_Circular(t, initial.x, delta.x, duration);
                        value.y = easeOut_Circular(t, initial.y, delta.y, duration);
                        break;
                    case MapAnimatorEasingType::None:
                    default:
                        assert(false);
                        break;
                    }
                }
                else if(easingOut == MapAnimatorEasingType::None)
                {
                    switch (easingIn)
                    {
                    case MapAnimatorEasingType::Linear:
                        value.x = linearTween(t, initial.x, delta.x, duration);
                        value.y = linearTween(t, initial.y, delta.y, duration);
                        break;
                    case MapAnimatorEasingType::Quadratic:
                        value.x = easeIn_Quadratic(t, initial.x, delta.x, duration);
                        value.y = easeIn_Quadratic(t, initial.y, delta.y, duration);
                        break;
                    case MapAnimatorEasingType::Cubic:
                        value.x = easeIn_Cubic(t, initial.x, delta.x, duration);
                        value.y = easeIn_Cubic(t, initial.y, delta.y, duration);
                        break;
                    case MapAnimatorEasingType::Quartic:
                        value.x = easeIn_Quartic(t, initial.x, delta.x, duration);
                        value.y = easeIn_Quartic(t, initial.y, delta.y, duration);
                        break;
                    case MapAnimatorEasingType::Quintic:
                        value.x = easeIn_Quintic(t, initial.x, delta.x, duration);
                        value.y = easeIn_Quintic(t, initial.y, delta.y, duration);
                        break;
                    case MapAnimatorEasingType::Sinusoidal:
                        value.x = easeIn_Sinusoidal(t, initial.x, delta.x, duration);
                        value.y = easeIn_Sinusoidal(t, initial.y, delta.y, duration);
                        break;
                    case MapAnimatorEasingType::Exponential:
                        value.x = easeIn_Exponential(t, initial.x, delta.x, duration);
                        value.y = easeIn_Exponential(t, initial.y, delta.y, duration);
                        break;
                    case MapAnimatorEasingType::Circular:
                        value.x = easeIn_Circular(t, initial.x, delta.x, duration);
                        value.y = easeIn_Circular(t, initial.y, delta.y, duration);
                        break;
                    case MapAnimatorEasingType::None:
                    default:
                        assert(false);
                        break;
                    }
                }
                else
                {
                    const auto halfDuration = 0.5f * duration;
                    const Point<T> halfDelta(0.5f * delta.x, 0.5f * delta.y);
                    const auto tn = t / halfDuration;
                    if(tn < 1.0f)
                    {
                        // Ease-in part
                        calculateValue(tn * halfDuration, initial, halfDelta, halfDuration, easingIn, MapAnimatorEasingType::None, value);
                    }
                    else
                    {
                        // Ease-out part
                        calculateValue((tn - 1.0f) * halfDuration, initial + halfDelta, halfDelta, halfDuration, MapAnimatorEasingType::None, easingOut, value);
                    }
                }
            }

            static float linearTween(const float t, const float initial, const float delta, const float duration);
            static float easeIn_Quadratic(const float t, const float initial, const float delta, const float duration);
            static float easeOut_Quadratic(const float t, const float initial, const float delta, const float duration);
            static float easeIn_Cubic(const float t, const float initial, const float delta, const float duration);
            static float easeOut_Cubic(const float t, const float initial, const float delta, const float duration);
            static float easeIn_Quartic(const float t, const float initial, const float delta, const float duration);
            static float easeOut_Quartic(const float t, const float initial, const float delta, const float duration);
            static float easeIn_Quintic(const float t, const float initial, const float delta, const float duration);
            static float easeOut_Quintic(const float t, const float initial, const float delta, const float duration);
            static float easeIn_Sinusoidal(const float t, const float initial, const float delta, const float duration);
            static float easeOut_Sinusoidal(const float t, const float initial, const float delta, const float duration);
            static float easeIn_Exponential(const float t, const float initial, const float delta, const float duration);
            static float easeOut_Exponential(const float t, const float initial, const float delta, const float duration);
            static float easeIn_Circular(const float t, const float initial, const float delta, const float duration);
            static float easeOut_Circular(const float t, const float initial, const float delta, const float duration);

        public:
            virtual ~AbstractAnimation()
            {}

            virtual bool process(const float timePassed) = 0;

            const float duration;

            const MapAnimatorEasingType easingIn;
            const MapAnimatorEasingType easingOut;
        };

        template <typename T>
        class Animation : public AbstractAnimation
        {
        public:
            typedef std::function<void (const T& newValue)> ApplierMethod;
            typedef std::function<T ()> GetInitialValueMethod;
        private:
        protected:
            T _initialValue;
            bool _initialValueCaptured;
        public:
            Animation(const T& deltaValue_, float duration, MapAnimatorEasingType easingIn, MapAnimatorEasingType easingOut, GetInitialValueMethod obtainer_, ApplierMethod applier_)
                : AbstractAnimation(duration, easingIn, easingOut)
                , _initialValueCaptured(false)
                , deltaValue(deltaValue_)
                , obtainer(obtainer_)
                , applier(applier_)
            {
                assert(obtainer != nullptr);
                assert(applier != nullptr);
            }
            virtual ~Animation()
            {}

            virtual bool process(const float timePassed)
            {
                // If this is first frame, and initial value has not been captured, do that
                if(!_initialValueCaptured)
                {
                    _initialValue = obtainer();
                    _initialValueCaptured = true;

                    // Return false to indicate that processing has not yet finished
                    return false;
                }

                // Calculate time
                _timePassed += timePassed;
                const auto currentTime = qMin(_timePassed, duration);
               
                // Obtain current delta
                T newValue;
                calculateValue(currentTime, _initialValue, deltaValue, duration, easingIn, easingOut, newValue);

                // Apply new value
                applier(newValue);

                return (_timePassed >= duration);
            }

            const T deltaValue;
            const GetInitialValueMethod obtainer;
            const ApplierMethod applier;
        };

        volatile bool _isAnimationPaused;
        QMutex _animationsMutex;
        QList< std::shared_ptr<AbstractAnimation> > _animations;
    public:
        ~MapAnimator_P();

    friend class OsmAnd::MapAnimator;
    };

}

#endif // __MAP_ANIMATOR_P_H_
