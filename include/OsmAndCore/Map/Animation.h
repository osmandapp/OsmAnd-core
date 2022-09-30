#ifndef _OSMAND_CORE_ANIMATION_H_
#define _OSMAND_CORE_ANIMATION_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QtMath>
#include <QHash>
#include <QMap>
#include <QList>
#include <QReadWriteLock>
#include <QMutex>
#include <QVariant>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "IAnimation.h"

namespace OsmAnd
{
    struct AnimationContext
    {
        QVariantList storageList;
        QVariantHash storageHash;
    };

    class GenericAnimation : public IAnimation
    {
        Q_DISABLE_COPY_AND_MOVE(GenericAnimation);
    public:
        typedef Animator::TimingFunction TimingFunction;
        typedef Animator::AnimatedValue AnimatedValue;
        typedef Animator::Key Key;

    private:
    protected:
        mutable QReadWriteLock _processLock;

        bool _isPaused;
        float _timePassed;

        AnimationContext _ownContext;
        const std::shared_ptr<AnimationContext> _sharedContext;
    
        GenericAnimation(
            const Key key,
            const AnimatedValue animatedValue,
            const float duration,
            const float delay,
            const TimingFunction timingFunction,
            const std::shared_ptr<AnimationContext>& sharedContext);

        static float properCast(const float value);
        static double properCast(const double value);
        static double properCast(const int32_t value);
        static double properCast(const int64_t value);

