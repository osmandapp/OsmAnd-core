#ifndef _OSMAND_CORE_MAP_ANIMATOR_H_
#define _OSMAND_CORE_MAP_ANIMATOR_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/MapTypes.h>

namespace OsmAnd
{
    class IMapRenderer;

    class MapAnimator_P;
    class OSMAND_CORE_API MapAnimator
    {
        Q_DISABLE_COPY(MapAnimator);
    public:
        enum class TimingFunction
        {
            Invalid = -1,

            Linear = 0,

#define _DECLARE_TIMING_FUNCTION(name)                                                                  \
    EaseIn##name,                                                                                       \
    EaseOut##name,                                                                                      \
    EaseInOut##name,                                                                                    \
    EaseOutIn##name

            _DECLARE_TIMING_FUNCTION(Quadratic),
            _DECLARE_TIMING_FUNCTION(Cubic),
            _DECLARE_TIMING_FUNCTION(Quartic),
            _DECLARE_TIMING_FUNCTION(Quintic),
            _DECLARE_TIMING_FUNCTION(Sinusoidal),
            _DECLARE_TIMING_FUNCTION(Exponential),
            _DECLARE_TIMING_FUNCTION(Circular),

#undef _DECLARE_TIMING_FUNCTION
        };

        enum class AnimatedValue
        {
            Unknown = -1,

            Zoom,
            Azimuth,
            ElevationAngle,
            Target
        };

        class OSMAND_CORE_API IAnimation
        {
            Q_DISABLE_COPY(IAnimation);

        private:
        protected:
            IAnimation();
        public:
            virtual ~IAnimation();

            virtual AnimatedValue getAnimatedValue() const = 0;

            virtual bool isActive() const = 0;

            virtual float getTimePassed() const = 0;
            virtual float getDelay() const = 0;
            virtual float getDuration() const = 0;
            virtual TimingFunction getTimingFunction() const = 0;

            virtual bool obtainInitialValueAsFloat(float& outValue) const = 0;
            virtual bool obtainDeltaValueAsFloat(float& outValue) const = 0;
            virtual bool obtainCurrentValueAsFloat(float& outValue) const = 0;

            virtual bool obtainInitialValueAsPointI64(PointI64& outValue) const = 0;
            virtual bool obtainDeltaValueAsPointI64(PointI64& outValue) const = 0;
            virtual bool obtainCurrentValueAsPointI64(PointI64& outValue) const = 0;
        };

    private:
        PrivateImplementation<MapAnimator_P> _p;
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

        QList< std::shared_ptr<const IAnimation> > getAnimations() const;
        std::shared_ptr<const IAnimation> getCurrentAnimationOf(const AnimatedValue value) const;

        void update(const float timePassed);

        void animateZoomBy(const float deltaValue, const float duration, const TimingFunction timingFunction);
        void animateZoomTo(const float value, const float duration, const TimingFunction timingFunction);
        void animateZoomWith(const float velocity, const float deceleration);

        void animateTargetBy(const PointI& deltaValue, const float duration, const TimingFunction timingFunction);
        void animateTargetBy(const PointI64& deltaValue, const float duration, const TimingFunction timingFunction);
        void animateTargetTo(const PointI& value, const float duration, const TimingFunction timingFunction);
        void animateTargetWith(const PointD& velocity, const PointD& deceleration);

        void parabolicAnimateTargetBy(const PointI& deltaValue, const float duration, const TimingFunction targetTimingFunction, const TimingFunction zoomTimingFunction);
        void parabolicAnimateTargetBy(const PointI64& deltaValue, const float duration, const TimingFunction targetTimingFunction, const TimingFunction zoomTimingFunction);
        void parabolicAnimateTargetTo(const PointI& value, const float duration, const TimingFunction targetTimingFunction, const TimingFunction zoomTimingFunction);
        void parabolicAnimateTargetWith(const PointD& velocity, const PointD& deceleration);

        void animateAzimuthBy(const float deltaValue, const float duration, const TimingFunction timingFunction);
        void animateAzimuthTo(const float value, const float duration, const TimingFunction timingFunction);
        void animateAzimuthWith(const float velocity, const float deceleration);

        void animateElevationAngleBy(const float deltaValue, const float duration, const TimingFunction timingFunction);
        void animateElevationAngleTo(const float value, const float duration, const TimingFunction timingFunction);
        void animateElevationAngleWith(const float velocity, const float deceleration);

        void animateMoveBy(
            const PointI& deltaValue, const float duration,
            const bool zeroizeAzimuth, const bool invZeroizeElevationAngle,
            const TimingFunction timingFunction);
        void animateMoveBy(
            const PointI64& deltaValue, const float duration,
            const bool zeroizeAzimuth, const bool invZeroizeElevationAngle,
            const TimingFunction timingFunction);
        void animateMoveTo(
            const PointI& deltaValue, const float duration,
            const bool zeroizeAzimuth, const bool invZeroizeElevationAngle,
            const TimingFunction timingFunction);
        void animateMoveWith(const PointD& velocity, const PointD& deceleration, const bool zeroizeAzimuth, const bool invZeroizeElevationAngle);
    };
}

#endif // !defined(_OSMAND_CORE_MAP_ANIMATOR_H_)
