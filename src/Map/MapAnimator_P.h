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

#include <QtMath>
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
                        value = initial + linearTween(t, delta, duration);
                        break;
                    case MapAnimatorEasingType::Quadratic:
                        value = initial + easeOut_Quadratic(t, delta, duration);
                        break;
                    case MapAnimatorEasingType::Cubic:
                        value = initial + easeOut_Cubic(t, delta, duration);
                        break;
                    case MapAnimatorEasingType::Quartic:
                        value = initial + easeOut_Quartic(t, delta, duration);
                        break;
                    case MapAnimatorEasingType::Quintic:
                        value = initial + easeOut_Quintic(t, delta, duration);
                        break;
                    case MapAnimatorEasingType::Sinusoidal:
                        value = initial + easeOut_Sinusoidal(t, delta, duration);
                        break;
                    case MapAnimatorEasingType::Exponential:
                        value = initial + easeOut_Exponential(t, delta, duration);
                        break;
                    case MapAnimatorEasingType::Circular:
                        value = initial + easeOut_Circular(t, delta, duration);
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
                        value = initial + linearTween(t, delta, duration);
                        break;
                    case MapAnimatorEasingType::Quadratic:
                        value = initial + easeIn_Quadratic(t, delta, duration);
                        break;
                    case MapAnimatorEasingType::Cubic:
                        value = initial + easeIn_Cubic(t, delta, duration);
                        break;
                    case MapAnimatorEasingType::Quartic:
                        value = initial + easeIn_Quartic(t, delta, duration);
                        break;
                    case MapAnimatorEasingType::Quintic:
                        value = initial + easeIn_Quintic(t, delta, duration);
                        break;
                    case MapAnimatorEasingType::Sinusoidal:
                        value = initial + easeIn_Sinusoidal(t, delta, duration);
                        break;
                    case MapAnimatorEasingType::Exponential:
                        value = initial + easeIn_Exponential(t, delta, duration);
                        break;
                    case MapAnimatorEasingType::Circular:
                        value = initial + easeIn_Circular(t, delta, duration);
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
                        value.x = initial.x + static_cast<T>(linearTween(t, static_cast<float>(delta.x), duration));
                        value.y = initial.y + static_cast<T>(linearTween(t, static_cast<float>(delta.y), duration));
                        break;
                    case MapAnimatorEasingType::Quadratic:
                        value.x = initial.x + static_cast<T>(easeOut_Quadratic(t, static_cast<float>(delta.x), duration));
                        value.y = initial.y + static_cast<T>(easeOut_Quadratic(t, static_cast<float>(delta.y), duration));
                        break;
                    case MapAnimatorEasingType::Cubic:
                        value.x = initial.x + static_cast<T>(easeOut_Cubic(t, static_cast<float>(delta.x), duration));
                        value.y = initial.y + static_cast<T>(easeOut_Cubic(t, static_cast<float>(delta.y), duration));
                        break;
                    case MapAnimatorEasingType::Quartic:
                        value.x = initial.x + static_cast<T>(easeOut_Quartic(t, static_cast<float>(delta.x), duration));
                        value.y = initial.y + static_cast<T>(easeOut_Quartic(t, static_cast<float>(delta.y), duration));
                        break;
                    case MapAnimatorEasingType::Quintic:
                        value.x = initial.x + static_cast<T>(easeOut_Quintic(t, static_cast<float>(delta.x), duration));
                        value.y = initial.y + static_cast<T>(easeOut_Quintic(t, static_cast<float>(delta.y), duration));
                        break;
                    case MapAnimatorEasingType::Sinusoidal:
                        value.x = initial.x + static_cast<T>(easeOut_Sinusoidal(t, static_cast<float>(delta.x), duration));
                        value.y = initial.y + static_cast<T>(easeOut_Sinusoidal(t, static_cast<float>(delta.y), duration));
                        break;
                    case MapAnimatorEasingType::Exponential:
                        value.x = initial.x + static_cast<T>(easeOut_Exponential(t, static_cast<float>(delta.x), duration));
                        value.y = initial.y + static_cast<T>(easeOut_Exponential(t, static_cast<float>(delta.y), duration));
                        break;
                    case MapAnimatorEasingType::Circular:
                        value.x = initial.x + static_cast<T>(easeOut_Circular(t, static_cast<float>(delta.x), duration));
                        value.y = initial.y + static_cast<T>(easeOut_Circular(t, static_cast<float>(delta.y), duration));
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
                        value.x = initial.x + static_cast<T>(linearTween(t, static_cast<float>(delta.x), duration));
                        value.y = initial.y + static_cast<T>(linearTween(t, static_cast<float>(delta.y), duration));
                        break;
                    case MapAnimatorEasingType::Quadratic:
                        value.x = initial.x + static_cast<T>(easeIn_Quadratic(t, static_cast<float>(delta.x), duration));
                        value.y = initial.y + static_cast<T>(easeIn_Quadratic(t, static_cast<float>(delta.y), duration));
                        break;
                    case MapAnimatorEasingType::Cubic:
                        value.x = initial.x + static_cast<T>(easeIn_Cubic(t, static_cast<float>(delta.x), duration));
                        value.y = initial.y + static_cast<T>(easeIn_Cubic(t, static_cast<float>(delta.y), duration));
                        break;
                    case MapAnimatorEasingType::Quartic:
                        value.x = initial.x + static_cast<T>(easeIn_Quartic(t, static_cast<float>(delta.x), duration));
                        value.y = initial.y + static_cast<T>(easeIn_Quartic(t, static_cast<float>(delta.y), duration));
                        break;
                    case MapAnimatorEasingType::Quintic:
                        value.x = initial.x + static_cast<T>(easeIn_Quintic(t, static_cast<float>(delta.x), duration));
                        value.y = initial.y + static_cast<T>(easeIn_Quintic(t, static_cast<float>(delta.y), duration));
                        break;
                    case MapAnimatorEasingType::Sinusoidal:
                        value.x = initial.x + static_cast<T>(easeIn_Sinusoidal(t, static_cast<float>(delta.x), duration));
                        value.y = initial.y + static_cast<T>(easeIn_Sinusoidal(t, static_cast<float>(delta.y), duration));
                        break;
                    case MapAnimatorEasingType::Exponential:
                        value.x = initial.x + static_cast<T>(easeIn_Exponential(t, static_cast<float>(delta.x), duration));
                        value.y = initial.y + static_cast<T>(easeIn_Exponential(t, static_cast<float>(delta.y), duration));
                        break;
                    case MapAnimatorEasingType::Circular:
                        value.x = initial.x + static_cast<T>(easeIn_Circular(t, static_cast<float>(delta.x), duration));
                        value.y = initial.y + static_cast<T>(easeIn_Circular(t, static_cast<float>(delta.y), duration));
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

            template <typename T>
            static T linearTween(const float t, const T delta, const float duration)
            {
                const auto nt = t / duration;
                return nt*delta;
            }

            template <typename T>
            static T easeIn_Quadratic(const float t, const T delta, const float duration)
            {
                const auto nt = t / duration;
                return (nt*nt)*delta;
            }

            template <typename T>
            static T easeOut_Quadratic(const float t, const T delta, const float duration)
            {
                const auto nt = t / duration;
                return -nt*(nt-2.0f)*delta;
            }

            template <typename T>
            static T easeIn_Cubic(const float t, const T delta, const float duration)
            {
                const auto nt = t / duration;
                return nt*nt*nt*delta;
            }

            template <typename T>
            static T easeOut_Cubic(const float t, const T delta, const float duration)
            {
                auto nt = t / duration;
                nt -= 1.0f;
                return (nt*nt*nt + 1.0f)*delta;
            }

            template <typename T>
            static T easeIn_Quartic(const float t, const T delta, const float duration)
            {
                const auto nt = t / duration;
                return nt*nt*nt*nt*delta;
            }

            template <typename T>
            static T easeOut_Quartic(const float t, const T delta, const float duration)
            {
                auto nt = t / duration;
                nt -= 1.0f;
                return -(nt*nt*nt*nt - 1.0f)*delta;
            }

            template <typename T>
            static T easeIn_Quintic(const float t, const T delta, const float duration)
            {
                const auto nt = t / duration;
                return nt*nt*nt*nt*nt*delta;
            }

            template <typename T>
            static T easeOut_Quintic(const float t, const T delta, const float duration)
            {
                auto nt = t / duration;
                nt -= 1.0f;
                return (nt*nt*nt*nt*nt + 1.0f)*delta;
            }

            template <typename T>
            static T easeIn_Sinusoidal(const float t, const T delta, const float duration)
            {
                const auto nt = t / duration;
                return delta - qCos(nt * M_PI_2)*delta;
            }

            template <typename T>
            static T easeOut_Sinusoidal(const float t, const T delta, const float duration)
            {
                const auto nt = t / duration;
                return qSin(nt * M_PI_2)*delta;
            }

            template <typename T>
            static T easeIn_Exponential(const float t, const T delta, const float duration)
            {
                const auto nt = t / duration;
                return qPow( 2.0f, 10.0f * (nt - 1.0f) )*delta;
            }

            template <typename T>
            static T easeOut_Exponential(const float t, const T delta, const float duration)
            {
                const auto nt = t / duration;
                return ( -qPow( 2.0f, -10.0f * nt ) + 1.0f )*delta;
            }

            template <typename T>
            static T easeIn_Circular(const float t, const T delta, const float duration)
            {
                const auto nt = t / duration;
                return -(qSqrt(1.0f - nt*nt) - 1.0f)*delta;
            }

            template <typename T>
            static T easeOut_Circular(const float t, const T delta, const float duration)
            {
                auto nt = t / duration;
                nt -= 1.0f;
                return qSqrt(1.0f - nt*nt)*delta;
            }

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
                    return (_timePassed >= duration);
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