        template <typename T>
        static void calculateValue(const float t, const T initial, const T delta, const float duration, const TimingFunction timingFunction, T& value)
        {
            switch(timingFunction)
            {
            case TimingFunction::Linear:
                value = initial + static_cast<T>(linearTween(t, properCast(delta), duration));
                break;

            case TimingFunction::Victor_ReverseExponentialZoomOut:
                    value =  static_cast<T>(1 / qPow(2.0f, - t / duration) *properCast(delta) - properCast(delta) ) + initial;
                break;
            case TimingFunction::Victor_ReverseExponentialZoomIn:
                    value = static_cast<T>(1 / qPow(2.0f, t / duration) * (- 2) * properCast(delta) + 2 * properCast(delta)) + initial;
                break;

#define _DECLARE_USE(name)                                                                                                          \
case TimingFunction::EaseIn##name:                                                                                              \
    value = initial + static_cast<T>(easeIn_##name(t, properCast(delta), duration));                                            \
    break;                                                                                                                      \
case TimingFunction::EaseOut##name:                                                                                             \
    value = initial + static_cast<T>(easeOut_##name(t, properCast(delta), duration));                                           \
    break;                                                                                                                      \
case TimingFunction::EaseInOut##name:                                                                                           \
    value = initial + static_cast<T>(easeInOut_##name(t, properCast(delta), duration));                                         \
    break;                                                                                                                      \
case TimingFunction::EaseOutIn##name:                                                                                           \
    value = initial + static_cast<T>(easeOutIn_##name(t, properCast(delta), duration));                                         \
    break;

            _DECLARE_USE(Quadratic)
            _DECLARE_USE(Cubic)
            _DECLARE_USE(Quartic)
            _DECLARE_USE(Quintic)
            _DECLARE_USE(Sinusoidal)
            _DECLARE_USE(Exponential)
            _DECLARE_USE(Circular)

#undef _DECLARE_USE

            case TimingFunction::Invalid:
            default:
                assert(false);
                break;
            }
        }

        template <typename T>
        static void calculateValue(const float t, const Point<T>& initial, const Point<T>& delta, const float duration, const TimingFunction timingFunction, Point<T>& value)
        {
            calculateValue(t, initial.x, delta.x, duration, timingFunction, value.x);
            calculateValue(t, initial.y, delta.y, duration, timingFunction, value.y);
        }

        template <typename T>
        static T linearTween(const float t, const T delta, const float duration)
        {
            const auto nt = t / duration;
            return nt*delta;
        }

#define _DECLARE_IN_OUT_AND_OUT_IN(name)                                                                                                    \
template <typename T>                                                                                                                   \
static T easeInOut_##name(const float t, const T delta, const float duration)                                                           \
{                                                                                                                                       \
    const auto halfDuration = 0.5f * duration;                                                                                          \
    const auto halfDelta = 0.5f * delta;                                                                                                \
    const auto tn = t / halfDuration;                                                                                                   \
    if (tn < 1.0f)                                                                                                                      \
        return easeIn_##name(tn * halfDuration, halfDelta, halfDuration);                                                               \
    else                                                                                                                                \
        return halfDelta + easeOut_##name((tn - 1.0f) * halfDuration, halfDelta, halfDuration);                                         \
}                                                                                                                                       \
template <typename T>                                                                                                                   \
static T easeOutIn_##name(const float t, const T delta, const float duration)                                                           \
{                                                                                                                                       \
    const auto halfDuration = 0.5f * duration;                                                                                          \
    const auto halfDelta = 0.5f * delta;                                                                                                \
    const auto tn = t / halfDuration;                                                                                                   \
    if (tn < 1.0f)                                                                                                                      \
        return easeOut_##name(tn * halfDuration, halfDelta, halfDuration);                                                              \
    else                                                                                                                                \
        return halfDelta + easeIn_##name((tn - 1.0f) * halfDuration, halfDelta, halfDuration);                                          \
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
        _DECLARE_IN_OUT_AND_OUT_IN(Quadratic);

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
        _DECLARE_IN_OUT_AND_OUT_IN(Cubic);

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
        _DECLARE_IN_OUT_AND_OUT_IN(Quartic);

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
        _DECLARE_IN_OUT_AND_OUT_IN(Quintic);

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
        _DECLARE_IN_OUT_AND_OUT_IN(Sinusoidal);

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
        _DECLARE_IN_OUT_AND_OUT_IN(Exponential);

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
        _DECLARE_IN_OUT_AND_OUT_IN(Circular);

#undef _DECLARE_IN_OUT

        static bool isZero(const float value)
        {
            return qFuzzyIsNull(value);
        }

        static bool isZero(const PointI64 value)
        {
            return (value.x == 0 && value.y == 0);
        }
    public:
        virtual ~GenericAnimation();

        virtual bool process(const float timePassed) = 0;

        const Key key;
        const AnimatedValue animatedValue;

        const float delay;
        const float duration;

        const TimingFunction timingFunction;

        virtual Key getKey() const;
        virtual AnimatedValue getAnimatedValue() const;

        virtual float getTimePassed() const;
        virtual float getDelay() const;
        virtual float getDuration() const;
        virtual TimingFunction getTimingFunction() const;

        virtual void pause();
        virtual void resume();
        virtual bool isPaused() const;

        virtual bool isPlaying() const;
    };

    template <typename T>
    class Animation : public GenericAnimation
    {
        Q_DISABLE_COPY_AND_MOVE(Animation);
    public:
        typedef std::function<void (const Key key, const T newValue, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)> ApplierMethod;
        typedef std::function<T (const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)> GetInitialValueMethod;
        typedef std::function<T (const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)> GetDeltaValueMethod;
    private:
    protected:
        T _initialValue;
        bool _initialValueCaptured;
        T _deltaValue;
        bool _deltaValueCaptured;
        T _currentValue;
        bool _currentValueCalculatedOnce;
    public:
        Animation(
            const Key key_,
            const AnimatedValue animatedValue_,
            const T deltaValue_,
            const float duration_,
            const float delay_,
            const TimingFunction timingFunction_,
            const GetInitialValueMethod obtainer_,
            const ApplierMethod applier_,
            const std::shared_ptr<AnimationContext>& sharedContext_ = nullptr)
            : GenericAnimation(key_, animatedValue_, duration_, delay_, timingFunction_, sharedContext_)
            , _initialValueCaptured(false)
            , _deltaValue(deltaValue_)
            , _deltaValueCaptured(true)
            , _currentValueCalculatedOnce(false)
            , deltaValue(_deltaValue)
            , deltaValueObtainer(nullptr)
            , obtainer(obtainer_)
            , applier(applier_)
        {
            assert(obtainer != nullptr);
            assert(applier != nullptr);
        }

        Animation(
            const Key key_,
            const AnimatedValue animatedValue_,
            const GetDeltaValueMethod deltaValueObtainer_,
            const float duration_,
            const float delay_,
            const TimingFunction timingFunction_,
            const GetInitialValueMethod obtainer_,
            const ApplierMethod applier_,
            const std::shared_ptr<AnimationContext>& sharedContext_ = nullptr)
            : GenericAnimation(key_, animatedValue_, duration_, delay_, timingFunction_, sharedContext_)
            , _initialValueCaptured(false)
            , _deltaValueCaptured(false)
            , deltaValue(_deltaValue)
            , deltaValueObtainer(deltaValueObtainer_)
            , obtainer(obtainer_)
            , applier(applier_)
        {
            assert(obtainer != nullptr);
            assert(applier != nullptr);
        }

        virtual ~Animation()
        {
        }

        virtual bool process(const float timePassed)
        {
            QWriteLocker scopedLocker(&_processLock);

            // Increment time
            _timePassed += timePassed;

            // Check for delay
            if (_timePassed < delay)
                return false;

            // If this is first frame, and initial value has not been captured, do that
            if (!_initialValueCaptured)
            {
                _initialValue = obtainer(key, _ownContext, _sharedContext);
                _initialValueCaptured = true;
                if (deltaValueObtainer)
                {
                    _deltaValue = deltaValueObtainer(key, _ownContext, _sharedContext);
                    _deltaValueCaptured = true;
                }
                if (isZero(_deltaValue))
                    return true;

                return false;
            }

            // Calculate time
            const auto currentTime = qMin(_timePassed - delay, duration);
           
            // Obtain current delta
            calculateValue(currentTime, _initialValue, _deltaValue, duration, timingFunction, _currentValue);
            _currentValueCalculatedOnce = true;

            // Apply new value
            applier(key, _currentValue, _ownContext, _sharedContext);

            // Return false to indicate that processing has not yet finished
            return ((_timePassed - delay) >= duration);
        }

        const T& deltaValue;
        const GetDeltaValueMethod deltaValueObtainer;
        const GetInitialValueMethod obtainer;
        const ApplierMethod applier;

        virtual bool obtainInitialValueAsFloat(float& outValue) const
        {
            QReadLocker scopedLocker(&_processLock);

            if (!std::is_same<T, float>::value || !_initialValueCaptured)
                return false;

            outValue = *reinterpret_cast<const float*>(&_initialValue);
            return true;
        }
        virtual bool obtainDeltaValueAsFloat(float& outValue) const
        {
            QReadLocker scopedLocker(&_processLock);

            if (!std::is_same<T, float>::value || !_deltaValueCaptured)
                return false;

            outValue = *reinterpret_cast<const float*>(&deltaValue);
            return true;
        }
        virtual bool obtainCurrentValueAsFloat(float& outValue) const
        {
            QReadLocker scopedLocker(&_processLock);

            if (!std::is_same<T, float>::value || !_currentValueCalculatedOnce)
                return false;

            outValue = *reinterpret_cast<const float*>(&_currentValue);
            return true;
        }

        virtual bool obtainInitialValueAsPointI64(PointI64& outValue) const
        {
            QReadLocker scopedLocker(&_processLock);

            if (!std::is_same<T, PointI64>::value || !_initialValueCaptured)
                return false;

            outValue = *reinterpret_cast<const PointI64*>(&_initialValue);
            return true;
        }
        virtual bool obtainDeltaValueAsPointI64(PointI64& outValue) const
        {
            QReadLocker scopedLocker(&_processLock);

            if (!std::is_same<T, PointI64>::value || !_deltaValueCaptured)
                return false;

            outValue = *reinterpret_cast<const PointI64*>(&deltaValue);
            return true;
        }
        virtual bool obtainCurrentValueAsPointI64(PointI64& outValue) const
        {
            QReadLocker scopedLocker(&_processLock);

            if (!std::is_same<T, PointI64>::value || !_currentValueCalculatedOnce)
                return false;

            outValue = *reinterpret_cast<const PointI64*>(&_currentValue);
            return true;
        }
    };
}

#endif // !defined(_OSMAND_CORE_ANIMATION_H_)
